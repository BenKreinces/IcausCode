# Ethernet RMII/MDIO bring-up

## Goal and boundary

This was the required acceptance gate before LwIP configuration. It proves the
KSZ8081 management interface without starting Ethernet packet DMA or accepting
application traffic. All actuator outputs remain in their startup-safe state.

The firmware polls Clause-22 MDIO once per second from `NetworkTask`. It scans
addresses 0 through 31 until it finds a KSZ8081, then refreshes that PHY's
status. `ethernet_link_monitor` retains the link state and counters for the
debugger. UART8 reports state transitions only; it is helpful evidence but a
scope and debugger are the authority for the electrical checks.

The state-transition logic has a host test independent of STM32 HAL:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I Core/Inc \
  Core/Src/ksz8081.c Core/Src/ethernet_link_monitor.c \
  tests/ethernet_link_monitor_test.c -o /tmp/ethernet-link-monitor-test && \
  /tmp/ethernet-link-monitor-test
```

## CubeMX: phase 1 only

The checked-in `.ioc` already describes this configuration. Open it in the
same or a newer STM32CubeMX/CubeIDE release, retain user code during
regeneration, and verify these items before generating:

1. Under **Connectivity -> ETH**, enable Ethernet in **RMII** mode.
2. Confirm the RMII/MDIO pins:

   | Pin | Signal |
   |---|---|
   | `PA1` | `ETH_REF_CLK` |
   | `PA2` | `ETH_MDIO` |
   | `PA7` | `ETH_CRS_DV` |
   | `PC1` | `ETH_MDC` |
   | `PC4` | `ETH_RXD0` |
   | `PC5` | `ETH_RXD1` |
   | `PG11` | `ETH_TX_EN` |
   | `PG13` | `ETH_TXD0` |
   | `PG14` | `ETH_TXD1` |

3. Leave the temporary isolated-network MAC as `02:00:00:00:00:01`. Do not
   attach two devices using this address to the same LAN.
4. Enable the ETH global interrupt at pre-emption priority 5, subpriority 0.
   This is compatible with the configured FreeRTOS syscall priority boundary.
5. In **Project Manager -> Code Generator**, enable **Keep User Code when
   re-generating**. Generate code and inspect the diff. The generated MSP must
   enable ETH1 MAC/TX/RX clocks, configure AF11 on the pins above, and retain
   `HAL_ETH_MODULE_ENABLED`.
6. **Do not enable LwIP, DHCP, sockets, netconn, or application UDP yet.** Do
   not hand-edit generated ETH initialization to make up for a CubeMX setting.

If regeneration changes clocks, FreeRTOS priority configuration, unrelated pin
ownership, or handwritten code outside `USER CODE` blocks, stop and inspect
the diff before flashing.

## Bench test

1. Keep actuators de-energized. Connect a known-good Ethernet cable to a link
   partner and connect the UART8 diagnostic adapter at 115200 8-N-1.
2. Probe `PA1`. With the PHY powered and out of reset, it must be a 50 MHz RMII
   reference clock. Check PHY reset and board power before debugging firmware
   if that clock is absent.
3. Flash and reset. Expect normal startup/heartbeat text, then one of:

   - `KSZ8081 discovered`, followed by `Ethernet link down` with no cable;
   - `KSZ8081 discovered`, `Ethernet negotiating`, then `Ethernet link up`;
   - `Ethernet MDIO/PHY fault` if the MDIO bus cannot identify the PHY.

4. In the debugger, inspect `ethernet_link_monitor` and `ethernet_phy_status`.
   Record the discovered `phy_address`, `phyid1`, `phyid2`, `bmcr`, `bmsr`,
   `link_up`, `auto_negotiation_complete`, and the monitor transition counts.
5. Disconnect and reconnect the cable at least five times. Each disconnect
   must lead to `DOWN`; each successful reconnect must reach `UP`, without a
   reset. Record failures and counters.

Acceptance requires the 50 MHz reference clock, correct KSZ8081 ID at a
recorded address, stable link transitions, and no growing MDIO fault count.
This milestone deliberately cannot be pinged: ping requires the next LwIP
static-IP milestone.

## Recorded evidence and remaining electrical check

Debugger evidence recorded during bring-up found the KSZ8081 at PHY address
`0`, with IDs `0x0022` and `0x1561`. It reported link up, completed
autonegotiation, 100 Mbit/s full duplex, and zero MDIO bus errors. Cable
disconnect/reconnect reached down and up without a reset.

The `PA1` 50 MHz RMII reference-clock observation and PHY reset/power
measurement remain pending until a suitable scope/bench setup is available.
The LwIP diagnostic is not authorization to mark this electrical check passed.

## LwIP transition

The LwIP configuration has now been generated with FreeRTOS, static IPv4,
DHCP disabled, and Netconn/Socket APIs disabled. Its 64 KB D2 MPU window and
DMA memory layout are documented in `ethernet-memory.md`.

During the LwIP diagnostic, the generated `ethernetif` owns ETH and PHY access;
the pre-LwIP MDIO monitor is compiled out to avoid concurrent HAL access. The
next acceptance gate is build/map review, ARP, ping, and LwIP-path cable
recovery. Raw-API UDP echo remains after that gate.
