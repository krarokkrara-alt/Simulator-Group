# ตัวอย่างหน้าจอออฟไลน์

เปิด `index.html` ด้วยเบราว์เซอร์ได้โดยตรง ไม่ต้องเปิด server หรือเชื่อมอินเทอร์เน็ต ทดลอง START/STOP, RPM, CKP/CMP, sensor sliders, vehicle profiles และ SELF TEST ได้

แถบสีเหลืองระบุชัดว่าเป็นตัวอย่างจำลอง ไม่เชื่อมต่อบอร์ดและไม่ใช่ผลทดสอบฮาร์ดแวร์ ไม่มี GPIO, การสลับ Wi-Fi หรือแฟลช firmware ค่าทั้งหมดอยู่ในหน่วยความจำเบราว์เซอร์และกลับค่าเริ่มต้นเมื่อ reload

Generator ดึง HTML จริงจาก `../ESP32_WiFi_StandAlone_Test/WebPage.h` โดยไม่คัดลอกหน้าจอด้วยมือ ดึงข้อมูลโปรไฟล์และ version/MAX_RPM จาก source ด้วย แล้วเพิ่มแถบแจ้งเตือนกับ fetch mock ก่อน JavaScript ของหน้าเว็บเดิม ไม่แก้ source firmware

สร้างใหม่หลัง source UI เปลี่ยน:

```powershell
node .\development\preview\generate-preview.mjs
node .\development\tests\offline_preview_tests.mjs
```

Mock เลียนแบบ contract ของ POST strict-body สำหรับการสาธิต แต่ไม่ใช่ C++ parser จริง ไม่ใช้ Core WebServer และไม่ได้จำลอง raw HTTP headers/framing จึงไม่ใช้ผลนี้รับรอง API transport หรือ release ข้อมูลรถยังเป็น metadata อ้างอิงจาก source การเลือกโปรไฟล์ไม่ได้จำลอง trigger เฉพาะรถ
