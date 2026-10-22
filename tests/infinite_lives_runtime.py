"""Verify Normal Game infinite lives using a local host/ROM.

Each scenario enters Zone 1-1, sets the life counter to zero and forces a
time-over death. Without the mod the stock retry charges every participant and
the negative counter starts Game Over; with it the counter stays at zero and
the stage restarts. Captures stay under ignored .bootstrap/.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--rom', type=Path, required=True)
parser.add_argument('--players', type=int, choices=[1, 2, 5], action='append')
parser.add_argument('--out', type=Path, default=ROOT / '.bootstrap/infinite-lives-validation')
args = parser.parse_args()
exe, rom, out = args.exe.resolve(), args.rom.resolve(), args.out.resolve()
assert exe.is_file() and rom.is_file()


def enter(players: int) -> str:
    route = ('turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\n'
             'press start 2\nwait 180\n')
    route += 'press down 2\nwait 25\n' * (players - 1)
    route += ('press start 2\nwait 180\npress a 2\nwait 90\n'
              'press a 2\nwait 180\npress a 2\nwait 900\n')
    return route + 'press start 2\nwait 800\ndump stage\n'


def run(name: str, route: str, mods: list[tuple[str, str]]) -> Path:
    out.mkdir(parents=True, exist_ok=True)
    folder = Path(tempfile.mkdtemp(prefix=name + '-', dir=out))
    shutil.copy2(exe, folder / 'game.exe')
    shutil.copytree(ROOT / 'mods', folder / 'mods', dirs_exist_ok=True)
    state = 'format_version = 1\n'
    for package, feature in mods:
        state += (f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n'
                  f'[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\nenabled = true\n')
    (folder / 'mods/preloaded/state.toml').write_text(state, encoding='utf-8')
    (folder / 'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nWindowScale=1\n[Sound]\nEnableAudio=0\n')
    (folder / 'route.input').write_text(route + 'quit\n')
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SNESRECOMP_DUMP_DIR=str(folder))
    with (folder / 'run.log').open('w') as log:
        result = subprocess.run([str(folder / 'game.exe'), '--no-launcher', '--config', str(folder / 'headless.ini'),
                                 '--script', str(folder / 'route.input'), str(rom)], cwd=folder, env=env,
                                stdout=log, stderr=log, timeout=600)
    assert result.returncode == 0, (name, result.returncode)
    log = (folder / 'run.log').read_text(errors='replace').lower()
    for failure in ['unknown button', 'architectural brk', 'watchdog tripped', 'dispatch_abandon', 'interp_cap',
                    'unresolved-abandon', 'fatal']:
        assert failure not in log, (name, failure)
    return folder


def ram(folder: Path, tag: str) -> bytes:
    return (folder / f'{tag}.wram.bin').read_bytes()


death = ('poke 0x1d23 00\npoke 0xe1c 00\npoke 0xe1e 00\npoke 0xe1d 01\nwait 400\ndump died\n'
         'wait 900\ndump after\n')
for players, extra in [(1, []), (2, []), (5, [('normal-coop', 'five_players')])]:
    if args.players and players not in args.players:
        continue
    for enabled in (False, True):
        mods = extra + ([('infinite-lives', 'infinite_lives')] if enabled else [])
        folder = run(f'lives-{players}-{"on" if enabled else "off"}', enter(players) + death, mods)
        stage, after = ram(folder, 'stage'), ram(folder, 'after')
        assert stage[0x1031] == 0x0d and not (stage[0x1d1a] & 1), (players, enabled)
        if enabled:
            assert after[0x1d23] == 0, (players, after[0x1d23])
            assert after[0x1031] == 0x0d, (players, hex(after[0x1031]))
        else:
            assert after[0x1d23] >= 0x80 or after[0x1031] != 0x0d, (players, after[0x1d23], hex(after[0x1031]))
        print(f'{players} player(s), mod {"on" if enabled else "off"}: lives {after[0x1d23]}, '
              f'mode {after[0x1031]:#x}', flush=True)
print('Infinite lives runtime scenarios passed.')
