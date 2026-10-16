import hashlib
import importlib.util
import pathlib
import tempfile
import unittest
import zipfile
from unittest.mock import patch

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit', ROOT / 'tools/audit-publication.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class PublicationGuardTests(unittest.TestCase):
    def test_rejects_dumps_generated_code_and_secrets(self):
        for path in ['game.SFC', 'renamed.rom', 'audio/song.spc', 'other/src/gen/bank00.c',
                     'copied/bankC0_v2.c', 'recomp/funcs.h', '.env', 'secret.pem',
                     '.tools/tool.exe', 'captures/image.png']:
            with self.subTest(path=path):
                self.assertIsNotNone(audit.forbidden(path))

    def test_allows_source_scripts_symbols_and_notices(self):
        for path in ['tools/build-windows.ps1', 'src/game_rtl.c', 'recomp/symbols.toml',
                     'rom_identity.txt', 'LICENSE', 'THIRD_PARTY_NOTICES.md',
                     '.agents/skills/super-bomberman5-repository/SKILL.md']:
            with self.subTest(path=path):
                self.assertIsNone(audit.forbidden(path))

    def test_allows_documentation_gameplay_images_without_allowing_extracts(self):
        for path in ['docs/images/five-player-showcase.png', 'docs/images/pause.jpg',
                     'docs/images/battle.jpeg', 'docs/images/menu.webp']:
            with self.subTest(path=path):
                self.assertIsNone(audit.forbidden(path))
                self.assertIsNone(audit.forbidden(path, release=True))
        for path in ['assets/sprites.png', 'docs/images/rom.bin',
                     'docs/images/audio.spc', 'docs/images/extracted/tiles.png',
                     '.bootstrap/docs/images/capture.png', 'captures/frame.png']:
            with self.subTest(path=path):
                self.assertIsNotNone(audit.forbidden(path))

    def test_former_louie_audio_is_rejected_in_source_and_packages(self):
        for relative in audit.HISTORICAL_AUDIO:
            self.assertIsNotNone(audit.forbidden(relative))
            self.assertIsNotNone(audit.forbidden('setup/' + relative, release=True))
        with tempfile.TemporaryDirectory(dir=ROOT / '.bootstrap') as directory:
            archive_path = pathlib.Path(directory) / 'audio.zip'
            with zipfile.ZipFile(archive_path, 'w') as archive:
                archive.writestr(next(iter(audit.HISTORICAL_AUDIO)), b'synthetic')
            self.assertTrue(any('extracted audio' in e for e in audit.audit_zip(archive_path)))

    def test_icon_exception_is_limited_to_reviewed_asset(self):
        self.assertIsNone(audit.forbidden('assets/app.ico'))
        for path in ['assets/other.ico', 'other/assets/app.ico', 'assets/app.png']:
            with self.subTest(path=path):
                self.assertIsNotNone(audit.forbidden(path))

    def test_archive_rejects_renamed_rom_and_traversal(self):
        local_tmp = ROOT / '.bootstrap'
        local_tmp.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='publication-test-', dir=local_tmp) as directory:
            self.assertTrue(pathlib.Path(directory).resolve().is_relative_to(local_tmp.resolve()))
            archive_path = pathlib.Path(directory) / 'test.zip'
            synthetic = b'\0' * 4194304
            with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as archive:
                archive.writestr('setup/data.dat', synthetic)
                archive.writestr('../escape.txt', 'synthetic')
                archive.writestr('setup/snesrecomp/docs/assets/games/game.png', 'synthetic')
            with patch.object(audit, 'rom_digest', return_value=hashlib.sha256(synthetic).hexdigest()):
                errors = audit.audit_zip(archive_path)
            self.assertTrue(any('reference ROM bytes' in e for e in errors))
            self.assertTrue(any('path traversal' in e for e in errors))
            self.assertTrue(any('dependency screenshot' in e for e in errors))


if __name__ == '__main__':
    unittest.main()
