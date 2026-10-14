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
                     '.tools/tool.exe', 'captures/image.png', 'assets/game.AppImage']:
            with self.subTest(path=path):
                self.assertIsNotNone(audit.forbidden(path))

    def test_allows_source_scripts_symbols_and_notices(self):
        for path in ['tools/build-windows.ps1', 'src/game_rtl.c', 'recomp/symbols.toml',
                     'rom_identity.txt', 'LICENSE', 'THIRD_PARTY_NOTICES.md',
                     '.agents/skills/super-bomberman5-repository/SKILL.md']:
            with self.subTest(path=path):
                self.assertIsNone(audit.forbidden(path))

    def test_icon_exception_is_limited_to_reviewed_asset(self):
        self.assertIsNone(audit.forbidden('assets/app.ico'))
        for path in ['assets/other.ico', 'other/assets/app.ico', 'assets/app.png']:
            with self.subTest(path=path):
                self.assertIsNotNone(audit.forbidden(path))

    def test_showcase_exception_is_limited_to_reviewed_image(self):
        self.assertIsNone(audit.forbidden('docs/images/five-player-showcase.png'))
        for path in ['docs/images/other.png', 'docs/five-player-showcase.png',
                     'screenshots/five-player-showcase.png']:
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
                archive.writestr('setup/docs/images/five-player-showcase.png', 'synthetic')
            with patch.object(audit, 'rom_digest', return_value=hashlib.sha256(synthetic).hexdigest()):
                errors = audit.audit_zip(archive_path)
            self.assertTrue(any('reference ROM bytes' in e for e in errors))
            self.assertTrue(any('path traversal' in e for e in errors))
            self.assertTrue(any('dependency screenshot' in e for e in errors))
            self.assertTrue(any('README screenshot' in e for e in errors))


if __name__ == '__main__':
    unittest.main()
