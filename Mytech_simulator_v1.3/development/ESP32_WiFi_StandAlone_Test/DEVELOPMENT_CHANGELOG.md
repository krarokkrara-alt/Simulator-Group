# v0.4.0-dev (Mytech_simulator_v1.3)

- โปรไฟล์รถขับรูปคลื่นจริง: `TriggerPattern {teeth, missing, camCount, cams[]}` ต่อรุ่นใน `VehicleProfiles.cpp` ตรวจค่าด้วย `static_assert` ตอน compile
- `SignalGenerator` ใช้ตาราง slot ครึ่งฟัน (4 x teeth ต่อ 720°, bit0 = CKP, bit1 = CMP) สร้างใน loop task แล้ว copy ภายใต้ `mux` ขณะ disarm; เปลี่ยนรุ่นจะเริ่มรอบใหม่ที่ slot 0
- คาบ timer = 60e6 / (rpm x 2 x teeth) µs ปัดใกล้สุด (เท่ากับสูตรเดิมเมื่อ teeth = 60)
- Web UI: dropdown ยี่ห้อ → รุ่น, รูปคลื่น SVG 720°, คำเตือน UNVERIFIED, escape ข้อความจาก JSON
- `/api/profiles` เพิ่ม `teeth`, `missing`, `cams`, `verified`, `source`; id 0-2 ยังเป็น generic 60-2 + cam 90° เหมือน v1.2
- ข้อมูลยืนยัน: 1KD-FTV (DENSO CRS Hilux/Innova: NE 36-2, G 1/720°), 4JJ1-TC (คู่มือ Isuzu: 56 ฟัน 6° + ช่อง 24°, CMP 4+1), 4D56 CMP (DENSO HP3: 5 พัลส์ 30/180/180/180°CA)
- ยังไม่ยืนยัน: 4D56 CKP, R18A, YD25 (verified = false)
- เปลี่ยนรุ่นขณะ RUNNING: หน้าเว็บถามยืนยันก่อน เพราะ output หยุดสั้นๆ แล้วเริ่มรอบใหม่ที่ slot 0 (ECU จะหลุด sync ชั่วคราว)
- หน้าเว็บส่ง `text/html; charset=utf-8`; ข้อมูลรุ่นวาดใหม่เฉพาะเมื่อ id เปลี่ยน
- ข้อจำกัดเดิม: ถ้าโหลด `/api/profiles` ไม่สำเร็จ ต้อง refresh หน้าเว็บเอง
- ผลตรวจทีม R&D: Code review = no blocking findings; QC = APPROVED FOR MERGE (เทสต์ node 4/4, preview ตรง source, ตาราง slot ตรงข้อมูลอ้างอิง) — ไม่มีผลฮาร์ดแวร์
- ยังไม่ compile / ยังไม่ทดสอบบนบอร์ด / ยังไม่วัด oscilloscope

# v0.3.1-dev

พัฒนาต่อจากข้อความ v0.3.0 ที่กู้จากแชต ฐานกู้และ CHANGELOG ของต้นทางอยู่ใน recovery/recovered-v0.3.0/ และโฟลเดอร์ baseline

- CKP/CMP ใช้ hardware timer แทนการเปลี่ยนขาจาก main loop ที่ติด delay/HTTP
- รูปคลื่น generic 60-2 ต่อรอบ crank และ CMP HIGH 90° หนึ่งครั้งต่อ720°
- SCOPE ใช้ timer แยก toggle500µs มี self-test budget6000ticks รองรับกดซ้ำ
- START/RPM change เริ่มphaseใหม่; STOPหรือRPM0 ปิด CKP/CMP LOW ภายใต้ critical section
- ISR อ่าน primitive mirror ผ่านportMUX ไม่อ่าน String หรือ SimulatorState ตรง
- handler publish configก่อนตอบ API

เป้าหมาย build: classic ESP32, Arduino Core3.3.11 รุ่นบอร์ดที่ต่อจริงยังไม่ยืนยัน โปรไฟล์รถทุกตัวเป็น metadata เท่านั้น ยังไม่ได้เลือก waveformเฉพาะรุ่น และไม่มีDAC/VR

timer1MHzมีความละเอียด1µs ที่คำสั่ง8000RPM half-tooth period62.5µsถูกปัด63µs ให้ค่าnominal~7936.5RPM (-0.794%) ต้องกำหนดtoleranceและวัดoscilloscope ก่อนรับรอง

ผลผู้ตรวจโค้ด/QCจากstatic: ไม่พบP1blockerในเส้นทาง ISR config/STOP lock แต่ต้องcompileและทดสอบinterrupt jitter, AP recovery, scope duration, timer failureจริงก่อนRelease ชื่อ v0.3.1-dev ไม่ใช่รุ่นที่อัปโหลดแล้ว

## 2026-10-04 IRAM placement revision

Replaced ARDUINO_ISR_ATTR with explicit IRAM_ATTR in both callback declarations/definitions because the default Core 3.3.11 config leaves ARDUINO_ISR_ATTR empty. Full build completed exit 0; program storage 953499 bytes, globals 47644 bytes. Application SHA256 13846793120ce6a8d2e8c9d2e537e7f5294f01e5cf2cd658a1999aebea5cdbd9. No warning/error appeared in captured compiler log. Linked ELF placement is independently checked in QC_IRAM_PLACEMENT.md when available. GPTIMER cache-safe allocation is still disabled; no cache-off continuity or hardware timing claim is made.

Parsed API values are staged before state changes and reject malformed/out-of-range/duplicate arguments. WebServer strips bare query segments before this handler, so raw-query validation remains a documented limitation; see API_VALIDATION.md.
