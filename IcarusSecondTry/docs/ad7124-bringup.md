# AD7124-8 raw-acquisition slice

## Scope and status

The AD7124 software path is complete through raw, single-channel acquisition,
but is **not hardware verified**. `LoadCellAcq` is the sole SPI2 owner. It
resets and identifies the converter, disables every channel, writes the one
provisional channel profile, reads each programmed register back, then polls
for and records raw 24-bit samples. Every SPI transfer has a 20 ms timeout.
A failed transfer, unexpected device ID, non-ready converter, or configuration
readback mismatch is returned explicitly and counted; no error becomes a
plausible sample.

The implementation deliberately stops at raw codes. It does not apply a tare,
calibration, load-cell scaling, or make an ADC reading available to an actuator
decision. Those steps need the verified load-cell model, excitation path,
reference voltage, gain, polarity, and calibration procedure.

The components are separated by responsibility:

- `Core/Src/ad7124.c` implements device protocol, register configuration,
  readback verification, and raw samples without HAL or FreeRTOS dependencies.
- `Core/Src/ad7124_stm32_hal.c` adapts that interface to HAL SPI and software
  chip select on `PD4`.
- `Core/Inc/load_cell_adc_profile.h` contains the only board-specific
  configuration values. Its assumptions are explicitly provisional.
- `LoadCellAcq` in `Core/Src/main.c` owns SPI2, retries failed configuration at
  a bounded interval, and exposes its latest raw sample, sequence, diagnostic
  snapshot, and driver counters to the debugger.

## Provisional profile

The profile was recovered as a starting point from legacy code; it is not an
electrical authority. It must be compared with the schematic before a board is
connected or the samples are used.

| Setting | Current value | Status |
|---|---:|---|
| Input pair | AIN2 positive, AIN3 negative | Unverified historical assumption |
| Setup | 0 | Unverified |
| ADC_CONTROL | `0x0000` (transactional reads; no continuous-read mode) | Unverified |
| CONFIG_0 | `0x0860` | Unverified reference/gain/buffer assumption |
| FILTER_0 | `0x060180` (SINC4, FS 384) | Unverified sample-rate assumption |
| Raw representation | 24-bit offset binary; signed code is `raw - 0x800000` | Device protocol |

## Required CubeMX configuration

CubeMX owns this setup; do not hand-edit generated SPI configuration or HAL
module selection. The checked-in `.ioc` enables SPI2 and the generated code
creates `hspi2`/`MX_SPI2_Init()`. Its present `/2` prescaler is a recovered
starting configuration, not an accepted AD7124 clock rate; change it in
CubeMX after checking the converter clock limit and board signal integrity.

1. Confirm **SPI2** full-duplex master on `PB13` (SCK), `PB14` (MISO), and
   `PB15` (MOSI).
2. Confirm 8-bit frames, MSB first, software NSS, and SPI Mode 3 (CPOL high,
   CPHA second edge).
3. Begin with a conservative prescaler and record the resulting SCLK rate.
4. Keep `PD4` as GPIO output `SPI2_CS_LC`, initially high. It is an active-low
   software chip select; do not assign hardware NSS to it.
5. Regenerate after any configuration change. CubeMX must retain
   `HAL_SPI_MODULE_ENABLED`, `hspi2`, `MX_SPI2_Init()`, and the SPI HAL source.
   The existing USER CODE calls `MX_SPI2_Init()` before binding the task; no
   generated-file edits are required.
6. Build in STM32CubeIDE. At runtime the UART transition message should be
   either `AD7124 acquisition configured` or `AD7124 acquisition fault`.

## Software verification

The host test covers reset framing, ID mismatch, read-only diagnostics,
configuration writes, disabling unused channels, configuration readback,
raw-code decoding, normal not-ready polling, and transport failures:

```sh
gcc -std=c11 -Wall -Wextra -Werror -I Core/Inc \
  Core/Src/ad7124.c tests/ad7124_protocol_test.c -o /tmp/ad7124-test && \
  /tmp/ad7124-test
```

This does not exercise STM32 SPI, pin muxing, timing, or the ADC itself.

## Hardware acceptance still required

Before treating ADC data as valid, verify reset clocks, Mode 3 polarity/phase,
chip-select timing, ID, all configuration readbacks, conversion cadence,
AIN2/AIN3 polarity, reference selection, excitation routing, saturated and
unplugged behavior, transport timeout/recovery, and a known-input calibration.
Record the board revision, firmware revision, SPI rate, jumper/wiring setup,
and results in the hardware test record.
