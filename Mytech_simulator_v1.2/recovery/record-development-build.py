"""Record candidate build evidence; never approve a release or access a board."""
import hashlib
import json
import argparse
from pathlib import Path
from datetime import datetime, timezone

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--observed-build-exit', type=int, help='Exit code observed by caller; never inferred from file presence.')
args = parser.parse_args()
sketch = root / 'development' / 'ESP32_WiFi_StandAlone_Test'
build = root / 'build' / 'development-v0.3.1'

def records(paths):
    return [{'path': str(p.relative_to(root)).replace('\\', '/'),
             'bytes': p.stat().st_size,
             'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
            for p in sorted(paths)]

result = {
    'recordedUtc': datetime.now(timezone.utc).isoformat(),
    'purpose': 'Candidate evidence only; file presence does not prove compile success or hardware QC.',
    'fqbn': 'esp32:esp32:esp32',
    'arduinoCore': '3.3.11',
    'arduinoCli': '1.5.1',
    'buildCommand': 'powershell -NoProfile -ExecutionPolicy Bypass -File .\\Build-Firmware.ps1',
    'observedBuildExitCode': args.observed_build_exit,
    'releaseApproved': False,
    'sources': records(p for p in sketch.iterdir() if p.suffix in ('.h', '.cpp', '.ino')),
    'artifacts': records(p for p in build.iterdir() if p.suffix in ('.bin', '.elf', '.map')),
    'log': records([root / 'recovery' / 'compile-development-v0.3.1.log']),
}
target = root / 'recovery' / 'development-build-evidence.json'
target.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(f'Recorded {len(result["sources"])} source files and {len(result["artifacts"])} artifacts; release locked.')
