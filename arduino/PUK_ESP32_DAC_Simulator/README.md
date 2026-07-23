# Arduino IDE Upload

## Requirements

- Arduino IDE 2.x
- ESP32 board package by Espressif Systems
- Board: `ESP32 Dev Module` (classic ESP32 with DAC on GPIO25/26)
- USB upload speed: start with `115200`

## Upload

1. Open `PUK_ESP32_DAC_Simulator.ino`.
2. Change `#define VARIANT_ID 1` to the required mode, 1–10.
3. Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**.
4. Select the correct USB port.
5. Click **Verify**, then **Upload**.
6. Open Serial Monitor at `115200`.

## Modes

| ID | Mode | Control |
|---:|---|---|
| 1 | Stand-alone Rotary/Pot | GPIO36 RPM, GPIO39 load |
| 2 | Stand-alone Buttons | GPIO32 start, GPIO33 RPM step |
| 3 | Stand-alone Preset | GPIO32 start, GPIO33 preset |
| 4 | Stand-alone Diagnostic | Pots plus Serial report |
| 5 | Stand-alone Burn-in | Automatic sweep |
| 6 | Web Dashboard | Wi-Fi AP |
| 7 | Web Mobile | Wi-Fi AP |
| 8 | Web Lab | Wi-Fi AP |
| 9 | Web Training | Wi-Fi AP |
| 10 | Web Burn-in | Wi-Fi AP |

For Web modes, connect to `PUK_SIM_6` through `PUK_SIM_10`, password
`puk-sim-32`, then open `http://192.168.4.1`.

## Board test

Connect feedback jumpers only for the DAC feedback test:

- GPIO25 → GPIO34
- GPIO26 → GPIO35

Hold GPIO33 low during boot, send `T` through Serial Monitor, or use
**RUN FULL TEST** in Web mode. The report checks DAC feedback, ADC reading,
signal scheduling jitter, free heap, Wi-Fi state, and reset/watchdog reason.

## Safety

CKP/CMP is a generic 36-1 demonstration profile. Do not connect it directly to
an ECU until the target tooth pattern, CMP phase, voltage domain, connector
pinout, open-collector/level interface, fuse, current limit, and grounds have
been confirmed. Never apply 5 V or 12 V directly to ESP32 GPIO.

## Verification status

- Arduino sketch structure/static inspection: checked
- Arduino IDE compile: not yet verified in this environment
- Oscilloscope/logic analyzer: not tested
- ECU bench: not tested
