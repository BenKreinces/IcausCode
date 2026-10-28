# Load-Cell ADC (AD7124-8) Bring-Up Test Firmware

STM32H743IIT6 test program to validate the load-cell ADC path on your custom PCB: AD7124-8 on SPI2, differential input AIN2–AIN3, UART console for tare/cal/mode/stats.

## Pins and macros to configure

- **AD7124 CS**: Uses `SPI2_CS_LC` (PD4) from your Cube project. Defined in `Core/Inc/board_pins.h` (and `main.h`).
- **SPI2**: SCK=PB13, MISO=PB14, MOSI=PB15 (STM32H7). **SPI2 must be 8-bit** for AD7124; the code re-inits SPI2 to 8-bit in `main.c` USER CODE if Cube generated 4-bit.
- **Optional DRDY**: In `board_pins.h`, set `AD7124_USE_DRDY_PIN` to 1 and define `AD7124_DRDY_GPIO_PORT` and `AD7124_DRDY_GPIO_PIN` if you have a Data-Ready pin (otherwise the driver polls STATUS).
- **Optional RESET**: In `board_pins.h`, set `AD7124_USE_RESET_PIN` to 1 and define the RESET GPIO if you have a hardware reset pin.
- **UART**: `LOADCELL_UART` in `board_pins.h` — set to `&huart1` or `&huart8` for the console (115200 8N1).

## Compile-time knobs (optional)

In `main.h` (or build defines):

| Macro | Default | Description |
|-------|--------|-------------|
| `EXCITATION_V` | 5.0 | Load-cell excitation voltage (V) |
| `AMP_GAIN` | 101.0 | Analog front-end gain |
| `VREF_V` | 2.5 | AD7124 reference voltage (V) |
| `PGA_GAIN_DEFAULT` | 2 | AD7124 PGA (1,2,4,8,16,32,64,128) |
| `ODR_LOW_NOISE_HZ` | 16 | Low-noise mode ODR (Hz) |
| `ODR_HIGH_SPEED_HZ` | 1000 | High-speed mode ODR (Hz) |
| `AD7124_USE_CRC` | 0 | 1 = enable SPI CRC |

## Build

- **STM32CubeIDE**: Ensure `Core/Src/ad7124.c` and `Core/Src/loadcell_app.c` are under the Core source folder (they are included automatically if the Core folder is a source entry).
- Build the project; the default task runs the load-cell test and UART shell.

## UART command mini-shell

| Command | Description |
|---------|-------------|
| `info` | Print AD7124 config, ID, tare, cal, and scaling constants |
| `tare` | Tare over 2 s and set zero offset |
| `mode low` | Switch to low-noise filter (e.g. ~16 Hz ODR) |
| `mode fast` | Switch to high-speed filter (~1 kHz ODR) |
| `cal0` | Store current reading as zero (cal point 0) |
| `cal1 <known_load_kg>` | Store slope from known load in kg (2-point cal) |
| `stats` | Print RMS noise (uV and ton) and comm/saturation counters |
| `calint` | Run AD7124 internal zero-scale calibration (valid when diff input = 0 V) |

## Output

At ~10 Hz the firmware prints:

- Raw 24-bit code, differential voltage (µV), load (ton and kg), and flags `[SAT]` / `[ERR]` when applicable.
- Stuck-reading warning if the same code repeats for several cycles.

## Files

- `Core/Inc/board_pins.h` — CS, optional DRDY/RESET, UART handle.
- `Core/Inc/ad7124.h`, `Core/Src/ad7124.c` — AD7124 register-level driver (reset, R/W, channel/config/filter, data, optional CRC, internal cal).
- `Core/Inc/loadcell_app.h`, `Core/Src/loadcell_app.c` — Conversion (code → Vdiff, load), tare, 2-point cal, stats, UART commands.
- `Core/Inc/main.h`, `Core/Src/main.c` — Init (clocks, GPIO, UART, SPI2, AD7124, tare), default task loop (sample, average, print, command parser).

## Internal calibration

`calint` runs the AD7124 internal zero-scale calibration. It is **valid only when the differential input (AIN2 − AIN3) is 0 V** (e.g. no load on the cell or shorted inputs). Run it after the front-end has settled; the result is stored in the AD7124 OFFS register and applied to subsequent conversions.
