# IcarusSecondTry: FreeRTOS, safe GPIO, and staged Ethernet bring-up

This is the clean STM32H743 replacement firmware. It provides safe GPIO
startup, FreeRTOS diagnostics, generated static-IP LwIP, bounded raw-API UDP
echo/command/telemetry handling, a versioned wire protocol, and clean-room
device-driver foundations. Ethernet packet behavior remains an unperformed
hardware acceptance item; software protocol and driver tests run with
`make host-test`.

## Expected behavior

The firmware asserts every configured peripheral chip-select output inactive
before starting the scheduler. The onboard LED on `PI0` changes state every
500 ms, producing one complete on/off cycle per second. `HeartbeatTask` sends
a UART8 startup banner, a safe-output confirmation, and a heartbeat once per
second using `PE1` at 115200 baud, 8 data bits, no parity, and one stop bit
(115200 8-N-1).

## Program flow

1. `HAL_Init()` initializes the STM32 HAL and its SysTick time base.
2. `SystemClock_Config()` enables the board's external HSE oscillator and
   configures the existing PLL settings.
3. `GPIO_Init()` enables the GPIOI clock and configures `PI0` as a low-speed,
   push-pull output.
4. `Board_AssertSafeOutputs()` reasserts inactive chip-select levels after
   GPIO configuration and before any peripheral or task can use them.
5. `MX_UART8_Init()` enables UART8 and assigns `PE0`/`PE1` to its RX/TX
   alternate functions.
6. A bounded polling transmit sends the startup banner. FreeRTOS starts,
   `HeartbeatTask` toggles the LED every 500 ms and sends the heartbeat every
   second. The network task initializes LwIP and owns the application UDP
   queue, echo, command acknowledgement, and periodic telemetry publisher.

## Ethernet status and boundary

The RMII/MDIO diagnostic found the KSZ8081 at PHY address 0 and observed cable
loss/recovery without reset. The 50 MHz `PA1` clock observation remains a
pending electrical check. LwIP is configured for static `192.168.1.50/24`,
and its Ethernet DMA memory map has passed review, but it has not yet passed
ARP or ping on hardware.

The generated LwIP Ethernet interface owns ETH/PHY access during this slice;
the earlier MDIO-only diagnostic task is compiled out to prevent concurrent
HAL ETH access. The application UDP layer is enabled, but no actuator target
binding exists until its peripherals are configured and a dedicated
`ActuatorTask` takes ownership.
See `docs/lwip-ping-test.md`, `docs/milestones.md`,
`docs/ethernet-cubemx.md`, and `docs/ethernet-memory.md` before flashing an
LwIP build.

## Remaining intentionally small subsystems

The AD7124-8 raw-acquisition slice is present but remains raw and uncalibrated;
see `docs/ad7124-bringup.md`.
It configures one provisional load-cell channel and exposes raw codes only;
it does not calibrate or publish engineering measurements. UART8 is used only
for this transmit-only diagnostic.
If the LED does not blink, the failure is limited to programming/debugging,
clock setup, GPIO setup, the board LED wiring, or the LED's electrical polarity.

The CubeMX `.ioc` file is the source of truth for pins, ETH/LwIP middleware,
and MPU configuration. Handwritten application behavior is kept in retained
USER CODE regions or separate modules.

## Error indication

If clock initialization fails, `Error_Handler()` disables interrupts and holds
the LED on continuously. A solid LED is therefore different from a successful
blink.

If UART8 initialization or a bounded transmit fails, the same solid-LED error
indication is used. A successful transmit proves the MCU-side UART setup, but
does not by itself prove the connector wiring or USB-to-UART adapter; those
must be checked using the procedure in `docs/uart-diagnostics.md`.

## Build and flash

Open the `IcarusSecondTry` folder as an STM32CubeIDE project, build the Debug
configuration, and flash using the included debug launch configuration.
