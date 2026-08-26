#include <Arduino.h>
#include "HX711.h"
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <VescUart.h>

VescUart UART(10);

// Throttle constraints (adjust to your ESC scale)
const int THROTTLE_MIN = 0;
const int THROTTLE_MAX = 2000; // <-- change if your ESC uses a different range
int current_throttle = 0;

/**
CONFIGURE UDP CONNECTION
**/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,2);
IPAddress laptopIP(192,168,1,1);
unsigned int laptopPort = 6000;
unsigned int localPort = 5000;
EthernetUDP Udp;


/**
SOLENOIDS
**/
const int SINGLE_SOLENOIDS[8] = {1, 7, 2, 8, 3, 9, 6, 12};
const int DSOL1_PIN1 = 5, DSOL1_PIN2 = 11; //LOX Side
const int DSOL2_PIN1 = 4, DSOL2_PIN2 = 10; //LNG Side
boolean dualSolenoidState[2] = {true, true}; // true = one direction, false = other



// ---------- Flowmeter: use your previous FlowmeterReader (interrupt-based) ----------
template<uint8_t PIN, uint8_t ALPHA = 0, uint32_t MIN_PERIOD_US = 5>
class FlowmeterReader {
public:
  void begin() { pinMode(PIN, INPUT); instance_ = this;
    attachInterrupt(digitalPinToInterrupt(PIN), &FlowmeterReader::isrTrampoline, RISING); }
  bool readHz(double& hzOut, uint32_t timeout_ms = 500) const {
    uint32_t lastSeen = lastSeenMs_, p = periodUs_;
    if (p > 0 && (millis() - lastSeen) <= timeout_ms) { hzOut = 1000000.0 / (double)p; return true; }
    return false;
  }
private:
  static void isrTrampoline(){ instance_->onEdge(); }
  void onEdge(){
    uint32_t nowUs = micros(), prev = lastEdgeUs_; lastEdgeUs_ = nowUs;
    uint32_t d = nowUs - prev;
    if (prev && d >= MIN_PERIOD_US){
      if constexpr (ALPHA == 0) periodUs_ = d;
      else { uint32_t p = periodUs_ ? periodUs_ : d;
             periodUs_ = ((uint32_t)((uint64_t)(256-ALPHA)*d + (uint64_t)ALPHA*p)) >> 8; }
      lastSeenMs_ = millis();
    }
  }
  volatile uint32_t lastEdgeUs_ = 0, periodUs_ = 0, lastSeenMs_ = 0;
  static FlowmeterReader* instance_;
};
template<uint8_t P, uint8_t A, uint32_t M>
FlowmeterReader<P,A,M>* FlowmeterReader<P,A,M>::instance_ = nullptr;

// ---------- Pressure transducer: polled ADC at your chosen rate ----------
template<uint8_t PIN, int BITS = 12>
class PressureTransducerReader {
public:
  void begin() { analogReadResolution(BITS); pinMode(PIN, INPUT); }
  bool read(double& voltsOut, double& psiOut){
    int adc = analogRead(PIN);
    double v = (adc / ((1 << BITS) - 1.0)) * 3.3;
    double psi = (v - 0.594) * (1000.0 / 2.376);
    voltsOut = v; psiOut = psi;
    return true;
  }
};

// ---------- HX711 wrapper: non-blocking, capped to its own rate ----------
template<uint8_t DOUT, uint8_t SCK, uint8_t GAIN = 128>
class HX711Reader {
public:
  void begin(uint8_t tareSamples = 20) {
    hx_.begin(DOUT, SCK);
    hx_.power_up();               // ensure chip is awake
    delay(1);                     // short settle
    hx_.set_gain(GAIN);           // apply gain
    delay(1);                     // let conversion settle
    //tare(tareSamples);            // automatically zero at startup
    hx_.tare(20);
    hx_.set_scale(scale_);
    hx_.wait_ready_timeout(20);
  }
  /*
  void tare(uint8_t samples = 20) {
    long sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
      while (!hx_.is_ready());    // wait for next sample ready
      sum += hx_.read();
    }
    offset_ = static_cast<double>(sum) / samples;
  }
  */
  //void setScaleCountsPerGram(double cpg) { scale_ = cpg; }
  //void setOffset(double offs) { offset_ = offs; }
  bool tryReadGrams(double& gramsOut) {
    if (!hx_.is_ready()) return false;
    //long raw = hx_.read();
    gramsOut = hx_.get_units(1);
    //gramsOut = (raw - offset_) / scale_;
    return true;
  }

  double getOffset() const { return offset_; }

private:
  HX711 hx_;
  double scale_  = 11.070694; // counts per gram
  double offset_ = 0.0;       // updated by tare()
};

// ===================== Instantiate sensors ======================
using FM0 = FlowmeterReader<39>;
using PT0 = PressureTransducerReader<A10>;
using PT1 = PressureTransducerReader<A11>;
using PT2 = PressureTransducerReader<A12>;
using PT3 = PressureTransducerReader<A13>;
using PT4 = PressureTransducerReader<A17>;

using LC1 = HX711Reader<15,14>;  // DOUT=15, SCK=14

FM0 fm0;
PT0 pt0;
PT1 pt1;
PT2 pt2;
PT3 pt3;
PT4 pt4;

LC1 lc1;

// ===================== Soft timers ======================
elapsedMicros t_adc_us;
elapsedMicros t_connected;
elapsedMicros t_hx_us;
elapsedMillis t_print_ms;

const uint32_t ADC_PERIOD_US = 1000;    // 1 kHz PT reads
const uint32_t TIMEOUT = 2000000;    // 1 sec timout
const uint32_t HX_PERIOD_US  = 12500;   // 80 Hz max → 12.5 ms
const uint32_t PRINT_PERIOD_MS = 1;  

// Latest samples (shared with printer)
double pt0_v=0, pt0_psi=0, pt1_v=0, pt1_psi=0, pt2_v=0, pt2_psi=0, pt3_v=0, pt3_psi=0, pt4_v=0, pt4_psi=0;
double lc1_g = 0;
double fm0_hz = 0;



/**
DATA PACKET STRUCTURE
**/
const int BUFFER_SIZE = 10;
struct DataPacket {
    uint32_t timestamp;
    double pt[5];
    double flowmeter[2];
    double loadcell[2];
    float tempMosfet, avgMotorCurrent, avgInputCurrent, rpm, inpVoltage, ampHours, tachometerAbs;
    uint8_t solenoidState;
    uint8_t dualSolenoidState;
} dataBuffer[BUFFER_SIZE];
volatile int bufferIndex = 0;


/*
  data.tempMosfet 		= buffer_get_float16(message, 10.0, &index); 	// 2 bytes - mc_interface_temp_fet_filtered()
	data.tempMotor 			= buffer_get_float16(message, 10.0, &index); 	// 2 bytes - mc_interface_temp_motor_filtered()
	data.avgMotorCurrent 	= buffer_get_float32(message, 100.0, &index); // 4 bytes - mc_interface_read_reset_avg_motor_current()
	data.avgInputCurrent 	= buffer_get_float32(message, 100.0, &index); // 4 bytes - mc_interface_read_reset_avg_input_current()
	index += 4; // Skip 4 bytes - mc_interface_read_reset_avg_id()
	index += 4; // Skip 4 bytes - mc_interface_read_reset_avg_iq()
	data.dutyCycleNow 		= buffer_get_float16(message, 1000.0, &index); 	// 2 bytes - mc_interface_get_duty_cycle_now()
	data.rpm 				= buffer_get_float32(message, 1.0, &index);		// 4 bytes - mc_interface_get_rpm()
	data.inpVoltage 		= buffer_get_float16(message, 10.0, &index);		// 2 bytes - GET_INPUT_VOLTAGE()
	data.ampHours 			= buffer_get_float32(message, 10000.0, &index);	// 4 bytes - mc_interface_get_amp_hours(false)
	data.ampHoursCharged 	= buffer_get_float32(message, 10000.0, &index);	// 4 bytes - mc_interface_get_amp_hours_charged(false)
	data.wattHours			= buffer_get_float32(message, 10000.0, &index);	// 4 bytes - mc_interface_get_watt_hours(false)
	data.wattHoursCharged	= buffer_get_float32(message, 10000.0, &index);	// 4 bytes - mc_interface_get_watt_hours_charged(false)
	data.tachometer 		= buffer_get_int32(message, &index);				// 4 bytes - mc_interface_get_tachometer_value(false)
	data.tachometerAbs 		= buffer_get_int32(message, &index);				// 4 bytes - mc_interface_get_tachometer_abs_value(false)
	data.error 				= (mc_fault_code)message[index++];								// 1 byte  - mc_interface_get_fault()
	data.pidPos				= buffer_get_float32(message, 1000000.0, &index);	// 4 bytes - mc_interface_get_pid_pos_now()
	data.id					= message[index++];								// 1 byte  - app_get_configuration()->controller_id

Useful ones are tempMosfet, avgMotorCurrent, avgInputCurrent, rpm (eRPM), inpVoltage
ampHours, tachometer.

*/
//float esc_readings[7] = {0,0,0,0,0,0,0};
IntervalTimer escTimer;
volatile bool escReadingReady = false;
const int ESC_INTERVAL_US = 20000; // 10 Hz
void escISR() {
    escReadingReady = true;
}



void setup(){
  Serial.begin(115200);
  Serial5.begin(115200);
  Serial5.setTimeout(10);
  UART.setSerialPort(&Serial5);
  

  fm0.begin();
  pt0.begin();
  pt1.begin();
  pt2.begin();
  pt3.begin();
  pt4.begin();

  lc1.begin();
  escTimer.begin(escISR, ESC_INTERVAL_US);
  t_adc_us = 0; t_hx_us = 0; t_print_ms = 0;
  t_connected = 0;

  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
}

void loop(){
  
  int packetSize = Udp.parsePacket();
  if(packetSize){
    byte instruction = Udp.read();
    handleInstruction(instruction);
  }
  //read command
  if (t_connected < TIMEOUT){
  

  if (escReadingReady){
    escReadingReady = false;
  // pull latest ESC telemetry
    UART.setRPM(current_throttle); //desired eRPM
    UART.getVescValues();
  }


  // ---- Pressure transducers at 1 kHz ----
  if (t_adc_us >= ADC_PERIOD_US){
    t_adc_us -= ADC_PERIOD_US;
    pt0.read(pt0_v, pt0_psi);
    pt1.read(pt1_v, pt1_psi);
    pt2.read(pt2_v, pt2_psi);
    pt3.read(pt3_v, pt3_psi);
    pt4.read(pt4_v, pt4_psi);

  }

  // ---- HX711 at ≤80 Hz, non-blocking ----
  if (t_hx_us >= HX_PERIOD_US){
    t_hx_us -= HX_PERIOD_US;
    double g;
    if (lc1.tryReadGrams(g)) lc1_g = g; // update only if a fresh sample was ready
  }

  // ---- Flowmeter: updated by interrupts; just snapshot occasionally ----
  if (t_print_ms >= PRINT_PERIOD_MS){
    t_print_ms -= PRINT_PERIOD_MS;

    double hz; if (fm0.readHz(hz)) fm0_hz = hz;

    DataPacket& packet = dataBuffer[bufferIndex];
    packet.pt[0] = pt0_psi;
    packet.pt[1] = pt1_psi;
    packet.pt[2] = pt2_psi;
    packet.pt[3] = pt3_psi;
    packet.pt[4] = pt4_psi;
    
    packet.loadcell[0] = lc1_g;
    packet.loadcell[1] = 0;
    packet.tempMosfet = UART.data.tempMosfet;
    packet.avgMotorCurrent = UART.data.avgMotorCurrent;
    packet.avgInputCurrent = UART.data.avgInputCurrent;
    packet.rpm = UART.data.rpm;
    packet.inpVoltage = UART.data.inpVoltage;
    packet.ampHours = UART.data.ampHours;
    packet.tachometerAbs = UART.data.tachometerAbs;
    packet.timestamp = millis();
    packet.flowmeter[0] = fm0_hz;
    packet.flowmeter[1] = 0;
    packet.solenoidState = 0;
    for (int i = 0; i < 6; i++) {
      if (digitalRead(SINGLE_SOLENOIDS[i])) {
        packet.solenoidState |= (1 << i);
      }
    }
    packet.dualSolenoidState = 0;
    if (!dualSolenoidState[0]) packet.dualSolenoidState |= 1;
    if (!dualSolenoidState[1]) packet.dualSolenoidState |= 2;

    if (++bufferIndex >= BUFFER_SIZE) {
      Udp.beginPacket(laptopIP, laptopPort);
      Udp.write((uint8_t*)dataBuffer, sizeof(dataBuffer));
      Udp.endPacket();
      bufferIndex = 0;
    }

  }
  }
  // Do any other work here; no busy-waiting, no blocking delays required.
}

/**
INSTRUCTION HANDLING
**/
void handleInstruction(byte instruction){
  switch(instruction){
    case 0x0A: { //heartbeat
      t_connected = 0;
      Udp.beginPacket(laptopIP, laptopPort);
      Udp.write(0x0B);
      Udp.endPacket();
      break;
    }
    case 0xAA: { //solenoid
      if(t_connected  < TIMEOUT){
      byte solenoidCode = Udp.read();
      handleSolenoid(solenoidCode);
      Udp.beginPacket(laptopIP, laptopPort);
      Udp.write(0xAC);
      Udp.endPacket();
      break;
      }
    }
    // ==== handleInstruction additions/changes ====
    case 0xBA: { // motor: raw throttle write (MANUAL OVERRIDE)
      //6 pole motor
      if(t_connected  < TIMEOUT){
      uint8_t highByte = Udp.read();
      uint8_t lowByte  = Udp.read();
      uint16_t throttle = ((uint16_t)highByte << 8) | lowByte;
      current_throttle = throttle *3;
      Udp.beginPacket(laptopIP, laptopPort);
      Udp.write(0xBB);
      Udp.endPacket();
      break;
      }
    }
  }
}

void handleSolenoid(byte code){
  if (code >= 0x01 && code <= 0x08) {
    int index = code - 0x01;
    digitalWrite(SINGLE_SOLENOIDS[index], !digitalRead(SINGLE_SOLENOIDS[index]));
  } else if (code == 0x11) {
    dualSolenoidState[0] = !dualSolenoidState[0];
    digitalWrite(DSOL1_PIN1, dualSolenoidState[0] ? HIGH : LOW);
    digitalWrite(DSOL1_PIN2, dualSolenoidState[0] ? LOW : HIGH);
  } else if (code == 0x12) {
    dualSolenoidState[1] = !dualSolenoidState[1];
    digitalWrite(DSOL2_PIN1, dualSolenoidState[1] ? HIGH : LOW);
    digitalWrite(DSOL2_PIN2, dualSolenoidState[1] ? LOW : HIGH);
  }
}

