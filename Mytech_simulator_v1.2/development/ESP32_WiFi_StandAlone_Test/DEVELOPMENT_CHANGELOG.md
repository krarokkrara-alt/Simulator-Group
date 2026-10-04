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
