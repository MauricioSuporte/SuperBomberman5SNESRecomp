"""Rollback determinism of the game and its mods, using a local host/ROM.

Netplay rolls back: it restores a snapshot and re-simulates frames with the
corrected inputs. Any host-side mod state the snapshot misses makes peers
diverge. This drives Battle (or Normal Game) with SNESRECOMP_RB_PROBE armed:
every PERIOD frames the framework saves, runs, restores and replays the same
frames in-process and reports `rb_probe: DIVERGED` when they disagree.
Captures stay under ignored .bootstrap/.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from pause_menu_runtime import BATTLE_ENTER

ROOT = Path(__file__).resolve().parents[1]
from sb5_mod_sets import ALL_MODS

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--rom', type=Path, required=True)
parser.add_argument('--scene', choices=['battle', 'normal'], default='battle')
parser.add_argument('--mods', choices=['none', 'all'], default='all')
parser.add_argument('--period', default='29:3')
parser.add_argument('--frames', type=int, default=1500, help='gameplay frames after the scene starts')
parser.add_argument('--out', type=Path, default=ROOT / '.bootstrap/netplay-determinism')
args = parser.parse_args()


def gameplay(frames: int) -> str:
    moves = ['right', 'down', 'left', 'up']
    route, left = '', frames
    i = 0
    while left > 0:
        route += f'press {moves[i % 4]} 20\npress b 2\npress p2:{moves[(i + 1) % 4]} 20\npress p2:b 2\nwait 16\n'
        left -= 60
        i += 1
    return route


if args.scene == 'battle':
    route = BATTLE_ENTER
else:
    route = ('turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\npress start 2\nwait 180\n'
             'press down 2\nwait 25\npress start 2\nwait 180\npress a 2\nwait 90\npress a 2\nwait 180\n'
             'press a 2\nwait 900\npress start 2\nwait 800\ndump stage\n')
route += 'turbo off\n' + gameplay(args.frames) + 'dump end\nquit\n'

args.out.mkdir(parents=True, exist_ok=True)
folder = Path(tempfile.mkdtemp(prefix=f'{args.scene}-{args.mods}-', dir=args.out.resolve()))
shutil.copy2(args.exe, folder / 'game.exe')
shutil.copytree(ROOT / 'mods', folder / 'mods')
state = 'format_version = 1\n'
for package, feature in (ALL_MODS if args.mods == 'all' else []):
    state += (f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n'
              f'[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\nenabled = true\n')
(folder / 'mods/preloaded/state.toml').write_text(state, encoding='utf-8')
(folder / 'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nWindowScale=1\n[Sound]\nEnableAudio=1\n')
(folder / 'route.input').write_text(route)
env = os.environ.copy()
env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SNESRECOMP_DUMP_DIR=str(folder),
           SNESRECOMP_RB_PROBE=args.period, SNESRECOMP_RB_PROBE_VERBOSE='1')
with (folder / 'run.log').open('w') as log:
    result = subprocess.run([str(folder / 'game.exe'), '--no-launcher', '--config', str(folder / 'headless.ini'),
                             '--script', str(folder / 'route.input'), str(args.rom.resolve())], cwd=folder, env=env,
                            stdout=log, stderr=log, timeout=900)
text = (folder / 'run.log').read_text(errors='replace')
assert 'rb_probe: armed' in text, 'this build has no rollback probe'
probes = len(re.findall(r'rb_probe: (?:symmetric|DIVERGED|FAIL|ASYMMETRIC|LOSSY)', text))
diverged = [line for line in text.splitlines() if re.search(r'rb_probe: (DIVERGED|FAIL|ASYMMETRIC|LOSSY)', line)]
print(f'{args.scene}/{args.mods}: exit {result.returncode}, probe lines {probes}, diverged {len(diverged)} ({folder})')
for line in diverged[:10]:
    print('  ' + line)
assert result.returncode == 0, result.returncode
assert not diverged, 'rollback replay diverged'
