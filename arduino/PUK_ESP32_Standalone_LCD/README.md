# PUK ESP32 Standalone LCD ECU Simulator v1.0.0

Arduino IDE sketch สำหรับ ESP32 classic แบบทำงานได้โดยไม่ใช้โทรศัพท์หรือคอมพิวเตอร์ มี LCD I²C 20×4, ปุ่มควบคุม, VR และ Health Test บนตัวเครื่อง

## ความสามารถ

| Mode | การทำงาน |
|---|---|
| ROTARY | VR1 ปรับ RPM และ VR2 ปรับ load |
| PRESET | เลือกชุด 800/1500/2500/4000 RPM |
| DIAGNOSTIC | แสดง RPM, voltage และ timing jitter |
| BURN-IN | Sweep 800–4000 RPM อัตโนมัติ |
| ECU HEALTH | Generic response test และแสดง GOOD/CHECK/BAD/NOT VERIFIED |

ผล `GOOD` หมายถึงผ่านเฉพาะรายการที่ต่อวัดในครั้งนั้น ไม่ใช่การรับรองว่า ECU สมบูรณ์ทุกวงจร ส่วน profile CKP/CMP เป็น generic 36-1; ข้อมูลของ ECU เฉพาะรุ่น **ต้องยืนยันจากเอกสาร/การวัดจริง**

## Arduino IDE

1. ติดตั้ง ESP32 board package by Espressif Systems
2. ติดตั้ง library `LiquidCrystal_I2C`
3. เปิด `PUK_ESP32_Standalone_LCD.ino`
4. เลือก `ESP32 Dev Module` และ Upload speed 115200
5. Verify และ Upload แล้วเปิด Serial Monitor 115200

## การควบคุม

- START/STOP GPIO32: เริ่มหรือหยุด outputs
- MODE GPIO33: เปลี่ยนโหมด; ใน PRESET ขณะ RUN ใช้เปลี่ยน preset
- TEST GPIO27: เริ่ม Health Test 800 และ 1500 RPM
- ทุก output เป็น OFF ตอน boot และเมื่อกด STOP

## เอกสาร

- [ตารางสาย, BOM และคำอธิบายวงจร](../../docs/standalone-lcd-wiring.md)
- [แผนผังวงจร SVG](../../docs/standalone-lcd-wiring.svg)
- [Test record](../../docs/standalone-lcd-test-record.md)

## สถานะการตรวจสอบ

- ตรวจ source/pin conflict และ fail-safe แบบ static แล้ว
- ยังไม่ได้ Compile ใน Arduino IDE ในสภาพแวดล้อมนี้
- ยังไม่ได้วัด waveform ด้วย oscilloscope/logic analyzer
- ยังไม่ได้ทดสอบกับ ECU จริง

สถานะ release: **partially verified / not bench-tested**
