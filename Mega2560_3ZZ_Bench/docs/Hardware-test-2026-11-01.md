# ผลตรวจบอร์ด 1 พฤศจิกายน 2026

- ผู้ใช้ยืนยันเหลือเฉพาะ USB ก่อนอัปโหลด
- COM4: USB-SERIAL CH340, USB VID_1A86 PID_7523, Windows status OK
- COM3: Intel AMT SOL ไม่ใช่พอร์ตบอร์ดนี้
- Compile Mega 2560: exit 0, Flash 4666 bytes, RAM 270 bytes
- Upload COM4: exit 0
- Serial 115200: startup banner, STATUS, RPM 800, START, STATUS, STOP, STATUS ตอบครบ
- Timeout: START แล้วรอ 5.5 วินาที ได้ `STOP: START timeout` และ `STOP 800 RPM; CAM DISABLED; ECU UNVERIFIED`
- ปิดพอร์ตหลังส่ง STOP

ไม่พบ error ในการทดสอบชุดนี้ บูตโหลดเดอร์และเฟิร์มแวร์ตอบสนอง ไม่ได้วัดรูปคลื่นจริงหรือทดสอบขา I/O ทุกขา CAM ยังไม่เปิด และยังไม่พร้อมต่อ ECU
