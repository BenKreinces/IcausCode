# IcarusSecondTry milestones

This record distinguishes demonstrated hardware behavior from generated or
source-level preparation. A configuration that builds is not an accepted
hardware milestone.

## Current status

| Milestone | Status | Evidence / remaining gate |
|---|---|---|
| Safe GPIO, FreeRTOS heartbeat, UART8 diagnostics | Bring-up baseline | Present in the firmware; retain the existing bench procedure and build check. |
| RMII/MDIO and KSZ8081 link monitor | Hardware evidence collected; electrical check pending | GDB observed KSZ8081 at PHY address `0`, PHY IDs `0x0022` / `0x1561`, 100 Mbit/s full duplex, completed autonegotiation, and zero MDIO bus errors. Cable disconnect/reconnect reached down and up without reset. The 50 MHz `PA1` RMII clock and PHY reset/power measurements still require a scope/bench check. |
| LwIP static-IP diagnostic | Built and map reviewed; hardware test pending | The command-line build includes LwIP, Ethernet, and FreeRTOS. Its map places descriptors/RX pool at `0x30000000` through `0x30004a60` and the LwIP heap at `0x30005000` through `0x30009000`. ARP and ping remain required on hardware; see `lwip-ping-test.md`. |
| Raw-API UDP echo | Implemented; hardware acceptance deferred | `app/udp_service.c` marshals raw-API calls through `tcpip_callback`, bounds ingress at 532 bytes, and exposes RX/TX/drop/socket counters. The application network task echoes validated requests and sends replies asynchronously. |
| Versioned command/telemetry protocol | Implemented and host-tested | `services/` defines explicit framing, CRC, command acknowledgement, duplicate/stale rejection, telemetry, heartbeat, and a 500 ms fail-safe command timeout. See `packet-format.md` and `make host-test`. |
| Clean-room non-HAL drivers | Implemented and host-tested | AD7689, MCP23S17, flowmeter, VESC UART, and the existing AD7124 driver are isolated from application/network policy. STM32 transport bindings and CubeMX enablement remain separate work. |

## LwIP static-IP acceptance plan

1. Build the Debug configuration in STM32CubeIDE and resolve all warnings or
   linker errors.
2. Inspect `Debug/IcarusSecondTry.map`:
   - `DMARxDscrTab`, `DMATxDscrTab`, and `memp_memory_RX_POOL_base` must be
     in D2 SRAM at `0x3000....`.
   - `.lwip_heap` must begin at `0x30005000` and end at or before
     `0x30009000`.
   - No Ethernet DMA descriptor or RX-pool symbol may be in DTCM or D1 SRAM.
3. Flash with all actuator outputs de-energized. Configure the host Ethernet
   interface as static `192.168.1.10/255.255.255.0` for a direct cable test.
4. Verify ARP resolution and repeated `ping 192.168.1.50` after boot.
5. Repeat ping across at least five cable disconnect/reconnect cycles. Record
   link recovery, packet loss during recovery, and any ETH/LwIP errors.
6. Record firmware revision, host address, link partner, and result before
   accepting Ethernet hardware behavior. The software UDP slice is tested
   separately with `make host-test`.

## Ownership during the LwIP diagnostic

CubeMX's generated `ethernetif` owns ETH, PHY access, link polling, DMA, and
the LwIP interface during this diagnostic. The earlier read-only MDIO
`NetworkTask` is intentionally compiled out in this configuration so two
tasks do not call ETH HAL/MDIO concurrently. The handwritten network runtime
now owns application UDP; raw LwIP operations are marshalled to the TCP/IP
core and receive callbacks copy datagrams to a bounded queue.
