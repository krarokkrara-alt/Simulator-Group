# QC linked placement หลัง raw-only handler — 4 ตุลาคม 2026

ผล: callback/literal placement ผ่านการตรวจ linked artifact รอบ raw-only handler นี้ ผู้จัดการรายงาน compile session 33997 exit 0, sketch 956311 bytes, globals 47916 bytes QC ไม่ compile ซ้ำและไม่เข้าบอร์ด ผลนี้ไม่รับรอง hardware waveform หรือ cache-disabled continuity

## SHA256 ที่ QC อ่านอิสระ

Artifacts อยู่ `build/development-v0.3.1/`:

| File | SHA256 |
|---|---|
| ESP32_WiFi_StandAlone_Test.ino.elf | `d0bf18d3cbeb35f28d6668b29ed5b2d81babdc16b79bf13184f8af7b3ba0f2e2` |
| ESP32_WiFi_StandAlone_Test.ino.map | `9e57f28cb278c190c16b37bc8649db824c4b8be5ee1e5d6c533b5820bedb6345` |
| ESP32_WiFi_StandAlone_Test.ino.bin | `d70c0c45de64241b0dd19874bfd3e007cdb1975cbc632ba79e5b79851313d8af` |

## Symbol/section/disassembly/map

ใช้ toolchain `xtensa-esp-elf-objdump.exe` จริง ตรวจ symbol table และ disassembly:

- crankInterrupt `0x400812c4`, size `0xb1`, `.iram0.text`; map input `.iram1.2`
- scopeInterrupt `0x40081378`, size `0x8f`, `.iram0.text`; map input `.iram1.3`
- timerFnWrapper `0x400882ec`, xPortEnterCriticalTimeout `0x4008cae8`, vPortExitCritical `0x4008cbf4`, esp_cpu_compare_and_set `0x40089b5c` อยู่ `.iram0.text`
- signals `0x3ffc3c04`, size `0x2c`, `.dram0.bss`; ISR mirror/mux จึงเป็น DRAM
- callback l32r โหลด literal ที่ `0x40080404` ค่า arithmetic `0x88888889` และ `0x40080408` ค่า GPIO MMIO `0x3ff44000`; literal bytes ใน `.iram0.text` คือ `89888888 0040f43f`
- direct calls จาก callback ที่ตรวจมีเฉพาะ critical enter/exit; ไม่พบ String/Serial/digitalWrite/flash function calls ใน callback

Config esp32-libs 3.3.11 ยังปิด CONFIG_ARDUINO_ISR_IRAM และ CONFIG_GPTIMER_ISR_CACHE_SAFE; CONFIG_GPTIMER_OBJ_CACHE_SAFE=y เท่านั้น จึงไม่สรุปว่า driver interrupt ทำงานต่อได้ทุกช่วง cache disabled Critical diagnostic/assert paths ยังมี flash rodata ตามรายงาน placement รอบก่อน

การรับ source custom handler อยู่ใน `QC_TRANSPORT_VALIDATION.md` โดย hashes WebInterface.cpp `1a69ae575110e36240e370bcbc67e19740693ede057e1028a49700615fcc8a77` และ WebInterface.h `866a00848eb4fd4fc556c232433e9bab63c59685394887b801e2662a860f36c1` ผู้จัดการต้องรักษา build evidence/source hashes ให้ตรง หาก rebuild ต้องตรวจ hashes/placement อีกครั้ง

Release ยังค้างการยืนยันชิป, transport tests จริง, installer process-tree timeout และ hardware waveform/AP stress ผล compile และ placement ไม่แทนผลทดสอบเหล่านี้

## Checkpoint ล่าสุดหลัง active-slider UI

ผู้จัดการรายงาน compile session 35800 exit 0, sketch 958551 bytes, globals 47916 bytes QC อ่าน artifact ใหม่อิสระและตรวจ linked symbols/disassembly ซ้ำโดยไม่ full compile หรือแก้ source:

| รายการ | SHA256 ล่าสุด |
|---|---|
| WebPage.h source | `be59f568dd24929a35422ff1fc0ce4e2f8c960e399184cbdbce8b6af0c36f4f5` |
| ESP32_WiFi_StandAlone_Test.ino.bin | `3d8db7f35a06d9f0bb648459ab0cedba165594ce6e97af44056de505513d3a80` |
| ESP32_WiFi_StandAlone_Test.ino.elf | `0c7313686b224ccbd3d44153c4a0420994203c35608c4fac7f5da01b585e9e79` |
| ESP32_WiFi_StandAlone_Test.ino.map | `b36363cc239717f090d9d08ac6363675f8e687c276fe949b0e7f2d50c586817f` |

ผล placement ผ่านเฉพาะ artifact ล่าสุดนี้: crank `0x400812c4`, scope `0x40081378`, timerFnWrapper `0x400882ec` และ critical enter/exit `0x4008cae8`/`0x4008cbf4` อยู่ `.iram0.text`; signals `0x3ffc3c04` อยู่ `.dram0.bss` Callback disassembly ยังคงโหลด arithmetic/GPIO literals จาก IRAM `0x40080404`/`0x40080408` และ direct calls เฉพาะ critical enter/exit

ข้อจำกัด cache-safe/driver/error paths และ Release ที่ระบุข้างต้นยังมีอยู่ รายการ hashes รอบก่อนเป็นประวัติ ไม่ใช่ identity ของ binary ล่าสุด UI tests รอบนี้อยู่ `QC_UI_SLIDERS.md`; tests mock ไม่แทน browser/device หรือ measured waveform
