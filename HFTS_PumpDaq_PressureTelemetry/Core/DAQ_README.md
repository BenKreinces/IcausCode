# HFTS PumpTestingHuman DAQ Firmware

## Libraries added (manual implementation)

- **tim1_freq_capture** – Flowmeter on TIM1 CH1 (PE9), DMA input capture, `get_freq_hz(1)`.
- **ad7689** – 8-channel pressure ADC (IN0–IN7), rotating conversions, raw codes; CNV = SPI1_CS_PT1_8.
- **ad7124_ll** – Low-level SPI for load cell ADC; read on DRDY (PD8), raw 24-bit in `ad7124_raw24`.
- **mcp23s17** – Solenoid expander on SPI4 (CS = SPI4_CS_SOL1_12). Mapping: 1–12 = B4,B5,B6,B7,A2,A1,A0,A3,B1,B0,A7,A6 (single 1–8, DSOL1=9,10, DSOL2=11,12).
- **mcp23s17_stm32_hal** – STM32 HAL adapter for MCP23S17 (SPI + CS).
- **daq_packet** – `DataPacket_t` matching Arduino layout (pt[5], flowmeter[2], loadcell[2], VESC fields, solenoid state).
- **daq_udp** – Stub; real UDP requires LwIP (see below).

## Build

Add to the build (if your IDE does not auto-include all `Core/Src/*.c`):

- `Core/Src/tim1_freq_capture.c`
- `Core/Src/ad7689.c`
- `Core/Src/ad7124_ll.c`
- `Core/Src/mcp23s17.c`
- `Core/Src/mcp23s17_stm32_hal.c`
- `Core/Src/daq_udp.c`

## SPI

- **SPI1** (AD7689): 16-bit, Mode 0 (CPOL=0, CPHA=1 edge).
- **SPI2** (AD7124): 8-bit, Mode 3 (CPOL=1, CPHA=2 edge).
- **SPI4** (MCP23S17): 8-bit, Mode 0.

## TIM1 + DMA

DMA for TIM1 CH1 input capture is initialized in `stm32h7xx_hal_msp.c` (USER CODE). `DMA1_Stream0_IRQHandler` in `stm32h7xx_it.c` calls `tim1_freq_capture_set_dma_channel(0)` then `HAL_DMA_IRQHandler`. Ensure DMA1 clock is enabled (`MX_DMA_Init()` in main).

## UDP (LwIP)

**If Wireshark shows no traffic or the connection never updates:** the firmware is using the **UDP stub** in `daq_udp.c`, which does not send or receive. You must add LwIP and define `USE_LWIP` (see below).

To match Arduino behaviour (IP 192.168.1.2, laptop 192.168.1.1:6000, local port 5000, MAC `DE:AD:BE:EF:FE:ED`):

1. In **CubeMX** enable **LwIP** (Middleware), static IP, no DHCP. Set IP to **192.168.1.2**, netmask 255.255.255.0. In ETH configuration set MAC to `{0xDE,0xAD,0xBE,0xEF,0xFE,0xED}` (in `main.c` USER CODE **MACADDRESS** if needed). Regenerate code so that `LWIP/App/lwip.c`, `LWIP/Target/ethernetif.c`, and Middlewares are added.
2. In your build (preprocessor definitions) add **`USE_LWIP`**. `main.c` already calls `MX_LWIP_Init()` after `MX_ETH_Init()` when `USE_LWIP` is defined; `DAQ_UDP_Process()` calls `MX_LWIP_Process()`.
3. With `USE_LWIP` defined, `daq_udp.c` uses LwIP: binds UDP port 5000, sends packets to 192.168.1.1:6000, and the receive callback passes the first byte as instruction and the rest as payload to `handleInstruction`.

Reference: HFTS_EthernetTest (LwIP init, `MX_LWIP_Process`, UDP in `lwip.c`).

**Cache, MPU, LwIP config (match EthernetTest):**  
- **I-Cache and D-Cache** are enabled in `main.c` after `MPU_Config()` so LwIP/ETH run correctly on STM32H7.  
- **MPU region for 0x30000000 (64 KB)** is configured in `MPU_Config()` as non-cacheable, shareable so ETH DMA descriptors and LwIP heap in D2 SRAM are coherent with the DMA.  
- **Unaligned trap** is disabled in `main.c` USER CODE Init (`SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk`) so LwIP can use unaligned access.  
- **lwipopts.h**: `MEM_SIZE` 16384, `LWIP_DHCP` 0 (static IP), `ETHARP_SUPPORT_STATIC_ENTRIES` 1.  
- **ethernetif.c**: D-Cache clean before TX and invalidate for RX descriptors/Rx buffer (cache-line aligned) so CPU and DMA see consistent data.

## VESC / USART1

ESC telemetry (temp, currents, rpm, etc.) and throttle (0xBA) are not implemented. Packet VESC fields are set to 0. To add: implement VESC UART protocol on USART1 (e.g. COMM_GET_VALUES, COMM_SET_RPM), poll at ~10 Hz, and fill `packet->tempMosfet`, `packet->rpm`, etc., and apply `current_throttle` in the motor command path.

## Instructions (same as Arduino)

- **0x0A** – Heartbeat: reset connection timeout, reply 0x0B.
- **0xAA** – Solenoid: next byte = code (0x01–0x08 single toggle, 0x11 DSOL1, 0x12 DSOL2); reply 0xAC.
- **0xBA** – Motor: next 2 bytes = throttle high, low (big-endian); reply 0xBB.
