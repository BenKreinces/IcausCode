# Static-IP ping test

This is the first Ethernet packet-data acceptance test. It tests only ARP and
ICMP echo on the LwIP data path; it does not enable UDP, commands, telemetry,
or actuators.

## Image and memory acceptance

Build the project with the CubeIDE ARM toolchain, or run `make` with an
`arm-none-eabi-gcc` toolchain. Build artifacts default outside the repository
at `/tmp/icarus-second-try-build`, including flashable `.elf`, `.bin`, and
`.hex` images.

Before flashing, inspect the map. The required placement is:

```text
DMARxDscrTab              0x30000000
DMATxDscrTab              0x30000060
memp_memory_RX_POOL_base  0x300000c0
.lwip_heap                0x30005000 ... 0x30009000
```

Those addresses are inside the explicit 64 KiB non-cacheable D2 SRAM MPU
window. Do not flash an image whose Ethernet descriptors or RX pool are in
DTCM or D1 SRAM.

## Bench procedure

1. Keep every actuator disconnected or otherwise de-energized. Flash the
   image and retain the ST-Link debugger connection.
2. Connect the board to a known-good host Ethernet adapter or an isolated
   switch. Use the temporary locally administered MAC only on this isolated
   network.
3. Configure the host interface as static IPv4 `192.168.1.10`, netmask
   `255.255.255.0`, and no gateway. The board is fixed at `192.168.1.50/24`.
4. Reset the board. Its existing UART8 heartbeat confirms that the scheduler
   remains alive. Check link LEDs and, when available, `gnetif` in the
   debugger: it must be administratively up and link up before pinging.
5. Confirm ARP and ping from the host:

   ```sh
   # Linux
   arping -I <host-interface> -c 3 192.168.1.50
   ping -c 20 192.168.1.50
   ```

   ```text
   Windows PowerShell
   ping -n 20 192.168.1.50
   ```

6. Disconnect and reconnect the Ethernet cable five times. After each link
   recovery, repeat the 20-packet ping. Record packet loss during recovery,
   time to the first reply, PHY/ETH errors, and the firmware revision.

## Pass and failure interpretation

Pass requires ARP resolution and 20 ICMP replies after boot, followed by a
successful ping after each of five cable recovery cycles. One successful ping
does not establish Ethernet reliability.

If ARP does not resolve, first distinguish physical link/PHY state from the
MAC/LwIP path: verify the 50 MHz RMII clock on `PA1`, PHY power/reset, and the
KSZ8081 ID at address 0. If link is up but ARP has no reply, inspect the map
placement above, `gnetif` link state, and ETH DMA error state before changing
LwIP configuration. Do not add UDP until this test passes.
