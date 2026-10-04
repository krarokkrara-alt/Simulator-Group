# Transport validation plan — Core 3.3.11

สถานะล่าสุด: implement POST strict-body ใน candidate แล้วโดยใช้ raw callback และ bounded buffer แทน arg("plain") ไม่แก้ vendor ดูผล pure parser/UI tests และข้อจำกัด framing ใน API_VALIDATION.md ยังไม่ทดสอบ HTTP transport จริงหรือบอร์ด รายละเอียดด้านล่างเป็นแผนและหลักฐานก่อน implementation ไม่ใช่ผลทดสอบทั้งหมด

## ช่องว่างที่ยืนยันจาก source

`WebServer/src/Parsing.cpp` บรรทัด 102–110 แยก query เป็น local `searchStr` แล้วเก็บเฉพาะ path ใน `_currentUri`
`uri()` จึงคืน path ไม่ใช่ request target เต็ม
`_parseArguments()` บรรทัด 330–359 ข้าม segment ที่ไม่มี `=` ก่อนสร้างรายการ arg
เช่น GET `/api/set?run=1&unknown` ทำให้ handler เห็นเฉพาะ `run=1` จึงอาจเปลี่ยน state แม้มี bare unknown segment
validation ของ decoded args อย่างเดียวไม่สามารถกู้ข้อมูลที่ parser ทิ้งไปแล้ว

Public API มี `args`, `arg`, `argName`, `hasArg`, `uri`, `client`, `collectHeaders`, middleware และ RequestHandler hooks
แต่ไม่พบ public getter สำหรับ raw query และไม่มี hook รับ raw request line ก่อน parser แยก query
`_parseRequest` / `_parseArguments` เป็น protected และไม่ได้เป็น virtual override point
การอ่าน `client()` ใน route มาชดเชยไม่ได้: request line ถูก consume แล้ว; อาจไปอ่าน body/คำขอถัดไปแทน
middleware และ custom handler ใช้ `_currentUri` หลังแยก query จึงไม่คืน bare segment
HTTPRaw/canRaw hook ให้ raw BODY เป็น chunks ไม่ใช่ raw QUERY
ไม่เสนอแก้ vendor library หรือคัดลอก HTTP parser ทั้งชุดเพื่อ gap นี้

## ข้อเสนอ scope เล็ก

เปลี่ยน control endpoint เป็น POST `/api/set` และใช้ body เป็นแหล่งคำสั่งเพียงแห่งเดียว
GET `/api/set` ต้องตอบ 405 + Allow: POST และไม่เปลี่ยน state; state/profiles endpoints ยังคง GET
ใช้ `Content-Type: text/plain` และ body ASCII แบบ `run=1&rpm=1000` ที่ parse ด้วย strict tokenizer ของโครงการ
ไม่ใช้ application/x-www-form-urlencoded เพราะ Core จะรวม query/body แล้วเรียก parser เดิมที่ทิ้ง bare segment
ไม่ต้องเพิ่ม JSON dependency; JSON เป็นอีกตัวเลือกแต่ต้องเลือก parser ที่ปฏิเสธ duplicate keys และ trailing content อย่างชัดเจน

Body grammar แนะนำ: key จาก allowlist แบบตรงตัว, `=`, decimal digits; คั่นด้วย `&`; มีอย่างน้อยหนึ่งคู่
ไม่ยอมรับ whitespace, percent encoding, '+', non-ASCII, embedded NUL, empty segment, trailing '&', bare key, duplicate key หรือ '=' เพิ่ม
จากนั้นนำคู่ที่ parse ครบแล้วเข้า ApiValidation::Request เดิม ตรวจ range/boolean ทั้งหมดก่อน mutate state
กำหนด body cap เช่น 256 bytes (รองรับครบทุก field) และตรวจ Content-Type/Content-Length/body length ก่อน validation

ทางเลือกเล็กที่สุดคือ `server.arg("plain")` แต่ Core สร้าง `String(plainBuf)` ด้วย C string
จึงต้องตรวจ body String.length() เท่ากับ `clientContentLength()` เพื่อปฏิเสธ body ที่ถูกตัด ณ embedded NUL
ต้องยืนยันด้วย transport test ว่า body เป็น byte length, header parse และ EOF behavior ตรงที่คาด ไม่ใช่ทดสอบ pure validator อย่างเดียว
Route-level cap ตรวจหลัง Core allocate/read body แล้ว จึงเป็น cap ของคำสั่ง ไม่ใช่การป้องกัน allocation ของ HTTP parser
หากต้องป้องกัน allocation ตั้งแต่ต้น ใช้ custom RequestHandler `canRaw/raw` + bounded chunk buffer และ discard oversized body; scope ใหญ่กว่าแต่ยังไม่แก้ vendor

Query ใน POST ต้องมี contract ชัดเจน: ไม่ใช่ command source, ถูก ignore ทั้งหมด และไม่ถูก apply หรือ merge กับ body
POST body ที่ถูกต้องพร้อม bare query จึงยังผ่านตาม contract นี้; อย่าอ้างว่า reject malformed query ทั้งหมด
ตรวจ parsed query args ไม่ให้ถูกนำไปเป็นคำสั่ง (อ่านเฉพาะ raw body ที่เลือกไว้)
ถ้าข้อกำหนดคือ “query ใดก็ตามรวม bare query ต้องได้ 400” public API ชุดนี้ไม่เพียงพอ ต้องมี request-target hook/proxy หรือเปลี่ยน HTTP server framework และทบทวน scope ใหม่
การเปลี่ยน POST ปิด gap สำหรับ GET malformed control commands แต่ไม่ได้สร้าง raw-query visibility

## UI migration และ tradeoff

เปลี่ยน WebPage.h `cmd(k,v)` จาก fetch query เป็น fetch POST: header text/plain, body key=value, cache no-store
อ่าน response status ก่อน refresh และแสดง error จาก 4xx; ไม่แสดงว่าสั่งสำเร็จเมื่อ API ปฏิเสธ
UI สร้างเฉพาะ allowlist key และค่า decimal; API ยังต้องตรวจทุกครั้ง
Client ภายนอกที่ใช้ GET เดิมต้องย้ายเป็น POST; ไม่ควรเปิด GET compatibility mutation เพราะจะคง gap เดิม
POST กับ text/plain ไม่เพิ่ม authentication และไม่ใช่มาตรการ CSRF โดยตัวมันเอง; ไม่ขยายงาน authentication ใน scope นี้

## Tests ที่จำเป็นก่อนรับงาน

1. Strict-body unit tests: every valid field/min/max, overflow, invalid boolean, unknown/duplicate/bare key, empty body/segment, trailing &, extra '=', whitespace, percent/+ encoding, non-ASCII และ NUL ทุกตำแหน่ง
2. Atomicity: `run=1&rpm=bad`, `run=1&unknown`, duplicate และ out-of-range ต้อง 400 โดย state/output snapshot ไม่มีการเปลี่ยนบางส่วน
3. HTTP transport tests ผ่าน Core WebServer จริงหรือ harness ที่ใช้ parser จริง: GET `?run=1&unknown` ต้อง405/no mutation; valid POST ต้อง200; malformed bodyต้อง400
4. Mixed query/body: POST `?run=1` + body `run=0` ใช้เฉพาะbody; POST `?unknown` + validbody เป็นผลตาม contractignore; query-only/no bodyต้อง400
5. Content handling: wrong/missing Content-Type, form-urlencoded/multipart, absent/zero/negative/invalid/oversized Content-Length, truncated body, embedded raw NUL, conflicting/duplicate headers และ Transfer-Encoding; unsupported framingต้องไม่mutate
6. UI tests: buttons/sliders/selftestส่งPOST, 4xxแสดงerror, status/profilesGETยังทำงาน; กด STOP/RPM0 แล้ว outputs snapshot LOW โดยไม่มี side effect ก่อน validation
7. Compile candidate + ดู scope diff; ทดสอบ timing HTTP/Wi-Fi stress และ waveformจริงภายหลังบนบอร์ดที่ยืนยันชิปแล้วแยกจาก host tests

มีผล pure parser compile-time assertions และ UI mocked-network tests แล้วใน API_VALIDATION.md ส่วน HTTP transport ผ่าน Core จริง, framing และ hardware stress tests ยังไม่ทำ

แหล่งหลักฐาน: local vendor `tools/arduino-data/packages/esp32/hardware/esp32/3.3.11/libraries/WebServer/src/{WebServer.h,Parsing.cpp}` และ candidate `development/ESP32_WiFi_StandAlone_Test/{WebInterface.cpp,WebPage.h}`
