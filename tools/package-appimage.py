"""Turn an audited Linux setup ZIP into a writable-first-run AppImage."""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import tomllib
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit', ROOT / 'tools/audit-publication.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def complete_zip(path, analyzer, cargo_home, rust_sysroot):
    with zipfile.ZipFile(path) as archive:
        roots = {Path(i.filename).parts[0] for i in archive.infolist()}
        if len(roots) != 1:
            raise ValueError('Expected one setup root')
        root = roots.pop()
        entries = {i.filename: (i, archive.read(i)) for i in archive.infolist()
                   if '/docs/assets/games/' not in i.filename and '/test_data/' not in i.filename}
    entries[root + '/snesrecomp/recompiler-rs/target/release/snesrecomp-analyze'] = (None, analyzer.read_bytes())
    lock = tomllib.loads((ROOT / 'snesrecomp/recompiler-rs/Cargo.lock').read_text())
    licensed = set()
    for package in lock['package']:
        if 'source' not in package:
            continue
        crate = package['name'] + '-' + package['version']
        matches = list((cargo_home / 'registry/src').glob('*/' + crate))
        if not matches:
            continue
        notices = [p for p in matches[0].iterdir() if p.is_file() and p.name.startswith(('LICENSE', 'COPYING'))]
        if not notices:
            raise ValueError('Missing crate license: ' + crate)
        licensed.add(package['name'])
        for p in notices:
            entries[root + '/licenses/cargo/' + crate + '/' + p.name] = (None, p.read_bytes())
    if not {'serde_json', 'serde_core', 'memchr', 'itoa', 'zmij'} <= licensed:
        raise ValueError('Missing analyzer crate notices')
    docs = rust_sysroot / 'share/doc/rust'
    for p in [docs / 'COPYRIGHT-library.html', *sorted((docs / 'licenses').rglob('*'))]:
        if p.is_file():
            entries[root + '/licenses/rust/' + p.relative_to(docs).as_posix()] = (None, p.read_bytes())
    if not (docs / 'COPYRIGHT-library.html').is_file():
        raise ValueError('Missing Rust library notice')
    manifest = {'format': 1, 'kind': 'setup-host', 'platform': 'linux-x64',
                'frame_implementation': 'LLE', 'contains_game_code': False,
                'analyzer_sha256': hashlib.sha256(analyzer.read_bytes()).hexdigest()}
    entries[root + '/PACKAGE_MANIFEST.json'] = (None, (json.dumps(manifest, indent=2) + '\n').encode())
    sums = ''.join(hashlib.sha256(data).hexdigest() + '  ' + name[len(root)+1:] + '\n'
                   for name, (_, data) in sorted(entries.items()) if not name.endswith('/'))
    entries[root + '/FILE_SHA256SUMS'] = (None, sums.encode())
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, (info, data) in entries.items():
            if info is None:
                info = zipfile.ZipInfo(name)
                info.external_attr = (0o100755 if name.endswith('/snesrecomp-analyze') else 0o100644) << 16
            archive.writestr(info, data)
    errors = audit.audit_zip(path)
    if errors:
        raise ValueError('\n'.join(errors))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zip', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--analyzer', type=Path, required=True)
    parser.add_argument('--cargo-home', type=Path, default=Path(os.environ.get('CARGO_HOME', str(Path.home()/'.cargo'))))
    parser.add_argument('--rust-sysroot', type=Path, required=True)
    parser.add_argument('--appimagetool', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    args = parser.parse_args()
    cache = (args.build_dir/'CMakeCache.txt').read_text()
    if 'SNESRECOMP_SETUP_HOST:BOOL=ON' not in cache or 'SNESRECOMP_FRAME_IMPL:STRING=LLE' not in cache:
        raise ValueError('Requires ROM-independent LLE setup build')
    if list((ROOT/'src/gen').glob('*.c')) or (ROOT/'recomp/funcs.h').exists():
        raise ValueError('Generated game data present')
    complete_zip(args.zip, args.analyzer, args.cargo_home, args.rust_sysroot)
    version = (ROOT/'VERSION').read_text().strip()
    output = args.zip.parent / f'superbomberman5-{version}-linux-x86_64.AppImage'
    with tempfile.TemporaryDirectory(prefix='sb5-appdir-') as temp:
        appdir = Path(temp)/'SuperBomberman5.AppDir'
        appdir.mkdir()
        with zipfile.ZipFile(args.zip) as archive:
            archive.extractall(Path(temp)/'extract')
            for info in archive.infolist():
                p = Path(temp)/'extract'/info.filename
                if p.is_file():
                    p.chmod((info.external_attr >> 16) & 0o777 or 0o644)
        source = next((Path(temp)/'extract').iterdir())
        shutil.move(str(source), str(appdir/'setup'))
        shutil.copyfile(ROOT/'scripts/AppRun', appdir/'AppRun')
        (appdir/'AppRun').chmod(0o755)
        (appdir/'superbomberman5.desktop').write_text('[Desktop Entry]\nType=Application\nName=Super Bomberman 5 Setup\nExec=AppRun\nIcon=superbomberman5\nCategories=Game;\nTerminal=false\n')
        # Authored geometric icon; no cartridge artwork.
        (appdir/'superbomberman5.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128"><rect width="128" height="128" rx="24" fill="#172b46"/><text x="64" y="85" text-anchor="middle" font-family="sans-serif" font-size="64" fill="white">5</text></svg>')
        subprocess.run([str(args.appimagetool.resolve()), '--appimage-extract-and-run', '--runtime-file', str(args.runtime.resolve()), str(appdir), str(output.resolve())], check=True,
                       env={**os.environ, 'ARCH': 'x86_64'})
    output.chmod(0o755)
    print(output)

if __name__ == '__main__':
    main()
