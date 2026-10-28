# IcarusSecondTry Pin Map

This is the working pin map for the Icarus STM32H743 board. It records the
current CubeMX configuration and is not a substitute for the PCB schematic.
Assignments marked **provisional** must be checked against the schematic,
connector pinout, and electrical behavior before they are used in production
firmware.

## Pin naming

STM32 GPIO names use a port letter and pin number. For example, `PB13` means
GPIO port B, pin 13. A peripheral function such as `SPI2_SCK` is an alternate
function selected on that physical GPIO pin.

The board net/device name is more important than the CubeMX label. The board
schematic is authoritative if it disagrees with this document or the `.ioc`.

## Current assignments

| MCU pin | Current function | Board/device role | Direction | Active level / default | Status |
|---|---|---|---|---|---|
| `PI0` | GPIO output | Onboard LED | Output | High = on; reset low | Provisional; verify on hardware |
| `PA13` | SWDIO | Debug/programming | Bidirectional | Debug function | Required for debug |
| `PA14` | SWCLK | Debug/programming | Input/clock | Debug function | Required for debug |
| `PH0` | HSE oscillator input | External high-speed clock | Clock input | N/A | Verify oscillator frequency |
| `PH1` | HSE oscillator output | External high-speed clock | Clock output | N/A | Verify oscillator circuit |
| `PA1` | `ETH_REF_CLK` | RMII reference clock | Clock input | N/A | Provisional; verify KSZ8081 clock source |
| `PA2` | `ETH_MDIO` | KSZ8081 management data | Bidirectional | N/A | Provisional; verify PHY address/pull-up |
| `PA7` | `ETH_CRS_DV` | RMII receive data valid | Input | N/A | Provisional |
| `PC1` | `ETH_MDC` | KSZ8081 management clock | Output | N/A | Provisional |
| `PC4` | `ETH_RXD0` | RMII receive data bit 0 | Input | N/A | Provisional |
| `PC5` | `ETH_RXD1` | RMII receive data bit 1 | Input | N/A | Provisional |
| `PG11` | `ETH_TX_EN` | RMII transmit enable | Output | N/A | Provisional |
| `PG13` | `ETH_TXD0` | RMII transmit data bit 0 | Output | N/A | Provisional |
| `PG14` | `ETH_TXD1` | RMII transmit data bit 1 | Output | N/A | Provisional |
| `PB12` | GPIO output | SPI1 pressure ADC CS, `SPI1_CS_PT1-8` | Output | High = deselected | Provisional; verify device |
| `PD0` | GPIO output | SPI1 pressure ADC CS, `SPI1_CS_PT9-16` | Output | High = deselected | Provisional; verify device |
| `PD1` | GPIO output | SPI1 pressure ADC CS, `SPI1_CS_PT17-24` | Output | High = deselected | Provisional; verify device |
| `PB3` | `SPI1_SCK` | Pressure ADC SPI clock | Output | Peripheral-controlled | Conflict with SWO; resolve before use |
| `PB4` | `SPI1_MISO` | Pressure ADC SPI data to MCU | Input | Peripheral-controlled | Provisional |
| `PB5` | `SPI1_MOSI` | Pressure ADC SPI data from MCU | Output | Peripheral-controlled | Provisional |
| `PB13` | `SPI2_SCK` | AD7124 load-cell ADC clock | Output | Peripheral-controlled | Provisional |
| `PB14` | `SPI2_MISO` | AD7124 data to MCU | Input | Peripheral-controlled | Provisional |
| `PB15` | `SPI2_MOSI` | AD7124 data from MCU | Output | Peripheral-controlled | Provisional |
| `PD4` | GPIO output | AD7124 CS, `SPI2_CS_LC` | Output | High = deselected | Provisional |
| `PE2` | `SPI4_SCK` | MCP23S17 solenoid expander clock | Output | Peripheral-controlled | Provisional |
| `PE5` | `SPI4_MISO` | MCP23S17 data to MCU | Input | Peripheral-controlled | Provisional |
| `PE6` | `SPI4_MOSI` | MCP23S17 data from MCU | Output | Peripheral-controlled | Provisional |
| `PE3` | GPIO output | Solenoid expander CS, `SPI4_CS_SOL1-12` | Output | High = deselected | Provisional |
| `PE4` | GPIO output | Solenoid expander CS, `SPI4_CS_SOL13-24` | Output | High = deselected | Provisional |
| `PF7` | `SPI5_SCK` | Thermocouple interface clock | Output | Peripheral-controlled | Provisional |
| `PF8` | `SPI5_MISO` | Thermocouple data to MCU | Input | Peripheral-controlled | Provisional |
| `PF9` | `SPI5_MOSI` | Thermocouple data from MCU | Output | Peripheral-controlled | Provisional |
| `PF6` | GPIO output | Thermocouple CS, `SPI5_CS_TC` | Output | High = deselected | Provisional |
| `PE9` | `TIM1_CH1` | Flowmeter channel 1 | Input capture | N/A | Provisional; verify signal level |
| `PE11` | `TIM1_CH2` | Flowmeter channel 2 | Input capture | N/A | Provisional |
| `PE13` | `TIM1_CH3` | Flowmeter channel 3 | Input capture | N/A | Provisional |
| `PE14` | `TIM1_CH4` | Flowmeter channel 4 | Input capture | N/A | Provisional |
| `PB8` | `I2C1_SCL` | I2C clock, likely TMP117 bus | Open-drain | Requires pull-up | Provisional; verify devices |
| `PB9` | `I2C1_SDA` | I2C data, likely TMP117 bus | Open-drain | Requires pull-up | Provisional; verify devices |
| `PE0` | `UART8_RX` | UART8 console input | Input | N/A | Configured; hardware verification pending |
| `PE1` | `UART8_TX` | UART8 console output | Output | N/A | Configured; hardware verification pending |
| `PA9` | `USART1_TX` | VESC UART transmit | Output | N/A | Provisional |
| `PA10` | `USART1_RX` | VESC UART receive | Input | N/A | Provisional |

## Known conflicts and open questions

- `PB3`, `PB4`, and `PB5` are also debug/JTAG-related pins. The current map
  assigns them to SPI1. This means SWO tracing should not be assumed available.
- The current `.ioc` includes many peripheral assignments. The current
  `main.c` initializes only GPIO safe states and UART8; the other peripherals
  are not initialized. An assignment in this table does not mean the hardware
  driver is implemented or electrically verified.
- Ethernet requires confirmation of KSZ8081 reset wiring, PHY address, 50 MHz
  RMII reference-clock direction, and all required strap resistors.
- Every chip-select net must be checked against the schematic and must default
  high before its SPI peripheral is enabled.
- Flowmeter input voltage levels and required pull resistors are not yet
  documented.
- The UART console assignment and VESC UART assignment need confirmation from
  the board connector and wiring documentation.

## Pin-map change procedure

When changing a pin:

1. Confirm the physical board net in the schematic.
2. Check the STM32H743 alternate-function table and CubeMX conflicts.
3. Check debug, boot, oscillator, Ethernet, and safety implications.
4. Update the `.ioc` and this document together.
5. Record the reason for the change.
6. Regenerate code and inspect the complete diff.
7. Rebuild and repeat the relevant hardware bring-up test.
