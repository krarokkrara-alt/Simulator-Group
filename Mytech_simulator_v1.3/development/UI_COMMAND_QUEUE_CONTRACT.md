# UI command queue contract — 4 ตุลาคม 2026

ส่ง POST ครั้งละหนึ่งคำขอใน browser ไม่ dispatch คำขอถัดไปจน fetch และ JSON response ของคำขอปัจจุบัน settle ไม่เปลี่ยน API/body/timing ของ firmware

- คำสั่ง field เดียวกันที่ยังไม่ส่งถูกแทนด้วยค่าล่าสุดโดยรักษาตำแหน่ง field ในคิว promises ของรายการที่ถูกแทน resolve false โดยไม่มีคำขอส่งและไม่แสดง error ว่าบอร์ดปฏิเสธ
- คิวมีขอบเขต 14 รายการที่ยังไม่ส่ง: 13 UI fields และพื้นที่สำหรับ STOP/START ของ run คำสั่ง UI ไม่รู้จักถูกปฏิเสธในหน้าเว็บ
- STOP (`run=0`) ขึ้นต้นคิว และแทนคำสั่ง run ที่ยังไม่ส่ง รวม queued START แต่ไม่ยกเลิก request ที่ส่งไปแล้ว
- START ที่ผู้ใช้กดใหม่หลัง STOP ถือเป็นเจตนาใหม่ จึงเข้า queue ภายหลัง STOP และไม่ลบ STOP ออก การยืนยัน STOP แสดง state จาก API ก่อน START ถัดไป dispatch/ตอบกลับ
- ค่า slider ที่ยัง pending หรือแก้ใหม่ยังถูกป้องกันไม่ให้ response ของคำสั่งก่อนเขียนทับ Toggle ใช้ pending intent เพื่อให้การกดซ้ำพลิกค่าตามเจตนาล่าสุด แต่แสดงสถานะ active จาก API acknowledgement
- แต่ละ promise resolve true เฉพาะ HTTP success ที่อ่าน JSON และ render สำเร็จ resolve false เมื่อผิดพลาด/ถูกแทน/คิวเต็ม ไม่มี promise ของรายการที่ถูกถอดค้างไว้ Poll ไม่ล้าง error และไม่ apply snapshot ที่เริ่มระหว่าง pending
- เมื่อ current request ล้มเหลวคิวเดินต่อ โดยแสดง error จากผลนั้นจนมีคำสั่งถัดไปสำเร็จ

## STOP และ network outcome ที่ยังรับรองไม่ได้

STOP ไม่ต้องรอ RPM/sensor batch ที่ยังไม่ส่ง แต่ต้องรอหนึ่ง in-flight request ถ้า fetch ค้าง ไม่มี deadline ใหม่ในงานนี้ STOP อาจรอจน browser ให้ผลล้มเหลว/ตอบกลับ ไม่รับรอง STOP ทันทีหรือเวลาสูงสุด

ไม่มี abort/cancel request ที่ส่งแล้ว และ network error ไม่ใช่หลักฐานว่า server ไม่ใช้คำสั่งนั้น Browser queue serialization ลด request overlap ในสภาวะตอบกลับปกติ แต่หลัง connection failure อาจมี server processing เก่าที่จบภายหลัง จึงยังไม่รับรอง latest intent หรือ physical STOP สำหรับทุกเงื่อนไขเครือข่าย ต้องตรวจ state และ waveform จริง หรือออกแบบ protocol acknowledgement/command sequencing ของ server ในงานแยกก่อนอ้างคุณสมบัติเหล่านั้น

## ผลตรวจโดยไม่ใช้บอร์ด

`command_queue_tests.mjs` execute actual JavaScript ของ WebPage.h ด้วย deferred fetch/DOM mocks ตรวจ 10 สถานการณ์: coalesce/รักษาตำแหน่ง field, STOP priority/supersede START, START ใหม่หลัง STOP, network failure release, hung current ไม่อ้าง STOP สำเร็จ, repeated STOP promises, double-toggle intent, burst 600 คำสั่งที่เหลือ 6 unsent fields และการรักษา profile choice ที่ยัง pending

`active_slider_tests.mjs` ปรับกรณี response ผิดลำดับเดิมเป็นการตรวจว่าคำสั่งถัดไปยังไม่ dispatch จนคำสั่งก่อน settle พร้อมคง 9 event/pending/poll scenarios เดิม ใช้ fixture ui_deferred_harness.mjs ที่อ่าน JavaScript จริง ไม่จำลอง HTTP parser

```powershell
node .\development\tests\command_queue_tests.mjs
node .\development\tests\active_slider_tests.mjs
node .\development\tests\web_command_tests.mjs
node .\development\preview\generate-preview.mjs
node .\development\tests\offline_preview_tests.mjs
```

ผลเป็น UI mock tests เท่านั้น ไม่ใช่การทดสอบ Core server order, HTTP จริง หรือบอร์ด ต้อง QC แยกและ compile candidate ใหม่ก่อน release
