# GPIO safe-state bring-up

## Purpose

Before any SPI, I2C, timer, or Ethernet driver is enabled, prove that reset
and firmware startup leave the board in a non-actuating state. This milestone
does not prove that a connected peripheral works; it proves the MCU is driving
the intended default GPIO levels.

`Board_AssertSafeOutputs()` in `Core/Src/main.c` is the handwritten safety
policy. It runs immediately after CubeMX's `MX_GPIO_Init()` and before UART or
the FreeRTOS scheduler starts. CubeMX owns GPIO modes and pin assignments;
the `.ioc` file remains the source of truth for those settings.

## Required states

| Signal | MCU pin | Required startup level | Reason |
|---|---|---:|---|
| `SPI1_CS_PT1-8` | `PB12` | High | Deselect pressure ADC group 1 |
| `SPI1_CS_PT9-16` | `PD0` | High | Deselect pressure ADC group 2 |
| `SPI1_CS_PT17-24` | `PD1` | High | Deselect pressure ADC group 3 |
| `SPI2_CS_LC` | `PD4` | High | Deselect load-cell ADC |
| `SPI4_CS_SOL1-12` | `PE3` | High | Deselect solenoid expander group 1 |
| `SPI4_CS_SOL13-24` | `PE4` | High | Deselect solenoid expander group 2 |
| `SPI5_CS_TC` | `PF6` | High | Deselect thermocouple interface |
| Onboard LED | `PI0` | Low during initialization | Known non-fault indication |

## Hardware test

1. Disconnect or otherwise make safe any actuator power sources.
2. Attach a scope or logic analyzer to the listed chip-select pins and UART8
   TX (`PE1`). Use a common board ground.
3. Reset the board. Verify each chip select is high from startup onward; none
   may pulse low during initialization.
4. Verify the LED begins its 1 Hz cycle and UART8 emits the startup line, the
   safe-output confirmation, and a heartbeat each second.
5. Record board revision, firmware revision, probe location, supply state,
   and result in the bring-up log.

If any chip select is low or pulses low, stop before attaching a peripheral.
Check the schematic, the `.ioc` pin assignment, and the generated GPIO code.

## Adding an output

For every new output, decide its electrically safe reset level first. Set that
level in CubeMX so its generated GPIO initialization establishes it before the
pin becomes an output. Then add the output to `Board_AssertSafeOutputs()` and
this table. Do not enable the connected peripheral until this test passes.
