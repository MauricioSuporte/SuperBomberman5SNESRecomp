"""Isolated Windows/LLE checks for portal hints using a user-owned ROM.

Enemy completion is forced in this fixture; this is not a full combat playthrough.
Raw cartridge dumps stay under ignored .bootstrap/. No assets are packaged.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def run(args, name, enabled, renderer, combined=False, one_only=False):
    folder = Path(tempfile.mkdtemp(prefix=name+'-', dir=args.out))
    shutil.copy2(args.exe, folder/'game.exe')
    shutil.copytree(ROOT/'mods', folder/'mods')
    state = 'format_version = 1\n'
    mods = [('portal-labels', 'portal_labels', enabled)]
    if combined:
        mods += [('ptbr', 'menus_ptbr', True), ('pause-menu', 'pause_menu', True)]
    for package, feature, active in mods:
        state += (f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n'
                  f'[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\n'
                  f'enabled = {str(active).lower()}\n')
    (folder/'mods/preloaded/state.toml').write_text(state, encoding='utf-8')
    (folder/'headless.ini').write_text(
        f'[Graphics]\nOutputMethod=SDL-Software\nWindowScale=1\nNewRenderer={int(renderer)}\n'
        '[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    route = (ROOT/'validation/first-stage.input').read_text().split('press right 30')[0]
    route += ('dump hidden\npoke 0x15a4 '+'80'*22+'\npoke 0x1d50 00\n'
              'wait 60\ndump queued_hidden\n'+
              # Remove the three covering blocks together in this test-only
              # collision fixture. The native gate queue now reveals them.
              ('poke 0x10c7 00\n' if one_only else 'poke 0x10a9 '+'00'*35+'\n')+
              'wait 601\nturbo off\nuntil 0x4a9 != 00 600\nwait 2\ndump revealed\n'
              # Poke only the left native gate's frame index. Its OAM disappears;
              # the other two gates must retain their own independent hints.
              'pokefor 0x4a9 00 20\nwait 2\ndump left_hidden\n'
              'wait 25\npress start 2\nwait 20\ndump paused\nquit\n')
    (folder/'route.input').write_text(route)
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/'run.log').open('w') as log:
        result = subprocess.run([str(folder/'game.exe'), '--no-launcher', '--config', str(folder/'headless.ini'),
                                 '--script', str(folder/'route.input'), str(args.rom)], cwd=folder,
                                env=env, stdout=log, stderr=log, timeout=300)
    assert result.returncode == 0, (folder, result.returncode)
    log = (folder/'run.log').read_text(errors='replace').lower()
    for bad in ['interp_cap', 'unresolved-abandon', 'dispatch_abandon', 'architectural brk', 'fatal',
                'unknown button', 'watchdog tripped', 'failed to activate']:
        assert bad not in log, (folder, bad)
    return folder


def compare(reference, overlay, one_only=False):
    for tag in ['hidden', 'queued_hidden', 'revealed', 'left_hidden', 'paused']:
        for suffix in ['wram.bin', 'vram.bin', 'oam.bin', 'cgram.bin', 'sram.bin']:
            assert (reference/f'{tag}.{suffix}').read_bytes() == (overlay/f'{tag}.{suffix}').read_bytes(), (
                tag, suffix, 'presentation changed native state', overlay)
        a = (reference/f'{tag}.fb.bgrx').read_bytes()
        b = (overlay/f'{tag}.fb.bgrx').read_bytes()
        changed = {(i//4 % 256, i//4//256) for i in range(0, len(a), 4) if a[i:i+4] != b[i:i+4]}
        if tag in ['hidden', 'queued_hidden', 'paused']:
            assert not changed, (tag, overlay)
        elif tag == 'revealed':
            locations=[(96,150)] if one_only else [(96,150),(128,118),(160,150)]
            for x, y in locations:
                assert any(abs(px-x) < 16 and y <= py < y+16 for px, py in changed), (tag, x, overlay)
            assert all(any(abs(px-x) < 20 and y <= py < y+16
                           for x, y in locations) for px, py in changed)
        else:
            assert not any(abs(px-96) < 16 and 150 <= py < 166 for px, py in changed), overlay


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--out', type=Path, default=ROOT/'.bootstrap/portal-labels-validation')
    args = parser.parse_args()
    args.exe = args.exe.resolve(); args.rom = args.rom.resolve(); args.out.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=4) as pool:
        jobs = [pool.submit(run, args, *case) for case in [
            ('off', False, True), ('on', True, True), ('accurate', True, False), ('combined', True, True, True),
            ('left-off',False,True,False,True),('left-on',True,True,False,True)]]
        off, on, accurate, combined, left_off, left_on = [job.result() for job in jobs]
    compare(off, on)
    compare(left_off,left_on,True)
    # Native scene and label positions must agree in both qualified compositors.
    assert (on/'revealed.fb.bgrx').read_bytes() == (accurate/'revealed.fb.bgrx').read_bytes()
    assert (combined/'revealed.fb.bgrx').read_bytes() == (on/'revealed.fb.bgrx').read_bytes()
    print('Portal hints: hidden/revealed gates, individual hiding, pause, native-state parity, both PPUs and PT-BR/pause passed.')
    for folder in [off, on, accurate, combined,left_off,left_on]:
        print(folder)
