"""Fail publication when local game data enters tracked source or a release ZIP."""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import re
import subprocess
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
RESTRICTED = {'.sfc', '.smc', '.fig', '.swc', '.rom', '.bin', '.spc', '.brr',
              '.pcm', '.srm', '.sav', '.state', '.savestate', '.wram', '.vram',
              '.cgram', '.oam'}
LOCAL_DIRS = {'.bootstrap', '.tools', '.cache', 'roms', 'extracted', 'captures',
              'screenshots', 'audio-dumps', 'profiles', 'coverage', 'saves',
              'logs', 'dist'}
MEDIA = {'.ico', '.png', '.bmp', '.jpg', '.jpeg', '.webp', '.gif', '.tga', '.wav', '.mp3', '.ogg',
         '.flac', '.mp4', '.webm'}
BINARIES = {'.exe', '.dll', '.obj', '.o', '.a', '.lib', '.pdb', '.zip', '.snesmod', '.7z',
            '.rar', '.tar', '.gz', '.appimage'}


def git(*args: str) -> str:
    return subprocess.check_output(['git', '-C', str(ROOT), *args], text=True,
                                   encoding='utf-8').strip()


def forbidden(path: str, release: bool = False) -> str | None:
    p = pathlib.PurePosixPath(path)
    parts = [s.lower() for s in p.parts]
    name = p.name.lower()
    if p.suffix.lower() in RESTRICTED:
        return 'cartridge/audio/memory/save dump'
    if 'src' in parts and 'gen' in parts:
        return 'ROM-derived generated sources'
    if name in {'funcs.h', 'program_manifest.json', '.snesrecomp-cache.json',
                'dispatch_v2.c', 'module_v2.c', 'unresolved_stubs_v2.c'} or re.fullmatch(r'bank[0-9a-f]+_v2\.c', name):
        return 'ROM-derived compiler output'
    if any(s in LOCAL_DIRS or s.startswith('build-') or s == 'build' for s in parts[:-1]):
        return 'machine-local output'
    if '.git' in parts or name.startswith('.env') and name != '.env.example' or p.suffix.lower() in {'.pem', '.key'}:
        return 'credentials or Git internals'
    if name.startswith(('last_run_report', 'crash_report', 'crash_minidump')) or name == 'rom.cfg':
        return 'local diagnostic or ROM path'
    # User-supplied application icon approved for this release; provenance is
    # recorded in assets/README.md. Other media still require separate review.
    if not release and path == 'assets/app.ico':
        return None
    # Maintainer authorized documentation gameplay screenshots on 2026-10-15.
    # This path permission does not authorize extracted cartridge assets.
    if len(parts) == 3 and parts[:2] == ['docs', 'images'] and p.suffix.lower() in {'.png', '.jpg', '.jpeg', '.webp'}:
        return None
    if not release and p.suffix.lower() in MEDIA | BINARIES:
        return 'binary/media asset requires an explicit provenance review'
    return None


def rom_digest() -> str:
    text = (ROOT / 'rom_identity.txt').read_text(encoding='utf-8')
    return re.search(r'expected_sha256\s*=\s*([0-9a-fA-F]{64})', text)[1].lower()


def audit_source(history: bool) -> list[str]:
    errors = []
    paths = git('ls-files', '-z').split('\0')
    for path in filter(None, paths):
        reason = forbidden(path)
        if reason:
            errors.append(f'tracked {path}: {reason}')
    if history:
        # Top-level objects only; upstream submodule histories are not rewritten.
        objects = git('rev-list', '--objects', '--all').splitlines()
        expected = rom_digest()
        for line in objects:
            oid, _, path = line.partition(' ')
            if not path:
                continue
            reason = forbidden(path)
            if reason:
                errors.append(f'history {path}: {reason}')
            kind = git('cat-file', '-t', oid)
            if kind == 'blob' and int(git('cat-file', '-s', oid)) in {4194304, 4194816}:
                data = subprocess.check_output(['git', '-C', str(ROOT), 'cat-file', 'blob', oid])
                if hashlib.sha256(data[-4194304:]).hexdigest() == expected:
                    errors.append(f'history {path}: reference ROM bytes, even if renamed')
    return sorted(set(errors))


def audit_zip(path: pathlib.Path) -> list[str]:
    errors = []
    expected = rom_digest()
    with zipfile.ZipFile(path) as archive:
        for item in archive.infolist():
            p = pathlib.PurePosixPath(item.filename)
            if p.is_absolute() or '..' in p.parts or '\\' in item.filename:
                errors.append(f'archive path traversal: {item.filename}')
                continue
            reason = forbidden(item.filename, release=True)
            if reason:
                errors.append(f'archive {item.filename}: {reason}')
            if item.file_size in {4194304, 4194816} and hashlib.sha256(archive.read(item)[-4194304:]).hexdigest() == expected:
                errors.append(f'archive {item.filename}: reference ROM bytes')
            if '/docs/assets/games/' in item.filename or '/test_data/' in item.filename:
                errors.append(f'archive {item.filename}: dependency screenshot or test fixture')
    return sorted(set(errors))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--history', action='store_true')
    parser.add_argument('--zip', type=pathlib.Path)
    args = parser.parse_args()
    errors = audit_zip(args.zip) if args.zip else audit_source(args.history)
    if errors:
        print('\n'.join(errors))
        return 1
    print('Publication audit passed: ' + (str(args.zip) if args.zip else 'tracked project source' + (' and Git history' if args.history else '')))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
