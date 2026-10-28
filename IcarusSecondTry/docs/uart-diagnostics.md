# UART8 diagnostics bring-up

**Hardware verification status: pending.** The firmware configuration and
transmit diagnostic are implemented, but no receiver, logic analyzer, or scope
has yet observed the `PE1` output. Do not treat UART8 as accepted until the
hardware test procedure below passes.

## Purpose and scope

This milestone establishes a simple, observable serial diagnostic channel
before FreeRTOS and the other peripherals are enabled. It is deliberately
transmit-only: it proves the MCU's UART8 initialization and gives later
bring-up work a bounded way to report progress. It does not implement a command
console, packet protocol, interrupt handling, DMA, or `printf` redirection.

## Configuration

| Item | Value |
|---|---|
| Peripheral | UART8 |
| MCU RX pin | `PE0` (`UART8_RX`) |
| MCU TX pin | `PE1` (`UART8_TX`) |
| Pin alternate function | AF8 |
| Frame format | 115200 8-N-1 |
| Flow control | None |
| TX method | HAL polling transmit |
| Per-write timeout | 20 ms |

The `PE0`/`PE1` assignment is corroborated by multiple legacy CubeMX
configurations, but the board connector and its electrical standard remain
provisional until tested against the schematic and hardware. The MCU pins are
3.3 V logic; do not attach a true RS-232-level interface directly.

## Firmware behavior

After the clock and GPIO safe states are established, firmware initializes
UART8. It transmits this line once:

```text
IcarusSecondTry: UART8 diagnostics online
```

It then transmits this line once per second while the LED continues blinking:

```text
IcarusSecondTry: heartbeat
```

Each call to `HAL_UART_Transmit` has a 20 ms timeout. At 115200 baud, even the
startup line takes well below that time to shift out. A HAL error or timeout
uses the existing solid-LED fatal-error indication. This makes the failure
observable and prevents the test from waiting forever.

## Hardware test procedure

1. Keep all actuator power paths in their normal safe state; this firmware does
   not command them, but bring-up should always begin safely.
2. Verify the board schematic or connector documentation identifies the UART8
   `PE1` net and board ground.
3. Connect a **3.3 V TTL USB-to-UART adapter**: adapter RX to board UART8 TX
   (`PE1`), and adapter ground to board ground. Do not omit the common ground
   and do not use a direct RS-232 adapter.
4. Open a terminal at 115200 baud, 8 data bits, no parity, one stop bit, and no
   flow control.
5. Reset or flash the board. Expect the startup line once followed by one
   heartbeat per second, with the LED continuing to blink at 1 Hz.
6. If available, probe `PE1` with a logic analyzer or oscilloscope. The idle
   state should be high and the transmitted data should decode as 115200 8-N-1.
7. Record the firmware revision, terminal/adapter used, connector details,
   voltage level, and result in the hardware test log.

## Interpreting failures

| Observation | Likely investigation order |
|---|---|
| LED blinks but no terminal text | Terminal settings, crossed TX/RX, missing common ground, connector mapping, USB adapter voltage level, then inspect `PE1` electrically. |
| Garbled but repeating text | Baud rate/clock mismatch, terminal framing, or signal integrity/logic-level mismatch. |
| Solid LED after reset | UART8 initialization or its initial bounded transmit failed; debug the UART8 clock, AF8 pin setup, and HAL status. |
| Startup line only, then solid LED | A later heartbeat transmit failed; inspect UART status with the debugger and the physical TX signal. |

## Next UART increment

Once this transmit test passes electrically, add a bounded receive path and a
small line/command parser with explicit buffer limits. Do not use UART commands
to control actuators until command validation, safety state ownership, and
timeout behavior are designed.
