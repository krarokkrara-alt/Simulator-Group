# Simulator Group

โครงการพัฒนา **ECU Simulator แบบ Stand Alone** สำหรับสร้างและควบคุมสัญญาณจำลองของเครื่องยนต์ โดยมีเป้าหมายเพื่อลดการพึ่งพาโปรแกรมที่เชื่อมต่อผ่าน COM Port และทำให้กล่อง Simulator สามารถควบคุมผ่าน Interface ที่ใช้งานง่ายขึ้น

> สถานะปัจจุบัน: เอกสารเริ่มต้นของโครงการ ยังไม่มีผลยืนยันการทดสอบกับ ECU จริงใน repository นี้

## เป้าหมายของโครงการ

- พัฒนา ECU Simulator ให้ทำงานแบบ Stand Alone
- เปลี่ยนหรือเพิ่มช่องทางเชื่อมต่อแทน COM Interface เดิม
- รองรับการเลือก Engine Profile จากข้อมูลที่กำหนด
- สร้างสัญญาณ CKP และ CMP ตามรูปแบบเครื่องยนต์
- ควบคุมสัญญาณ IGT, IGF, Injector และเซนเซอร์จำลอง
- แสดงสถานะการเชื่อมต่อและค่าการทำงานแบบ Real-time
- มีปุ่มหยุดการทำงานและปิด Output ทั้งหมดอย่างปลอดภัย
- รองรับการเพิ่มเครื่องยนต์หรือ ECU รุ่นใหม่ในอนาคต

## ขอบเขตระบบที่วางแผนไว้

### Signal Generation

- CKP (Crankshaft Position)
- CMP (Camshaft Position)
- IGT (Ignition Trigger)
- IGF (Ignition Feedback)
- Injector Signal

### Analog Sensor Simulation

- MAP
- TPS
- ECT
- IAT
- O2
- APP
- FRP

### Communication

ช่องทางเชื่อมต่อจริงต้องยืนยันจากฮาร์ดแวร์และซอร์สโค้ดก่อนเลือกใช้งาน โดยอาจเป็น:

- USB Serial
- Wi-Fi Access Point และ Web Interface
- Bluetooth
- การควบคุมจากหน้าจอและปุ่มบนกล่องโดยตรง

## แนวทางสถาปัตยกรรม

โครงการควรแยกส่วนการทำงานเพื่อให้ง่ายต่อการเพิ่ม Engine Profile และเปลี่ยนช่องทางเชื่อมต่อ:

1. **Engine Profile** — รูปแบบฟัน CKP/CMP, จำนวนสูบ, Firing Order และช่วง RPM
2. **Signal Engine** — การกำหนดเวลาและสร้างสัญญาณแบบแม่นยำ
3. **I/O Layer** — การแมปสัญญาณกับขา Timer, DAC, PWM และ CAN
4. **Control Interface** — ช่องทางสื่อสารระหว่างผู้ใช้กับกล่อง Simulator
5. **User Interface** — การเลือก Profile, ปรับค่า, Start, Stop และแสดงสถานะ
6. **Safety Layer** — ปิด Output เมื่อเปิดเครื่องใหม่ หลุดการเชื่อมต่อ หรือเกิด Timeout

## โครงสร้างโครงการที่แนะนำ

```text
Simulator-Group/
├── firmware/        # Source code สำหรับ Controller
├── interface/       # Web, Desktop หรือ Stand-alone UI
├── hardware/        # Wiring, schematic และข้อมูล Hardware Revision
├── profiles/        # Engine และ ECU profiles
├── docs/            # คู่มือใช้งานและเอกสารการพัฒนา
├── tests/           # Test plan, logs และหลักฐานการทดสอบ
├── CHANGELOG.md
└── README.md
```

## ข้อมูลที่ต้องยืนยันก่อนต่อ ECU จริง

- รุ่น Controller และ Hardware Revision
- ตารางขาที่ใช้งานจริง
- แรงดันของแต่ละสัญญาณ: 3.3 V, 5 V หรือ 12 V
- รูปแบบ Output: Open-collector หรือ Push-pull
- CKP/CMP tooth pattern และความสัมพันธ์ของเฟส
- Connector orientation และ ECU pinout จากแหล่งข้อมูลที่เชื่อถือได้
- วงจรป้องกัน, Fuse, Current limit และ Ground reference
- วิธีเชื่อมต่อที่จะใช้แทน COM Interface

ห้ามต่อ ECU จริงหากยังไม่ยืนยัน Voltage domain, Ground, Pinout และวงจรป้องกัน

## ระดับการตรวจสอบ

สถานะของแต่ละ Release ต้องระบุให้ชัดเจน:

| ระดับ | ความหมาย |
|---|---|
| Software verified | ตรวจโค้ดหรือ Compile ผ่าน |
| Signal verified | วัดสัญญาณด้วย Oscilloscope หรือ Logic Analyzer แล้ว |
| ECU bench verified | ทดสอบกับ ECU จริงบน Bench พร้อมบันทึกผลแล้ว |

การ Compile ผ่านเพียงอย่างเดียวไม่ถือว่าผ่านการทดสอบกับฮาร์ดแวร์หรือ ECU จริง

## ขั้นตอนการพัฒนา

1. เก็บ Baseline เวอร์ชันล่าสุดที่ใช้งานได้
2. กำหนดขอบเขตการเปลี่ยนแปลงและ Acceptance Test
3. ยืนยัน Pinout, Signal level และ Engine pattern
4. พัฒนา Firmware, Interface และเอกสารให้เป็นเวอร์ชันเดียวกัน
5. ทดสอบ Software ก่อนจ่ายไฟให้ Hardware
6. วัด Waveform ด้วยแหล่งจ่ายแบบจำกัดกระแส
7. ทดสอบ ECU Bench ผ่าน Harness ที่มี Fuse และวงจรป้องกัน
8. บันทึกผล ข้อจำกัด และหมายเลขเวอร์ชัน

## การติดตั้งและใช้งาน

ยังไม่สามารถระบุคำสั่ง Build, Upload หรือ Launch ได้จนกว่าจะเพิ่มซอร์สโค้ดและยืนยัน Controller ที่ใช้จริง

เมื่อเพิ่มโปรเจ็กต์แล้ว ควรบันทึกข้อมูลต่อไปนี้ในส่วนนี้:

- Board และ FQBN
- IDE หรือ Toolchain version
- Library และ Dependency
- ขั้นตอน Build และ Upload
- วิธีเชื่อมต่อ Interface
- ลำดับ Power on, Start, Stop และ Power off

## Roadmap

- [ ] นำซอร์สโค้ดเวอร์ชันล่าสุดเข้า repository
- [ ] บันทึก Hardware revision และ Wiring table
- [ ] เลือกช่องทางเชื่อมต่อสำหรับ Stand-alone mode
- [ ] แยก Engine Profile ออกจาก Signal Engine
- [ ] เพิ่ม Connection state และ Emergency Stop
- [ ] เพิ่ม Automated software tests
- [ ] ตรวจ CKP/CMP waveform
- [ ] ตรวจ Analog sensor outputs
- [ ] ทดสอบกับ ECU จริง
- [ ] จัดทำ Operator guide และ Release manifest

## License

ยังไม่ได้กำหนด License หากเป็นโครงการปิดหรือใช้เชิงพาณิชย์ ไม่ควรเพิ่ม Open-source license จนกว่าเจ้าของโครงการจะอนุมัติ
