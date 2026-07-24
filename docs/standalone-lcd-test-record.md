# Test Record — Standalone LCD v1.0.0

วันที่: 24 กรกฎาคม 2569  
สถานะ: **partially verified / not bench-tested**

| ID | Requirement | Method | Expected | Actual/Evidence | Status |
|---|---|---|---|---|---|
| LCD-01 | LCD แสดง mode/RPM/MAP/TPS/state | source inspection + hardware test | refresh 200ms, 4 lines | code present; hardware unavailable | Partial |
| SAFE-01 | outputs OFF at boot/STOP/mode change | source inspection + oscilloscope | CKP/CMP LOW, DAC=0 | `allOff()` called; unmeasured | Partial |
| SIG-01 | generic CKP 36-1 | logic analyzer 300–5000 RPM | 35 pulses + 1 missing slot/rev | algorithm present | Not bench-tested |
| SIG-02 | CMP phase stable | logic analyzer | high teeth 0–8 each rev | algorithm present | Not bench-tested |
| UI-01 | three buttons debounced | inspection + button test | one event per press | 30ms debounce present | Partial |
| DAC-01 | DAC feedback | GPIO25→34, GPIO26→35 | 1.00V/2.00V ±0.22V | test implemented | Not bench-tested |
| ECU-01 | response monitor A/B | protected optocoupler input | pulse counters increase | ISR implemented | Not bench-tested |
| HEALTH-01 | honest grade | inspection | no response/no jumper cannot return GOOD | grading logic checked | Pass (static) |

## Recommended next experiment

Compile in Arduino IDE, then test LCD and buttons without ECU. Measure CKP/CMP at GPIO and after 2N2222 stages at 300, 800, 1500, 3000 and 5000 RPM. Record amplitude, tooth count, missing-tooth spacing, phase and jitter. Only after the safety gate passes should ECU-specific pins/profile be added.
