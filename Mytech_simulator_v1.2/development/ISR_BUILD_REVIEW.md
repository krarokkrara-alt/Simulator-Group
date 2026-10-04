# ISR build review — 0.3.1-dev candidate

ตรวจวันที่ 4 ตุลาคม 2026 จาก build ที่ root คอมไพล์ไว้ ไม่คอมไพล์ซ้ำ ไม่แก้ source ไม่ใช้บอร์ด

## Artifact ที่ตรวจ

- `build/development-v0.3.1/ESP32_WiFi_StandAlone_Test.ino.elf`
- ELF SHA256: `C263FA8CEFD0D6BA42DC4DBE07E4ADAD68E04A119BA4EB5EB3DF01965112D618`
- `.ino.map`, `sdkconfig`, `sketch/SignalGenerator.cpp.o`
- Build options: `esp32:esp32:esp32`, Arduino-ESP32 Core `3.3.11`, optimization `-Os`
- เครื่องมือ: `tools/arduino-data/packages/esp32/tools/esp-x32/2601/xtensa-esp-elf/bin/objdump.exe`

## ตำแหน่งจริงจาก linked ELF

| Symbol | Address | Section | Size |
|---|---|---|---|
| SignalGenerator::crankInterrupt(void*) | 0x400d2b68 | .flash.text | 0xb1 |
| SignalGenerator::scopeInterrupt(void*) | 0x400d2c1c | .flash.text | 0x8f |
| timerFnWrapper | 0x400881a0 | .iram0.text | 0x1b |
| gptimer_default_isr | 0x4008996c | .iram0.text | 0x99 |
| xPortEnterCriticalTimeout | 0x4008c99c | .iram0.text | 0x10a |
| vPortExitCritical | 0x4008caa8 | .iram0.text | 0x8a |
| signals (instance) | 0x3ffc3af8 | .dram0.bss | 0x2c |

Map ยืนยัน callback ใน `.flash.text` ที่บรรทัด 71688–71696
crank literal pool อยู่ 0x400d005c ขนาด 8 bytes ใน flash; scope แชร์ literal GPIO address จาก pool นี้
Disassembly ของทั้งสอง callback มี external call เพียง critical enter/exit ที่ระบุข้างบน
การ modulo 120/240 ถูกแปลงเป็น arithmetic instructions ไม่เรียก division helper ใน ISR
`__udivdi3` ที่พบใน object เป็นของ update task สำหรับคำนวณคาบ ไม่ใช่ callback
GPIO register address 0x3ff44000 และ instance/state primitive อยู่ใน DRAM ตามหลักฐาน symbol; callback ไม่มี String/HTTP/Wi-Fi allocation

## แยก placement ออกจาก cache safety

`sdkconfig` ระบุ:

```text
# CONFIG_ARDUINO_ISR_IRAM is not set
CONFIG_GPTIMER_ISR_HANDLER_IN_IRAM=y
# CONFIG_GPTIMER_CTRL_FUNC_IN_IRAM is not set
# CONFIG_GPTIMER_ISR_CACHE_SAFE is not set
CONFIG_GPTIMER_OBJ_CACHE_SAFE=y
# CONFIG_GPTIMER_ISR_IRAM_SAFE is not set
# CONFIG_FREERTOS_PLACE_FUNCTIONS_INTO_FLASH is not set
```

Core `esp32-hal.h` บรรทัด 74–79 ทำให้ `ARDUINO_ISR_ATTR` ว่างเมื่อ CONFIG_ARDUINO_ISR_IRAM ไม่ได้ตั้ง
ดังนั้นชื่อ macro นี้ไม่ใช่หลักฐานว่า callback อยู่ IRAM; ELF ยืนยันว่าทั้งสองอยู่ flash
Core `esp32-hal-timer.c` ใช้ `IRAM_ATTR` โดยตรงกับ timerFnWrapper และเรียก callback ผ่าน function pointer
wrapper อยู่ IRAM ไม่ทำให้ callback ที่ถูกเรียกย้ายเข้า IRAM

ผลตรวจ placement: **ยังไม่ผ่านเกณฑ์ callback อยู่ IRAM**
แนวทางรอบต่อไปคือใช้ `IRAM_ATTR` โดยตรงใน callback แล้วตรวจ ELF และ literal pools อีกครั้งหลัง build เสร็จ
ห้ามอ้างว่าแก้ annotation อย่างเดียวทำให้ timer interrupt cache-safe: config ของ GPTIMER ยังไม่ได้เปิด CACHE_SAFE/IRAM_SAFE
ผลนี้เป็นข้อจำกัดการรับประกันช่วง flash cache ถูกปิด และ jitter/latency ไม่ใช่หลักฐานว่าบอร์ดล้มเหลวจริง
หากจะรับรองการทำงานช่วง cache-off ต้องทบทวน config, interrupt allocation, dependency/literal/data placement และทดสอบเพิ่มเติม

## สิ่งที่ยังไม่ยืนยัน

ไม่ได้วัด oscilloscope, ไม่ได้ทดสอบ HTTP load หรือ Wi-Fi restart/stress, ไม่ได้ยืนยันชนิดชิปของบอร์ดจริง
ยังไม่รับรองว่าคลื่นต่อเนื่องทุกช่วง cache-off หรือมี jitter อยู่ในเกณฑ์ใด
ผลตรวจนี้ใช้กับ ELF hash ข้างบนเท่านั้น ต้องตรวจใหม่เมื่อ source/config/build เปลี่ยน
