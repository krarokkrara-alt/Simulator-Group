# QC API validation — 4 ตุลาคม 2026

สถานะ: pure parser และการ staging ใน handler ผ่านการตรวจ source/compile assertions แต่ยังไม่รับรอง HTTP transport หรือ GPIO จริง และยังไม่อนุมัติ Release

## หลักฐานอิสระ

QC compile `tests/api_validation_compile_tests.cpp` ด้วย `tools/arduino-data/packages/esp32/tools/esp-x32/2601/bin/xtensa-esp-elf-g++.exe -std=c++17 -Wall -Wextra -Werror -fsyntax-only` ได้ compiler exit code 0 ไม่มีการเข้าบอร์ด Assertions ครอบคลุม boundary RPM/โปรไฟล์/เซนเซอร์, boolean encoding, overflow, ค่าว่าง, เครื่องหมาย, whitespace, decimal/exponent/suffix, unknown, duplicate และ mixed staging

ไฟล์ alias `xtensa-esp32-elf-g++.exe` ที่ลองก่อนหน้านี้ถูกสภาพแวดล้อมปฏิเสธการเข้าถึง path และจบ 101; จึงใช้ executable จริงข้างต้นในการตรวจที่สำเร็จ ไม่ใช่ผล source compile error

## ผลอ่าน handler

`WebInterface::handleSet()` สร้าง Request ภายในคำขอ ตรวจทุก argument จาก WebServer ก่อนแก้ SimulatorState, เริ่ม SELF TEST, เขียน sensor หรือเรียก SignalGenerator ทุกเส้นทาง 400 คืนก่อน commit จึงไม่เกิดผลข้างเคียงจาก parsed request เช่น `run=1&rpm=bad` หรือ `selftest=1&tps=101`

การตรวจ `String.length()` กับ `strlen()` อยู่ก่อน commit เพื่อปฏิเสธ embedded NUL แม้ pure parser อ่านเพียง prefix จาก c_str; อ่าน core 3.3.11 พบ URL decoder ต่อ decoded character ลง String และ `concat(char)` เก็บ length 1 จึงสอดคล้องกับแนวทางตรวจนี้ ผลนี้เป็นการอ่าน source ยังไม่ได้ส่ง encoded NUL ผ่าน HTTP จริง

`selftest=0` เป็น no-op ตามนโยบาย ไม่ยกเลิก SELF TEST เดิม คำขอที่ผ่านใช้ค่าเฉพาะช่องที่ส่งมา ค่า sensor/rpm ไม่ clamp คำขอเกินช่วงอีกต่อไป

## ข้อจำกัด transport ที่ต้องติดตาม

อ่าน `WebServer/src/Parsing.cpp` ของ core 3.3.11 พบ `_parseArguments` ข้าม segment ที่ไม่มี `=` เช่น `/api/set?run=1&rpm` อาจส่งเพียง `run=1` ให้ handler และเริ่ม RUN ดังนั้นการปฏิเสธ malformed/unknown ครอบคลุม arguments ที่ WebServer ส่งมาเท่านั้น ยังไม่ใช่การปฏิเสธ raw query ทุกแบบ ควรทดสอบและกำหนดนโยบาย URL ที่มี bare/trailing segment ก่อนอ้างว่าคำขอผิดรูปแบบถูกปฏิเสธทั้งชุด

## ตรวจเพิ่มเติมก่อน Release

- [ ] Full firmware compile ที่รวม ApiValidation.h ล่าสุด พร้อม exit code/log/source hash
- [ ] HTTP invalid/mixed/duplicate/unknown/empty/bare query พร้อมบันทึก state ก่อนและหลัง
- [ ] Encoded NUL ในชื่อและค่า เช่น `rpm=1%00bad` ต้องไม่เปลี่ยน state
- [ ] ตรวจ PWM/SELF TEST/START/STOP ไม่เกิดผลข้างเคียงจากคำขอที่ตอบ 400
- [ ] ตรวจ UI รับมือ HTTP 400 และเครือข่ายหลุดได้ตามพฤติกรรมที่ต้องการ
- [ ] ยืนยันรุ่นชิปและทดสอบฮาร์ดแวร์โดยได้รับอนุญาตก่อนเขียนบอร์ด

ระหว่างตรวจเห็น compile log ชุด development แสดงสรุปขนาด Sketch 953967 bytes และ global variables 47644 bytes แต่รายงานนี้ไม่สรุปว่า build ล่าสุดผ่านจนผู้จัดการตรวจ process exit code และยืนยันว่า log เป็น source revision ที่รวม validation นี้
