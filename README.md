# PUK ESP32 DAC ECU Simulator — Arduino IDE

เวอร์ชันนี้จัดทำเป็น Arduino Sketch สำหรับเปิด ตรวจสอบ และ Upload ผ่าน
**Arduino IDE 2.x** โดยตรง ไม่ต้องใช้ PlatformIO

- Stand-alone 5 โหมด
- Wi-Fi Web Application 5 โหมด
- ESP32 internal DAC ที่ GPIO25/26
- Board Performance / Self-Test
- CKP/CMP แบบ Generic 36-1 สำหรับการพัฒนาเท่านั้น

เปิดไฟล์:

[`arduino/PUK_ESP32_DAC_Simulator/PUK_ESP32_DAC_Simulator.ino`](arduino/PUK_ESP32_DAC_Simulator/PUK_ESP32_DAC_Simulator.ino)

อ่านขั้นตอน Upload และตารางโหมด:

[`arduino/PUK_ESP32_DAC_Simulator/README.md`](arduino/PUK_ESP32_DAC_Simulator/README.md)

## Standalone LCD Edition

เวอร์ชันใช้งานบนตัวกล่องโดยไม่ต้องใช้โทรศัพท์หรือคอมพิวเตอร์ เพิ่ม LCD I²C 20×4,
ปุ่ม START/STOP, MODE, TEST, VR ปรับ RPM/load และ Generic ECU Health Test:

[`arduino/PUK_ESP32_Standalone_LCD/README.md`](arduino/PUK_ESP32_Standalone_LCD/README.md)

เอกสารต่อวงจรและภาพรวมระบบ:

[`docs/standalone-lcd-wiring.md`](docs/standalone-lcd-wiring.md) ·
[`docs/standalone-lcd-wiring.svg`](docs/standalone-lcd-wiring.svg)

สถานะ: ตรวจโครงสร้างโค้ดแล้ว แต่ยังไม่ได้ยืนยันการ Compile ใน Arduino IDE,
ยังไม่ได้วัดด้วย Oscilloscope และยังไม่ได้ทดสอบกับ ECU จริง
