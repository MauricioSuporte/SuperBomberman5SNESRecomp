"""Cold-boot two (or more) real desktop hosts on loopback UDP and check sync.

Each peer is a private copy of a locally built executable with its own mods
and saves. The rollback session must agree on the boot digest, run the frame
budget at no more than the SNES rate, never fork a replay against the
timeline it replaced (`RB POST/BASELINE FORK`), refuse no match and write no
offline autosave. RNET_RB_FORCE_MISPREDICT makes the peers actually roll
back. Captures stay under ignored .bootstrap/. Adapted from Mega Man X
Recompiled's tools/test_netplay_desktop.py.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--sb4', type=Path, help='Super Bomberman 4 ROM for the Bomber Kings mod')
    parser.add_argument('--peers', type=int, default=2, choices=[2, 3, 4, 5])
    parser.add_argument('--frames', type=int, default=1200)
    parser.add_argument('--port', type=int, default=18210)
    parser.add_argument('--mods', choices=['none', 'all'], default='none')
    parser.add_argument('--mispredict', type=int, default=0, help='RNET_RB_FORCE_MISPREDICT period on seat 0')
    parser.add_argument('--delay-sync', action='store_true')
    parser.add_argument('--out', type=Path, default=ROOT / '.bootstrap/netplay-pair')
    args = parser.parse_args()
    exe, rom = args.exe.resolve(strict=True), args.rom.resolve(strict=True)
    args.out.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix=f'pair{args.peers}-{args.mods}-', dir=args.out.resolve()))
    from sb5_mod_sets import ALL_MODS
    processes = []
    try:
        for seat in range(args.peers):
            peer = root / f'peer{seat}'
            peer.mkdir()
            shutil.copy2(exe, peer / exe.name)
            shutil.copytree(ROOT / 'mods', peer / 'mods')
            state = 'format_version = 1\n'
            for package, feature in (ALL_MODS if args.mods == 'all' else []):
                state += (f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n'
                          f'[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\nenabled = true\n')
            (peer / 'mods/preloaded/state.toml').write_text(state, encoding='utf-8')
            (peer / 'config.ini').write_text('[General]\nAutosave=1\nDisableFrameDelay=0\n[Graphics]\n'
                                             'OutputMethod=SDL-Software\nWindowScale=1\n[Sound]\nEnableAudio=1\n',
                                             encoding='utf-8')
            env = {k: v for k, v in os.environ.items()
                   if not k.startswith(('SNES_NET', 'SNES_RB_', 'SNESRECOMP_', 'RNET_'))}
            # Seat 0 hosts; every other seat joins it.
            env.update(SNES_NETPLAY='1', SNES_NET_SLOT=str(seat), SNES_NET_SLOTS=str(args.peers),
                       SNES_NET_TRANSPORT='lan', SNES_NET_BIND=f'127.0.0.1:{args.port + seat}',
                       SNES_NET_PEER=f'127.0.0.1:{args.port + (1 if seat == 0 else 0)}',
                       SNES_NET_INPUT_PLAYER='0', SNESRECOMP_RUN_FRAMES=str(args.frames),
                       SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
            if args.delay_sync:
                env['SNES_NET_MODE'] = 'delay'
            if args.mispredict and seat == 0:
                env['RNET_RB_FORCE_MISPREDICT'] = str(args.mispredict)
            log = (peer / 'desktop.log').open('wb')
            proc = subprocess.Popen([str(peer / exe.name), '--no-launcher', '--rom', str(rom)], cwd=peer, env=env,
                                    stdout=log, stderr=subprocess.STDOUT)
            processes.append((proc, log, peer))
        digests = []
        for proc, log, peer in processes:
            result = proc.wait(timeout=60 + args.frames // 15)
            log.close()
            text = (peer / 'desktop.log').read_text(errors='replace')
            assert result == 0, text[-4000:]
            if not args.delay_sync:
                found = re.findall(r'RB boot digest agreed \(([0-9a-f]+)\)', text)
                assert found, text[-4000:]
                digests.append(found)
            timing = re.search(r'video totals: simulations=(\d+) presentations=\d+ seconds=([0-9.]+)', text)
            assert timing, text[-4000:]
            simulated, elapsed = int(timing.group(1)), float(timing.group(2))
            slack = args.frames if args.mispredict else 10
            assert simulated == args.frames or (args.frames - slack <= simulated < args.frames and
                                                'netplay barrier requested exit' in text), text[-4000:]
            assert not re.search(r'RB (POST|BASELINE) FORK', text), text[-4000:]
            assert 'match refused' not in text and 'INPUT desync' not in text, text[-4000:]
            assert not (peer / 'saves/save0.sav').exists(), 'Online match wrote an offline autosave'
            rollbacks = len(re.findall(r'RB (?:resim|rollback|replay)', text, re.I))
            print(f'{peer.name}: {simulated} frames in {elapsed:.2f}s, rollback lines {rollbacks}', flush=True)
        if digests:
            assert all(d == digests[0] for d in digests), f'Boot states differ: {digests}'
        print(f'{args.peers} peers in sync ({args.mods} mods, mispredict {args.mispredict}): {root}')
    finally:
        for proc, log, _ in processes:
            if proc.poll() is None:
                proc.kill()
                proc.wait()
            log.close()


if __name__ == '__main__':
    main()
