# QC IRAM placement — POST build 4 ตุลาคม 2026

ผล: callback และ literal ที่ใช้อยู่ IRAM ใน linked ELF ล่าสุด **ผ่าน placement check** ผลนี้ไม่รับรอง cache-off continuity, waveform, Wi-Fi stress หรือ Release

Root แจ้ง compile session 80700 exit 0; ผู้ตรวจไม่ได้ compile ซ้ำ ไม่แก้ source และไม่ใช้บอร์ด
ตรวจด้วย actual toolchain objdump จาก `tools/arduino-data/packages/esp32/tools/esp-x32/2601/xtensa-esp-elf/bin/objdump.exe`
Artifacts อยู่ใน `build/development-v0.3.1/`

| Artifact | SHA256 |
|---|---|
| ESP32_WiFi_StandAlone_Test.ino.elf | `773032858f284bcf6f7eb39cdea6ae11cc048a5c6f962278626d22efed9c26ff` |
| ESP32_WiFi_StandAlone_Test.ino.map | `79ea04b1c5173add0d73d8cd4df5c850b2d9098db4315722378a11d8ed589ce5` |
| ESP32_WiFi_StandAlone_Test.ino.bin | `650553bdf04a9512c2b4244fa05180b1f48b8267c68be08eb375caf6b2be3f5d` |

| Symbol | Address | Section / size |
|---|---|---|
| crankInterrupt | 0x400812c4 | .iram0.text / 0xb1 |
| scopeInterrupt | 0x40081378 | .iram0.text / 0x8f |
| timerFnWrapper | 0x400882ec | .iram0.text / 0x1b |
| gptimer_default_isr | 0x40089ab8 | .iram0.text / 0x99 |
| xPortEnterCriticalTimeout | 0x4008cae8 | .iram0.text / 0x10a |
| vPortExitCritical | 0x4008cbf4 | .iram0.text / 0x8a |
| esp_cpu_compare_and_set | 0x40089b5c | .iram0.text / 0x62 |
| signals instance | 0x3ffc3c04 | .dram0.bss / 0x2c |

Disassembly ของ callback ตาม symbol boundaries ยืนยัน calls เพียง critical enter/exit
crank โหลด literal ที่ 0x40080404 = 0x88888889 (arithmetic constant) และ 0x40080408 = 0x3ff44000 (GPIO MMIO)
scope โหลด GPIO literal เดียวกัน; literal bytes เป็น `89888888 0040f43f` little endian
ทั้งสองอยู่ใน .iram0.text เริ่ม 0x40080404 ขนาด 0x149cf
ไม่พบ String/Serial/HTTP/Wi-Fi/digitalWrite/division-helper call ใน callback; modulo เป็น inline arithmetic
สมาชิก primitive และ mux อยู่ใน signals DRAM instance ไม่อ่าน state/String/profile ใน ISR
Address ของ signals เปลี่ยนจาก build ก่อน จึงบันทึกใหม่และไม่ใช้ artifact hashes จากรายงานเก่าแทน

ข้อจำกัดยังเหมือนเดิม: sdkconfig ตั้ง GPTIMER_ISR_HANDLER_IN_IRAM และ GPTIMER_OBJ_CACHE_SAFE แต่ GPTIMER_ISR_CACHE_SAFE / GPTIMER_ISR_IRAM_SAFE / ARDUINO_ISR_IRAM ไม่ตั้ง
Explicit IRAM_ATTR ทำให้ callbacks อยู่ IRAM แม้ Arduino macro config ปิด แต่ไม่ได้เปลี่ยน interrupt/cache-safe configuration
critical-section normal path อยู่ IRAM; diagnostic/assert error paths ยังไม่รับรอง cache-safe ทุก dependency
ยังไม่ทดสอบ scope timing, jitter, phase, STOP LOW, SELF TEST หรือ Wi-Fi recovery/load บนบอร์ดจริง และยังต้องยืนยัน chip ของบอร์ด

ตรวจประกอบ QC_TRANSPORT_VALIDATION.md แล้ว: ข้อจำกัด query-ignore, Core header overwrite, drain/allocation ก่อน handler, multipart parser และการไม่มี HTTP integration test ยังถูกเปิดเผยครบ
ไม่มี finding ใหม่ที่ต้องแก้ source ในรอบ placement review นี้; compile success ไม่ทดแทน transport/hardware tests
ผลรับรองเฉพาะ artifact hashes ข้างบน ถ้า rebuild ต้องตรวจใหม่
