# Wiring — ESP32 DAC Simulator

## Pin allocation

| Function | ESP32 pin | Direction | Domain | หมายเหตุ |
|---|---:|---|---|---|
| DAC MAP | GPIO25 | Output | 0–3.3 V | ห้ามต่อโหลดหนักโดยตรง |
| DAC TPS | GPIO26 | Output | 0–3.3 V | ห้ามป้อนแรงดันเข้าขานี้ |
| MAP feedback | GPIO34 | Input | 0–3.3 V | Jumper จาก GPIO25 เฉพาะตอน Self-Test |
| TPS feedback | GPIO35 | Input | 0–3.3 V | Jumper จาก GPIO26 เฉพาะตอน Self-Test |
| CKP logic | GPIO18 | Output | 3.3 V logic | ต้องผ่านวงจร Open-collector/Level interface ก่อน ECU |
| CMP logic | GPIO19 | Output | 3.3 V logic | ต้องผ่านวงจร Open-collector/Level interface ก่อน ECU |
| Start/Stop | GPIO32 | Input | 3.3 V | ปุ่มลง GND, ใช้ Pull-up ภายใน |
| Mode/Test | GPIO33 | Input | 3.3 V | กดค้างตอน Boot เพื่อ Self-Test |
| RPM Pot | GPIO36 | Input | 0–3.3 V | Pot 10 kΩ |
| Load Pot | GPIO39 | Input | 0–3.3 V | Pot 10 kΩ |
| Status LED | GPIO2 | Output | 3.3 V | LED บนบอร์ดบางรุ่น |

## Self-Test feedback

ต่อชั่วคราว:

```text
GPIO25 (DAC MAP) ─── GPIO34 (ADC MAP feedback)
GPIO26 (DAC TPS) ─── GPIO35 (ADC TPS feedback)
GND simulator     ─── GND measurement
```

หากไม่ต่อ Jumper ผล DAC/ADC จะแสดง `NOT_TESTED` ไม่ใช่ `FAIL`

## Safety gate ก่อนต่อ ECU

- ยืนยัน Pinout และ Connector orientation จากเอกสารของ ECU รุ่นนั้น
- ใช้ Fuse และ Current-limited supply
- CKP/CMP ต้องผ่านวงจร Interface ที่เหมาะกับ Open-collector/VR/Hall ของ ECU
- ห้ามนำ 12 V เข้า GPIO หรือ ADC/DAC ของ ESP32
- วัด Output ด้วย Oscilloscope ก่อนต่อ ECU
- กำหนดกราวด์ร่วมและตรวจว่าไม่มีแรงดันย้อนจาก ECU 5 V reference

