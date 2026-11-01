# IGF test — D3

เพิ่มพัลส์ทดสอบตามคำขอ คง CKP D7, CMP D2, sensor pins และ live RPM เดิม

- Mega D3 (PE5) อ้างอิง GND Mega ลอจิก0–Vccประมาณ5V ยังไม่ได้วัดVppจริง
- เริ่มต้นIGFเป็นON: STARTสร้างพร้อมCKP/CMP; STOP/timeout5วินาทีหยุดทุกพัลส์และD3LOW
- HIGHกว้าง5องศาข้อเหวี่ยง ทุก180องศา รวม4พัลส์/720องศา เฟสทดสอบ90องศาเทียบช่องแรกCKP
- ที่800RPM ประมาณ26.67Hz คาบ37.5ms HIGHประมาณ1.04ms ความกว้างเปลี่ยนตามRPM
- IGF ON/OFF เปลี่ยนได้ขณะRUNโดยCKP/CMPไม่หยุด ค่าที่เลือกคงผ่านSTOPจนกว่าจะสั่งใหม่หรือreset (resetกลับON)
- UIแท็บIGF TESTมีปุ่มON/OFF; Diagnosticsส่งSTATSอ่านตัวนับและsnapshotD3

```text
IGF ON
START
STATS
IGF OFF
IGF ON
STOP
```

`IGFSTATS EDGES 18 D3 0 ENABLED 1` เป็นตัวนับซอฟต์แวร์ISRและค่าdigitalชั่วขณะ ไม่ใช่ผลวัดoscilloscope

นี่คือสัญญาณสังเคราะห์ตามรอบ ไม่ได้รับIGTจากECU ไม่ตรวจประกายไฟหรือยืนยันการจุดระเบิด ค่าphase/widthเป็นค่าทดสอบ ไม่ใช่IGFที่คาลิเบรตกับECU89661-02860 ห้ามถือว่าพัลส์นี้ยืนยันว่าECUจะทำงานหรือไม่ขึ้นDTC และไม่ใช่คำแนะนำให้ต่อD3เข้าECUโดยตรง

## ผลตรวจ 2 พฤศจิกายน2026

- compileผ่าน Flash8372bytes/RAM387bytes และuploadCOM4ผ่าน
- Serial regressionเดิมผ่าน รวมCKP/CMP,sensors,liveRPM800→2500→300,STOP/timeout
- RUN800ได้CKP301/CMP5/IGF18 สอดคล้อง4IGF/720องศาโดยตำแหน่งเริ่มต่างกัน
- IGF OFFขณะRUN300: counterคง35,D3LOW แต่CKP611→665/CMP9→10เดินต่อ
- IGF ON: counter35→39; STOP: counterคง40,D3LOWในการอ่านซ้ำ
- ไม่มีผลวัดVpp/รูปคลื่นจริง ยังไม่ได้ทดสอบGUIครบทุกปุ่มหรือECU

## Agent review

review_simulator ตรวจเฟิร์มแวร์, pin/timer, atomic STOP/OFF, สูตรพัลส์, protocolUIและregression ไม่พบblocker พบP3ข้อความSTARTที่อาจทำให้เข้าใจว่าIGFเปิดเสมอ แก้เป็นIGF if enabledและตรวจซ้ำแล้วปิดข้อค้นพบ ไม่มีการเปิดพอร์ตโดยreviewer
