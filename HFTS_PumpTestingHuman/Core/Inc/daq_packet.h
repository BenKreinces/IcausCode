/**
 * @file daq_packet.h
 * @brief Data packet structure matching Arduino HFTS_model (UDP payload).
 * Little-endian. Sent when buffer fills (e.g. BUFFER_SIZE packets).
 */
#ifndef DAQ_PACKET_H
#define DAQ_PACKET_H

#include <stdint.h>

#define DAQ_BUFFER_SIZE 10

typedef struct {
    uint32_t timestamp;
    double pt[5];           /* pressure transducers; use 8 raw in driver, pack 5 for compatibility */
    double flowmeter[2];
    double loadcell[2];    /* raw AD7124 or scaled; [0]=ch0, [1]=0 */
    float tempMosfet;
    float avgMotorCurrent;
    float avgInputCurrent;
    float rpm;
    float inpVoltage;
    float ampHours;
    float tachometerAbs;
    uint8_t solenoidState;   /* bits 0-5 single solenoid states */
    uint8_t dualSolenoidState; /* bit0=DSOL1, bit1=DSOL2 */
} DataPacket_t;

#endif /* DAQ_PACKET_H */
