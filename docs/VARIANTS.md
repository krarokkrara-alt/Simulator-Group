# รูปแบบโปรแกรม 10 แบบ

โปรเจ็กต์ใช้ Source Core ชุดเดียวและแยก Build Environment เพื่อลดความคลาดเคลื่อนระหว่างรุ่น

| # | Environment | รูปแบบ | การควบคุมหลัก | จุดเน้น |
|---|---|---|---|---|
| 1 | `standalone_rotary` | Stand-alone | Pot/rotary + Start/Stop | ใช้งานหน้ากล่อง |
| 2 | `standalone_buttons` | Stand-alone | ปุ่มกด | งานสอนพื้นฐาน |
| 3 | `standalone_preset` | Stand-alone | Preset profiles | เรียกค่าซ้ำเร็ว |
| 4 | `standalone_diagnostic` | Stand-alone | Serial diagnostics | ตรวจสัญญาณ/บอร์ด |
| 5 | `standalone_burnin` | Stand-alone | Burn-in cycle | ทดสอบงานต่อเนื่อง |
| 6 | `web_dashboard` | Wi‑Fi Web App | Dashboard | หน้าจอ PC/Notebook |
| 7 | `web_mobile` | Wi‑Fi Web App | Mobile layout | โทรศัพท์ |
| 8 | `web_lab` | Wi‑Fi Web App | Lab metrics | ห้องทดลอง |
| 9 | `web_training` | Wi‑Fi Web App | Training theme | ใช้ประกอบคอร์ส |
| 10 | `web_burnin` | Wi‑Fi Web App | Burn-in theme | เฝ้าดูสุขภาพบอร์ด |

รุ่น 1–5 มีพฤติกรรมต่างกันจริงใน `main.cpp`: Pot/rotary, two-button step, preset, diagnostic report และ automatic burn-in sweep ตามลำดับ ส่วนรุ่น 6–10 ใช้ Responsive Web Core ร่วมกัน แต่แยกสี/หน้าตาและ SSID ตาม Variant ID เพื่อเลือกหน้าจอใช้งานแต่ละกลุ่ม

## คำสั่ง Build

```bash
pio run -e standalone_rotary
pio run -e web_dashboard
pio run -e web_mobile
```

Build ทั้งหมด:

```bash
pio run
```
