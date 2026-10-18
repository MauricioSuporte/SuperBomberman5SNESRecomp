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
parser.add_argument('--out', type=Path, default=ROOT / '.bootstrap/normal-coop-validation')
parser.add_argument('--fixture', action='store_true')
parser.add_argument('--new-renderer', action='store_true', help='Exercise the optimized renderer used by the launcher (NewRenderer=1).')
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
    mods = [('normal-coop', 'five_players')]
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


if args.cases in ['counts', 'all']:
    for players in range(1, 6):
        folder = run(f'count-{players}', enter(players), ptbr=players in [2, 4])
        stage(ram(folder, 'stage'), players)
        print(f'{players} players: {players + 2} lives, active seats and safe spawns passed.', flush=True)

if args.fixture and args.cases in ['five', 'all']:
    route = enter(5, skins=True)
    for seat in range(1, 6):
        route += death(seat) + f'dump dead{seat}\n' + press(seat, 'start', 90) + f'dump rejoined{seat}\n'
    route += 'poke 0x1d23 01\n' + death(2) + death(3) + death(4) + death(5)
    route += 'poke 0x1ffe0 080008000800\npress p2:start 2\nwait 3\npoke 0x1ffe0 000000000000\nwait 90\ndump last-life\n'
    route += death(1) + death(2) + death(3) + death(4) + death(5)
    route += 'wait 900\ndump game-over\npress a 2\nwait 1200\npress start 2\nwait 600\ndump continued\n'
    folder = run('five-skins', route, skins=True, ptbr=True)
    initial = ram(folder, 'stage')
    stage(initial, 5)
    chars, palettes = initial[0x16c2:0x16c7], initial[0x16da:0x16df]
    source = rom.read_bytes()
    assert len(set(chars)) == 5
    # Each stage palette is the character's base plus the color the seat confirmed.
    assert list(palettes) == [source[0xbb4e + chars[i]] + initial[0x6264 + i] for i in range(5)]
    assert len(set(palettes)) == 5
    for seat in range(1, 6):
        assert not alive(ram(folder, f'dead{seat}'), seat)
        returned = ram(folder, f'rejoined{seat}')
        assert alive(returned, seat) and returned[0x1d23] == 7 - seat and not (returned[0x1d1e] & 0x80)
    last = ram(folder, 'last-life')
    assert last[0x1d23] == 0 and sum(alive(last, seat) for seat in range(1, 6)) == 2
    assert ram(folder, 'game-over')[0x1031] == 8
    continued = ram(folder, 'continued')
    assert continued[0x1031] == 13 and continued[0x1d23] == 7
    assert all(alive(continued, seat) for seat in range(1, 6))
    assert continued[0x16c2:0x16c7] == chars and continued[0x16da:0x16df] == palettes
    print('Five independent skins/colors, five Start rejoins, last-life arbitration and Continue passed.', flush=True)

if args.fixture and args.cases in ['lifecycle', 'all']:
    route = enter(3) + death(1) + death(2) + 'dump sole-p3\n'
    route += press(1, 'start', 90) + 'dump p1-rejoined\npoke 0x1d23 00\n'
    route += death(1) + death(3) + 'wait 900\ndump game-over\npress a 2\nwait 1200\npress start 2\nwait 600\ndump continued\n'
    folder = run('three-lifecycle', route)
    sole = ram(folder, 'sole-p3')
    assert sole[0x1031] == 13 and [alive(sole, seat) for seat in range(1, 4)] == [False, False, True]
    assert alive(ram(folder, 'p1-rejoined'), 1) and ram(folder, 'p1-rejoined')[0x1d23] == 4
    assert not (ram(folder, 'p1-rejoined')[0x1d1e] & 0x80)
    assert ram(folder, 'game-over')[0x1031] == 8
    continued = ram(folder, 'continued')
    assert continued[0x1031] == 13 and continued[0x1d23] == 5 and all(alive(continued, seat) for seat in range(1, 4))
    print('P3 as sole survivor, P1 Start rejoin, Game Over and three-player Continue passed.', flush=True)

if args.cases in ['five', 'lifecycle'] and not args.fixture:
    parser.error('This case requires --fixture and the opt-in fixture host.')
