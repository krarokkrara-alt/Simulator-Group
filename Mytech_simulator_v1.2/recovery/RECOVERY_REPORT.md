# รายงานแผนก R and d for esp 32 wifi by Mytech Product

วันที่ตรวจ: 3 ตุลาคม 2026 (Asia/Bangkok)

อัปเดตสถานะต่อเนื่อง: 4 ตุลาคม 2026 ตามวันของผู้ใช้

## สถานะและที่มา

กู้ข้อความโค้ดจาก [แชตแชร์ต้นทาง](https://chatgpt.com/s/cx_6abe50737f908191a31663a6ba160733) โดยเปิด diff ทั้ง 42 รายการ และประกอบตามลำดับที่ปรากฏในแชต ตรวจบรรทัด deletion และ context ก่อน/หลังประกอบ ไม่พบความขัดแย้งใน 42 รายการ

ไฟล์อยู่ใน `ESP32_WiFi_StandAlone_Test/` มี 16 ไฟล์ที่มีข้อความใน diff: โค้ด 11 ไฟล์, Markdown 3 ไฟล์ และ SVG 2 ไฟล์ ค่าคงที่ firmware ที่กู้ได้คือ `0.3.0` ไม่ได้เพิ่มรุ่นใหม่

เริ่มงานพบ workspace มี `.git` แต่ไม่มีไฟล์โปรเจกต์จากการแสดงไฟล์ทั้งปกติและซ่อน ไม่พบ AGENTS.md ใน workspace จึงไม่มีไฟล์โค้ดเดิมที่ถูกเขียนทับ การกู้ทั้งหมดเพิ่มไฟล์ใหม่

ไฟล์กู้รักษาเนื้อหาจากแชต รวมข้อความประวัติใน CHANGELOG ว่าเคย compile/upload ผ่าน ข้อความนั้น **ไม่ใช่ผลทดสอบบนเครื่องหรือบอร์ดครั้งนี้**

## ขอบเขตความครบถ้วน

- กู้ข้อความโค้ดและ SVG ได้จาก diff ใช้ LF และ newline ท้ายไฟล์ จึงไม่อ้างว่า bytes ตรงไฟล์หรือ Git commit เดิม
- แชตเก่าอ้าง 18 ไฟล์ แต่กู้เนื้อหาข้อความได้ 16 ไฟล์ ส่วนภาพ PNG วงจร 2 ภาพยังไม่กู้ไฟล์ binary และยังไม่ยืนยันชื่อไฟล์เดิม
- ยังไม่ได้กู้ Git object `68b075a` หรือ full tree จึงตรวจเทียบ commit ต้นฉบับไม่ได้
- XML โปรไฟล์ Windows Wi-Fi ถูกสร้างแล้วลบในลำดับ diff จึงไม่คืนไฟล์นี้เป็น firmware และไม่ได้ import เข้าระบบ
- การกู้ไม่เติม SPI DAC, VR waveform, calibration หรือ trigger ของรถที่ต้นทางยังไม่ได้ทำ
- ข้อมูลประเภทเซนเซอร์ของรถใน VehicleProfiles เป็นข้อมูลจากต้นทางที่ยังไม่ได้ตรวจ service manual ของรุ่น/ปีจริง

`source-manifest.json` เก็บ checksum ของข้อความที่ประกอบใน browser ส่วน `verify-recovery.cjs` ตรวจไฟล์ที่บันทึกเทียบกับข้อความดังกล่าว พร้อมคำนวณ SHA-256 ของไฟล์ในเครื่อง FNV ใช้ตรวจการถ่ายข้อความ ไม่ใช่หลักฐานยืนยัน commit/bytes ต้นฉบับ

ผลตรวจสุดท้ายบันทึกใน verification-results.json: ผ่าน16/16ไฟล์ รวมSVGทั้งคู่ ตรวจXMLผ่าน เก็บสำเนาใน recovered-v0.3.0/ESP32_WiFi_StandAlone_Test/

## แยกคนทำ คนตรวจ และ QC

| หน้าที่ | งาน | หลักฐานรับงาน |
|---|---|---|
| ผู้จัดการ R&D | คุมขอบเขต กู้ตามลำดับ แยกสถานะอดีต/ปัจจุบัน | รายงานนี้และรายการหลักฐาน |
| ผู้ปฏิบัติงาน | กู้โค้ด คู่มือ และวงจร SVG | ไฟล์ที่บันทึกและ checksum |
| ผู้ตรวจโค้ดอิสระ | อ่านพฤติกรรมและข้อบกพร่อง ไม่แก้ baseline | findings พร้อมบรรทัดจริงด้านล่าง |
| QC อิสระ | ตรวจ include, API, ความครบถ้วนและคำกล่าวผลทดสอบ | เกณฑ์รับงานและข้อจำกัดด้านล่าง |

ผู้ตรวจโค้ดและ QC ตรวจอ่านแยกกัน บางครั้ง snapshot ของ QC ยังมีเพียง 14 ไฟล์ก่อนกู้ SVG เสร็จ จำนวนสุดท้ายให้ดู `verification-results.json` หลังงานกู้ทั้งหมดจบ

## ผลตรวจโค้ด (ยังไม่ได้แก้ baseline)

1. **P1: CKP timing ไม่ทัน RPM ที่ตั้ง** — `ESP32_WiFi_StandAlone_Test.ino:22` มี `delay(1)` แต่ `SignalGenerator.cpp:19` ต้องการ edge ทุก 500 µs ที่ 1000 RPM และประมาณ 62 µs ที่ 8000 RPM โค้ดสร้างเพียงหนึ่ง edge ต่อ loop และตั้ง `lastCkpUs = now` จึงขาดเวลาที่พลาดและเกิดความคลาดเคลื่อนสะสม
2. **P1: AP recovery ทำให้ waveform ค้าง** — `WebInterface.cpp:11` มี `delay(250)` และถูกเรียกจาก `handleClient()` เมื่อ health check ไม่ผ่าน ทำให้ main loop ไม่อัปเดต CKP/CMP ระหว่างนั้น ไม่มีคำสั่งลดขา LOW ก่อนพัก
3. **P2: SCOPE 1 kHz ยังรับรองไม่ได้** — `SignalGenerator.cpp:30` ต้อง toggle ทุก 500 µs แต่ถูกจำกัดด้วย loop เดียวกับ CKP
4. **P2: โปรไฟล์รถเปลี่ยนข้อมูลเท่านั้น** — `WebInterface.cpp:61` เปลี่ยน `vehicleProfile` แต่ SignalGenerator ไม่ใช้ค่านี้ จึงไม่เปลี่ยนรูปคลื่นตามรถหรือประเภท Hall/VR
5. **P2: CMP วนทุกหนึ่งรอบ crank** — `SignalGenerator.cpp:22,26` ใช้ 120 edge แล้วเริ่มใหม่ ไม่มีสถานะ 720° สำหรับเครื่องยนต์สี่จังหวะ
6. **P2: START หลัง STOP มี phase ไม่แน่นอน** — `SignalGenerator.cpp:13–16` รีเซ็ต tooth แต่ไม่รีเซ็ต ckpLevel/lastCkpUs
7. **P2: API ไม่มีการตรวจค่าผิดรูปแบบ** — `WebInterface.cpp:56,61,63` ใช้ toInt() โดยไม่ตรวจรูปแบบ เช่น rpm=abc กลายเป็น 0 และยังตอบ HTTP 200
8. **P3: RPM slider ถูก refresh ขณะลาก** — `WebPage.h:28,32` เขียน RPM กลับทุกวินาที ไม่มี activeElement guard สำหรับ RPM

PWM ของ TPS/MAP/ECT/IAT/O2 ยังเป็น duty 0–100% ไม่มีการแปลงแรงดัน/อุณหภูมิ/ความต้านทาน SELF TEST เป็นการสั่ง LED/SCOPE ไม่มีการวัด feedback เพื่อรับรองฮาร์ดแวร์ STOP ปิด CKP/CMP แต่ไม่ได้ปิด PWM และ SCOPE ทั้งหมด

## QC และสถานะทดสอบปัจจุบัน

include ใน workspace และ declaration/definition สอดคล้องกัน API `/api/state`, `/api/set`, `/api/profiles` และ field ที่ UI ใช้ตรงกับ firmware จากการอ่านโค้ด

ตอนเริ่มตรวจไม่พบ arduino-cli/pio/platformio ใน PATH และไม่พบเครื่องมือในตำแหน่ง Arduino IDE ที่ตรวจครั้งนี้ ต่อมาดาวน์โหลด Arduino CLI 1.5.1 ทางการไว้ใน tools/ และตรวจ SHA-256 ตรง release digest ติดตั้ง esptool 5.4.0 ใน workspace เพื่ออ่านรุ่นชิป และกำลังเตรียม Core 3.3.11 สำหรับ compile แบบไม่ upload สถานะ compile สุดท้ายจะบันทึกใน log แยก

สถานะล่าสุด: Core3.3.11ดาวน์โหลด/ติดตั้งลงโฟลเดอร์workspaceแล้ว แต่Arduino CLIโหลดplatformไม่ได้: following symlink .../tools/arduino-data/packages: Access is denied. ทดลองCLI1.5.1,1.3.1และขอสิทธิ์pathที่จำเป็นแล้ว ยังไม่ผ่าน การcompileฐานกู้และชุดพัฒนาexit1ก่อนโหลดplatform ดูcompile-recovered-v0.3.0.logและcompile-development-v0.3.1.log **ไม่มีfirmware binaryที่compileผ่าน**

ชุดพัฒนา development/ESP32_WiFi_StandAlone_Test v0.3.1-dev แยกจากฐาน: hardware timer2ตัว, generic60-2/720°, STARTphase/reset, STOPmutex/LOW และscope1kHznominal ผ่านการอ่านตรวจคนละคนกับผู้ทำ ยังไม่ได้compileหรือวัดบอร์ด ที่8000RPMปัดperiodเป็น63µsจึงnominal~7936.5RPM โปรไฟล์รถยังmetadataและไม่มีVRDAC ดูDEVELOPMENT_CHANGELOG.md

ตัวติดตั้งWindowsในcustomer-installerมีesptool.exeทางการ5.4.0และLICENSE ตรวจarchiveSHA256ตรงrelease, รันversionได้จริง Installerมีmanifest/hash/chip guard, log/timeout และล็อกretryหากยืนยันหยุดtoolไม่ได้ Parserผ่าน แต่ไม่มีrelease manifest/firmware จึงปิดInstallไว้ ไม่ใช่ชุดพร้อมส่งลูกค้า TimeoutterminationfixtureถูกOSAccessDeniedจึงไม่ถือว่าผ่านQC

การตรวจ USB แบบไม่เปิดพอร์ตพบ COM5, COM6, COM10; registry ของอุปกรณ์ serial ปัจจุบันแสดง `\Device\Serial2` ที่ COM10 ส่วน registry USB ที่ตรง COM10 ระบุ `USB-Enhanced-SERIAL CH9102 (COM10)`, VID_1A86, instance `5CDB011179` ไม่ใช่ CP210x ตามข้อมูลอดีต พอร์ต COM5/6 เป็น Bluetooth modem จาก registry

การอ่าน PnP ผ่าน CIM ถูกปฏิเสธสิทธิ์ จึงยังไม่ยืนยัน live PnP status หรือ ConfigManagerErrorCode ต่อมาผู้ใช้ให้ตรวจรุ่นบอร์ดโดยตรง จึงเรียก esptool --no-stub chip-id บน COM10 พร้อม reset เข้า/ออก bootloader เครื่องมือตอบ No serial data received (exit 1) บันทึกใน board-identification.txt ไม่มีการเขียน firmware หรือ erase flash ยังรอผู้ใช้กด BOOT/EN เพื่ออ่านซ้ำ และยังไม่ยืนยันรุ่นชิป, flash, firmware ที่ติดตั้ง, AP หรือ Web UI ปัจจุบัน

ผล serial.tools.list_ports และ Arduino CLI board list พบ USB COM10 VID:PID=1A86:55D4, serial=5CDB011179 ตรง CH9102; COM5/6 เป็น Bluetooth ดู usb-ports.txt

## แผน R&D ต่อจากฐานที่กู้

1. รับงานกู้ด้วย checksum ครบ 16 ไฟล์ และเก็บ snapshot ของฐานกู้ก่อนแก้
2. ตรวจเครื่องมือและ compile แบบไม่ upload พร้อมบันทึก core version, FQBN และ log เมื่อเครื่องมือพร้อม
3. ทำชุดพัฒนาแยกจาก snapshot: แยก timing ออกจาก HTTP/Wi-Fi, กำหนด safe state ตอน recovery และ phase ตอน START/STOP
4. เพิ่มรูปแบบ trigger 720°, polarity และ profile ที่มีข้อมูล service manual ยืนยัน; แยก metadata profile ออกจาก waveform profile
5. ตรวจ input/API และ Web UI จากนั้น QC กับ dummy load/oscilloscope จึงค่อยรับรองความถี่และ phase
6. ออกแบบ DAC/VR/ECT/IAT ต่อเมื่อกำหนด ECU และข้อกำหนดไฟฟ้าจริงแล้ว

## เกณฑ์รับงาน

งานกู้: ข้อความเทียบ manifest ผ่านครบ, รวม SVG ที่ตรวจ XML, ระบุ PNG/Git object ที่ขาด และแยกคำกล่าวผลทดสอบเก่าจากผลปัจจุบัน

งาน firmware ใช้งานจริง: ต้องมี compile log ของชุดพัฒนาและหลักฐานวัด CKP/CMP, START/STOP, SELF TEST, PWM และ AP recovery ก่อนระบุว่าใช้งานผ่าน

ยังไม่ได้ commit, push, upload หรือเปลี่ยนเครือข่าย Wi-Fi ในงานนี้

## Update 2026-10-04 01:48 Bangkok

Latest development firmware including strict parsed-argument API validation compiled successfully with Arduino CLI 1.5.1 / ESP32 Core 3.3.11 / FQBN esp32:esp32:esp32. Build-Firmware.ps1 completed with exit 0 after replacing unavailable Get-FileHash with .NET SHA256. Program storage 953515 bytes (72%); globals 47644 bytes (14%). Application SHA256: a60e8926e21bec8205c3ef418e53e363b86571f65b1671f025c0316f8de2a385. Merged image exists, 4194304 bytes. These are candidate artifacts only; no board was flashed.

Elevated workspace-only execution resolved Arduino CLI platform access. Baseline recovered source has not been compiled in this successful run. API parser compile assertions passed; independent QC found WebServer omits bare query segments without '=' before handler validation. See development/API_VALIDATION.md and QC_API_VALIDATION.md. ISR object inspection found callbacks in flash text because CONFIG_ARDUINO_ISR_IRAM is disabled; ELF/map review ongoing. Release remains locked pending hardware and installer QC.

## Update 2026-10-04 02:16 Bangkok

Independent QC verified the explicit IRAM revision from linked ELF: crank/scope callbacks and their literal pools are in IRAM; primitive mirror instance is in DRAM. See development/QC_IRAM_PLACEMENT.md for exact artifact hashes, normal-path dependencies and limitations. This confirms placement only; GPTIMER cache-safe allocation remains disabled and no hardware waveform result is available.

Installer SHA256 check now uses .NET directly instead of Get-FileHash, which was unavailable during actual Windows PowerShell build execution. Tests invoke the actual extracted function without launching installer or accessing a port; correct and uppercase hashes accepted, malformed/wrong/tampered hashes rejected (exit 0). Independent review pending. POST strict-body API migration is being developed and must be reviewed/rebuilt before updating build evidence again.

## Update 2026-10-04 02:31 Bangkok

POST strict-body API migration compiled successfully, session 80700 exit 0. Program storage 955811 bytes (72%), globals 47916 bytes (14%). Application SHA256 650553bdf04a9512c2b4244fa05180b1f48b8267c68be08eb375caf6b2be3f5d. Updated source/artifact/log hashes in development-build-evidence.json. GET control endpoint now returns 405 by source; POST reads only bounded raw body and UI reports rejected commands. Independent parser and embedded JavaScript tests passed; see QC_TRANSPORT_VALIDATION.md. HTTP transport, duplicate framing headers, multipart regression, hardware and waveform checks are still unverified. Additional review is examining duplicate Content-Type classification versus raw callback safety. Customer release remains locked.

## Update 2026-10-04 03:02 Bangkok

Recovered snapshot baseline compile completed with exit 0 (session83673), program storage 947251 bytes, globals 47604 bytes. See baseline-build-evidence.json. This confirms buildability of recovered visible text, not original byte identity or hardware timing.

Development raw-only custom RequestHandler compile completed exit0 (session33997), storage956311 bytes, globals47916 bytes, application SHA256 d70c0c45de64241b0dd19874bfd3e007cdb1975cbc632ba79e5b79851313d8af. Raw events are passed directly; upload paths are disabled; capture resets at request selection. Independent source review and parser/UI tests passed. Updated build evidence. Linked IRAM placement for this exact binary is being refreshed independently.

Installer actual-function AST guard tests passed 28 cases in Windows PowerShell5.1 without UI/COM or executing fixture files. See customer-installer/tests/Release-Validation.Report.md. These validate manifest/path/hash checks only, not real flash or provenance. Hardware chip identification, HTTP transport and installer timeout/Windows clean-machine checks remain pending. No release manifest or board flash occurred.

## Update 2026-10-04 03:16 Bangkok

Independent linked IRAM check of raw-only build passed scoped placement (QC_IRAM_RAW_HANDLER_BUILD.md), still no cache-off/hardware continuity certification. Actual-source offline preview is available at development/preview/index.html and root reran offline preview tests exit0; simulation only, no HTTP or board connection. Installer preview ZIP refreshed with current source/QC status, archive integrity passed and contains neither firmware nor approved manifest. Additional process-tree timeout fixture is underway using only dedicated spawned helper processes, without board access.

## Update 2026-10-04 03:31 Bangkok

Installer runner actual-function tests passed stdout/stderr success capture and rejection of nonzero exit7. Timeout process-tree termination remains unverified because taskkill was denied by sandbox (exit1); unsafeTermination retry latch correctly remained true. Dedicated helpers expired within their bounded lifetime; follow-up checks found no retained helper PIDs. Automatic approval review also rejected cleanup of fixture directories, which remain under tests and are excluded from the customer ZIP. Details in customer-installer/tests/Process-Validation.Report.md; no actual esptool, UI or COM invocation occurred.

## Update 2026-10-04 03:47 Bangkok

HTTP regression tool is available under development/tests, defaults to dry-run with no sockets. Independent QC passed localhost mock21cases and safety6tests; reports explicitly mark Core and hardware untested. Commands are limited to STOP/RPM0; board execution has not occurred. See HTTP_REGRESSION_README.md and QC_TRANSPORT_VALIDATION.md.

Hardware static audit found production blockers: PWM outputs do not implement 0–5V DAC or NTC resistance; VR/profile-specific waveforms are absent; Hall transistor inversion changes connector polarity; recovered SVG transistor connectivity is ambiguous. See HARDWARE_STATIC_REVIEW.md. Baseline diagrams remain preserved and cannot be treated as assembly netlists or ECU compatibility evidence. ECU/load/pinout requirements and measurements are needed before release.

## Update 2026-10-04 04:02 Bangkok

Active slider/polling revision passed independent actual-script mock9cases plus command/preview tests. Full compile session35800 exit0, storage958551 bytes(73%), globals47916(14%), appSHA256 3d8db7f35a06d9f0bb648459ab0cedba165594ce6e97af44056de505513d3a80. Build evidence refreshed. Source protects pointer/keyboard edits and pending controls from stale polling/response UI; this does not guarantee server applies last user intent in network-reordered POSTs. See QC_UI_SLIDERS.md. Native browser preview QA and linked placement refresh are pending. No board/UI-on-board tests or customer release approval.

## Update 2026-10-04 04:16 Bangkok

Independent linked IRAM review of active-slider binary passed placement and matches current source/build hashes in QC_IRAM_RAW_HANDLER_BUILD.md. Native browser preview QA could not run because browser inventory is empty; no visual/layout/touch pass is claimed (preview/VISUAL_QA_STATUS.md). Installer preview ZIP refreshed with current QC documentation and passed archive integrity. Further UI dispatch ordering work is underway; do not use a prior binary's evidence to certify source changes that have not been rebuilt.

## Update 2026-10-04 04:32 Bangkok

UI control POST dispatch is now serialized with bounded coalescing of unsent same-field jobs and STOP priority. Independent deferred tests10cases plus slider9cases passed, and fullcompile38020exit0 produced appSHA93b6d48126bc53bdc7f74cabf1120a34a5fe4a4d3efba4334561b482cb424f4b. STOP still waits for currentfetch and no physical stop deadline is claimed. See UI_COMMAND_QUEUE_CONTRACT.md / QC_UI_COMMAND_QUEUE.md. Review-only packaging with actual binary, source hashes and build-derived merged offset is being prepared; releaseApproved remainsfalse until hardware/transport/installer checks are complete.

## ผู้ใช้อนุญาตลง simulator — 4 ตุลาคม 2026

บอร์ดปัจจุบัน COM7, ESP32-D0WD-V3 revision3.1, flash4MB, MAC b4:bf:e9:61:4b:e0 ตรวจได้จริง esptool5.4.0 เปลี่ยน AP SSID ตามผู้ใช้เป็น Mytech_simulator_v1.2 รหัส12345678 IP192.168.4.1 sourceเปิดAPในsetupและhealthcheckทุก5วินาที

Compileล่าสุดexit0 appSHA130d6e59c3a5b6b902ef530db253062e61a49d3da15462d4d37acd16549c89be Upload COM7 exit0 ทุกsegment Hash of data verified ตาม build flash_args ดู upload-COM7-Mytech.log นี่เป็นการลงโดยผู้ใช้อนุญาตล่าสุด แทนสถานะเดิมที่ยังไม่upload ไม่ถือว่าhardwarewaveformหรือcustomerreleaseQCผ่าน การสแกนSSIDด้วยWindowsถูกlocationpermissionปิด ไม่เปลี่ยนWi-Fiคอม
Boot COM7 หลังreset: AP config OK, AP start OK, SSID Mytech_simulator_v1.2, IP192.168.4.1 ตรวจได้จริงใน boot-COM7-Mytech.txt ไม่ใช่ผลสแกน Wi-Fi จากคอม และยังไม่ได้ทดสอบ cold power cycle หลายรอบหรือความต่อเนื่องตลอดเวลา
