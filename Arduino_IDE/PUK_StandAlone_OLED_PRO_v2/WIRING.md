# การต่อวงจร PUK Stand Alone OLED PRO v2

> ใช้กับ Arduino UNO/Nano, OLED SSD1306, Rotary Encoder, IGT Output และ IGF Input

## 1. ผังระบบรวม

```text
12V INPUT
   |
   +-- Fuse 1A
   +-- Reverse polarity protection
   +-- TVS 18V
   +-- Buck 12V -> 5V
            |
            +--> Arduino UNO/Nano
            +--> OLED SSD1306
            +--> Encoder / LED / Buzzer

Arduino D9 ----> IGT Driver Stage ----> ECU IGT
ECU IGF ------> IGF Protection Stage -> Arduino D2
Arduino A4/A5 ------------------------> OLED
Arduino D3/D4/D5 --------------------> Encoder
Arduino D6 --------------------------> START/STOP
Arduino D7 --------------------------> Buzzer
Arduino D8 --------------------------> Fault LED
```

---

## 2. Pin Mapping

| ฟังก์ชัน | ขา Arduino | หมายเหตุ |
|---|---:|---|
| IGF Input | D2 | External interrupt, FALLING edge |
| Encoder A/CLK | D3 | ใช้หมุนปรับค่า |
| Encoder B/DT | D4 | ใช้บอกทิศทาง |
| Encoder SW | D5 | กดเปลี่ยนหน้าเมนู |
| START/STOP | D6 | ปุ่มกดแยก |
| Buzzer | D7 | Active buzzer หรือ passive buzzer |
| Fault LED | D8 | LED แสดงความผิดปกติ |
| IGT Logic Output | D9 | ห้ามต่อ ECU โดยตรง |
| OLED SDA | A4 | I2C |
| OLED SCL | A5 | I2C |

---

## 3. การต่อ OLED SSD1306 0.96 นิ้ว

| OLED | Arduino UNO/Nano |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

ค่า address ที่ใช้ในโปรแกรมคือ `0x3C`

---

## 4. การต่อ Rotary Encoder

| Encoder | Arduino |
|---|---|
| CLK / A | D3 |
| DT / B | D4 |
| SW | D5 |
| + | 5V |
| GND | GND |

โปรแกรมเปิดใช้ `INPUT_PULLUP` อยู่แล้ว จึงไม่จำเป็นต้องใส่ pull-up เพิ่มในกรณีใช้โมดูล KY-040

---

## 5. ปุ่ม START/STOP

```text
Arduino D6 ---- Push Button ---- GND
```

โปรแกรมใช้ `INPUT_PULLUP`

- ไม่กด = HIGH
- กด = LOW

แนะนำใช้ปุ่ม Momentary แบบ NO

---

## 6. Fault LED

```text
Arduino D8 ---- 330R ---- LED Anode
LED Cathode ------------------- GND
```

เมื่อพบ `MISS`, `LOW`, `HIGH` หรือ `NOISE` ไฟจะติด

---

## 7. Buzzer

### แบบ Active Buzzer 5V

```text
Arduino D7 ---- 1k ---- Base 2N2222
2N2222 Emitter -------- GND
2N2222 Collector ------ Buzzer (-)
Buzzer (+) ------------ 5V
```

ใส่ diode 1N4148/1N4007 คร่อม buzzer หากเป็นชนิดมีขดลวด

---

## 8. ภาค IGT Output แบบ Open Collector

> แนะนำรูปแบบนี้สำหรับงาน ECU เพราะแยก logic Arduino ออกจากสาย IGT และปรับระดับ pull-up ได้

```text
Arduino D9 ---- R1 1k ---- Base Q1 2N2222
                         |
                         +---- R2 10k ---- GND

Q1 Emitter -------------------- GND
Q1 Collector ------------------ IGT_OUT

IGT_OUT ---- R3 4.7k ---- +5V หรือ +12V ตามสเปก ECU
IGT_OUT ---- TVS/Clamp optional
```

### ค่าอุปกรณ์

- Q1: 2N2222, BC337 หรือ 2N3904
- R1: 1k
- R2: 10k
- R3 pull-up: เริ่มที่ 4.7k
- จุดทดสอบ: TP-IGT ที่ขา Collector

### หมายเหตุสำคัญ

- ต้องเช็กก่อนว่า ECU เป้าหมายต้องการ IGT แบบ 5V หรือ 12V
- ห้ามนำ D9 ต่อเข้าขา ECU โดยตรง
- GND ของ simulator และ ECU ต้องอ้างอิงร่วมกัน ยกเว้นออกแบบแยกด้วย optocoupler

---

## 9. ภาค IGF Input แบบ Optocoupler

### กรณี IGF ประมาณ 5V

```text
ECU IGF ---- R4 1k ---- PC817 LED Anode
PC817 LED Cathode ----- ECU GND

PC817 Collector ------- Arduino D2
PC817 Collector ------- R5 10k ------- 5V
PC817 Emitter --------- Arduino GND

Arduino D2 ------------ C1 1nF ------- GND (optional)
```

### กรณี IGF ประมาณ 12V

เปลี่ยน R4 เป็น 2.2k ถึง 3.3k ขนาด 1/4W หรือ 1/2W ตามแรงดันจริง

### ค่าแนะนำ

- PC817 หรือ EL817
- R4:
  - IGF 5V: 1k
  - IGF 12V: 2.2k–3.3k
- R5: 10k pull-up
- C1: 1nF–4.7nF
- จุดทดสอบ: TP-IGF-IN ที่ D2

> หลีกเลี่ยง C ค่าสูงเกินไป เพราะจะทำให้ pulse แคบถูกกรองทิ้ง

---

## 10. ภาคจ่ายไฟ 12V -> 5V

```text
12V IN
  |
 Fuse 1A
  |
 Schottky diode / Reverse MOSFET
  |
 TVS SMBJ18A ลง GND
  |
 Buck Converter 12V -> 5V
  |
 +5V Rail
```

### อุปกรณ์แนะนำ

- Fuse 1A หรือ Polyfuse 0.75–1.1A
- Reverse protection: SS34 หรือ P-channel MOSFET
- TVS: SMBJ18A
- Buck: LM2596 Module หรือ MP1584 Module
- Capacitor input: 470uF/25V + 100nF
- Capacitor output: 220uF/10V + 100nF

ปรับ Buck ให้ได้ 5.00V ก่อนเสียบ Arduino และ OLED

---

## 11. Grounding

แนะนำแยกเส้นกราวด์เป็น 3 กลุ่มแล้วรวมที่จุดเดียวแบบ Star Ground

```text
Power GND -----+
Arduino GND ---+---- STAR GROUND ---- ECU GND
Signal GND ----+
```

อย่าพาดสาย IGT/IGF ใกล้สายโหลดกำลังสูงหรือสายคอยล์จุดระเบิด

---

## 12. Connector แนะนำ

| ขั้ว | หน้าที่ |
|---|---|
| VIN+ | ไฟเข้า 12V |
| GND | Ground |
| IGT OUT | สัญญาณออกไป ECU |
| IGF IN | สัญญาณกลับจาก ECU |
| TP-IGT | จุดวัด Oscilloscope |
| TP-IGF | จุดวัด IGF หลัง Protection |
| +5V TEST | จุดตรวจไฟ 5V |

แนะนำใช้ terminal block แบบขันสกรู และพิมพ์ชื่อขั้วบนหน้ากล่องให้ชัดเจน

---

## 13. ขั้นตอนตรวจสอบก่อนต่อ ECU

1. เปิดเครื่องโดยยังไม่ต่อ ECU
2. วัดไฟ 5V ต้องอยู่ประมาณ 4.9–5.1V
3. ตรวจ OLED และ Encoder
4. กด START และวัด D9 ด้วย Oscilloscope
5. วัด TP-IGT หลัง transistor
6. ป้อน pulse ทดสอบเข้าภาค IGF ผ่านตัวต้านทาน
7. ตรวจสถานะ `OK`, `MISS`, `LOW`, `HIGH`, `NOISE`
8. ปิดเครื่องก่อนต่อ ECU จริง

---

## 14. คำเตือน

- เครื่องนี้ยังเป็นเครื่องต้นแบบ ไม่ควรต่อโหลดคอยล์หรือหัวฉีดโดยตรง
- ต้องยืนยันระดับแรงดัน IGT/IGF ของ ECU แต่ละรุ่นก่อนใช้งาน
- ใช้แหล่งจ่ายที่จำกัดกระแสได้ในการทดลองครั้งแรก
- ควรมีฟิวส์แยกด้านไฟเข้าและด้าน ECU
