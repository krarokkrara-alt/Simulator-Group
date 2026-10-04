# ชุดติดตั้งลูกค้า Mytech Product — ต้นแบบ ยังไม่ใช่ Release

เป้าหมาย: ลูกค้าเสียบสาย USB ข้อมูล เปิด `START_INSTALLER.cmd` เลือก COM แล้วกด Install โดยไม่ต้องใช้ Arduino IDE มีเครื่องมือ `tools/esptool.exe` ที่จัดเตรียมไว้แล้ว แต่ยังไม่มี firmware และ manifest ที่อนุมัติ จึงติดตั้งไม่ได้ในสถานะปัจจุบัน

## วิธีใช้เมื่อชุด Release ผ่าน QC แล้ว

1. แตก ZIP ทั้งชุดลงโฟลเดอร์ในเครื่อง Windows ที่เขียนไฟล์ได้
2. เสียบบอร์ด ESP32 ด้วยสาย USB ข้อมูล และปิดโปรแกรมอื่นที่ใช้ COM
3. เปิด `START_INSTALLER.cmd` เลือก COM ของบอร์ด และกด Install การกดปุ่มนี้อนุญาตให้เขียน firmware ซึ่งอาจทับโปรแกรมและข้อมูลเดิมภายในช่วงภาพ firmware
4. รอผลสำเร็จ อย่าถอด USB ระหว่างติดตั้ง ถ้าล้มเหลวให้อ่านข้อความและส่งไฟล์ log ให้ฝ่ายบริการ
5. เมื่อสำเร็จ เชื่อม Wi-Fi `PUK_SIMULATOR_TEST` รหัส `12345678` ด้วยตนเอง แล้วกดปุ่มเปิดเว็บ `http://192.168.4.1`

ชุดติดตั้งไม่สลับ Wi-Fi อัตโนมัติ ไม่ติดตั้ง driver ไม่ยกระดับสิทธิ์ และไม่ดาวน์โหลดเครื่องมือ หากไม่พบ COM ให้ตรวจสายและ driver ตามรุ่น USB ของบอร์ด หากบอร์ดเข้าโหมดดาวน์โหลดอัตโนมัติไม่ได้ ฝ่าย QC ต้องระบุขั้นตอน BOOT/RESET ของฮาร์ดแวร์รุ่นที่จำหน่ายให้ลูกค้าก่อน Release

## สำหรับผู้จัดทำ Release

- Build และ QC firmware พร้อมยืนยันว่า board target เป็น `esp32` รุ่นที่ทดสอบจริง
- สร้าง merged binary จากผล build ที่ตรวจสอบแล้ว ระบุ offset เริ่มต้นจริงจากขั้นตอน merge ห้ามเดา offset และห้ามใช้ application-only binary แทน merged image
- จัดชุด `tools/esptool.exe` รุ่น v5 ที่ทดสอบบน Windows จริง พร้อม dependencies และ license ที่จำเป็น ไม่ใช้ไฟล์ placeholder
- คัดลอก `manifest.development.example.json` เป็น `manifest.json` แล้วกรอก version, SHA256 ของ firmware และเครื่องมือ, offset จริง และยืนยันค่า Wi-Fi ของ firmware ชุดนั้น กำหนด `releaseApproved: true` เฉพาะหลังผู้จัดการและ QC อนุมัติ
- ใส่ภาพ firmware ตาม `image.file` เครื่องมืออยู่ที่ `tools/esptool.exe` เสมอ ทุก path ต้องอยู่ในโฟลเดอร์ชุดติดตั้ง ไม่รองรับ symlink/junction
- SHA256 ตรวจไฟล์ตรงกับ manifest แต่ไม่ใช่ลายเซ็นรับรองผู้เผยแพร่ ควรแจก ZIP ผ่านช่องทางที่บริษัทควบคุม และกำหนดการเซ็น release ก่อนเผยแพร่

ไฟล์ตัวอย่างตั้ง `offset`, `sha256` และ version ของเครื่องมือเป็น null โดยเจตนา ไม่มีค่าที่อ้างว่าเป็นผล build จริง

## ตรวจโดยไม่แตะบอร์ด

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Installer.ps1 -ValidateOnly
```

ตรวจ manifest/path/hash เท่านั้น ไม่เรียก esptool ไม่เข้าถึง COM ในชุดต้นแบบจะจบด้วย exit 1 เพราะไม่มี `manifest.json` ซึ่งเป็นผลที่คาดไว้

เมื่อใช้ GUI exit 0 หมายถึง esptool เขียนสำเร็จ; exit 1 หมายถึงข้อผิดพลาด; exit 2 หมายถึงปิดก่อนติดตั้ง ไม่ได้ยืนยันว่าการทำงานของ firmware ผ่านการวัดจริง Log อยู่โฟลเดอร์ `logs` ในชุดติดตั้ง ระหว่างรอเครื่องมือหน้าต่างประมวลผล UI ต่อ แต่ปิดหน้าต่างหรือติดตั้งซ้ำไม่ได้ คำสั่งมี timeout 180 วินาที จากนั้นหยุด process tree ด้วย Windows taskkill และตรวจผลการหยุด ต้อง QC ประสบการณ์ใช้งานและ timeout บนชุดเครื่องมือจริงก่อนเผยแพร่

## รายการ QC ก่อนส่งลูกค้า

- ทดสอบติดตั้งสะอาดบน Windows ที่ไม่มี Python/Arduino IDE ด้วย tools จริง
- ตรวจ chip ไม่ตรงรุ่น, COM หาย, COM ถูกใช้งาน, hash ผิด, manifest ไม่ครบ, USB หลุด, timeout และ retry
- ตรวจ offset/partition/build target จากหลักฐาน build และทดสอบบูตจริงหลังเขียน
- วัด CKP/CMP/SCOPE และ PWM ให้ผ่านข้อกำหนด firmware แยกจากผล installer
- ตรวจ AP SSID/password/IP, เปิดเว็บบนเครื่องลูกค้า และตรวจคู่มือ BOOT/RESET กับ driver ของบอร์ดรุ่นจำหน่าย

อ้างอิงคำสั่ง esptool v5: [Basic commands](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/basic-commands.html), [Advanced options](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/advanced-options.html) ใช้ `--chip esp32`, `flash-id`, `write-flash` และ reset options ตามเอกสาร ไม่มี `--force` หรือ `erase-flash` ในชุดนี้
