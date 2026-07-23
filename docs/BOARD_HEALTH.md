# Board Performance Check

ระบบตรวจสุขภาพบอร์ดทำงานได้ทั้ง 10 รูปแบบ โดยกด MODE ค้างตอนเปิดเครื่อง, ส่ง `T` ผ่าน Serial หรือกด **RUN FULL TEST** ใน Web App

| Test | วิธีตรวจ | PASS | WARN/FAIL |
|---|---|---|---|
| DAC25 → ADC34 | สั่ง 1.00 V แล้วอ่านกลับ | error ≤ 0.22 V | คลาดเคลื่อนมากกว่าเกณฑ์ |
| DAC26 → ADC35 | สั่ง 2.00 V แล้วอ่านกลับ | error ≤ 0.22 V | คลาดเคลื่อนมากกว่าเกณฑ์ |
| Signal timing | เก็บค่า loop edge jitter | ≤ 120 µs | มากกว่า 120 µs |
| Memory | อ่าน Free Heap | ≥ 45,000 bytes | ต่ำกว่าเกณฑ์ |
| Wi‑Fi | ตรวจว่า Web mode เริ่ม AP | AP พร้อม | ไม่พร้อม/ไม่ทดสอบ |
| Watchdog/reset | อ่าน Reset reason | ไม่มี Panic/WDT reset | พบ Panic/WDT reset |

## ข้อจำกัด

- ESP32 DAC มีความละเอียด 8-bit และแรงดันเต็มสเกลจริงขึ้นกับบอร์ด/โหลด ต้อง Calibrate ด้วย Multimeter
- ADC ของ ESP32 ไม่เป็นเชิงเส้นสมบูรณ์ ค่าในโค้ดเป็น Screening Test ไม่ใช่มาตรฐานสอบเทียบห้อง Lab
- Timing test ใน Release นี้ตรวจ Software scheduling เบื้องต้น ควรยืนยัน Jitter ด้วย Oscilloscope/Logic Analyzer
- `PASS` ของ Self-Test ไม่ได้ยืนยันว่า ECU-specific waveform, voltage interface หรือ Pinout ถูกต้อง

