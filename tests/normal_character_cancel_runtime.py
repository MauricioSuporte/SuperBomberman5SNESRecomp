"""Exercise native Normal picker cancellation using a local host and user ROM.

All saves and captures stay in an isolated ignored directory. Continue records
are fixtures derived from the cartridge-initialized SRAM, not a playthrough.
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import argparse
import os
import shutil
import subprocess
import tempfile
import tomllib

ROOT = Path(__file__).resolve().parents[1]


def execute(folder, rom, route, name):
    script = folder / (name + '.input')
    script.write_text(route, encoding='utf-8')
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
               SNESRECOMP_DUMP_DIR=str(folder))
    with (folder / (name + '.log')).open('wb') as log:
        subprocess.run([str(folder / 'game.exe'), '--no-launcher', '--config',
                        'headless.ini', '--script', script.name, str(rom)],
                       cwd=folder, env=env, stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=180)
    log = (folder / (name + '.log')).read_text(errors='replace').lower()
    for bad in ('interp_cap', 'unresolved-abandon', 'architectural brk',
                'fatal', 'failed to activate'):
        assert bad not in log, (folder, bad)


def setup(args, folder, ptbr):
    shutil.copy2(args.exe, folder / 'game.exe')
    shutil.copytree(ROOT / 'mods', folder / 'mods')
    (folder / 'headless.ini').write_text(
        '[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=0\n'
        '[Sound]\nEnableAudio=1\n')
    packages = ['normal-characters', 'normal-coop']
    if not args.minimal:
        packages += ['selection-colors', 'extra-characters']
    if ptbr:
        packages += ['ptbr']
    state = 'format_version = 1\n'
    for package in packages:
        path = folder / f'mods/preloaded/packages/superbomberman5.{package}/0.1.0/manifest.toml'
        manifest = tomllib.loads(path.read_text(encoding='utf-8'))
        state += f'[[package]]\nid = "{manifest["id"]}"\nversion = "0.1.0"\n'
        for feature in manifest['feature']:
            state += (f'[[feature]]\npackage_id = "{manifest["id"]}"\n'
                      f'id = "{feature["id"]}"\nenabled = true\n')
    (folder / 'mods/preloaded/state.toml').write_text(state, encoding='utf-8')


def pc(ram):
    return ram[0xc90] | ram[0xcb8] << 8 | ram[0xce0] << 16


def run(args, case, initial_save):
    count, continuing, ptbr = case
    name = f'{"continue" if continuing else "new"}-{count}p-{ptbr}'
    folder = Path(tempfile.mkdtemp(prefix=name + '-', dir=args.out))
    setup(args, folder, ptbr)
    (folder / 'saves').mkdir()
    save = bytearray(initial_save)
    if continuing:
        for record in (0x32, 0x45, 0x58):
            save[record:record+19] = bytes(19)
            save[record+17] = 0x0d
        save[0x30:0x32] = sum(save[0x32:0x6b]).to_bytes(2, 'little')
    (folder / 'saves/save.srm').write_bytes(save)
    route = ('turbo on\nwait 900\npress start 6\nwait 180\n'
             'press start 6\nwait 180\npress start 6\nwait 180\n')
    route += 'press down 6\nwait 25\n' * (count - 1)
    route += 'press start 6\nwait 480\n'
    # A valid first record makes the native menu default to Continue. Empty
    # SRAM defaults to New Game, so both fixtures accept their default item.
    route += 'press a 6\nwait 360\npress a 6\n'
    route += 'wait 360\ndump opening\nuntil cb8 == 8e 2000\nwait 30\ndump picker\n'
    for cycle in range(2):
        route += (f'press b 6\nwait 240\ndump back{cycle}\n'
                  f'press down 6\nwait 30\ndump moved{cycle}\n')
        if cycle == 0:
            route += 'press a 6\nwait 180\ndump after_a\n'
            route += 'wait 180\nuntil cb8 == 8e 2000\nwait 30\ndump reopened\n'
    route += 'press b 6\nwait 180\ndump previous\nquit\n'
    execute(folder, args.rom, route, 'run')
    for tag in ('picker', 'reopened'):
        ram = (folder / (tag + '.wram.bin')).read_bytes()
        assert ram[0x1031] == 4 and pc(ram) == 0xc68e68, (folder, tag, pc(ram))
        outer = bytes.fromhex('ea 0b c7' if continuing else 'ff 0b c7')
        assert ram[0x2003:0x2006] == outer, (folder, 'wrong Normal entry')
    for cycle in range(2):
        saved = 'picker' if cycle == 0 else 'reopened'
        assert (folder / (saved + '.sram.bin')).read_bytes() == (folder / f'back{cycle}.sram.bin').read_bytes(), (folder, 'cancel changed SRAM')
        before = (folder / f'back{cycle}.wram.bin').read_bytes()
        after = (folder / f'moved{cycle}.wram.bin').read_bytes()
        assert before[0x1031] == after[0x1031] == 3, folder
        assert before[0x1032] != after[0x1032], (folder, 'file menu does not respond')
        assert before[0x0d08] == 1, (folder, 'cancel leaked a VM call frame')
        assert before[0x67] == after[0x67] == 0, (folder, 'NMI stuck')
    previous = (folder / 'previous.wram.bin').read_bytes()
    assert previous[0x1031] == 3 and previous[0x67] == 0, folder
    assert pc(previous) != pc(before), (folder, 'B did not leave the file menu')
    print(name, 'two cancels, reentry, file navigation and Back passed:', folder, flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--out', type=Path, default=ROOT / '.bootstrap/normal-cancel-runtime')
    parser.add_argument('--counts', type=int, nargs='+', default=[1, 2, 5])
    parser.add_argument('--minimal', action='store_true')
    parser.add_argument('--mode', choices=['new', 'continue', 'all'], default='all')
    args = parser.parse_args()
    args.exe = args.exe.resolve()
    args.rom = args.rom.resolve()
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    assert all(1 <= count <= 5 for count in args.counts)
    initialize = Path(tempfile.mkdtemp(prefix='initialize-', dir=args.out))
    setup(args, initialize, False)
    execute(initialize, args.rom, 'turbo on\nwait 900\nquit\n', 'initialize')
    initial_save = (initialize / 'saves/save.srm').read_bytes()
    cases = [(count, continuing, ptbr) for count in args.counts
             for continuing in (False, True) for ptbr in (False, True)
             if args.mode == 'all' or continuing == (args.mode == 'continue')]
    pool = ThreadPoolExecutor(max_workers=2)
    try:
        list(pool.map(lambda case: run(args, case, initial_save), cases))
    finally:
        pool.shutdown(wait=True, cancel_futures=True)
