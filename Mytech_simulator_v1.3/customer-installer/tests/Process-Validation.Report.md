# Actual process runner fixtures — 4 ตุลาคม 2026

รัน actual Invoke-Esptool และ Write-InstallLog functions ที่ extract จาก Installer.ps1 AST เท่านั้น
ใช้ Windows PowerShell 5.1.26100.9444 เป็น helper runtime, ไม่มี UI/COM/real esptool execution
Installer source hash และผลจริงอยู่ใน Process-Validation.Results.json; diagnostic log อยู่ใน Process-Validation.TimeoutEvidence.json

- Success stdout + stderr capture: PASS
- Nonzero exit 7 rejected: PASS
- Timeout process-tree termination: ยังไม่ผ่าน เนื่องจาก taskkill exit 1 `ERROR: Access denied` ใน sandbox
- Actual function รายงาน `Process-tree termination failed. Close the tool before retrying.` และ unsafeTermination=true ตาม guard
- Helpers จำกัด lifetime ไว้ 25 วินาที; ตรวจ PID parent 6760/child 4552 และ initial parent 22448/child 13532 ภายหลังไม่พบ process ค้าง

รอบแรก assertion พบ parent ยังอยู่แต่ไม่ได้เก็บ Invoke error จึงปรับเฉพาะ test เพิ่ม diagnostic evidence และรันอีกครั้งหนึ่งเพื่อแยกสาเหตุ ได้หลักฐาน Access denied ชัดเจน ไม่ retry termination เพิ่ม
ไม่ใช้ kill-by-name ไม่มี process อื่นถูกระบุเป้าหมาย
สรุปนี้ไม่อ้างว่า timeout/tree cleanup ผ่านบนเครื่องลูกค้า ต้องทดสอบใน execution environment ที่มีสิทธิ์ยุติเฉพาะ helper process tree
Approval category sandbox_approval ถูกปิดใน session จึงไม่เรียก require_escalated ที่ระบบไม่รองรับ

การลบ fixture directories หลัง process หมดอายุถูก approval policy ปฏิเสธด้วยข้อความ `approval required by policy, but AskForApproval::Granular.sandbox_approval is false`
เก็บ directory เหล่านี้ไว้เป็นหลักฐานชั่วคราวภายใน tests ไม่ใช่ customer package/release:

- process-fixture-68c07e8d55af4802bd752c5066cde7cf
- process-fixture-ab7bc37a5bd5436ca19aff0dc7e5553d

ไม่มี customer manifest.json ถูกสร้าง และไม่มี Installer.ps1 source mutation
