# Independent installer guards — 4 ตุลาคม 2026

Windows PowerShell 5.1 รัน `Release-Validation.Tests.ps1` ได้ exit 0: **28 cases ผ่าน**
รายละเอียด case และ Installer.ps1 SHA256 อยู่ใน `Release-Validation.Results.json`

Tests extract actual Get-PackageFile, Test-Hash และ Read-Release functions จาก PowerShell AST ไม่ dot-source entry point
ใช้ known SHA256 ของ ASCII `abc` และ empty file; fixture tamper ใช้ `abd` เพื่อให้ hash mismatch เป็นผลที่วัดได้
สร้าง package จำลองใน unique tests subdirectory และลบเฉพาะ directory ที่ตรวจ absolute path containment แล้ว
fake tools/esptool.exe เป็นข้อมูล ASCII 3 bytes ไม่ถูก execute และ fake firmware ไม่ใช่ภาพที่ flash ได้
ไม่มี UI/COM/esptool invocation; ไม่มี customer-installer/manifest.json ถูกสร้างหรืออนุมัติ release จริง

ครอบคลุม unapproved และ approval ที่เป็น string/number, schema/chip, merged kind, file missing/directory/absolute/escape,
hash malformed/wrong/tampered, empty firmwareแม้hashตรง, offset null/numeric/decimal/injection/over32bits,
version/tool pinning/path และ Wi-Fi URL รวม valid fixture และ uppercase SHA256

ข้อจำกัด: valid fixture ผ่านหมายถึง static manifest/path/hash guards ทำงาน ไม่พิสูจน์ว่าภาพเป็น merged firmwareจริงหรือ offsetตรงbuild
Read-Release ตรวจ syntax offset เท่านั้น; release approval ต้องตรวจ provenance/build/chip/offset จากหลักฐานแยก
ไม่ได้ทดสอบ symlink/junction creation, GUI, process timeout/tree termination, hardware detection/flash/verify หรือ real esptool version
Tests ไม่ทำให้ package พร้อมส่งลูกค้า และไม่แทน hardware QC
