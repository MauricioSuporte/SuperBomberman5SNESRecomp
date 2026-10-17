"""Complete the official Windows setup pack with the ROM-independent analyzer."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import pathlib
import re
import tomllib
import zipfile


def add_file_checksums(path: pathlib.Path) -> None:
    """Include checksums of every packaged file, excluding the checksum itself."""
    with zipfile.ZipFile(path, 'a', zipfile.ZIP_DEFLATED) as archive:
        root = archive.namelist()[0].split('/')[0]
        lines = []
        for item in sorted(archive.infolist(), key=lambda item: item.filename):
            if item.is_dir():
                continue
            relative = item.filename[len(root) + 1:]
            lines.append(hashlib.sha256(archive.read(item)).hexdigest() + '  ' + relative + '\n')
        archive.writestr(root + '/FILE_SHA256SUMS', ''.join(lines))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zip', type=pathlib.Path, required=True)
    parser.add_argument('--build-dir', type=pathlib.Path, required=True)
    parser.add_argument('--analyzer', type=pathlib.Path, required=True)
    parser.add_argument('--toolchain', type=pathlib.Path, required=True)
    parser.add_argument('--cargo-home', type=pathlib.Path,
                        default=pathlib.Path(os.environ.get('CARGO_HOME', str(pathlib.Path.home() / '.cargo'))))
    parser.add_argument('--rust-sysroot', type=pathlib.Path, required=True)
    args = parser.parse_args()
    cache = (args.build_dir / 'CMakeCache.txt').read_text(encoding='utf-8')
    if not re.search(r'^SNESRECOMP_SETUP_HOST:BOOL=ON$', cache, re.M):
        raise SystemExit('Refusing to release a game build: setup host must be ON.')
    if 'SNESRECOMP_FRAME_IMPL:STRING=LLE' not in cache:
        raise SystemExit('The first release requires the validated LLE frame path.')
    for p in (args.analyzer, args.toolchain / 'bin/libunwind.dll', args.toolchain / 'LICENSE.TXT'):
        if not p.is_file():
            raise SystemExit(f'Missing release dependency: {p}')
    lock = tomllib.loads((pathlib.Path(__file__).resolve().parents[1] / 'snesrecomp/recompiler-rs/Cargo.lock').read_text())
    crate_notices = []
    found_crates = set()
    for package in lock['package']:
        if 'source' not in package:
            continue
        crate_id = package['name'] + '-' + package['version']
        candidates = list((args.cargo_home / 'registry/src').glob('*/' + crate_id))
        if not candidates:
            continue  # Optional locked features need not have been compiled.
        files = list(candidates[0].glob('LICENSE*')) + list(candidates[0].glob('COPYING*'))
        if not files:
            raise SystemExit(f'License notice missing for {crate_id}')
        crate_notices.extend((crate_id, p) for p in files if p.is_file())
        found_crates.add(package['name'])
    required = {'serde_json', 'serde_core', 'memchr', 'itoa', 'zmij'}
    if not required.issubset(found_crates):
        raise SystemExit('Cargo license notices missing: ' + ', '.join(sorted(required - found_crates)))
    rust_docs = args.rust_sysroot / 'share/doc/rust'
    if not (rust_docs / 'COPYRIGHT-library.html').is_file() or not (rust_docs / 'licenses').is_dir():
        raise SystemExit('Rust standard-library copyright/license notices missing.')
    temp = args.zip.with_suffix('.auditing.zip')
    with zipfile.ZipFile(args.zip) as src, zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED) as out:
        setup_host = None
        roots = {i.filename.split('/')[0] for i in src.infolist()}
        if len(roots) != 1:
            raise SystemExit('Expected a single package directory.')
        root = roots.pop()
        for item in src.infolist():
            # These upstream assets are not needed to generate or run the game.
            if '/docs/assets/games/' in item.filename or '/test_data/' in item.filename or '/docs/images/' in item.filename:
                continue
            if item.filename == root + '/SuperBomberman5SNESRecomp.exe':
                setup_host = (item, src.read(item))
                continue
            data = src.read(item)
            if item.filename == root + '/README.txt':
                text = data.decode('utf-8')
                for old, new in (('Start SuperBomberman5SNESRecomp.exe.', 'Start SuperBomberman5Recomp.exe.'),
                                 ('started for you; run it from there afterwards.',
                                  'started for you; afterwards SuperBomberman5Recomp.exe opens it.')):
                    if old not in text:
                        raise SystemExit(f'README.txt changed upstream; update finalize-release.py: {old}')
                    text = text.replace(old, new)
                data = text.encode('utf-8')
            out.writestr(item, data)
        # One executable at the package root: the setup host itself, under the
        # player-facing name (it rebuilds and relaunches SuperBomberman5SNESRecomp
        # through SNESRECOMP_GAME_TARGET). The forwarding entry stays a
        # source-tree convenience.
        if setup_host is None:
            raise SystemExit('Missing setup host executable')
        item, data = setup_host
        info = zipfile.ZipInfo(root + '/SuperBomberman5Recomp.exe', date_time=item.date_time)
        info.compress_type = zipfile.ZIP_DEFLATED
        info.external_attr = item.external_attr
        out.writestr(info, data)
        analyzer_dir = root + '/snesrecomp/recompiler-rs/target/release/'
        out.write(args.analyzer, analyzer_dir + 'snesrecomp-analyze.exe')
        out.write(args.toolchain / 'bin/libunwind.dll', analyzer_dir + 'libunwind.dll')
        out.write(args.toolchain / 'LICENSE.TXT', root + '/licenses/LLVM-LICENSE.txt')
        for crate_id, notice in crate_notices:
            out.write(notice, root + '/licenses/cargo/' + crate_id + '/' + notice.name)
        out.write(rust_docs / 'COPYRIGHT-library.html', root + '/licenses/rust/COPYRIGHT-library.html')
        for notice in (rust_docs / 'licenses').rglob('*'):
            if notice.is_file():
                out.write(notice, root + '/licenses/rust/licenses/' + notice.relative_to(rust_docs / 'licenses').as_posix())
        manifest = {
            'format': 1, 'kind': 'setup-host', 'platform': 'windows-x64',
            'frame_implementation': 'LLE', 'contains_game_code': False,
            'analyzer_sha256': hashlib.sha256(args.analyzer.read_bytes()).hexdigest(),
            'toolchain_embedded': False,
            'cargo_license_notices': sorted(found_crates),
            'game_code_generation': 'local, from the player-supplied ROM',
        }
        out.writestr(root + '/PACKAGE_MANIFEST.json', json.dumps(manifest, indent=2) + '\n')
        version = (pathlib.Path(__file__).resolve().parents[1] / 'VERSION').read_text().strip()
        out.writestr(root + '/LEIA-ME.txt',
            f'Super Bomberman 5 {version} — protótipo Windows x64\n\n'
            'Este pacote não contém a ROM nem código gerado a partir dela.\n'
            '1. Extraia todo o ZIP para uma pasta gravável.\n'
            '2. Abra SuperBomberman5Recomp.exe.\n'
            '3. Permita baixar as ferramentas oficiais e selecione sua própria ROM.\n'
            '4. Use Generate & rebuild para gerar, compilar e iniciar o jogo localmente.\n'
            'O analisador nativo está incluído; Rust não é necessário para usar este pacote.\n'
            'Internet é necessária para obter as ferramentas na primeira instalação.\n'
            'Recompilação parcial: a maior parte da lógica ainda usa o interpretador.\n'
            'Multiplayer e jogo completo ainda não foram validados.\n'
            'Consulte README.md, docs/STATUS.md e THIRD_PARTY_NOTICES.md.\n')
    add_file_checksums(temp)
    spec = importlib.util.spec_from_file_location('audit', pathlib.Path(__file__).with_name('audit-publication.py'))
    audit = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(audit)
    errors = audit.audit_zip(temp)
    if errors:
        temp.unlink()
        raise SystemExit('\n'.join(errors))
    temp.replace(args.zip)
    digest = hashlib.sha256(args.zip.read_bytes()).hexdigest()
    (args.zip.parent / 'SHA256SUMS').write_text(f'{digest}  {args.zip.name}\n', encoding='utf-8')
    print(f'Audited setup pack: {args.zip} SHA256={digest}')


if __name__ == '__main__':
    main()
