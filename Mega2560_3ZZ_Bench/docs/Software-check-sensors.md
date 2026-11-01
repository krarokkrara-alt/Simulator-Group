# ผลตรวจซอฟต์แวร์เพิ่มเซนเซอร์

1 พฤศจิกายน 2026

แก้กรณี RUN แต่ CAM DISABLED: START เปิดCMPทุกครั้ง พร้อมCKP โดยไม่ต้องโหลดPROFILE ทดสอบ CAM OFF→START→STATUS ได้ RUN800 / CAM TEST ENABLED ผ่าน การปรับRPMระหว่างRUNยังผ่าน CKP306→567/CMP5→9 ไม่รีเซ็ตตัวนับ STOP/timeoutปิดทั้งคู่ ไม่มีผลวัด5Vppจริง

- Arduino Mega 2560 รุ่น Live RPM CKP D7/CMP D2/MAF D8 compile: exit0, Flash7842 bytes, RAM371 bytes และ upload COM4 exit0
- ทดสอบปรับ800→2500→300RPM ขณะRUN: ACKและRUN statusถูกต้อง CKP countเพิ่ม305→563, CMP5→9 โดยไม่รีเซ็ตตัวนับ จบSTOP/CAM OFF ทดสอบนี้ไม่ใช่การวัดjitterหรือรูปคลื่นด้วยสโคป
- ทดสอบ PROFILE TOYOTA_3ZZ_TEST และ CONFIG ยืนยันขาใหม่ ผ่าน STATS หลัง START ได้ CKP301 CMP5, snapshot D7=1 D2=0 นี่คือตัวนับ ISR และการอ่านขาดิจิทัลชั่วขณะ ไม่ใช่การวัดรูปคลื่น/Vpp
- PowerShell Control-Panel.ps1 -ValidateOnly: exit 0, WinForms dependencies loaded; script parsed
- ตรวจข้อความ protocol UI รองรับ ACK SENSOR, SENSOR OFF, SENSOR TIMEOUT, ACK CAM และ STATUS ทั้ง CAM DISABLED/TEST ENABLED
- ผู้ใช้ยืนยัน Mega เหลือเฉพาะ USB ก่อนทดสอบ อัปโหลด COM4 สำเร็จ exit 0
- scripts/Test-BenchSerial.ps1 -Port COM4 ผ่าน exit 0: TPS500/MAP1000/MAF1200 ACK, CAM0/2 ACK และ RUN status, STOP ล้าง PWM, SENSOR TPS2500 timeout5s, START timeout5s, สถานะท้าย STOP/CAM DISABLED, จบส่ง STOP/SENSOR OFF/CAM OFF
- ERROR ที่ตั้งใจทดสอบ: SENSOR TPS5001 ถูกปฏิเสธ, CAM phase144 ถูกปฏิเสธ, เปลี่ยน CAM ขณะ RUN ถูกปฏิเสธ ไม่ใช่ความล้มเหลวของ test
- ยังไม่ได้วัด PWM/CKP/CAM ที่ขาจริง ไม่ได้ทดสอบ GUI end-to-end หรือ ECU
- ค่า CAM เป็นค่าทดสอบ ยังไม่มีเฟสอ้างอิงเครื่องยนต์จริง
- Sensor mV เป็นเป้าหมายค่าเฉลี่ย PWM ที่สมมติ Vcc=5V ไม่ใช่ค่าที่วัดและไม่ใช่ DAC

ผลใน Hardware-test-2026-11-01.md เป็นประวัติรุ่น CKP ก่อนเพิ่ม sensor/CAM ส่วนผลรุ่น sensor/CAM อยู่ในเอกสารฉบับนี้
