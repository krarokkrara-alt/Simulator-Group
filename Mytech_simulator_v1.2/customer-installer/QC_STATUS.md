# สถานะ QC — 4 ตุลาคม 2026 (เวลาไทย)

รับโครง installer เพื่อพัฒนาต่อได้ แต่ **ยังไม่อนุมัติ Release หรือส่งลูกค้า** รายงานนี้เป็นผลตรวจ source และอ่านหลักฐานที่มี ไม่ใช่ผลติดตั้งบนบอร์ด

## ตรวจ release guards อิสระเพิ่มเติม

QC รัน `tests/Release-Validation.Tests.ps1` ด้วย PowerShell 7.6.5 ผ่าน 28 cases, failed 0; installer SHA256 `6d84842b2137c22b16f5f0e7b199eef1ca1edec3fb5dc83b58673bbe13c36e92` ผลรายละเอียดอยู่ `tests/Release-Validation.Results.json` โหลดเฉพาะ actual validation functions จาก AST ไม่เรียก entry point/UI/COM/esptool ใช้ไฟล์ข้อมูล `abc` เป็น fixture ทั้ง image/tool ไม่ใช่ executable หรือ firmware ที่นำไป flash

กรณีครอบคลุม approval bool, schema, missing files, path escape/absolute/directory, correct/uppercase/malformed/wrong/tampered SHA256, empty image, image kind, chip, offset syntax/injection/32-bit limit, version/tool pin และ Wi-Fi URL ผลนี้ไม่ทดสอบ symlink/junction, signing, offset/partition semantics, executable version/run, timeout/process tree หรือเขียนบอร์ดจริง จึงไม่แทน release evidence เหล่านั้น

ฟังก์ชัน Test-Hash ใช้ .NET SHA256 และ dispose resources แทน Get-FileHash ใน runtime installer; QC เคยรัน hash fixture actual function แยกด้วย Windows PowerShell ได้ exit 0 ผล 28-case ชุดนี้เป็น PowerShell 7.6.5 ต้องไม่อ้างว่าเป็น Windows PowerShell 5.1 ครบทุกกรณี

## ผลตรวจโครง

- มีตัวตรวจ manifest ที่อนุมัติ, chip `esp32`, merged image, offset รูปแบบ hex, SHA256 ของ image/tool และ path ภายในแพ็กเกจ
- เรียกเครื่องมือโดยไม่ผ่าน shell ตรวจ COM และ exit code รวมทั้งตรวจรุ่น esptool ก่อนเขียน
- แก้กรณี log ล้มเหลวให้คืนผลล้มเหลว และตั้งผลสำเร็จหลังขั้นตอนรายงานสำเร็จ
- มี startup guard ปิด Install เมื่อชุดไฟล์ไม่ครบ และป้องกันการติดตั้งซ้อน/ปิดหน้าต่างระหว่างงาน
- ตัวล็อก `unsafeTermination` ตั้งก่อนหยุด process tree; หากหยุดไม่ได้ ปุ่มและ handler ป้องกัน retry ในหน้าต่างเดิม จาก source และ fixture failure evidence พบ latch=true แต่ไม่ได้ทดสอบ GUI จริง
- ยังไม่ถือว่า timeout/process-tree termination ผ่าน: fixture ก่อนหน้าพบ OS Access Denied ต้องทดสอบใหม่ใน Windows เป้าหมายจริง

## หลักฐาน compile ณ จุดตรวจ

ภาพตรวจแรกพบ Access denied/Platform not installed; หลักฐานล่าสุดแทนสถานะ compile blocker นั้นแล้ว: `recovery/baseline-build-evidence.json` ระบุ observedCompileExitCode=0 พร้อม Core 3.3.11/FQBN esp32:esp32:esp32 และ log baseline แสดง sketch 947251/global 47604 bytes ส่วน `recovery/development-build-evidence.json` ระบุ observedBuildExitCode=0 และ source hashes ตรง raw-only handler ล่าสุด; log development แสดง sketch 956311/global 47916 bytes

QC ตรวจ final development app SHA256 `d70c0c45de64241b0dd19874bfd3e007cdb1975cbc632ba79e5b79851313d8af` และ ELF SHA256 `d0bf18d3cbeb35f28d6668b29ed5b2d81babdc16b79bf13184f8af7b3ba0f2e2` ตรง build evidence ผล placement อยู่ `development/QC_IRAM_RAW_HANDLER_BUILD.md` Compile pass ไม่ใช่ chip/hardware/transport pass

## Process runner fixture — ผลที่ยืนยันได้

อ่าน Process-Validation.Results.json/TimeoutEvidence.json: actual functions จับ stdout/stderr และปฏิเสธ exit 7 ผ่าน; timeout taskkill exit 1 Access denied และ unsafeTermination=true จึง **process-tree termination ยังไม่ผ่าน** ไม่มีการทดสอบ timeout ซ้ำในสภาพแวดล้อมเดิม ผล residualProcesses=[] เป็นการตรวจภายหลัง helpers หมด lifetime 25 วินาที ไม่ใช่หลักฐานว่า taskkill หยุดทั้ง tree ได้

QC รันอิสระเฉพาะ actual Invoke-Esptool/Write-InstallLog ที่โหลดจาก AST โดย helper powershell.exe แบบ bounded 5 วินาที: stdout+stderr success ผ่าน และ exit 7 rejection ผ่าน ไม่เปิด UI/COM/real esptool ไม่รัน timeout และไม่สร้าง helper files ผลนี้แยกจาก 28 release guards; ต้องทดสอบ process-tree termination ใน Windows เป้าหมายที่มีสิทธิ์เหมาะสมก่อน Release

การตรวจ chip ที่ COM10 ก่อนหน้านี้ timeout จึงยังไม่ยืนยันรุ่นชิปจริง การใช้ target classic ESP32 ในชุดพัฒนาเป็นข้อกำหนดของ candidate ไม่ใช่ผลตรวจฮาร์ดแวร์

## เกณฑ์ก่อน Release

- [ ] ยืนยันรุ่นชิป บอร์ด USB interface และขั้นตอน BOOT/RESET ของสินค้าที่จำหน่าย
- [ ] Compile สำเร็จ พร้อมบันทึก core/tool versions, FQBN, source hash, command, exit code และ log
- [ ] สร้าง merged binary จาก build ที่ผ่าน ระบุ offset/partition/ขนาด flash ตามหลักฐาน และตรวจ SHA256
- [ ] จัด esptool พร้อม dependencies/license และทดสอบบน Windows ที่ไม่มี Python/Arduino IDE
- [ ] ทดสอบ manifest/hash ผิด, chip ผิด, COM หาย/ถูกใช้, nonzero exit, USB หลุด, timeout และการหยุด process ลูก โดยไม่มีเครื่องมือเขียนต่อหลังแจ้งหยุดสำเร็จ
- [ ] ทดสอบ Access Denied ตอนหยุดเครื่องมือ: ต้องปิด retry และแจ้งให้ยืนยัน process หยุดก่อนเปิดโปรแกรมใหม่
- [ ] ทดสอบการติดตั้งและบูตจริงบนฮาร์ดแวร์รุ่นจำหน่าย โดยมีการอนุญาตก่อนเขียนบอร์ด
- [ ] วัด CKP/CMP/SCOPE/PWM, START/STOP, SELF TEST ซ้ำ และการทำงานขณะใช้ Web UI/AP recovery
- [ ] กำหนด tolerance RPM: ชุดพัฒนาใช้ timer 1 MHz; คำสั่ง 8000 RPM ปัดเป็น 63 µs ให้ประมาณ 7936.5 RPM ก่อนรวม jitter จริง
- [ ] ตรวจ SSID/password/IP/API และคู่มือลูกค้าให้ตรงกับ binary ที่จัดส่ง
- [ ] ผู้จัดการและ QC อนุมัติหลักฐานทั้งหมดก่อนตั้ง `releaseApproved: true` และแจกแพ็กเกจ

ผล installer exit 0 หมายถึงขั้นตอนเขียนและรายงานสำเร็จตามเครื่องมือ ไม่ใช่การรับรอง waveform หรือ ECU compatibility โปรไฟล์รถใน candidate ยังเป็นข้อมูลแสดงผล ไม่ได้เปลี่ยน waveform

## Update 2026-10-04 01:48 Bangkok

Historical update: development compile became successful after the initial blocker. The newer QC sections above now record baseline compile and the final raw-only-handler development build as well. Hardware and real HTTP transport remain unverified. Release checklist is not fully passed; raw query is ignored by POST contract and ISR/cache continuity limitations remain documented. No customer release manifest was created.
