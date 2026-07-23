# PUK ESP32 DAC ECU Simulator — 10 Variants

ชุดโปรแกรม ECU Simulator สำหรับ ESP32 ที่มี DAC ภายใน 2 ช่อง รองรับทั้ง **Stand-alone 5 แบบ** และ **Wi‑Fi Web Application 5 แบบ** พร้อมระบบตรวจประสิทธิภาพบอร์ด (Board Health / Self-Test)

## สิ่งที่มีในรุ่น v0.1.0

- Build Environment 10 รูปแบบ
- CKP/CMP ตัวอย่าง Generic 36-1
- MAP/TPS analog output ผ่าน ESP32 DAC GPIO25/26
- Stand-alone control ด้วยปุ่มและ Potentiometer
- Web App ผ่าน ESP32 Access Point ไม่ต้องใช้ COM Interface ตอนใช้งาน
- ALL OFF เมื่อสั่งหยุดหรือ Web client timeout
- Board Self-Test: DAC feedback, timing jitter, heap, Wi‑Fi และ reset/watchdog
- เอกสาร Wiring, Variant matrix, Health criteria และ Test record

## เริ่มต้น

1. ติดตั้ง VS Code + PlatformIO
2. เปิดโฟลเดอร์นี้
3. เลือก Environment จาก `platformio.ini`
4. Build และ Upload
5. อ่าน [Wiring](docs/WIRING.md) ก่อนต่ออุปกรณ์

Web mode:

1. เชื่อม Wi‑Fi `PUK_SIM_6` ถึง `PUK_SIM_10`
2. Password: `puk-sim-32`
3. เปิด `http://192.168.4.1`

Stand-alone:

- GPIO32: Start/Stop
- GPIO36: RPM Pot
- GPIO39: Load/MAP Pot
- กด GPIO33 ค้างตอน Boot หรือส่ง `T` ผ่าน Serial เพื่อรัน Self-Test

## การตรวจสอบ

```bash
python tools/static_check.py
pio run -e web_dashboard
```

ดูรายละเอียด [รูปแบบทั้ง 10](docs/VARIANTS.md), [Board Health](docs/BOARD_HEALTH.md) และ [Test Record](docs/TEST_RECORD.md)

## คำเตือน

โค้ด CKP/CMP เป็น **Generic demonstration profile** เท่านั้น ไม่ใช่ข้อมูลของ ECU รุ่นใดรุ่นหนึ่ง ต้องยืนยัน Tooth pattern, phase, voltage level, Pinout และวงจรป้องกันก่อนต่อ ECU จริง ห้ามต่อสัญญาณ 12 V เข้าขา ESP32 โดยตรง

## Verification status

- Static project checks: **PASS**
- PlatformIO compile: **ยังไม่ได้ยืนยันใน environment นี้**
- Oscilloscope/logic analyzer: **ยังไม่ได้ทดสอบ**
- ECU bench: **ยังไม่ได้ทดสอบ**

