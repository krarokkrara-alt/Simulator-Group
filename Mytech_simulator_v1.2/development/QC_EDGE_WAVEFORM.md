# Independent QC: pure EdgeWaveform candidate

2026-10-05. Accept the pure candidate for further development after the initial-deadline correction. This header is not integrated with live firmware/timers or an OEM catalogue. No full firmware compile, board access, flash or timing measurement was performed.

QC found that a positive first angle could round to deadline zero at high RPM while all cyclic intervals stayed positive. That would violate the documented initial timer contract. Example: CKP rises at tick 1 and falls at tick 360000, 8000 RPM. Maker added a Resolution rejection for nonzero first angle with zero rounded deadline. Independent regression assertion now passes against the corrected header.

Reviewed behavior: initialLevels means immediately before angle zero; exact angle-zero events apply synchronously at START. All listed channels must transition; simultaneous CKP/CMP transitions use one merged edge. Unique increasing angles exclude tick 720000. Explicit final state must equal initial state, so wrapping adds no hidden transition. Per-channel polarity XOR is separate from logical edge geometry; STOP is explicitly physical LOW regardless of polarity.

Storage/count/mask checks reject null, empty, over-256, invalid initial/polarity/channel masks, levels outside the selected channels, redundant transitions, decreasing/duplicate/out-of-cycle angles and nonclosing seams. Caller ownership, array capacity and immutable lifetime cannot be verified from a pointer/count and remain caller obligations. Timing/helper functions assume validated tables and valid indices; no live caller exists in this candidate.

Cumulative rounded deadlines telescope to the rounded 720-degree cycle duration, preventing independent-interval rounding drift. RPM range is 1..8000. Zero intervals, rounded endpoint collisions and the corrected zero initial delay reject with Resolution rather than clamping. This quantization model does not establish physical ISR execution latency or minimum hardware pulse width.

Independent actual xtensa-esp-elf-g++.exe -std=c++17 -Wall -Wextra -Werror -fsyntax-only runs both exited 0:

- tests/edge_waveform_tests.cpp: maker constexpr tests including synthetic irregular CKP/multi-pulse CMP, wrapped interval sums, polarity, malformed tables and timing bounds.
- tests/edge_waveform_qc_tests.cpp: independently added assertions for near-origin initial deadline, simultaneous inverted channels, partially redundant merged edge, endpoint rounding and decreasing angles.

The assertions execute at compile time; no executable was run. Header SHA256: 1b9792f7972b2d101cec5d4dddbb9db732f1a564f42222d64be414387658f93f.

Before live integration: specify validated table publication/lifetime and STOP synchronization; keep 64-bit deadline arithmetic outside ISR where possible; verify linked IRAM/DRAM and literal placement; measure startup, simultaneous transitions, wrap and STOP on an oscilloscope. Exact sourced vehicle angle origin/polarity/phase/duty and electrical interface evidence are required for any OEM claim.
