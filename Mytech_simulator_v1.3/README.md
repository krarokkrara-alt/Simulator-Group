# Mytech_simulator_v1.3

R and d for esp 32 wifi by Mytech Product

Wi-Fi: **Mytech_simulator_v1.3**, password **12345678**, URL **http://192.168.4.1**. Firmware development version **0.4.0-dev** (replaces v1.2 / 0.3.1-dev).

## ใหม่ใน v1.3: เลือกยี่ห้อ → รุ่น ผ่าน Wi-Fi แล้ว CKP/CMP เปลี่ยนตามรุ่นจริง

- หน้าเว็บมี dropdown **Brand** และ **Model / Engine** แยกกัน พร้อมรูปคลื่น CKP/CMP 720° ของรุ่นที่เลือก
- รูปคลื่นสร้างจากตาราง `PROFILES[]` ใน `development/ESP32_WiFi_StandAlone_Test/VehicleProfiles.cpp` (จำนวนฟัน, ฟันหาย, ตำแหน่งพัลส์ CMP) ไม่ใช่แค่ป้ายชื่ออีกต่อไป
- API เดิม `POST /api/set` body `profile=<id>` ใช้ได้เหมือนเดิม; `GET /api/profiles` เพิ่มฟิลด์ `teeth`, `missing`, `cams`, `verified`, `source`

| ยี่ห้อ / รุ่น | เครื่อง | CKP | CMP | สถานะข้อมูล |
|---|---|---|---|---|
| Toyota Hilux Vigo / Fortuner / Innova | 1KD/2KD-FTV | 36-2 | 1 พัลส์/720° | ยืนยันจากคู่มือ DENSO CRS |
| Isuzu D-Max / MU-7 | 4JJ1-TC | 60-4 (56 ฟัน, ช่องว่าง 24°) | 5 พัลส์/720° | ยืนยันจำนวนฟันจากคู่มือ Isuzu; มุม reference ประมาณ |
| Mitsubishi Triton / Pajero Sport | 4D56 Di-D | 36-2 (สมมติ) | 5 พัลส์/720° (30-180-180-180°) | **CKP ยังไม่ยืนยัน** |
| Honda Civic FD | R18A | 60-2 (placeholder) | 1 พัลส์ | **ยังไม่ยืนยัน** |
| Nissan Navara D40 | YD25DDTi | 60-2 (placeholder) | 1 พัลส์ | **ยังไม่ยืนยัน** |
| Generic | Template | 60-2 / 36-1 / 36-2 | 1 พัลส์ 90° | template |

รุ่นที่ยังไม่ยืนยันจะแสดงคำเตือนสีแดง **UNVERIFIED** บนหน้าเว็บ ห้ามใช้กับ ECU จริงจนกว่าจะใส่ข้อมูลฟันจากคู่มือซ่อมและวัดด้วย oscilloscope แล้ว มุมเฟส CMP เทียบช่องว่าง CKP เป็นค่าประมาณทุกรุ่น ต้องวัดเทียบกับเครื่องจริงก่อนรับรอง

ยังไม่ได้ compile v0.4.0-dev บน arduino-cli และยังไม่ได้ทดสอบบนบอร์ด ผ่านเฉพาะเทสต์ UI/preview ด้วย node

ทีม R&D (agent) อยู่ที่ `.claude/agents/` ใน root ของ repo: `rd-manager`, `rd-researcher`, `rd-firmware-engineer`, `rd-code-reviewer`, `rd-qc`

## ประวัติจาก v1.2

On 4 October 2026, the v1.2 firmware (0.3.1-dev) on ESP32-D0WD-V3 / 4MB on COM7 was flashed successfully; boot log confirmed AP config/start OK. Power-on AP starts automatically; health check runs every 5 seconds. Long-duration Wi-Fi and ECU waveform tests remain pending.

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
