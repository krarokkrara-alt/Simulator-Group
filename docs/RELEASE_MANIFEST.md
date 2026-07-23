# Release Manifest

| Field | Value |
|---|---|
| Project | PUK ESP32 DAC ECU Simulator |
| Version | 0.1.0 |
| Release date | 2026-07-23 |
| Target | ESP32 Dev Module with internal DAC |
| Framework | Arduino on PlatformIO |
| Stand-alone builds | 5 |
| Web application builds | 5 |
| Control transport | GPIO controls / ESP32 Wi‑Fi Access Point + HTTP |
| Analog outputs | GPIO25, GPIO26 internal 8-bit DAC |
| Demonstration waveform | Generic 36-1 CKP + simple CMP phase |
| Static verification | PASS |
| Compile verification | NOT TESTED in current environment |
| Signal bench | NOT TESTED |
| ECU bench | NOT TESTED |

## Included source

- `src/main.cpp` — application state and 10 variant behavior
- `src/signal_engine.cpp` — CKP/CMP and DAC output
- `src/board_health.cpp` — board performance checks
- `src/web_ui.cpp` — ESP32 access point, HTTP API and embedded Web UI
- `include/` — shared configuration and interfaces
- `platformio.ini` — 10 build environments

## Compatibility and known limits

- ESP32 classic DAC is required; ESP32-S3/C3 do not provide the same GPIO25/26 DAC peripheral.
- HTTP is local and unauthenticated after joining the password-protected AP.
- Waveform is a demonstration pattern and must not be treated as an ECU-specific profile.
- External 5 V/12 V signal conditioning is not included in this software release.

