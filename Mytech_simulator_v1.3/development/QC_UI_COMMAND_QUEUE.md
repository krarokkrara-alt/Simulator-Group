# QC serialized UI commands — 4 ตุลาคม 2026

ผล: รับ queue revision เพื่อ compile/browser verification ต่อได้ QC ไม่แก้ source ไม่ full compile และไม่เข้าบอร์ด

WebPage.h SHA256 `716fcdb828be868cd3144378c22883a4013e5411ee7fb1cb154791d20c6358af`; preview/index.html SHA256 `1e18fafa7772a0063623cf92e276c413461cff82b33c78dfe21c2a13dbd87edd`

## ตรวจอิสระ

รัน command_queue_tests.mjs ผ่าน 10 deferred scenarios, active_slider_tests.mjs ผ่าน 9 scenarios, web_command_tests.mjs และ offline_preview_tests.mjs ผ่าน ทุกชุด exit 0 ใช้ actual embedded script กับ mocks ไม่ใช่ native browser/Core transport/hardware

## ผลอ่าน contract/source

- inFlightCommand ล็อกให้มี control POST เดียวจน fetch/JSON settle; คำสั่งถัดไป dispatch ใน finally หลัง release current
- unsent field เดียวกัน coalesce โดยรักษาตำแหน่งในคิว และ resolve false ของ promise ที่ถูกแทน; ไม่ปล่อย pending promises ค้างจากการถอดรายการ
- known fields จำกัด unsent queue 14 รายการ รองรับ run STOP/START คู่หนึ่ง; burst 600 commands ใน test เหลือ 6 fields ที่ยังไม่ส่ง
- STOP แทน queued run jobs และขึ้นหัวคิว; ไม่แทน current request START ที่กดใหม่หลัง STOP ไม่ลบ STOP และรอ STOP ก่อน dispatch
- finishCommand ตรวจ field sequence จึงไม่ล้าง pending intent/edit revision ใหม่เมื่อ current คำสั่งเก่าจบ; toggle ใช้ pending intent; pending profile ไม่ถูก acknowledgement ของ field อื่นทับ
- response success render server snapshot โดยยังใช้ rangeBusy guard; error คงอยู่ผ่าน polls จนมีคำสั่งถัดไปสำเร็จ คิวเดินต่อหลัง failure ตาม contract
- poll ที่เริ่มก่อน command หรือระหว่าง pending ไม่ apply stale response ตาม generation/startedPending guards

## ข้อจำกัดที่ไม่รับรอง

ไม่มี deadline หรือ cancellation ของ current fetch ใน revision นี้ STOP อาจรอ in-flight request ที่ค้าง และไม่ใช่ physical emergency stop การรับ HTTP success ไม่แทน measured GPIO/ECU outputs

Network rejection ไม่พิสูจน์ว่า server ไม่ apply คำสั่งเก่า หาก server processing ยังจบภายหลัง connection failure การ serialize ฝั่ง browser ไม่รับรอง latest intent หรือ STOP outcome ในทุกเงื่อนไข ต้องตรวจ state/hardware หรือเพิ่ม server protocol ในงานแยก

Promise false มีหลายความหมายตาม contract: superseded unsent, rejected command, queue full หรือ network failure ไม่ใช่ข้อสรุปว่า server ไม่ใช้คำสั่งนั้น และไม่มีการส่ง START/sensors/selftest ถึงบอร์ดในการทดสอบรอบนี้

VISUAL_QA_STATUS ระบุ native browser QA ยังไม่ผ่าน/ไม่พร้อม; script mocks และ preview generation ไม่เปลี่ยนสถานะนั้น ต้องตรวจ mouse/touch/keyboard/device/HTTP จริง พร้อม full compile ของ source hash ใหม่และ linked-artifact evidence ก่อน Release
