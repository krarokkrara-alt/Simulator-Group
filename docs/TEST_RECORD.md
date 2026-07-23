# Test Record — v0.1.0

| ID | Requirement | Method | Expected | Actual | Status |
|---|---|---|---|---|---|
| SW-01 | มี Build variant 10 แบบ | Static script | 10 environments | 10 environments | PASS |
| SW-02 | ไม่มี Pin ซ้ำใน Config | Static script | no duplicate | no duplicate | PASS |
| SW-03 | มี Board health checks ครบ | Static inspection | 6 groups | 6 groups | PASS |
| FW-01 | Compile ESP32 ทุก Environment | PlatformIO | no errors | ยังไม่ได้รันใน environment นี้ | NOT TESTED |
| HW-01 | DAC25 feedback | Multimeter/ADC | 1.00 V ±0.22 V | ต้องวัดจริง | NOT TESTED |
| HW-02 | DAC26 feedback | Multimeter/ADC | 2.00 V ±0.22 V | ต้องวัดจริง | NOT TESTED |
| HW-03 | CKP 36-1 waveform | Oscilloscope | missing tooth/jitter within limit | ต้องวัดจริง | NOT TESTED |
| HW-04 | CMP phase | Oscilloscope | profile phase correct | ต้องวัดจริง | NOT TESTED |
| ECU-01 | ECU recognizes RPM/sync | Protected ECU bench | stable RPM, no sync loss | ต้องยืนยัน ECU/Pinout | NOT TESTED |

สถานะ Release นี้: **Software structure verified / ไม่ได้ Bench Test**

