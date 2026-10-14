"""Exercise the native campaign with a supplied, locally built host and ROM.

Captures stay under ignored .bootstrap/. Use --fixture only with a host built
with SB5_COOP_INPUT_FIXTURE=ON; the default host never reads its mailboxes.
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
parser.add_argument('--out', type=Path, default=ROOT / '.bootstrap/infinite-time-validation')
parser.add_argument('--fixture', action='store_true')
parser.add_argument('--new-renderer', action='store_true', help='Exercise the optimized renderer used by play.cmd.')
parser.add_argument('--cases', choices=['counts', 'five', 'lifecycle', 'all'], default='all')
args = parser.parse_args()
exe, rom, out = args.exe.resolve(), args.rom.resolve(), args.out.resolve()
assert exe.is_file() and rom.is_file()


def press(seat: int, button: str, wait: int = 40) -> str:
    if seat <= 2:
        return f'press {"" if seat == 1 else "p2:"}{button} 2\nwait {wait}\n'
    assert args.fixture, 'Extra-pad injection requires the opt-in test fixture.'
    bits = {'a': 0x100, 'r': 0x800, 'right': 0x80, 'left': 0x40, 'start': 8}[button]
    address = 0x1ffe0 + (seat - 3) * 2
    return f'poke {address:#x} {bits.to_bytes(2, "little").hex()}\nwait 3\npoke {address:#x} 0000\nwait {wait}\n'


def enter(players: int, skins: bool = False) -> str:
    route = ('turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\n'
             'press start 2\nwait 180\n')
    route += 'press down 2\nwait 25\n' * (players - 1)
    route += ('dump players\npress start 2\nwait 180\npress a 2\nwait 90\n'
              'press a 2\nwait 180\npress a 2\nwait 700\n')
    if skins:
        route += 'dump picker\n'
        for seat in range(1, players + 1):
            route += press(seat, 'right') * (seat - 1)
            route += press(seat, 'r') * seat
            route += press(seat, 'a', 180) + f'dump confirm{seat}\n'
        route += 'wait 1200\n'
    return route + 'press start 2\nwait 800\ndump stage\n'


def death(seat: int) -> str:
    assert args.fixture
    return f'poke 0x1ffe7 {1 << (seat - 1):02x}\nwait 20\n'


def run(name: str, route: str, skins: bool = False, ptbr: bool = False) -> Path:
    out.mkdir(parents=True, exist_ok=True)
    # Each run starts with fresh SRAM. Reusing a completed campaign file
    # changes the file-menu route and can bypass New Game confirmation.
    folder = Path(tempfile.mkdtemp(prefix=name + '-', dir=out))
    shutil.copy2(exe, folder / 'game.exe')
    shutil.copytree(ROOT / 'mods', folder / 'mods', dirs_exist_ok=True)
    mods = []
    if unlimited:
        mods += [('infinite-time', 'infinite_time')]
    if skins:
        mods += [('normal-characters', 'character_select'), ('selection-colors', 'color_controls')]
    if ptbr:
        mods += [('ptbr', 'menus_ptbr')]
    state = 'format_version = 1\n'
    for package, feature in mods:
        state += (f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n'
                  f'[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\nenabled = true\n')
    (folder / 'mods/preloaded/state.toml').write_text(state, encoding='utf-8')
    (folder / 'headless.ini').write_text(f'[Graphics]\nOutputMethod=SDL-Software\nWindowScale=1\nNewRenderer={int(args.new_renderer)}\n[Sound]\nEnableAudio=1\n')
    (folder / 'route.input').write_text(route + 'quit\n')
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SNESRECOMP_DUMP_DIR=str(folder))
    with (folder / 'run.log').open('w') as log:
        result = subprocess.run([str(folder / 'game.exe'), '--no-launcher', '--config', str(folder / 'headless.ini'),
                                 '--script', str(folder / 'route.input'), str(rom)], cwd=folder, env=env,
                                stdout=log, stderr=log, timeout=300)
    assert result.returncode == 0, (name, result.returncode)
    log = (folder / 'run.log').read_text(errors='replace').lower()
    for failure in ['unknown button', 'architectural brk', 'watchdog tripped', 'dispatch_abandon']:
        assert failure not in log, (name, failure)
    return folder


def ram(folder: Path, tag: str) -> bytes:
    return (folder / f'{tag}.wram.bin').read_bytes()


def alive(data: bytes, seat: int) -> bool:
    return not (data[0x17fa + seat - 1] & 0x80 or data[0x15a2 + seat - 1] & 0x80)


def stage(data: bytes, players: int) -> None:
    assert data[0x1031] == 13 and data[0x1d23] == players + 2
    assert all(alive(data, seat) == (seat <= players) for seat in range(1, 6))
    cells = [data[0x16aa + seat - 1] for seat in range(1, players + 1)]
    assert len(set(cells)) == players
    for cell in cells[2:]:
        assert not (data[0x1052 + cell] & 0xe0 or data[0x1152 + cell] & 0xc0)
        assert any(not (data[0x1052 + neighbor] & 0xe0) for neighbor in [cell - 1, cell + 1, cell - 16, cell + 16])



for players in [1,2]:
    unlimited=True
    route=enter(players, skins=players==2)
    route+='poke 0xe1c 00\npoke 0xe1d 0100\nwait 180\ndump after-limit\nwait 600\ndump later\n'
    folder=run(f'infinite-{players}',route,skins=players==2,ptbr=players!=1)
    for tag in ['stage','after-limit','later']:
        data=ram(folder,tag)
        assert data[0xe1d:0xe1f]==bytes([99,9]),(players,tag,data[0xe1d:0xe1f])
        assert not (data[0x1d1e]&0x22),(players,tag)
    print(f'{players} players: native unlimited timer survives forced expiration.',flush=True)
unlimited=False
folder=run('timer-disabled',enter(1)+'poke 0xe1c 00\npoke 0xe1d 0100\nwait 180\ndump expired\n')
assert ram(folder,'stage')[0xe1d]<60
assert ram(folder,'expired')[0x1d1e]&0x22
print('Disabled mod: normal countdown and expiration preserved.',flush=True)
