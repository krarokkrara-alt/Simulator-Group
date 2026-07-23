#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
required = [
    root / "platformio.ini",
    root / "src/main.cpp",
    root / "src/signal_engine.cpp",
    root / "src/board_health.cpp",
    root / "src/web_ui.cpp",
    root / "include/config.h",
]
missing = [str(p) for p in required if not p.exists()]
if missing:
    print("FAIL missing:", *missing, sep="\n")
    sys.exit(1)

ini = (root / "platformio.ini").read_text(encoding="utf-8")
envs = re.findall(r"\[env:([^\]]+)\]", ini)
if len(envs) != 10:
    print(f"FAIL expected 10 build variants, found {len(envs)}")
    sys.exit(1)

config = (root / "include/config.h").read_text(encoding="utf-8")
pins = dict(re.findall(r"constexpr uint8_t ([A-Z0-9_]+) = (\d+);", config))
values = list(pins.values())
duplicates = sorted({v for v in values if values.count(v) > 1})
if duplicates:
    print("FAIL duplicate configured pins:", duplicates)
    sys.exit(1)

health = (root / "src/board_health.cpp").read_text(encoding="utf-8")
checks = ["adcMap", "adcTps", "timing", "memory", "wifi", "watchdog"]
if not all(name in health for name in checks):
    print("FAIL incomplete health checks")
    sys.exit(1)

print("PASS static project check")
print("Build variants:", ", ".join(envs))
print("Configured pins:", ", ".join(f"{k}={v}" for k, v in pins.items()))

