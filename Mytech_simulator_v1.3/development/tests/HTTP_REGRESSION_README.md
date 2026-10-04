# HTTP regression tool — ยังไม่ใช่ผลทดสอบบอร์ด

`http_regression.py` ใช้ Python standard library ค่าเริ่มต้น dry-run แสดงรายการคำขอโดยไม่เปิด socket ไม่เข้าบอร์ด และไม่เขียน report ถ้าไม่ได้ระบุ --report

คำสั่งตรวจออฟไลน์:

```powershell
python .\development\tests\http_regression.py
python .\development\tests\http_regression_tests.py
python .\development\tests\http_regression.py --self-test --report .\development\tests\http_regression_mock_report.json
```

`--self-test` สร้าง fixture ชั่วคราวบน 127.0.0.1 random port และปิดเมื่อจบ เป็น Python HTTP server อิสระ ไม่ใช้ Core WebServer/ESP32 parser จึงตรวจ plumbing ของเครื่องมือและ assertion เท่านั้น ไม่ใช้รับรอง HTTP บนบอร์ดหรือความพร้อม Release

ผลที่ทำจริง: dry-run exit 0 ไม่มี network; localhost mock ทั้ง 21 cases exit 0 และบันทึก http_regression_mock_report.json พร้อม core_transport_verified=false/hardware_verified=false ชุด tool safety tests ตรวจ default ไม่เปิด socket, --execute ต้องมี URL, nonlocal guard และการตรวจจับ status/state ผิดพร้อมผล failed/nonzero โดยไม่เข้าบอร์ด หากพบข้อผิดพลาด --report จะเขียนผล passed=false แทนทิ้ง report success เดิม

ตรวจ GET405, valid POST200, query/body authority, malformed/bare/duplicate/NUL body, body cap, wrong/missing Content-Type, missing/zero/invalid/negative Content-Length, truncated body และ nonempty Transfer-Encoding คำสั่งที่ถูกต้องมีเฉพาะ STOP/RPM0 ไม่มี START, self-test, sensor หรือ profile mutation fixture ไม่ได้จำลอง firmware ทุก feature

ทุก case อ่าน state ก่อนและหลัง เปรียบเทียบ snapshot โดยเว้นเฉพาะ uptime/uptimeMs/millis/timestamp; ห้ามเว้น running/rpm/sensor/flags ที่เป็นสถานะคำสั่ง คำขอแรกอนุญาตให้เปลี่ยนเฉพาะ running→false และ rpm→0 จากนั้นทุก case ต้องคง STOP/RPM0 และสถานะอื่นเดิม ถ้า SELF TEST ทำงานอยู่จะหยุดการตรวจเพื่อไม่ให้ค่าที่เปลี่ยนตามเวลาเกิดผลเท็จ

Raw requests ไม่เกิน 2048 bytes response ไม่เกิน 65536 bytes มี timeout เริ่มต้น 3 วินาที (ปรับได้ 0.1–10) Truncated/framing บาง case ยอมรับการปิด connection แทน HTTP400 แต่ยังต้องอ่าน state ใหม่และไม่เปลี่ยนคำสั่ง การที่ framing ได้400ใน Python fixtureไม่ได้แปลว่า Core จะตอบ400เหมือนกัน

## ใช้ในอนาคตเมื่ออนุมัติการทดสอบจริงแยกแล้ว

ต้องยืนยัน chip/board, firmware hash และผู้ใช้อนุญาตให้ส่ง STOP/RPM0 ก่อน เครื่องมือนี้ไม่เปลี่ยน Wi-Fi ไม่อัปโหลด และไม่ค้นหาบอร์ดอัตโนมัติ ไม่กำหนด IP บอร์ดเป็น default

`--execute` ต้องมี `--base-url` แบบ http root URL ชัดเจน โดยปกติรับเฉพาะ 127.0.0.1 เช่น mock fixture ที่ผู้ทดสอบเปิดเอง Nonlocal target ต้องเพิ่ม --allow-nonlocal และมีการอนุมัติทดสอบฮาร์ดแวร์ก่อน การระบุ flag ไม่ใช่หลักฐานการอนุมัติจากผู้ใช้ และ agent ไม่ควรรันเป้าหมายบอร์ดเองในงานปัจจุบัน

```powershell
python .\development\tests\http_regression.py --execute --base-url http://127.0.0.1:8080
```

Full state-changing tests เช่น START/RPMจริง/self-test/sensors/profiles และ waveform measurements ยังต้องแผนทดสอบกับการอนุมัติแยก ไม่รวมในเครื่องมือนี้ Duplicate/conflicting headers และ Transfer-Encoding ว่างเป็นข้อจำกัด vendor ที่ระบุใน API_VALIDATION.md ยังไม่มี claim ว่าผ่าน Core transport
