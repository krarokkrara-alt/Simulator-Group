# QC POST body validation — 4 ตุลาคม 2026

ผล: รับ source migration เพื่อ full compile และทดสอบ HTTP ต่อได้ Pure body parser และ embedded UI tests ผ่านอิสระ แต่ **ยังไม่ใช่ผล transport/hardware ผ่าน และยังไม่อนุมัติ Release** QC ไม่ full compile ไม่เข้าบอร์ด และไม่แก้ source ร่วมกับฝ่ายพัฒนา

## Source ที่ตรวจ (SHA256)

| File ใน ESP32_WiFi_StandAlone_Test | SHA256 |
|---|---|
| WebInterface.cpp | `1a69ae575110e36240e370bcbc67e19740693ede057e1028a49700615fcc8a77` |
| WebInterface.h | `866a00848eb4fd4fc556c232433e9bab63c59685394887b801e2662a860f36c1` |
| ApiValidation.h | `2d0946f3d436a47d25f5166e470520f047e566e3c3e5944e3c19ecf4c6ca73c8` |
| WebPage.h | `450494ef51007715b3442e6390612560ef790b056ffcd9ac0578b924e6b11404` |

## ตรวจอิสระที่รันแล้ว

- `xtensa-esp-elf-g++.exe -std=c++17 -Wall -Wextra -Werror -fsyntax-only development/tests/api_validation_compile_tests.cpp` ได้ exit 0 ครอบคลุมทุก field, range, malformed/duplicate/bare key, overflow, empty/trailing segment, extra equals, whitespace/encoding/non-ASCII, NUL ทุกตำแหน่งของ body ตัวอย่าง, 256/257 bytes และ output staging ไม่เปลี่ยนเมื่อ body ผิด
- `node development/tests/web_command_tests.mjs` ได้ exit 0 ใช้ script จริงจาก WebPage.h กับ DOM/network mock ตรวจ POST text/plain body, HTTP rejection, network failure, state refresh และคำสั่งสำเร็จหลัง error ไม่ได้เปิด browser หรือ server จริง

## ผลตรวจ handler และ vendor Core 3.3.11

1. `/api/set` ลงทะเบียน POST ก่อน HTTP_ANY fallback; GET และ method อื่นตอบ 405 พร้อม Allow: POST โดย fallback ไม่มี mutation ตาม source ต้องทดสอบ response ผ่าน HTTP จริงอีกครั้ง
2. คำสั่งมาจาก HTTPRaw chunks ใน buffer จำกัด 256 bytes ไม่ใช้ `arg("plain")` หรือ parsed query เลย จึงไม่มี query `plain` แย่ง body และไม่ apply query แม้ query เป็นคำสั่งหรือ bare key
3. RAW_START รีเซ็ตสถานะ buffer, RAW_WRITE จำกัด copy, RAW_ABORTED ไม่ complete และ RAW_END ระบุ complete; handler ตรวจ Content-Length ที่เป็นเลขฐานสิบ 1–256 ให้ตรง Core clientContentLength และ raw byte length ก่อน parse
4. NUL ใน raw body ยังคงอยู่ใน explicit length และถูก parseBody ปฏิเสธ ไม่อาศัย String(plainBuf) จึงไม่เกิด prefix truncation จาก C string แบบแนวทางเดิม ไม่มีการใช้ plain.length ใน candidate ใหม่นี้
5. ทุก 400/415 path คืนก่อน SimulatorState/sensor PWM/SELF TEST/SignalGenerator update; parseBody สร้าง staged Request แล้ว assign output เมื่อครบเท่านั้น `run=1&rpm=bad` และ `selftest=1&unknown` ใน BODY จึงไม่ commit บางส่วนตาม source
6. UI ส่ง POST, อ่าน HTTP status, แสดง commandError แยกจากสถานะ RUN/STOP และ refresh หลัง error ไม่อ้างว่าคำสั่งสำเร็จ

QC เคยแจ้งช่องว่าง multipart: callback ของ FunctionRequestHandler ใช้ร่วม upload/raw จึงอ่าน server.raw() โดยไม่มี guard ไม่ได้ Source hashes ล่าสุดเปลี่ยนเป็น custom `RawControlHandler` แยก raw/upload virtual dispatch โดย `canUpload` ทั้งสอง overload คืน false และ raw event ถูกส่งเป็น `HTTPRaw&` จาก Core ตรงไป `receiveControlBody(const HTTPRaw&)` ไม่ใช้ raw getter หรือ shared upload callback อีกแล้ว

ตรวจ signatures เทียบ `detail/RequestHandler.h` และ selection ใน Core `Parsing.cpp` แล้ว: override `canHandle` ทั้งสอง overload; overload ที่รับ WebServer รีเซ็ต capture ตอนเลือก POST ก่อนอ่าน headers/body; `canRaw`, `raw` และ `handle` ใช้ overload เดิมที่ไม่รีเซ็ต จึงไม่ล้างข้อมูลระหว่าง chunks/ตอน commit Multipart ที่ไม่เรียก raw ยังมี capture reset จาก selection และไม่สามารถ apply body เก่าได้ RAW_ABORTED ทำ complete=false; GET ไม่ match custom POST handler และไป 405 fallback ที่ไม่มี mutation ทั้งหมดนี้ผ่าน static review ยังต้องส่ง multipart/abort/GET ผ่าน HTTP จริงเพื่อตรวจ regression

QC รัน pure C++ assertions และ embedded UI tests ซ้ำหลัง custom-handler revision ทั้งสองได้ exit 0 ผลเหล่านี้ไม่ compile หรือทดสอบ custom handler กับ HTTP server จริง

## ข้อจำกัด framing ที่ยังต้องเปิดเผย

- Core อ่าน headers/body ก่อน handler และใช้ Content-Length.toInt(); raw path มี buffer คงที่ แต่ยังอ่าน/drain body ตามจำนวนที่ Core แปลได้ก่อน handler ตรวจ cap/รูปแบบ จึงไม่รับรองการ reject oversized/negative/invalid framing ตั้งแต่ต้น หรือป้องกัน resource/time exhaustion
- Core เก็บ collected header แบบ overwrite; duplicate/conflicting Content-Type หรือ Content-Length ไม่ได้ถูกตรวจครบจาก raw headers โดย candidate
- `hasHeader("Transfer-Encoding")` ของ Core หมายถึง collected value มีความยาวมากกว่า 0 จึง reject nonempty Transfer-Encoding แต่ยังไม่รับรอง detection ของ header ว่าง หรือค่าที่ duplicate header ทับไปแล้ว
- multipart ถูก Core parse ก่อนตอบ 415; guard ป้องกัน raw dereference ไม่ได้เปลี่ยน vendor multipart parser
- media type รับ lowercase `text/plain` (trim และละส่วนหลัง semicolon) โดยเจตนา; uppercase ไม่อยู่ใน contract นี้
- POST query ถูก ignore ตาม contract ไม่ได้มีการตรวจ raw query ทุกส่วน คำขอ body ถูกพร้อม query ผิดรูปแบบยังอาจสำเร็จได้ตามนโยบายนี้

## เกณฑ์ค้างก่อนรับ HTTP/Release

- [ ] Full firmware compile ของ source hashes นี้ พร้อม exit code/artifact hashes ใหม่ (ผล IRAM report รุ่นก่อนยังไม่ใช่ hash binary ของ migration นี้)
- [ ] Core WebServer transport tests: GET405/no mutation; POST valid200; invalid body400; wrong/multipart415; NUL/truncated/empty/oversized body และ exact state/output before-after
- [ ] POST query/body collision รวม query plain และ bare key ต้องใช้เฉพาะ body; query-only ต้องไม่เปลี่ยน state
- [ ] ทดสอบ unsupported TE และ duplicate/conflicting framing บน transport จริง บันทึกผลและข้อจำกัดของ Core ไม่เหมาว่า reject ทุกแบบ
- [ ] Multipart callback regression รวม request แรกและหลัง valid raw POST
- [ ] ยืนยันชิปจริงและวัด waveform/STOP/SELF TEST/AP stress โดยได้รับอนุญาตก่อนเขียนบอร์ด

การทดสอบ installer SHA256 แยกจากงานนี้: QC รัน actual-function AST fixture ด้วย Windows PowerShell `-ExecutionPolicy Bypass` ได้ exit 0 สำหรับ correct/uppercase accept และ malformed/wrong/tampered reject ไม่เปิด installer หรือ COM; ผลนี้ไม่ใช่การทดสอบ flash

## เครื่องมือ HTTP regression — ตรวจอิสระเพิ่มเติม

QC อ่าน `tests/http_regression.py` แล้วรันด้วย bundled Python executable (คำสั่ง `python` ไม่อยู่ใน PATH ของ session นี้): default/dry-run ได้ exit 0 โดยไม่เปิด socket; `--self-test` ได้ exit 0 กับ 21 cases บน localhost fixture เท่านั้น ผลแยกอยู่ `tests/http_regression_qc_mock_report.json` มี `core_transport_verified=false` และ `hardware_verified=false`

QC รัน `tests/http_regression_tests.py` ได้ 6 tests ผ่าน exit 0 ครอบคลุม default ไม่เปิด socket, execute ต้องระบุ URL, nonlocal ปฏิเสธโดย default, rejected response แต่ state เปลี่ยนต้อง fail, invalid body ที่ได้ 200 ต้อง fail และ failed suite ต้องคืน nonzero/JSON passed=false Tests ชุดนี้ mock networking ไม่ใช่ Core parser

Tool ตรวจ state ก่อน/หลังแต่ละ case ไม่ยกเว้น command fields, ห้ามทำเมื่อ SELF TEST active, จำกัด request/response bytes/timeout และใช้เพียง STOP/RPM0 เป็นคำสั่ง valid ไม่มี START, sensor, profile หรือ SELF TEST mutation ใน fixture Case malformed บางอันยอมรับ disconnect/timeout แต่ยังต้องอ่าน state ใหม่ให้คงเดิม

ผล mock ยืนยัน plumbing/assertions ของเครื่องมือเท่านั้น Python HTTP fixture ไม่ใช่ firmware source และไม่ใช้ Espressif WebServer การอนุมัติ test นี้ไม่ครอบคลุม `--execute` ไปบอร์ดหรือ nonlocal target ต้องยืนยันชิป/firmware และได้รับอนุญาตทดสอบจริงก่อน Release checklist เรื่อง Core transport/hardware จึงยังไม่ถูกทำเครื่องหมายผ่าน
