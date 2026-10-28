# Ethernet CubeMX bring-up configuration

This procedure enables one vertical slice at a time. The RMII/MDIO evidence
has been collected; the LwIP static-IP configuration below is generated but
not accepted until it builds, its map is reviewed, and ping passes.

## Phase 1: ETH MAC, RMII, and MDIO only

The board pin assignment below was recovered consistently from the frozen
legacy projects. It remains a board fact to verify electrically, not proof
that the old Ethernet application was reliable.

1. Open `IcarusSecondTry.ioc` in CubeMX and keep **Keep User Code when
   re-generating** enabled in Project Manager -> Code Generator.
2. Under **Connectivity -> ETH**, select **RMII** as the media interface.
3. Confirm the following pins. CubeMX should assign their alternate functions;
   do not retain them as GPIO.

   | Pin | RMII signal |
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

4. Set a temporary locally administered MAC address, initially
   `02:00:00:00:00:01`. It is valid for an isolated bring-up network only and
   must not collide with another device.
5. Enable the **ETH global interrupt**. Use an RTOS-safe priority compatible
   with the Cube-generated FreeRTOS configuration; leave the generated handler
   unchanged.
6. Do not enable ETH DMA, LwIP, DHCP, sockets, application UDP, or extra
   peripherals in this phase. STM32H7 Ethernet uses its own DMA descriptors;
   generic DMA streams are not part of its setup.
7. Generate code, inspect the diff, and build. Expected generated items include
   `HAL_ETH_MODULE_ENABLED`, `heth`, `MX_ETH_Init()`, ETH GPIO/clock setup,
   and `ETH_IRQHandler` forwarding to HAL.

Stop before flashing if CubeMX changes unrelated pin ownership, interrupt
priorities, clocks, FreeRTOS configuration, or handwritten files outside USER
CODE regions. Keep the generated diff for review.

## Phase 1 hardware evidence

Before enabling LwIP, verify the PHY reset behavior, 50 MHz RMII reference
clock at `PA1`, MDIO reads, discovered PHY address, KSZ8081 ID, and link
transitions over cable connect/disconnect. Record the PHY address rather than
assuming a strap value.

## Phase 2: LwIP static-IP diagnostic (configured, pending acceptance)

CubeMX is configured with **Middleware -> LwIP**, FreeRTOS support, IPv4
static `192.168.1.50/255.255.255.0`, and DHCP disabled. It uses `USER_PHY`,
with Netconn and Socket APIs disabled for the initial raw-API path. Use no
gateway for a direct host-to-board test.

CubeMX generated the LwIP sources and Ethernet interface adaptation. The MPU
window is a 64 KB D2 SRAM region at `0x30000000`, full-access,
execute-never, shareable, non-cacheable, and non-bufferable. The linker maps
CubeMX's `.RxDecripSection`, `.TxDecripSection`, and `.Rx_PoolSection` into
that window; the 16 KB LwIP heap begins at `0x30005000`.

Before flashing, build and verify the map as specified in `milestones.md` and
`ethernet-memory.md`. Do not copy a legacy Ethernet implementation or enable
UDP until ARP/ping and link-loss recovery pass using this LwIP data path.
