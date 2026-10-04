# Japanese CKP/CMP research — 2026-10-04

Research only. No firmware, catalogue or wiring changes; no board/ECU measurements. Primary aftermarket publisher documentation is evidence of documented engine patterns, not OEM ECU compatibility certification. Counts alone do not define a waveform. In this review no named vehicle family achieved a complete, source-confirmed CKP/CMP rising/falling edge table with widths, polarity and shared TDC origin sufficient for customer-ready implementation.

Angles below retain the publisher's shaft context. Where a tooth spacing on the cam is 90 camshaft degrees, that is 180 crankshaft degrees. It does not establish tooth leading edges, tooth width, sensor switching threshold or phase relative to a crank gap. Never fill the missing information using generic CMP.

| Family/variant | Primary documented facts | Exact edge/width status and blocker |
|---|---|---|
|Toyota/Lexus early 1UZ-FE 1989–1997|12 crank teeth; one cam tooth per bank; reluctor|No cam phase/width; bank selection and sensor polarity needed|
|Toyota/Lexus 1UZ VVTi, 2UZ, 3UZ|36-2 crank; three cam teeth; two cam channels|Width and crank-relative phase absent; VVT/bank channels required|
|Toyota JZ non-VVTi/VVTi crank-sensor variants|12/single cam versus36-2/three cam; distributor variants distinct|No complete edge table; selected G1/G2 signal matters|
|Toyota 1FZ-FE100-series waste spark|36-2; two cam teeth90° apart; reluctor|Camshaft spacing only; widths/absolute phase unknown|
|Honda F20C AP1 S2000|12 equal crank; three cam teeth90° apart on both cams; reluctor|Relative cam spacing known; widths, common origin and chosen channel unknown|
|Honda K20/K24 cited article variants|12+1 crank; exhaust4+1cam, intake4cam|Extra-tooth offsets and widths absent; cannot synthesize from counts|
|Nissan SR20 S13/S14/S15 CAS|360 optical slots plus4 low-resolution slots per720crank°|Cam-driven CAS; unequal low-resolution slot widths and phase unconfirmed. 360 is NOT per crank revolution|
|Nissan RB20/25/26 DOHC CAS|Cam-driven360-slot and6-slot tracks; low-resolution events120crank° apart|Slot widths, edge polarity, shared origin missing. Publisher labels differ between overview and wiring; retain physical tracks rather than assume CKP/CMP names|
|Mitsubishi4G63 early CAS /EVO4–8 /EVO9|Four primary events per720; two cam events; later2crank teeth; EVO9cam inverted|Full high/low durations absent. Same counts do not prove same voltage polarity or VVT phase|
|Subaru EJ20G/K|Six crank/seven cam; reluctor|Uneven edge angles and widths missing|
|Subaru EJ207 early/later|36-2-2-2 crank; two/three cam teeth90° apart|Variant split not exact year mapping; widths/common phase missing|
|Subaru EJ25 WRX scope|36-2-2-2 crankVR; three cam teeth90° apart; camHall;2–4cam sensors|Cylinder1 intake used for position; other channels/VVT and edge widths missing|
|Subaru EZ30D|36multi-gap crank; three equally spaced teeth onBank2cam|Crank gaps displaced versus four-cylinder layout; widths/common origin unknown|
|Mazda NB2 BP-Z3/BP-VE|Four uneven crank/three cam Hall|Close extra cam event and exact edge layout unresolved; article year discrepancy|
|Mazda LF-VE NC|36-2-2-2 crank; four equal cam teeth plus two extras; Hall|Extra angles/widths and VVT phase missing|
|Suzuki M16A ZC31S SwiftSportMY05–11|36-2-2-2 crank; six cam teeth, four equal plus two extras|Extra angles/widths missing. Publisher explicitly says ZC32S untested/unsupported; do not extend mapping|
|Suzuki Swift distributor /Vitara2.0|12 uneven distributor events /11 uneven crank respectively|No complete CMP layout confirmed; not a basis for M16A or allSuzuki|
|Isuzu|No engine-specific primary CKP/CMP geometry established in this search|Need engine code/year/ECU part number and OEM service waveform or independently documented pattern. No count/angle invented|

## Sources and provenance

Factual paraphrase only, publisher copyright; no copied decoder code or GPL waveform arrays.

- Toyota/Lexus UZ: https://support.haltech.com/portal/en/kb/articles/1uz-fe-engine
- Toyota JZ: https://support.haltech.com/portal/en/kb/articles/2jz-1jz-engine
- Toyota1FZ: https://support.haltech.com/portal/en/kb/articles/fz-engine
- General Toyota spacing explanation: https://support.haltech.com/portal/en/kb/articles/engine-trigger-systems-explained
- HondaF20C: https://support.haltech.com/portal/en/kb/articles/f20c-ap1-engine
- HondaK: https://support.haltech.com/portal/en/kb/articles/thomas-te
- NissanSR: https://support.haltech.com/portal/en/kb/articles/sr20-trigger
- NissanRB: https://support.haltech.com/portal/en/kb/articles/rb-engine
- Mitsubishi: https://support.haltech.com/portal/en/kb/articles/4g63-engine
- SubaruEJ20: https://support.haltech.com/portal/en/kb/articles/ej20-engine
- SubaruEJ25: https://support.haltech.com/portal/en/kb/articles/ej25-engine
- SubaruEZ30: https://support.haltech.com/portal/en/kb/articles/ez30-engine
- MazdaNB2: https://support.haltech.com/portal/en/kb/articles/01-05-nb2-1-8l-w-vvt-bp-z3-bp-ve
- MazdaLF: https://support.haltech.com/portal/en/kb/articles/mazda-lf-mzr-engine
- SuzukiM16A: https://support.haltech.com/portal/en/kb/articles/m16a-engine
- SuzukiSwift/Vitara: https://megasquirt.co.uk/doc/pdf/MS3XV357_Hardware-1.3.pdf sections6.22–6.23; copyrightJamesMurray2014

The Haltech embedded SR20 configuration image link was opened but the web reader did not expose a readable numeric edge table. An image placeholder or screenshot of ECU configuration is insufficient to infer OEM slot width. Search results containing experimental community decoder modifications were excluded from verified geometry.

## Implementation candidates and required evidence

Early single-cam-tooth Toyota1UZ/JZ is the smallest eventual waveform model, followed by HondaF20C or Subaru3-tooth cam. These are candidates for further measurement, not currently verified CMP implementations. Establish each rising/falling angle over720° against one specified crank gap/TDC, default/parked VVT state, selected bank, sensor signal inversion, ECU thresholds/pullups and minimum pulse-width tolerance. Multi-cam ECU expectations need additional channels or a documented restricted bench target.

Until those facts exist, a regular CKP-only digital template with CMP disabled may demonstrate pulse count/gap timing if clearly labelled a bench approximation. It must not be described as vehicle sync, OEM RUN, or VR simulation. VR waveform amplitudes, zero crossings and speed dependence require separate electrical evidence.

For a future completed edge table, validate edge order, high/low width,720°wrap, gap-to-CMP phase, polarity and per-channel count using a software oracle; then compare logic-analyzer traces under RPM changes and network stress. These checks were not performed by this research task.
