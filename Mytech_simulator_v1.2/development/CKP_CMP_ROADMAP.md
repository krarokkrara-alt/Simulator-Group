# CKP/CMP development roadmap

2026-10-05. Proposal for team review through Git; no firmware, profile, wiring or release changes performed. Based on JAPANESE_CMP_RESEARCH.md and japanese-cmp-coverage.json. The inventory records documented families, not implemented CMP coverage. Current regular-wheel plus one rectangular CMP engine cannot express the uneven/multi-edge/multi-channel Japanese patterns. No researched vehicle currently has a complete verified edge table in these files.

## Proposed schema

Keep a versioned research definition separate from the executable bench profile. The latter references the former by immutable ID and revision; it must not silently substitute assumed values for unknown source geometry.

| Group | Required fields and meaning |
|---|---|
|Identity|schemaVersion, definitionId, revision, brand, engineCode, vehicleCodes, market, modelYearRange, ECU part/firmware IDs, explicit exclusions; unknowns null|
|Provenance|sources with publisher, URL, document revision/page/figure, retrieved date, license, artifact hash when permitted; per-field evidence references, reviewer and verification method|
|Source geometry|cycleCrankDegrees=720 for these four-stroke definitions; channel ID, physical track, sensor shaft, revolutionsPerCrankRevolution, sensor type, bank/cam role, nominal teeth, gap positions, actual pulses per cycle|
|Angle origin|named shared origin, cylinder and compression stroke, defined reference edge/gap, source units and conversion; measured offset uncertainty; unresolved origin null|
|Edges|explicit ordered angleCrankDeg and levelAfterEdge per channel; initialLevel; each edge evidence reference and tolerance. Alternatively paired high windows normalized into this same representation; never mix both as independent truth|
|Dynamic state|VVT parked position, supported phase range, phase direction and channel coupling; fixed-phase-only scope when dynamic feedback unsupported|
|Electrical target|sensor voltage class, pullup/source/sink model, active polarity, ECU input thresholds, load/current bounds, idle behavior at connector, compatible interface revision; unsupported or unknown explicit|
|Bench approximation|separate approximationId, generated edge definition, assumed width/duty/phase with rationale, supported RPM, channels intentionally disabled, digital-only label, sourceDefinitionRef. No OEM compatibility inference|
|Validation|stage, blocking reasons, test artifact references and hashes, firmware/build identity, board identity, measured timing limits, ECU-tested target, approvers; claims limited to tested scope|

Represent rational shaft ratios exactly (e.g. cam=1/2 crank speed). Nissan CAS360 means360 events per cam turn, hence per720 crank degrees; it is not360 per360 crank degrees. Names such as Trigger/Home are ECU-specific assignments, so retain physical track identity as well as logical output role.

For complete digital geometry, pulse width is the angular distance between rising and falling edges, including the720° wrap. Counts and relative tooth spacing do not establish width or absolute CKP/CMP phase. VR mechanical teeth/zero crossings must not be called digital high/low windows without a documented conditioning model. Unknown cam geometry remains null and its output disabled; empty edge arrays mean a deliberately disabled channel, not missing evidence.

## Work sequence and owners

1. Research owner narrows one exact engine/vehicle/ECU target and supplies both edge polarities, widths and common origin. Early single-tooth Toyota JZ/UZ offers the smallest prospective model, but its current width/phase evidence is incomplete. Isuzu remains a research gap. Do not expand by brand alone.
2. Implementer adds a channel-aware edge scheduler only after a reviewed definition exists. Compile immutable bounded arrays outside the ISR; ISR performs prevalidated edge changes and scheduling without String/parsing/allocation. Reject unsupported channel count, minimum interval, geometry and RPM before START. Treat profile, RPM, phase and enable state as one coherent transition, preserving defined START/STOP behavior.
3. Independent code reviewer checks scheduler timing, arithmetic overflow, wrap, channel ordering, simultaneous edges, critical-section scope and linked interrupt dependencies. A compile proves build compatibility, not waveform correctness.
4. QC owner checks source assertions and an independent edge oracle, then captures actual connector traces using the selected interface and board. Manager records blockers and approves the limited claim only when evidence matches the artifact being reviewed.

## Staged acceptance

| Stage | Evidence required | Allowed claim |
|---|---|---|
|Inventory|Primary source engine mapping/counts; unknowns preserved|Research reference only|
|Geometry reviewed|Complete per-channel edge table, polarity, shared origin, provenance and independent reviewer; variant exclusions|Defined source geometry, not tested output|
|Software validated|Independent expected edge sequence; counts, widths, phase,720wrap, simultaneous transitions, inverted polarity, disabled CMP, invalid definitions and RPM rejection; START/STOP/profile changes checked|Software model verified for stated definitions|
|Build checked|Exact source/definition hashes, compiler/core version, target chip, linked timing/interrupt dependency review, binary hashes|Candidate firmware builds|
|Board scope checked|Board and interface IDs; measured edge timestamps and widths across RPM range, jitter tolerance, START/STOP and profile transitions, network stress; capture files linked to binary hash|Measured digital bench output within recorded bounds|
|Specific ECU bench checked|Known ECU/load/input specifications and interface; CKP/CMP sync result and error counters; bounded parked/dynamic VVT and required channels; independently reviewed traces|Compatibility limited to tested ECU/variant/interface|
|Customer release|QC and manager approval tied to exact immutable artifacts; installer guards and release manifest reflect approved hardware/software scope; repeatable instructions and unresolved limitations|Approved release for stated target only|

RPM, jitter, quantization and minimum high/low time thresholds must be stated before hardware acceptance using the target input requirements and timer resolution; this roadmap invents no universal tolerances. Test acceleration/phase changes as well as steady RPM. Electrical idle level at the connector can differ from GPIO LOW through an inverting interface, so measure both and define the intended STOP result.

QC requires the executable table to combine simultaneous channel edges into one timestamp with an explicit valid channel mask and levels. Validate sorted unique event angles, initial logical levels, final-to-initial seam behavior and physical inversion mask, marking each electrical assumption separately. Reject out-of-range/no-op edges, invalid masks, null geometry, unsafe table lifetime, deadline collisions or intervals below supported timer resolution. Record minimum angular separation, converted time at maximum RPM and computed quantization error. Offline oracle results remain separate from ISR behavior and hardware evidence.

Keep changes reviewable in separate Git commits: schema/evidence first, scheduler next, definition import next, tests/captures next, release metadata last. Each definition change invalidates dependent validation unless a reviewer documents why prior evidence still applies. Never mark a profile enabled merely because its brand appears in the inventory or its pulse count matches.

## Current checkpoint

This document completes the planning/research review only. Exact Japanese CMP geometry acquisition, scheduler implementation, board waveform measurements, ECU compatibility and release approval remain pending. No speculative CMP, flash or customer release is authorized by this file.

Code-review coordination reports a separate pure candidate EdgeWaveform.h using combined masked edges, 1000 ticks/degree, at most256 events and rounded cumulative deadlines to avoid segment rounding drift. It reportedly rejects collisions rather than clamping and applies angle0 synchronously. This roadmap did not independently inspect that candidate. Those implementation limits must become explicit schema capability checks; immutable pointer lifetime, atomic publication/reset, firmware integration and linked/hardware timing evidence remain separate requirements. A helper implementation does not resolve missing source geometry.
