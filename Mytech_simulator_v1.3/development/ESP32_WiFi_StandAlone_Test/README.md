# ESP32 WiFi Stand Alone Test

**Firmware version: `v0.3.1-dev` — ชุดทดลอง ยังไม่ผ่านการทดสอบบอร์ดหรืออนุมัติส่งลูกค้า**

พัฒนาจากฐานที่กู้ `v0.3.0` เป้าหมาย classic ESP32 เท่านั้น รุ่นชิปของบอร์ดที่เสียบยังอ่านยืนยันไม่ได้ ข้อมูลการ compile/upload ของ v0.3.0 ใน CHANGELOG เป็นประวัติต้นทาง

โปรเจกต์ Arduino IDE สำหรับทดสอบ ESP32 ผ่าน Web UI โดย ESP32 สร้าง Wi-Fi Access Point เอง ไม่ต้องใช้ Serial/COM เป็นส่วนติดต่อผู้ใช้

## การเชื่อมต่อ

- Wi-Fi SSID: `PUK_SIMULATOR_TEST`
- Password: `12345678`
- Web UI: `http://192.168.4.1`

## ขา GPIO เริ่มต้น

| Function | GPIO | Output |
|---|---:|---|
| Built-in/test LED | 2 | Digital |
| Oscilloscope test | 4 | 1 kHz square wave |
| CKP | 18 | 60-2-style pulse basis |
| CMP | 19 | Phase pulse |
| TPS | 25 | PWM 0-100% |
| MAP | 26 | PWM 0-100% |
| ECT | 27 | PWM 0-100% |
| IAT | 32 | PWM 0-100% |
| O2 | 33 | PWM 0-100% |

> GPIO ของ ESP32 เป็นสัญญาณ 3.3 V เท่านั้น ห้ามต่อ 5 V/12 V เข้าขาโดยตรง และควรใช้วงจรกรอง RC/DAC ภายนอกเมื่อต้องการแรงดัน analog ที่เรียบ

## ติดตั้งและอัปโหลด

1. ติดตั้ง Arduino IDE 2.x
2. เพิ่ม ESP32 board package จาก Espressif Systems ผ่าน Boards Manager
3. เปิดไฟล์ `ESP32_WiFi_StandAlone_Test.ino`
4. เลือกบอร์ด ESP32 ที่ตรงกับฮาร์ดแวร์ เช่น **ESP32 Dev Module** และเลือกพอร์ตเพื่ออัปโหลดครั้งแรก
5. กด Upload จากนั้นถอดสายข้อมูลได้หากจ่ายไฟด้วยแหล่งอื่น
6. เชื่อมโทรศัพท์หรือคอมพิวเตอร์กับ `PUK_SIMULATOR_TEST` รหัส `12345678`
7. เปิด `http://192.168.4.1`

## วิธีทดสอบ

1. กด **SELF TEST**: LED จะกระพริบและ GPIO 4 จะสร้างสัญญาณประมาณ 1 kHz เป็นเวลา 3 วินาที
2. ต่อ oscilloscope โดยต่อกราวด์ร่วมกับ ESP32 แล้ววัด GPIO 4
3. ตั้ง RPM, เปิด CKP/CMP และกด START จากนั้นวัด GPIO 18/19
4. เลื่อน TPS/MAP/ECT/IAT/O2 แล้ววัด duty cycle ที่ขาตามตาราง
5. กด STOP และยืนยันว่า CKP/CMP กลับเป็น LOW

## โครงสร้างสำหรับขยายต่อ

- `AppConfig.h`: SSID, IP, GPIO และค่าคงที่
- `SimulatorState.*`: สถานะและค่าเซนเซอร์
- `SignalGenerator.*`: การสร้าง CKP, CMP และสัญญาณทดสอบ
- `WebInterface.*`: Access Point, HTTP routes และ API
- `WebPage.h`: Web UI ฝังใน flash
- `VehicleProfiles.*`: ฐานข้อมูลยี่ห้อ/รุ่นและชนิด CKP/CMP สำหรับขยายเพิ่มเติม
- `HARDWARE_INTERFACE_GUIDE.md`: แนวทางวงจรแยกสัญญาณ Hall, Magnetic VR, analog 0–5 V และ ECT/IAT แบบความต้านทาน
- `ESP32_ECU_Interface_Wiring.svg`: ภาพรวมวงจรและจุดต่อทั้ง 4 โมดูล
- `CKP_CMP_Interface_Wiring.svg`: ภาพวงจร CKP/CMP แยก Hall open-collector และ Magnetic VR differential

## Vehicle / Trigger profiles

หน้า Web UI มีเมนูเลือกยี่ห้อ รุ่น และเครื่องยนต์ พร้อมแสดงชนิด CKP/CMP (`Hall effect` หรือ `Magnetic VR`) และข้อมูล trigger เบื้องต้น รายการรถเป็นข้อมูลอ้างอิงสำหรับตั้งต้นเท่านั้น เพราะชนิดเซนเซอร์และรูปแบบฟันอาจเปลี่ยนตามปีผลิต รหัสเครื่องยนต์ และ ECU

เอาต์พุต CKP/CMP จาก ESP32 เป็นลอจิก 3.3 V เหมาะกับการจำลอง Hall ผ่านวงจรป้องกัน/level shifting ส่วนการจำลอง Magnetic VR ต้องใช้วงจรขับสัญญาณ bipolar ภายนอก ห้ามต่อ ESP32 แทนเซนเซอร์ VR เข้ากับ ECU โดยตรง

โครงสร้างแยกหน้าที่ไว้เพื่อเพิ่มรูปแบบ trigger wheel, calibration, sensor model, persistent settings หรือ driver output ภายหลังได้ง่าย

## หมายเหตุ

- Serial ใช้พิมพ์ข้อมูลวินิจฉัยเท่านั้น ไม่ใช่ User Interface
- เมื่อจ่ายไฟ บอร์ดจะรอให้ไฟนิ่ง 1.5 วินาทีแล้วเปิด Access Point อัตโนมัติ โดยลองซ้ำสูงสุด 5 ครั้ง และตรวจสุขภาพ Wi-Fi ทุก 5 วินาทีเพื่อกู้คืนหาก AP หยุดทำงาน
- Web API อยู่ใน LAN ของ Access Point และไม่มี authentication เพิ่มเติม
- ตัวสร้างสัญญาณนี้เหมาะสำหรับ bench test ระดับต้น ไม่ใช่อุปกรณ์ safety-critical หรือใช้กับรถจริงโดยตรง
# Development candidate: v0.3.1-dev

ชุดนี้เป็นสำเนาพัฒนาจากโค้ดกู้คืน ไม่ใช่ต้นฉบับ v0.3.0 และยังไม่ผ่านการทดสอบบอร์ดจริง
เป้าหมายคอมไพล์คือ classic ESP32 กับ Arduino-ESP32 Core 3.3.11 เท่านั้น ยังไม่ยืนยันชนิดชิปของบอร์ดผู้ใช้

CKP/CMP สร้างด้วย hardware timer ที่ความละเอียด 1 microsecond ไม่ขึ้นกับ loop, HTTP หรือ delay ตอน restart Wi-Fi
CKP เป็น generic 60-2: 120 half-tooth slots ต่อรอบ มีช่องว่าง 4 slots ท้ายแต่ละรอบ
CMP เป็น generic pulse กว้าง 90 องศาเพลาข้อเหวี่ยงหนึ่งครั้งต่อ 720 องศา
START และการเปลี่ยน RPM เริ่ม phase ใหม่จาก slot 0; STOP และ RPM 0 ทำให้ CKP/CMP LOW
Scope GPIO4 มี hardware timer แยก ให้ 1 kHz (500 microseconds ต่อครึ่งคาบ) เมื่อเปิด SCOPE หรือ SELF TEST
SCOPE ทดสอบแยกจาก START/STOP; SELF TEST จำกัด scope ประมาณ 3 วินาทีใน ISR แม้ HTTP/Wi-Fi ค้าง
ISR ใช้เฉพาะค่า primitive และ GPIO registers; task ส่ง snapshot ผ่าน critical section ไม่เรียก String, HTTP หรือ Wi-Fi จาก ISR
ถ้าจอง timer ไม่สำเร็จ จะปิด waveform และรายงาน Serial

ทุก profile รถเป็น metadata เท่านั้น ทุก profile ยังได้ waveform generic เดียวกัน
ยังไม่รับรอง tooth pattern, phase, polarity, ระดับแรงดัน หรือความเข้ากันได้กับรถ/ECU ใด
ยังไม่รองรับ VR waveform หรือ SPI DAC และไม่มีหลักฐานอัปโหลด/วัดสัญญาณสำหรับชุดพัฒนานี้
การปัดคาบเป็น microsecond ทำให้ RPM จริงคลาดเคลื่อนจากค่าตั้ง โดยเฉพาะ RPM สูง; ต้องวัดด้วย oscilloscope ก่อนใช้งาน
Hardware timer ยังมี interrupt latency/jitter ต้องทดสอบภายใต้ HTTP load และ Wi-Fi reconnect จริง

API อ้างอิง: [Timer documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html)
และ [timer header Core 3.3.11](https://github.com/espressif/arduino-esp32/blob/3.3.11/cores/esp32/esp32-hal-timer.h)

