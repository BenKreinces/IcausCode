# Peripheral enablement required before the actuator/sensor tasks are activated

The software drivers and actuator owner are complete, but CubeMX must generate
the following board configuration before their HAL bindings are instantiated.
Do not hand-edit generated initialization.

| Peripheral | Pins | Purpose | Required mode |
|---|---|---|---|
| SPI1 | PB3/PB4/PB5; PB12, PD0, PD1 as software conversion/select | AD7689 pressure groups | 16-bit full-duplex master; mode and clock rate confirmed against datasheet |
| SPI4 | PE2/PE5/PE6; PE3 and PE4 software CS | MCP23S17 banks | 8-bit full-duplex master; MCP23S17-compatible mode |
| TIM1 | PE9/PE11/PE13/PE14 | Four flowmeter captures | 1 MHz input capture with DMA or bounded ISR handoff |
| USART1 | PA9/PA10 | VESC UART | 115200 8-N-1 RX interrupt/DMA and bounded TX |
| IWDG | none | Critical-task watchdog | Started only after `IcarusHealth_WatchdogEligible()` is true |

After generation, bind `hspi1`, `hspi4`, `htim1`, and `huart1` with the HAL
adapters in `drivers/`, create the Sensor/Actuator/Diagnostics tasks, and pass
`IcarusActuatorService_Apply` to `IcarusNetworkRuntime_Init`. This sequence
does not alter the packet protocol or safety policy.
