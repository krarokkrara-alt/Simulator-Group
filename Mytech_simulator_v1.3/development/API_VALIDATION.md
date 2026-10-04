# POST command validation — development, 4 ตุลาคม 2026

`/api/set` รับ POST โดยใช้ raw body เป็นแหล่งคำสั่งเพียงแห่งเดียว Query ทุกตัวถูก ignore ไม่ merge กับ body รวมถึง query ชื่อ `plain` หน้าเว็บเปลี่ยนเป็น POST แล้ว Client ภายนอกที่ใช้ GET ต้องย้ายตาม GET และ method อื่นที่ถึง route จะตอบ 405 พร้อม Allow: POST และไม่มีการเปลี่ยน state

## Body และการเปลี่ยนสถานะ

ส่ง Content-Type `text/plain` พร้อม Content-Length จริง และ body ASCII เช่น `run=1&rpm=1000` รองรับ MIME parameters เช่น `text/plain; charset=UTF-8` แต่ชื่อ media type ต้องเป็นตัวพิมพ์เล็กตามการจำแนกของ Core candidate นี้ Content-Type อื่น/ไม่มีตอบ 415 เมื่อถึง handler

ขนาด body ต้อง 1–256 bytes รับเฉพาะคู่ key=decimal digits คั่นด้วย `&` ตรวจทุกคู่ให้ผ่านก่อนเปลี่ยน SimulatorState, sensor PWM, SELF TEST หรือ SignalGenerator หากส่วนใดผิดตอบ 400 และไม่ commit ส่วนอื่น เช่น `run=1&rpm=bad`, `run=1&unknown`, `run=1&run=0` ไม่มี partial mutation

- `run`, `ckp`, `cmp`, `led`, `scope`, `selftest`: รับ `0`/`1` ตรงตัวเท่านั้น `selftest=0` เป็น no-op ไม่ยกเลิก self-test เดิม
- `rpm`: 0–Config::MAX_RPM (ปัจจุบัน 8000)
- `profile`: 0–จำนวนโปรไฟล์จริงลบหนึ่ง
- `tps`, `map`, `ect`, `iat`, `o2`: 0–100
- ตัวเลขทั่วไปยอมรับศูนย์นำหน้า แต่ไม่รับเครื่องหมาย, ช่องว่าง, ทศนิยม, exponent, suffix หรือ overflow และไม่ clamp ค่าเกินช่วง
- ปฏิเสธ unknown/duplicate keys, bare key, ค่าว่าง, segment ว่าง, trailing `&`, `=` เพิ่ม, percent encoding, `+`, whitespace, non-ASCII และ raw NUL

Pure parser จัดเก็บผลใน Request ชั่วคราวและ assign output เมื่อสำเร็จทั้ง body เท่านั้น ใช้ custom RequestHandler ที่แยก raw dispatch จาก upload dispatch ตาม API Core 3.3.11 โดย canUpload ทั้งสอง overload คืน false และส่ง HTTPRaw event ตรงให้ตัวรับ body ไม่เรียก server.raw() getter หรือ arg("plain") เก็บ body ตามจำนวน bytes จริงลง buffer คงที่ 257 bytes และ discard chunks ที่เกิน cap

เมื่อ Core เลือก POST handler ก่อนอ่าน headers จะ reset สถานะ body ทุกคำขอ แม้ multipart จะไม่เรียก raw callback จึงไม่ใช้ body ค้างจากคำขอก่อน การแยก dispatch นี้ตัดความเสี่ยง shared upload/raw callback dereference HTTPRaw ในเส้นทาง multipart ไม่อาศัย Content-Type guard เพื่อรับรองชนิด event

## Framing ที่ตรวจและข้อจำกัด

เมื่อถึง handler จะตรวจ Content-Length ที่ Core เก็บไว้เป็น decimal 1–256, ไม่มี suffix/เครื่องหมาย, ตรงกับ clientContentLength() และจำนวน bytes ที่ raw callback รับครบ รวมทั้งปฏิเสธ Transfer-Encoding ที่ค่าถูกเก็บไว้และไม่ว่าง body ที่มี NUL ไม่ผ่าน pure parser ไม่ถูกตัดด้วย String C-string constructor

ข้อจำกัดที่ตรวจจาก source vendor โดยยังไม่ได้ทดสอบ HTTP จริง:

- Core อ่าน Content-Length ด้วย toInt() และอ่าน body ก่อน handler ดังนั้นค่าใหญ่/ติดลบ/ผิดรูปแบบอาจทำให้ Core รอหรือ abort ก่อนถึงคำตอบ 400 ของเรา แม้ command buffer ของโครงการมีขนาดคงที่ ยังไม่ได้กำหนด resource limit ให้ HTTP parser ทุกเส้นทาง
- Core raw path มี HTTPRaw buffer คงที่ของ vendor; multipart ที่ไม่รองรับเดิน parser ของ Core ก่อนตอบ 415 ไม่ได้อ้างว่าเราป้องกัน allocation/read ทุกแบบก่อนเกิดขึ้น
- `_collectHeader()` overwrite header ชื่อซ้ำ จึงไม่ทราบจาก public API ว่ามี duplicate/conflicting Content-Length หรือ Content-Type มาก่อน ไม่รับรองการปฏิเสธ duplicate headers ทุกแบบ
- `hasHeader()` ตรวจจากความยาว value ดังนั้น Transfer-Encoding ว่างอาจมองไม่เห็น รวมถึง raw header ที่ถูก Core normalize/truncate ไปแล้วไม่สามารถกู้คืนได้ใน route
- truncated body ทำให้ raw callback RAW_ABORTED และ Core อาจปิดคำขอก่อน dispatch ไม่มีคำตอบ JSON 400 รับรองไว้ แต่ไม่ได้ commit คำสั่งใน callback
- POST query ไม่ใช่ command source จึงไม่ตรวจ malformed query และไม่ได้อ้างว่าปฏิเสธ malformed request target ทุกแบบ การรับรอง raw HTTP framing อย่างครบถ้วนต้องปรับ transport/framework หรือเพิ่ม header hook ที่เห็นข้อมูลจริงในงานแยก

ไม่แก้ vendor library ไม่เพิ่ม authentication ใน scope นี้ POST/text/plain ไม่ใช่การป้องกัน CSRF โดยตัวมันเอง

## UI และ validation ที่ทำจริงโดยไม่ใช้บอร์ด

`WebPage.h` ส่ง POST body และตรวจ HTTP status ก่อน render state เมื่อ API ปฏิเสธจะแสดง error ในพื้นที่แยกที่ polling ไม่ลบทิ้ง และ refresh state จริงแทนการอ้างว่าคำสั่งสำเร็จ การเลือกโปรไฟล์ไม่เปลี่ยนข้อมูลอธิบายแบบ optimistic ก่อน API สำเร็จ

Pure C++ constexpr/static_assert tests ใช้ parser จริง ตรวจทุก field, boundaries, malformed/duplicate/bare segments, overflow, NUL ทุกตำแหน่ง, cap 256/257 และ output snapshot ไม่เปลี่ยนเมื่อคำขอผสมผิด:

```powershell
& '.\tools\arduino-data\packages\esp32\tools\esp-x32\2601\bin\xtensa-esp-elf-g++.exe' -std=c++17 -Wall -Wextra -Werror -fsyntax-only '.\development\tests\api_validation_compile_tests.cpp'
```

ผล exit 0 ทุก assertion ผ่าน ไม่ใช้ wrapper xtensa-esp32-elf-g++ ที่ sandbox ปฏิเสธ path

UI tests execute JavaScript จริงจาก WebPage.h ด้วย DOM/fetch mocks ตรวจ POST body, HTTP 400, network failure, polling ไม่กลบ error และ success ครั้งถัดไป:

```powershell
node .\development\tests\web_command_tests.mjs
```

ผล exit 0 ยังไม่ได้ทดสอบ HTTP transport ผ่าน Core จริง, multipart dispatch, raw callback chunking หรือ GPIO บนบอร์ด ต้อง full firmware compile และ independent QC ต่อก่อน Release

ไม่มีการแก้ baseline recovered ไม่มีการอัปโหลด เปลี่ยน Wi-Fi หรือ push
