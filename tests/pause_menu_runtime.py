"""Validate pause actions in isolated Windows/LLE runs with a user-owned ROM.
All SRAM, dumps and local game copies remain under ignored .bootstrap/.
"""
from __future__ import annotations
import argparse
import json
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def press(button: str, wait: int = 20) -> str:
    return f'press {button} 2\nwait {wait}\n'


def normal_route(players: int, characters: bool) -> str:
    route = ('turbo on\nwait 900\n' + press('start', 180) * 3 +
             press('down', 25) * (players - 1) + press('start', 180) +
             press('a', 90) + press('a', 180) + press('a', 900))
    if characters:
        route += press('a', 180) + press('p2:a', 1200)
    route += press('start', 600) + 'dump stage\n'
    # Start accepts Resume and No; A is ignored in the confirmation.
    route += press('start', 30) + 'dump default\n' + press('start', 30) + 'dump resumed\n'
    route += press('start', 30) + press('down') + 'dump moved\n' + press('start') + 'dump confirm_no\n'
    route += press('a') + 'dump confirm_a\n' + press('start') + 'dump cancelled\n'
    route += press('start') + press('b') + 'dump back\n'
    route += press('start') + press('down') + press('start', 900) + 'dump returned\n'
    route += press('start', 700) + 'dump reentered\nquit\n'
    return route


BATTLE_ENTER = 'turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\npress down 2\npress start 2\nwait 700\npress a 2\nwait 500\npress a 2\nwait 500\npress down 2\nwait 20\npress down 2\nwait 20\npress left 2\nwait 20\npress down 2\nwait 20\npress left 2\nwait 20\npress down 2\nwait 20\npress left 2\nwait 20\npress a 2\nwait 500\npress a 2\nwait 180\npress p2:a 2\nwait 180\npress a 2\nwait 180\npress p2:a 2\nwait 180\npress a 2\nwait 180\nwait 500\npress a 2\nwait 500\ndump selection\npress a 2\nwait 600\ndump battle\n'


def battle_route() -> str:
    route = BATTLE_ENTER + press('start', 30) + 'dump default\n'
    route += press('down') + 'dump moved\n' + press('start', 300) + 'dump draw\nwait 900\ndump next\n'
    route += press('start', 30) + press('down') * 2 + press('start') + 'dump confirm_no\n'
    route += press('a') + 'dump confirm_a\n' + press('start') + 'dump cancelled\n'
    route += press('start') + press('b') + 'dump back\n'
    route += press('start') + press('down') + press('start', 900) + 'dump returned\n'
    route += press('a', 700) + 'dump reentered\nquit\n'
    return route


def pc(data: bytes) -> int:
    return data[0xc90] | data[0xcb8] << 8 | data[0xce0] << 16


def check_pause_audio(folder: Path, cartridge: bytes, battle: bool) -> None:
    intervals=[('default','battle' if battle else 'stage')]
    if not battle:
        intervals.append(('resumed','default'))
    command=cartridge[0x86d0+5]
    for tag,previous in intervals:
        first=json.loads((folder/f'{previous}.info.json').read_text())['frame']
        last=json.loads((folder/f'{tag}.info.json').read_text())['frame']
        rows=[row.split('\t') for row in (folder/f'{tag}.apu.tsv').read_text().splitlines()[1:]]
        assert any(len(row)==7 and row[3:5]==['cpu_wr','0'] and
                   first<int(row[0])<=last and int(row[5],16)&0x7f==command
                   for row in rows),(folder,tag,'native pause audio was not sent in this interval')


def run(args, name: str, players: int, combined: bool) -> Path:
    folder = Path(tempfile.mkdtemp(prefix=name + '-', dir=args.out))
    shutil.copy2(args.exe, folder / 'game.exe')
    shutil.copytree(ROOT / 'mods', folder / 'mods')
    mods = [('pause-menu', 'pause_menu')]
    if players > 1 and name != 'battle':
        mods.append(('normal-coop', 'five_players'))
    if combined:
        mods += [('ptbr', 'menus_ptbr'), ('selection-colors', 'color_controls'),
                 ('extra-characters', 'extra_characters')]
        if name != 'battle':
            mods += [('normal-characters', 'character_select'), ('infinite-time', 'infinite_time')]
    state = 'format_version = 1\n'
    for package, feature in mods:
        state += (f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n'
                  f'[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\nenabled = true\n')
    (folder / 'mods/preloaded/state.toml').write_text(state, encoding='utf-8')
    (folder / 'headless.ini').write_text(
        f'[Graphics]\nOutputMethod=SDL-Software\nWindowScale=1\nNewRenderer={int(args.new_renderer)}\n'
        '[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    route = battle_route() if name == 'battle' else normal_route(players, combined)
    (folder / 'route.input').write_text(route)
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SNESRECOMP_DUMP_DIR=str(folder))
    with (folder / 'run.log').open('w') as log:
        result = subprocess.run([str(folder / 'game.exe'), '--no-launcher', '--config', str(folder / 'headless.ini'),
                                 '--script', str(folder / 'route.input'), str(args.rom)],
                                cwd=folder, env=env, stdout=log, stderr=log, timeout=300)
    assert result.returncode == 0, (name, result.returncode, folder)
    log = (folder / 'run.log').read_text(errors='replace').lower()
    for bad in ['interp_cap', 'unresolved-abandon', 'dispatch_abandon', 'architectural brk', 'fatal',
                'unknown button', 'watchdog tripped', 'not found', 'failed to activate']:
        assert bad not in log, (name, bad, folder)
    def ram(tag: str) -> bytes:
        return (folder / f'{tag}.wram.bin').read_bytes()
    # Authored white pixels must remain white rather than blend with the field.
    for tag, col, row, cursor_col in [('default', 7 if name == 'battle' else 8, 17,
                                     4 if name == 'battle' else 5),
                                    ('confirm_no', 16, 19, 13)]:
        pixels = (folder / f'{tag}.fb.bgrx').read_bytes()
        for x, y in [(cursor_col*8+5, row*8)]:
            at = (y*256+x)*4
            assert pixels[at:at+3] == b'\xff\xff\xff', (name, tag, 'opaque glyph/glove', x, y)
    # Labels differ with PT-BR. Check opaque white ink inside each text region
    # without assuming that CONTINUAR has RESUME's first glyph.
    for tag,row,col in [('default',17,7 if name=='battle' else 8),('confirm_no',19,16)]:
        pixels=(folder/f'{tag}.fb.bgrx').read_bytes()
        assert sum(pixels[(y*256+x)*4:(y*256+x)*4+3]==b'\xff\xff\xff'
                   for y in range(row*8-1,row*8+7) for x in range(col*8,(col+3)*8))>8
    default, confirmation = ram('default'), ram('confirm_no')
    assert default[0x1d1d] == confirmation[0x1d1d] == ram('confirm_a')[0x1d1d] == 1
    assert ram('cancelled')[0x1031] == confirmation[0x1031]
    assert ram('back')[0x1d1d] == 1
    # Observe the native APU command, derived from this cartridge's SFX table.
    cartridge = args.rom.read_bytes()
    check_pause_audio(folder,cartridge,name=='battle')
    for tag, effect in [('moved', 1), ('confirm_no', 4), ('back', 3)]:
        command = cartridge[0x86d0 + effect]
        rows = (folder / f'{tag}.apu.tsv').read_text().splitlines()[1:]
        assert any(len(parts := row.split('\t')) == 7 and parts[3:5] == ['cpu_wr', '0']
                   and int(parts[5], 16) & 0x7f == command for row in rows), (name, tag, 'SFX command', folder)
    returned, reentered = ram('returned'), ram('reentered')
    assert returned[0x1d1a] & 8 and not returned[0x1d1d]
    if name == 'battle':
        assert ram('draw')[0x1031] == 10  # Original result/score scene.
        assert ram('next')[0x1031] == ram('battle')[0x1031] == reentered[0x1031] == 116
        assert returned[0x1031] == 4 and pc(returned) == 0xc6a28d
    else:
        assert not ram('resumed')[0x1d1d]
        assert returned[0x1031] == 3 and pc(returned) == 0xc713ac
        assert reentered[0x1031] == 13 and reentered[0x1d23] == ram('stage')[0x1d23]
        before_save = (folder / 'stage.sram.bin').read_bytes()
        after_save = (folder / 'returned.sram.bin').read_bytes()
        # Native saving can normalize a new file's count/checksum. No cleared
        # stage bit may be added in any of the three Normal records.
        for record in [0x32, 0x45, 0x58]:
            assert before_save[record:record+16] == after_save[record:record+16]
        assert ram('stage')[0x2900:0x2910] == returned[0x2900:0x2910]
        assert ram('stage')[0x16c2:0x16c7] == reentered[0x16c2:0x16c7]
        assert ram('stage')[0x16da:0x16df] == reentered[0x16da:0x16df]
    print(f'{name}: pause, safe defaults, native transitions and reentry passed: {folder}', flush=True)
    return folder


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--out', type=Path, default=ROOT / '.bootstrap/pause-menu-validation')
    parser.add_argument('--case', choices=['normal', 'combined', 'five', 'battle', 'all'], default='all')
    parser.add_argument('--new-renderer', action='store_true')
    args = parser.parse_args()
    args.exe, args.rom, args.out = args.exe.resolve(), args.rom.resolve(), args.out.resolve()
    assert args.exe.is_file() and args.rom.is_file()
    args.out.mkdir(parents=True, exist_ok=True)
    cases = {'normal': (1, False), 'combined': (2, True), 'five': (5, False), 'battle': (2, True)}
    chosen = cases if args.case == 'all' else {args.case: cases[args.case]}
    with ThreadPoolExecutor(max_workers=2) as pool:
        futures = [pool.submit(run, args, name, *parameters) for name, parameters in chosen.items()]
        for future in futures:
            future.result()


if __name__ == '__main__':
    main()
