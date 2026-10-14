"""Run the shipped analyzer on a synthetic RTS ROM without game data or Rust."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--analyzer', type=Path, required=True)
args = parser.parse_args()
analyzer = args.analyzer.resolve()
if not analyzer.is_file():
    raise SystemExit('Packaged analyzer must exist.')
if analyzer.suffix.lower() == '.exe' and not analyzer.with_name('libunwind.dll').is_file():
    raise SystemExit('Windows analyzer and libunwind.dll must exist together.')
root = Path(__file__).resolve().parents[1]
local_tmp = root / '.bootstrap'
local_tmp.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='analyzer-smoke-', dir=local_tmp) as directory:
    tmp = Path(directory).resolve()
    assert tmp.is_relative_to(local_tmp.resolve())
    image = bytearray(32768)
    image[0] = 0x60  # RTS: entirely synthetic, no commercial ROM bytes.
    image[0x7FC0:0x7FD5] = b'SNESRECOMP SYNTHETIC  '
    image[0x7FD5] = 0x20
    image[0x7FD7] = 5
    image[0x7FD9] = 1
    image[0x7FDC:0x7FE0] = bytes([0xCB, 0xED, 0x34, 0x12])
    image[0x7FFC:0x7FFE] = bytes([0, 0x80])
    rom = tmp / 'synthetic.sfc'
    rom.write_bytes(image)
    cfg = tmp / 'cfg'
    cfg.mkdir()
    (cfg / 'bank00.cfg').write_text('bank = 0\n', encoding='utf-8')
    manifest = tmp / 'manifest.json'
    env = dict(os.environ)
    if os.name == 'nt':
        # Do not let the developer toolchain PATH hide a missing shipped DLL.
        env['PATH'] = str(Path(env.get('SystemRoot', 'C:/Windows')) / 'System32')
    subprocess.run([str(analyzer), '--rom', str(rom), '--cfg-dir', str(cfg),
                    '--manifest', str(manifest), '--root', '008000:1:1'],
                   check=True, env=env)
    result = json.loads(manifest.read_text())
    assert any(n.get('instruction_count') == 1 for n in result['nodes'].values())
print('Packaged analyzer passed synthetic-ROM smoke test without Rust/toolchain PATH.')
