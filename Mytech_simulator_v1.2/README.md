# Mytech_simulator_v1.2

R and d for esp 32 wifi by Mytech Product

Wi-Fi: **Mytech_simulator_v1.2**, password **12345678**, URL **http://192.168.4.1**. Firmware internal version remains 0.3.1-dev. On 4 October 2026, ESP32-D0WD-V3 / 4MB on COM7 was flashed successfully; boot log confirmed AP config/start OK. Power-on AP starts automatically; health check runs every 5 seconds. Long-duration Wi-Fi and ECU waveform tests remain pending.

สถานะล่าสุด 4 ตุลาคม 2026: ชุดพัฒนา POST API คอมไพล์ผ่านแล้ว (Core 3.3.11 / classic ESP32), มีหลักฐาน build และ QC parser/UI/IRAM แยกกัน ยังไม่ผ่าน HTTP transport หรือฮาร์ดแวร์จริง และยังไม่อนุมัติชุดลูกค้า หากกำลังแก้ source เพิ่ม ต้องใช้ผล build ที่ตรง revision นั้นก่อนรับงาน

ดู `recovery/AUTONOMOUS_WORK.md` สำหรับงานที่กำลังทำ และ `recovery/development-build-evidence.json` สำหรับ source/artifact hashes ของ build ล่าสุด

## เปิดตรวจงาน

- `development/preview/index.html` — หน้าตัวอย่างออฟไลน์ ดึงจาก UI จริง พร้อมสถานะจำลองและแถบระบุว่าไม่เชื่อมต่อบอร์ด
- `development/QC_IRAM_RAW_HANDLER_BUILD.md` — ผลตรวจ linked ELF ของชุดพัฒนาล่าสุด
- `customer-installer/tests/Release-Validation.Report.md` — ผลตรวจ guard ของตัวติดตั้ง ไม่ใช่ผลติดตั้งฮาร์ดแวร์

- `recovery/RECOVERY_REPORT.md` — สถานะจริง ที่มา ข้อที่กู้ไม่ครบ และแผน R&D
- `recovery/verification-results.json` — ตรวจข้อความที่บันทึกเทียบแชตผ่าน 16/16 ไฟล์ พร้อม SHA-256
- `recovery/board-identification.txt` — ผลอ่านรุ่นชิป COM10 รอบแรก: No serial data received
- `recovery/usb-ports.txt` — Windows เห็น CH9102 USB Serial ที่ COM10
- `recovery/compile-recovered-v0.3.0.log` — ผลคอมไพล์ฐานกู้ ต้องอ่านผลท้าย log ก่อนสรุป

## ชุดงาน

| โฟลเดอร์ | หน้าที่ | สถานะ |
|---|---|---|
| ESP32_WiFi_StandAlone_Test | ข้อความ v0.3.0 ที่กู้ตาม diff | รักษาฐานต้นทาง มีปัญหา timing จากโค้ด |
| recovery/recovered-v0.3.0 | สำเนาฐานกู้ก่อนพัฒนา | เก็บไว้เทียบเมื่อแก้ |
| development/ESP32_WiFi_StandAlone_Test | v0.3.1-dev ใช้ hardware timers | classic ESP32 candidate ยังไม่ผ่านบอร์ดจริง |
| customer-installer | ตัวติดตั้ง Windows + esptool ทางการ | ยังไม่มี firmware/release manifest ที่อนุมัติ Install ถูกล็อก |

## ทีม

ผู้จัดการดูแลขอบเขตและหลักฐาน ผู้ปฏิบัติกู้ไฟล์และพัฒนาชุดทดลอง ผู้ตรวจโค้ดอ่านต่างจากคนทำ และ QC ตรวจเกณฑ์รับงานแยกกัน ข้อพบและผลตรวจอยู่ในรายงานกู้และ DEVELOPMENT_CHANGELOG.md

## ลูกค้าจะติดตั้งอย่างไรเมื่อผ่าน QC

แตก ZIP → เสียบ USB → เปิด START_INSTALLER.cmd → เลือก COM → กด Install → เชื่อม Wi-Fi PUK_SIMULATOR_TEST รหัส12345678 → เปิด http://192.168.4.1 ไม่ต้องใช้ Arduino IDE/Python บนเครื่องลูกค้า

ยังไม่แจกเป็นชุดสำเร็จรูปจนกว่าจะยืนยันรุ่นชิป, สร้าง firmware ที่ compile ผ่าน และ QC การติดตั้ง/บูต/Web UI/รูปคลื่น ชุดปัจจุบันไม่เขียนบอร์ดเพราะไม่มี release manifest ที่อนุมัติ

งานนี้ยังไม่มีการ upload, erase flash, commit, push หรือเปลี่ยนเครือข่าย Wi-Fi ของคอม
