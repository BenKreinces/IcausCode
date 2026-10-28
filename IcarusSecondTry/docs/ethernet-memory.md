# STM32H743 Ethernet DMA memory contract

The STM32H743 Ethernet DMA owns descriptors and packet buffers asynchronously.
No application module may perform cache maintenance for Ethernet memory; that
belongs exclusively in the generated Ethernet interface adaptation reviewed in
the LwIP milestone.

## Current LwIP DMA memory layout

CubeMX generates `DMARxDscrTab` and `DMATxDscrTab` in the linker sections
`.RxDecripSection` and `.TxDecripSection`; its zero-copy receive pool uses
`.Rx_PoolSection`. `STM32H743IITX_FLASH.ld` maps all three to the start of D2
SRAM (`0x30000000`), aligned to a 32-byte cache line. The descriptors and pool
must not reside in DTCM or the ordinary D1 `.bss` region.

The preceding MDIO-only diagnostic did not start RX/TX DMA. The current LwIP
configuration does, so this layout is a prerequisite for every build, map-file
review, and hardware test. LwIP has been generated but is not yet accepted on
hardware.

## Configured MPU/cache policy

CubeMX configures this region in its Cortex-M7 MPU view:

| Region | Base | Minimum size | Attributes |
|---|---:|---:|---|
| Ethernet DMA window | `0x30000000` | 64 KB | Full access, execute-never, shareable, non-cacheable, non-bufferable |

Enable I-cache and D-cache only after this MPU region is active. The generated
LwIP RX pool and any DMA-visible TX staging buffers must also live within this
window or use centralized cache clean/invalidate operations. The LwIP heap is
reserved at `0x30005000` for 16 KiB, after the generated RX pool;
`LWIP_RAM_HEAP_POINTER` must match that address. Mixing memory/cache policies
ad hoc is prohibited.

## Map-file check

After each build, verify that the descriptors, RX pool, and LwIP heap are in
the expected D2 SRAM locations:

```text
DMARxDscrTab  0x3000....
DMATxDscrTab  0x3000....
memp_memory_RX_POOL_base  0x3000....
.lwip_heap  0x30005000
```

The previous generated build placed both at `0x2400....` in D1 SRAM. That is
not an acceptable final Ethernet DMA placement.
