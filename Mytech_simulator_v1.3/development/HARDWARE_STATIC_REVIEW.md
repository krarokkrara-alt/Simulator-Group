# Hardware static review — 4 ตุลาคม 2026

ขอบเขต: ตรวจ baseline HARDWARE_INTERFACE_GUIDE.md, ESP32_ECU_Interface_Wiring.svg, CKP_CMP_Interface_Wiring.svg เทียบ development AppConfig/SignalGenerator/SimulatorState/VehicleProfiles และ local Arduino Core 3.3.11 source
ไม่ได้แก้ baseline, firmware หรือ wiring ไม่สร้างวงจรใหม่ ไม่ใช้บอร์ดและไม่มีผลวัดไฟฟ้า

ผล: เอกสารเดิมระบุข้อจำกัด bench prototype/firmware pending ไว้แล้ว แต่ยังไม่ใช่วงจรประกอบพร้อมใช้หรือหลักฐานรองรับ ECU ลูกค้า

## Findings

| Severity | Finding และหลักฐาน | ผลต่อการใช้งาน |
|---|---|---|
| P1 | Module C ระบุ MCP4922 + buffer สำหรับ 0–5 V แต่ SimulatorState::setSensor ใช้ analogWrite กับ TPS25/MAP26/ECT27/IAT32/O2 33 และ map 0–100 เป็น duty 0–255; local esp32-hal-ledc.c default analog_frequency=1000 และ resolution=8 | เป็น PWM GPIO ไม่ใช่ calibrated 0–5 V DAC; แม้ GPIO25/26 มีหน้าที่อื่นในชิป โค้ดนี้ไม่ได้เรียก DAC. ไม่มี SPI/buffer transfer curve หรือ output feedback |
| P1 | ECT/IAT ใน guide Module D เป็น switched calibrated resistance + PhotoMOS LED driver แต่ firmware มีเพียง PWM GPIO27/32 | ไม่ได้จำลองความต้านทาน NTC ไม่มี relay selection/break-before-make/calibration. ห้ามถือ slider เป็นอุณหภูมิหรือความต้านทานที่ ECU ต้องการ |
| P1 | CKP/CMP timer ออก digital GPIO18/19 เท่านั้น; SVG CKP/CMP เสนอ HSPI14/13/16/17 +74AHCT125+MCP4922+OPA2197 | SPI pins เป็นข้อเสนอในรูป ไม่มี implementation; ไม่สร้าง bipolar VR, amplitude หรือ DAC samples. เลือก VR profile ไม่เปลี่ยน waveform |
| P1 | Guide และ SVG ระบุ Hall transistor ทำให้สัญญาณกลับ polarity และให้ firmware compensate แต่ SignalGenerator ไม่มี polarity setting และใช้ generic phase เดียว | START/STOP GPIO LOW ผ่าน NPN จะปล่อย collector ให้ ECU pull-up ทำให้ ECU signal HIGH ได้. STOP LOW รับรองเฉพาะ GPIO ไม่ใช่ LOW ที่ปลาย ECU หรือ fail-safe electrical state |
| P1 | SVG ใช้ path ต่อเนื่องแทนสัญลักษณ์ transistor: ESP SVG `M385 365 V330` และ `M385 365 V495`; CKP SVG `M425 235 V195` และ `M425 235 V315`; base path เข้าจุดเดียว | หากอ่านเป็น schematic wires C/E ดูเชื่อมกันตรง ไม่แสดง transistor junction/arrow/pin-number จริง จึงใช้เป็น netlist/คู่มือต่อสายตรงไม่ได้ ต้องยืนยันกับ datasheet pinout ของ transistor package ที่เลือก |
| P1 | CKP SVG มี Q1 emitter path `M425 315 V470` และ CMP collector path `M425 325 H550` ที่ตำแหน่ง x425 y325 เดียวกัน รวมทั้ง Q2 path บนแนวดิ่งเดียวกัน | ภาพมีจุดสัมผัส/ซ้อนของ emitter-ground และ collector CMP ที่ตีความกำกวม มีความเสี่ยงต่อการต่อ CMP ลงกราวด์ตามภาพ ต้องทบทวน connectivity ก่อนนำไปทำวงจร |
| P1 | VehicleProfiles เป็น reference metadata; SignalGenerator ไม่อ่าน vehicleProfile, engine code หรือ sensor type | generic 60-2 และ CMP90°ต่อ720° ใช้กับทุกโปรไฟล์ ไม่มีหลักฐานตรง Toyota/Isuzu/Honda/Nissan/Mitsubishi. ห้ามรับรอง sensor-type/pattern ของรถจากรายการนี้ |
| P2 | GPIO controls เริ่ม setSensor ด้วยค่าตั้งต้นทันทีใน begin; STOP หยุดเฉพาะ CKP/CMP, scope เป็น independent test และ PWM sensor ยังอยู่ | ผู้ใช้ต้องทราบว่า STOP ไม่ตัดทุกเอาต์พุตหรือ power. ยังไม่มี hardware output-enable/isolation interlock, measured-output feedback หรือ brownout fail-state definition |
| P2 | Guide VR สูตร V+=2(DAC−2.5), V−=−V+ ให้ differential=4(DAC−2.5); DAC0–5 จึงเสนอ ±10 V differential full range ก่อนโหลด แต่เริ่มทดสอบแนะนำ0.5–1.0 V peak differential | ต้องกำหนด amplitude limit/โหลด/phase จาก ECU และแยก single-ended vs differential measurement ให้ชัด; สูตรยังไม่มี firmwareใช้และไม่มีผลวัดจริง |
| P2 | VR amplifier และ subtraction ใน SVG เป็น block/formula ไม่ใช่ complete op-amp schematic; isolated±12V reference/ECU floating/common-mode ไม่มี wiring detailครบ | ยังสรุป galvanic isolation ของสัญญาณไม่ได้จาก isolated power supply เพียงอย่างเดียว ต้องระบุ ground/reference/measurement topology และ amplifier circuit ก่อนประกอบ |
| P2 | ESP SVG วาด TVS เป็นบล็อกใน power chain ส่วน guide ระบุ TVS shunt ลงGND; diagram ไม่แสดง pinและnetละเอียด | ต้องใช้ guideเป็นแนวคิดและตรวจ circuitจริง ไม่ต่อ TVS เป็น series จากตำแหน่งบล็อก. ยังไม่มีเลือก transient/load budget จาก ECUรุ่นจริง |
| P2 | Guide บอก optional ECU5V pull-up, open-collector resistor220Ω และ transistor30–40V; ไม่มี ECU pull-up current, threshold, clamp/backpower หรือ measurement | ค่าที่เสนอไม่ใช่ค่าผ่านQCกับทุกECU ต้องตรวจ loaded low/high voltage, edge rise/fall และ current/powerตามECUที่ระบุ |
| P2 | MAX_RPM8000 เป็นsoftwarelimit; timerคาบปัดmicrosecondและgenericpattern ไม่มีผลscope | ไม่ใช่การรับรองความถี่/phase/jitterที่โหลดจริง ต้องวัดก่อนระบุสเปกลูกค้า |

Severity P1 = ต้องแก้หรือระบุข้อจำกัดก่อนถือว่ารองรับการต่อ ECU; P2 = ต้องปิดข้อมูลและตรวจเพิ่มเติมก่อนออกสเปก/Release
findings เป็น static evidence ไม่ได้พิสูจน์ว่าบอร์ดที่มีอยู่ถูกต่อผิดหรือเสียหาย

## ข้อมูลที่ต้องได้ก่อนเลือกวงจรและรับรองลูกค้า

1. ECU part number, รถ/ปี/engine code และ connector pinout/service diagram ที่ตรวจได้; CKP/CMPเป็นHall/VRชนิดใดจริง และ trigger teeth/gap/polarity/phaseตามECU
2. แต่ละinputแรงดันจ่าย/pull-up, pull-up resistance/current, high/low thresholds, impedance, permissible voltage/current, common-mode range, diagnostic bias และ fault detection
3. ระบุ bench-only harness, ECU supply/current budget, ESP32 board/chipจริง, regulator/pin labels และวิธีจ่ายไฟ; USB/oscilloscope/ECU ground topology
4. อุปกรณ์ interface ที่ประกอบจริง: transistor package/pinout/rating, output series/pull-up, buffers/DAC/reference/op-amp supply, polarity และ power-up/STOP/disabled state ที่ปลายconnector
5. TPS/MAP/O2 transfer curvesและหน่วย, NTC resistance-temperature tableของECT/IAT, supported ranges/fault ranges; ไม่ใช้เปอร์เซ็นต์UIเป็นหน่วยไฟฟ้าโดยไม่มีcalibration
6. VR amplitude vs RPM, differential/individual line limits, waveform/zero-crossing, floating/referenceข้อกำหนด และโหลดสำหรับทดสอบ
7. แผน dummy-load/multimeter/oscilloscope checks: rail/load, actualvoltage/duty/resistance, open-collectorinversion, differentialVR, start/stop/reset/powerloss และ HTTP/WiFi timingstress

ยังไม่มีหลักฐานอนุมัติให้นำวงจรนี้ต่อ ECU จริงหรือส่งให้ลูกค้าเป็นผลิตภัณฑ์รองรับรถ ส่วนที่รองรับใน candidate ตอนนี้คือ digital/PWM bench-development outputs ตาม source เท่านั้น
