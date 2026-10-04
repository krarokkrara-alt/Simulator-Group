# QC linked IRAM placement — 4 ตุลาคม 2026

ผล: **callback และ literal ที่ callback ใช้ ผ่านการตรวจ placement จาก linked ELF ใหม่นี้** ไม่ใช่ผลวัด waveform หรือการรับรองการทำงานระหว่าง cache disabled

ผู้จัดการรายงาน compile session 90663 exit 0; QC ไม่ compile ซ้ำและไม่เข้าบอร์ด ใช้ `xtensa-esp-elf-objdump.exe` ตัวจริงจาก toolchain 2601 ตรวจ symbols, section headers, disassembly และ literal bytes แล้วอ่าน map/sdkconfig ประกอบ

## Artifact ที่ตรวจ

อยู่ใน `build/development-v0.3.1/`:

| Artifact | SHA256 |
|---|---|
| ESP32_WiFi_StandAlone_Test.ino.elf | `53964b109893a7470ca14cb35536539bd650507d9b1bf0d4022f00fab68ab5c1` |
| ESP32_WiFi_StandAlone_Test.ino.map | `4e6c9376d9253b81f580075034a751a89de24ed8234b6bb8ba24d6926417bc08` |
| ESP32_WiFi_StandAlone_Test.ino.bin | `13846793120ce6a8d2e8c9d2e537e7f5294f01e5cf2cd658a1999aebea5cdbd9` |

Source declaration และ definition ของ callback ใช้ `IRAM_ATTR` ชัดเจน ไม่อาศัย `ARDUINO_ISR_ATTR` ซึ่ง config นี้ปิด

## Symbols และข้อมูล

| รายการ | Address | Section / ผล |
|---|---|---|
| SignalGenerator::crankInterrupt(void*) | `0x400812c4` | `.iram0.text`, size `0xb1`; map input `.iram1.2` |
| SignalGenerator::scopeInterrupt(void*) | `0x40081378` | `.iram0.text`, size `0x8f`; map input `.iram1.3` |
| timerFnWrapper | `0x400882ec` | `.iram0.text` |
| xPortEnterCriticalTimeout | `0x4008cae8` | `.iram0.text` |
| vPortExitCritical | `0x4008cbf4` | `.iram0.text` |
| esp_cpu_compare_and_set | `0x40089b5c` | `.iram0.text` |
| signals | `0x3ffc3af8` | `.dram0.bss`, size `0x2c` |

Mirror flags/counters และ mux เป็นสมาชิก `signals` ใน DRAM; callback ไม่อ่าน SimulatorState/String หรือโปรไฟล์จาก flash การเข้าถึง GPIO ใช้ MMIO address `0x3ff44000`

Disassembly callback มี direct calls ไป critical enter/exit ข้างต้น ไม่มี call ไป String/Serial/digitalWrite/division helper ใน callback นี้ การ modulo ถูกลดเป็น arithmetic instructions

Literal ที่โหลดด้วย `l32r` ใน callback อยู่ที่ `0x40080404` (ค่า `0x88888889` สำหรับ arithmetic) และ `0x40080408` (ค่า GPIO address `0x3ff44000`) ทั้งสองอยู่ใน `.iram0.text` ซึ่งเริ่ม `0x40080404` ขนาด `0x149cf`; ตรวจ bytes ได้ `89888888 0040f43f` แบบ little endian

## ขอบเขตและข้อค้าง

- Critical-section normal path ใช้ IRAM code และ DRAM nesting/interrupt-state data; ยังมี assert diagnostic paths ที่อ้าง flash rodata จึงไม่กล่าวว่า dependency ทุก error path cache-safe
- `sdkconfig` ของ esp32-libs 3.3.11 ระบุ `CONFIG_GPTIMER_ISR_HANDLER_IN_IRAM=y` และ `CONFIG_GPTIMER_OBJ_CACHE_SAFE=y` แต่ `CONFIG_GPTIMER_ISR_CACHE_SAFE` ไม่ตั้ง; `CONFIG_ARDUINO_ISR_IRAM` ไม่ตั้งด้วย
- การวาง callback ใน IRAM ไม่ได้เปลี่ยน interrupt allocation/config หรือพิสูจน์ว่า interrupt ไม่ถูกพักระหว่าง flash/cache operation ต้องทดสอบ driver configuration และวัดบนฮาร์ดแวร์ก่อนอ้างคลื่นต่อเนื่อง
- ยังต้องยืนยันชิปจริง, วัด jitter/frequency/phase/STOP LOW, ทดสอบ Wi-Fi/AP recovery และ SELF TEST ภายใต้โหลด
- เก็บ release approval ไว้จนหลักฐาน hardware และชุด installer ครบ ผลนี้รับรองเฉพาะ artifact hashes ที่ระบุ หาก rebuild ต้องตรวจ placement/hash ใหม่
