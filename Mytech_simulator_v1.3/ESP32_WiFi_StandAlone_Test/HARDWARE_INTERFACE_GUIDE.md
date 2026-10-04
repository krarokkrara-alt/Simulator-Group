# ESP32 ECU Sensor Interface Board — Bench Test Guide

เอกสารนี้เป็นวงจรต้นแบบสำหรับ **ทดสอบ ECU บนโต๊ะด้วยแหล่งจ่าย 12 V แบบจำกัดกระแส** ไม่ใช่วงจรสำหรับติดตั้งถาวรในรถ และไม่ใช่วงจรที่ผ่านมาตรฐาน automotive transient/EMC

> ห้ามต่อ GPIO ของ ESP32 เข้าขา ECU โดยตรง ก่อนต่อทุกครั้งต้องทราบ pinout, แรงดัน pull-up, sensor ground และชนิด input จาก wiring diagram ของ ECU รุ่นนั้น

## 1. สถาปัตยกรรม

```text
ESP32 3.3 V
 ├─ CKP/CMP/VSS digital ──> Module A: open-collector / 5 V push-pull
 ├─ SPI DAC data ─────────> Module B: bipolar differential VR output
 ├─ SPI DAC data ─────────> Module C: buffered 0–5 V analog outputs
 └─ GPIO select ──────────> Module D: switched resistance for ECT/IAT

12 V bench supply
 └─ fuse + reverse protection + TVS
     ├─ 5 V regulator ──> DAC / logic / analog sensors
     └─ isolated ±12 V ─> VR output amplifier only
```

## 2. Power and protection

Recommended input chain for a current-limited 12 V bench supply:

```text
12V_BENCH --- F1 1A --- reverse-polarity P-MOSFET --- +12V_PROTECTED
                         |
                         +--- SMBJ18A TVS --- GND

+12V_PROTECTED --- 5V buck regulator (>=1 A) --- +5V_INTERFACE
+12V_PROTECTED --- isolated ±12 V DC/DC -------- +12V_A / 0V_A / -12V_A
```

- Put `100 nF + 10 µF` at every IC supply group.
- Use a star point for `ECU_SENSOR_GND`, interface ground and bench-supply ground.
- Start with the bench supply current limit at `0.2–0.5 A`.
- The ESP32 may be powered from USB during development, but do not create uncontrolled ground loops between the PC, oscilloscope and ECU.

## 3. Module A — Hall effect / open-collector CKP, CMP and VSS

Most Hall inputs expect a digital signal. Many ECUs provide their own pull-up, so an open-collector output is the safest first option.

```text
ESP_GPIO --- R1 2.2k ---B  Q1 2N3904 / MMBT3904
                         C---- R3 220R ---- ECU_HALL_SIGNAL
SENSOR_GND --------------E
                         |
                    R2 47k B-to-E

ECU_HALL_SIGNAL --- optional 10k pull-up --- ECU_5V
ECU_SENSOR_GND ----------------------------- SENSOR_GND
```

Per channel BOM:

- Q1: `MMBT3904` or another transistor rated at least 30–40 V
- R1: `2.2 kΩ`, R2: `47 kΩ`, R3: `220 Ω`
- Optional ECU-side pull-up: `10 kΩ to 5 V`; fit only if the ECU input has no internal pull-up
- Optional protection: low-capacitance TVS selected after measuring the ECU pull-up voltage

Behaviour: ESP high turns Q1 on and pulls the ECU signal low, so the signal is inverted in hardware. Compensate in firmware if phase polarity matters.

### Optional 5 V push-pull output

For an ECU that explicitly requires a 0/5 V driven signal, use a `74AHCT125` powered at 5 V:

```text
ESP_GPIO --> 74AHCT125 input
74AHCT125 output --> 100R --> ECU_DIGITAL_INPUT
OE tied low through 10k; SENSOR_GND common
```

Do not use push-pull mode until the ECU input is confirmed not to have a conflicting pull-up or output driver.

## 4. Module B — Magnetic VR CKP/CMP emulator

A VR input expects a waveform crossing below and above zero. A 0/3.3 V GPIO waveform is not a VR signal. Use a fast SPI DAC and a bipolar differential amplifier.

### DAC

Use one `MCP4922` channel per waveform, powered at 5 V with a stable 5.000 V reference. Connect SPI `SCK`, `MOSI`, `CS` and `LDAC` to the ESP32 through 3.3 V-compatible inputs as permitted by the selected DAC supply/logic thresholds; otherwise add a 3.3-to-5 V logic buffer.

The DAC produces `0–5 V` with the idle centre at `2.500 V`.

### Differential output stage

Use an `OPA2197` or two `OPA197` devices powered from isolated `±12 V`.

```text
                    U1A non-inverting level/scale stage
DAC 0..5 V ----> [ VOUT+ = 2 × (VDAC - 2.5 V) ] ---- 100R ---- VR+

                    U1B inverted level/scale stage
DAC 0..5 V ----> [ VOUT- = -2 × (VDAC - 2.5 V) ] --- 100R ---- VR-
```

Use matched `10.0 kΩ / 20.0 kΩ`, 0.1% resistor networks for subtraction and gain. With this scaling:

- DAC = 2.5 V → VR+ = 0 V, VR− = 0 V
- DAC = 3.0 V → VR+ ≈ +1 V, VR− ≈ −1 V; differential ≈ +2 V
- DAC = 2.0 V → VR+ ≈ −1 V, VR− ≈ +1 V; differential ≈ −2 V

Add `100 Ω` in series with each output and a removable `10 kΩ` resistor across VR+/VR−. Begin testing at `0.5–1.0 V peak differential`; increase only after checking the ECU service information. Never exceed the op-amp supply/output-current limits.

For galvanic isolation or a floating VR input, place a suitable signal transformer after a push-pull driver instead of connecting the op-amp grounds directly. Transformer bandwidth must cover cranking frequency through maximum RPM and must be verified on an oscilloscope.

## 5. Module C — TPS, MAP, MAF and narrowband O2 voltage outputs

Use three `MCP4922` dual DACs for up to six channels. Buffer each output with an `OPA4197`/`OPA197` channel powered at 5 V or a suitable rail-to-rail 5 V op-amp.

```text
ESP32 SPI --> MCP4922 DAC --> op-amp voltage follower --> 1k --> ECU_SENSOR_INPUT
                                                        |
                                                   10nF optional
                                                        |
                                                ECU_SENSOR_GND
```

Suggested software limits:

| Simulated sensor | Output range | Notes |
|---|---:|---|
| TPS | 0.5–4.5 V | Do not command 0 or 5 V unless testing a fault condition |
| MAP | 0.5–4.5 V | Convert kPa using the sensor transfer curve |
| Analog MAF | typically 0.5–4.5 V | Verify the exact sensor curve |
| Narrowband O2 | 0.1–0.9 V | High-impedance signal only |
| Generic analog | 0–5 V | Clamp range in firmware |

Use a `1 kΩ` series resistor on every output. Add Schottky clamps to the local 0 V and 5 V rails only if the chosen op-amp and ECU interface have been analysed for back-power current.

**Wideband O2 is not a simple voltage sensor.** Do not connect this analog output in place of a wideband pump cell/controller. Wideband simulation requires a dedicated controller emulator matched to the ECU.

## 6. Module D — ECT/IAT resistance emulator

ECT and IAT are commonly NTC resistors read by an ECU pull-up. A voltage DAC is not the correct universal replacement. Switch calibrated resistors between the ECU input and ECU sensor ground.

```text
ECU_TEMP_INPUT ----+---- AQY212 PhotoMOS ---- 100R ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 220R ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 470R ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 1.0k ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 2.2k ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 4.7k ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 10k  ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 22k  ---- SENSOR_GND
                   +---- AQY212 PhotoMOS ---- 47k  ---- SENSOR_GND
```

- Select **one resistor at a time** for a simple table-based emulator.
- Drive each PhotoMOS LED from 5 V through a transistor so the LED current meets its datasheet requirement; do not assume a 3.3 V GPIO alone is sufficient.
- The relay on-resistance must be included in calibration, especially below 220 Ω.
- Replace resistor values with values calculated from the exact NTC temperature/resistance table.
- A binary/parallel resistor network can give finer resolution, but its combinations must be calculated and validated before use.

## 7. Sensors requiring separate modules

| Sensor/input | Required interface |
|---|---|
| Frequency MAF / VSS | Module A open-collector |
| Knock/piezo | Arbitrary waveform source plus protected bipolar amplifier |
| Wideband O2 | Dedicated pump-cell/controller emulator |
| SENT | SENT protocol transmitter with correct timing and CRC |
| LIN | Automotive LIN transceiver, e.g. a dedicated LIN physical-layer IC |
| CAN sensor | CAN transceiver and sensor-specific message database |
| Active wheel-speed current sensor | Dedicated 7/14 mA current-loop emulator |

These cannot be safely represented by a generic 0–5 V output.

## 8. Connector and safety recommendations

- Put Hall, VR, analog and resistance outputs on separate keyed connectors.
- Use a jumper or rotary selector so Hall push-pull and VR outputs cannot reach the same ECU pin simultaneously.
- Add test points for every output and ground.
- Label `ECU 5V`, `ECU SENSOR GND`, `VR+`, `VR−` and `12V` clearly.
- First test every channel into a dummy load and oscilloscope, then into a spare ECU with a current-limited supply.
- Verify polarity, peak voltage, frequency and phase before enabling START.

## 9. Minimum bring-up order

1. Test the protected 12 V, 5 V and ±12 V rails with no ECU attached.
2. Test Hall open-collector into an external 5 V / 10 kΩ pull-up.
3. Test analog channels into a 10 kΩ dummy load.
4. Test the resistance bank with a multimeter over every selection.
5. Test VR+ to VR− on an oscilloscope at minimum and maximum RPM.
6. Connect one ECU input at a time using a fused/current-limited bench harness.

## 10. Firmware work still required

The current firmware outputs GPIO/PWM only. Building this interface board requires firmware changes for:

- SPI control of MCP4922 DAC devices
- bipolar VR waveform samples and adjustable amplitude
- calibrated voltage transfer curves
- ECT/IAT resistance-table selection and break-before-make relay timing
- per-profile pin, polarity and trigger-wheel settings

Do not assemble the complete board and connect it to an ECU until these firmware changes and the exact ECU electrical specifications are defined.
