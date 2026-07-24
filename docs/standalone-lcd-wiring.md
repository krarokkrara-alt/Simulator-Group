# การต่อวงจร — PUK ESP32 Standalone LCD v1.0.0

ดูภาพรวมที่ [standalone-lcd-wiring.svg](standalone-lcd-wiring.svg)

## Pin table

| Function | ESP32 | ต่อไปยัง | Protection/หมายเหตุ |
|---|---:|---|---|
| LCD SDA | GPIO21 | LV1 ของ I²C level shifter | ห้ามรับ pull-up 5V โดยตรง |
| LCD SCL | GPIO22 | LV2 ของ I²C level shifter | ฝั่ง HV ต่อ LCD backpack |
| MAP DAC | GPIO25 | ECU sensor input ผ่าน R 1kΩ | 0–3.3V เท่านั้น; RC 100nF optional |
| TPS DAC | GPIO26 | ECU sensor input ผ่าน R 1kΩ | 0–3.3V เท่านั้น |
| CKP logic | GPIO18 | R 1kΩ → base Q1 2N2222 | open-collector; emitter GND, collector ไป CKP |
| CMP logic | GPIO19 | R 1kΩ → base Q2 2N2222 | open-collector; emitter GND, collector ไป CMP |
| Start/Stop | GPIO32 | ปุ่ม NO → GND | INPUT_PULLUP |
| Mode | GPIO33 | ปุ่ม NO → GND | INPUT_PULLUP |
| Health Test | GPIO27 | ปุ่ม NO → GND | INPUT_PULLUP |
| RPM pot | GPIO36 | VR1 wiper | VR ปลายสองด้าน 3.3V/GND; input-only |
| Load pot | GPIO39 | VR2 wiper | VR ปลายสองด้าน 3.3V/GND; input-only |
| DAC feedback | GPIO34/35 | test jumper จาก GPIO25/26 | ต่อเฉพาะตอน board test; ไม่ต่อเมื่อขับ ECU |
| ECU response A/B | GPIO16/17 | PC817 collector | PC817 emitter GND; ฝั่ง ECU ต้องมี resistor ตาม voltage |

## Power และ interface

1. 12V bench supply ผ่าน fuse 2A, reverse-polarity protection และ buck converter 12V→5V
2. 5V จ่ายเข้า ESP32 VIN และ LCD VCC; 3.3V จาก ESP32 จ่าย LV ของ level shifter และ VR
3. LCD backpack 5V ต่อ SDA/SCL ผ่าน bidirectional I²C level shifter เท่านั้น
4. CKP/CMP collector ใช้ pull-up จากฝั่ง ECU ตามเอกสาร ECU; ห้ามใส่ 12V เข้า GPIO18/19
5. ECU output ที่จะวัดต้องผ่าน optocoupler/วงจร input protection ห้ามต่อ injector/IGT output เข้า ESP32 โดยตรง

## PC817 response input ตัวอย่าง

สำหรับสัญญาณ logic 12V ที่ยืนยันแล้ว: signal → resistor 2.2kΩ/0.5W → PC817 LED → signal ground. ฝั่ง ESP32: collector → GPIO16 หรือ 17, emitter → ESP32 GND. สำหรับ injector/ignition inductive waveform ต้องใช้วงจร clamp และค่าอุปกรณ์ที่ออกแบบตาม waveform จริง; ตัวอย่าง 2.2kΩ นี้ **ใช้แทนวงจร injector/coil protection ไม่ได้**

## Bench safety gate

ก่อนต่อ ECU ต้องยืนยัน connector orientation/pinout, power/grounds, fuse/current limit, 3.3V/5V/12V domains, CKP/CMP polarity and pull-up, ไม่มี active output ตอน boot, จุด emergency power-off และไม่ให้ DAC simulator ชนกับ ECU 5V reference.
