# ป้องกัน polling เขียนทับ slider — 4 ตุลาคม 2026

แก้เฉพาะ WebPage.h และตัวอย่างออฟไลน์ ไม่เปลี่ยน timing, API หรือ GPIO firmware

- Pointer Events รองรับเมาส์/สัมผัส โดยติดตาม pointer ID จึงไม่ต้องพึ่งการ focus ของ touch browser
- Keyboard range keys และ input dirty state ป้องกัน polling ทับค่าที่กำลังแก้ จน change command สำเร็จ/ล้มเหลว หรือ focusout; pointercancel คืนค่าจาก state
- Pending POST ป้องกันค่า slider กลับค่าเดิมหลังปล่อยนิ้วก่อนผลตอบกลับ
- Poll sequence ปฏิเสธ snapshot UI เก่า ส่วน POST ในรุ่นปัจจุบันส่งทีละคำขอตาม UI_COMMAND_QUEUE_CONTRACT.md จึงไม่ได้ใช้ผลสำเร็จ/error จากคำขอ POST พร้อมกันมาแข่งกัน render
- Poll ที่เริ่มขณะ POST ยัง pending ไม่ apply ภายหลัง แม้ POST จะเสร็จก่อน poll ตอบกลับ เพื่อกัน snapshot เก่าทับค่าคำสั่งสำเร็จ
- Edit revision ป้องกันคำสั่งเก่าที่เพิ่งจบล้าง dirty state ของค่าที่ผู้ใช้เริ่มแก้ใหม่
- ค่าที่แสดงระหว่างแก้คือค่าที่ผู้ใช้กำลังเลือก ยังไม่ใช่หลักฐานว่าบอร์ดใช้ค่านั้น ผลสำเร็จ render จาก API response และ error แสดงแยกจาก polling state

ทดสอบ actual JavaScript จาก WebPage.h ด้วย DOM/fetch mocks ผ่าน active_slider_tests.mjs: pointer สัมผัสหลายตัวโดยไม่มี focus, keyboard/blur, pending หลังปล่อย pointer, คำสั่งใหม่รอคำสั่งก่อนและยังเก็บค่าที่ผู้ใช้เลือก, poll ก่อน/ระหว่าง command, rollback เมื่ออีก sensor กำลังแก้ และแก้ใหม่ขณะคำสั่งก่อนยังรอ

```powershell
node .\development\tests\active_slider_tests.mjs
node .\development\tests\web_command_tests.mjs
node .\development\preview\generate-preview.mjs
node .\development\tests\offline_preview_tests.mjs
```

ผล mock tests ไม่ใช่ผล browser/device/HTTP จริง ต้อง QC เมาส์ สัมผัส และคีย์บอร์ดในเบราว์เซอร์ลูกค้า รวมถึงผล response ล่าช้าจากบอร์ดภายหลัง ไม่อัปโหลด ไม่เปลี่ยน Wi-Fi และไม่ส่งคำสั่งเข้าบอร์ดในงานนี้
