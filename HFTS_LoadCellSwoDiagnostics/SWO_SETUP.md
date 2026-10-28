# SWO (Trace) setup in STM32CubeIDE

## 1. CubeMX: PB3 = TRACESWO (done in .ioc)

PB3 is now set to **Trace Asynchronous SW** in `HFTS_LoadCellSwoDiagnostics.ioc` (no longer SPI1_SCK).  
**Regenerate code from CubeMX** so that PB3 is not driven as SPI1: open the .ioc in STM32CubeMX, then **Project → Generate Code** (or save and let the IDE regenerate). SPI1 is unused so leaving its SCK unassigned is fine.

## 2. Debug configuration

1. **Run → Debug Configurations…**
2. Select **HFTS_SolenoidBank Debug** (or your debug config).
3. Open the **Debugger** tab.
4. Enable **“Enable Serial Wire View (SWV)”** (or “Trace” / “SWV”).
5. Set **Core clock (Hz)** to your real HCLK (e.g. **200000000** if 200 MHz).  
   If trace is unstable, try a lower **SWO clock** (e.g. **2000000** = 2 MHz).
6. **Apply** then **Debug**.

## 3. Start a debug session and open SWV

1. Start **Debug** (F11 or Run → Debug). Wait until the target is halted at `main`.
2. **Resume** the target (**F8**) so the program runs.  
   Trace often does not work while the core is halted.
3. **Window → Show View → Other… → SWV → SWV ITM Data Console**. Open it.
4. In **SWV ITM Data Console**:
   - Click the **“Configure trace”** (gear) icon.
   - Enable **ITM Stimulus Port 0** (checkbox).
   - Click **“Start trace”** (red record button).

You should see lines like `ADC=0x801234` in the console.

## 4. If “Start trace” is greyed out

- **Resume the target first** (F8). Many boards only allow starting trace when the core is running.
- Confirm **Enable Serial Wire View (SWV)** is checked in the debug configuration (step 2).
- Confirm PB3 is **Trace Asynchronous SW** in CubeMX and code was regenerated (step 1).
- Try a **lower SWO clock** (e.g. 2 MHz) in the debugger settings.
- Ensure the **SWO pin** of your ST-Link (or probe) is connected to **PB3** on the board.

## 5. Hardware

- **PB3** = TRACESWO (SWO output from MCU).
- Connect your debugger’s **SWO** pin to **PB3** (check your board and probe docs).
- **PA13 (SWDIO)** and **PA14 (SWCLK)** stay for the debug connection.
