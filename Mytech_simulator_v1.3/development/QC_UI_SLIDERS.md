# QC active sliders — 4 ตุลาคม 2026

รับ UI revision เพื่อ build/browser verification ต่อได้ ไม่ full compile หรือเข้าบอร์ดในรอบ QC นี้

Source WebPage.h SHA256 `be59f568dd24929a35422ff1fc0ce4e2f8c960e399184cbdbce8b6af0c36f4f5`; offline preview index.html SHA256 `3ccc79c0615052afb2da58c5557a229aa361bb64a42d351c1b4f7da7bea9c78c`

QC รัน actual-script tests อิสระ: active_slider_tests.mjs ผ่าน 9 scenarios; web_command_tests.mjs ผ่าน; offline_preview_tests.mjs ผ่าน ทั้งหมด exit 0 ใช้ DOM/fetch mocks ไม่ใช่ browser/HTTP/device tests Preview extract source UI ล่าสุดและไม่มี native-fetch fallback ตาม tests

## ผลอ่านและกรณีตรวจ

- Pointer IDs แยกเมาส์/สัมผัสหลายตัวโดยไม่ต้อง focus; polling ไม่ทับ slider ขณะ pointer active และ cancel คืน state โดยไม่กระทบ pointer อีกตัว
- Keyboard range keys และ input dirty state กัน polling จน commit/blur; pending POST กันการ rollback หลังปล่อย pointer
- Edit revision กันคำสั่งเก่าที่เพิ่งจบล้าง dirty ของค่าที่เริ่มแก้ใหม่
- Command sequence กัน render/error จาก response เก่า และ poll sequence กัน polling ผิดลำดับ; rollback ของ RPM ที่ผิดยังไม่ทับ sensor อีกช่องที่กำลังแก้
- QC พบ poll ที่เริ่มระหว่าง POST pending แต่ตอบหลัง POST สำเร็จอาจ rollback ค่าใหม่; source ล่าสุดจับ `startedPending` ตอนเริ่ม refresh แล้วปฏิเสธ response นั้นทั้ง success/error แม้ pending จะถูก clear แล้ว กรณี deferred นี้ผ่าน actual-script test

## ขอบเขตผล

Mock event emission ยังไม่พิสูจน์ลำดับ native change/input/pointer/keyboard events ใน browser ลูกค้าหรือ touch WebView ต้องทดสอบจริงภายหลัง เช่น pointer release/cancel นอก slider, keyboard auto-repeat, focus switching และ request latency

ตัวกัน response UI เก่าไม่ใช่ server-side transaction sequence หรือการ serialize คำสั่ง POST หาก network/server ประมวลผลคำสั่งเก่าทีหลัง UI refresh ยังคงแสดงสถานะจริงของ server ได้ Test out-of-order response ตั้งให้ server authoritative state เป็นคำสั่งล่าสุด จึงไม่รับรองว่า user intent ล่าสุดจะถูก server apply เสมอจาก UI sequence เพียงอย่างเดียว

คำตอบผลสำเร็จ/สถานะ UI ไม่ใช่ measured GPIO/output feedback และไม่เปลี่ยนข้อจำกัด hardware/static review Release ยังค้าง chip, Core transport, browser/device interaction, installer timeout tree และ waveform measurement
