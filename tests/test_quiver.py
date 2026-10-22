"""Release layout Quiver Launcher relies on (docs/QUIVER.md)."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class QuiverLayout(unittest.TestCase):
    def test_release_assets_are_platform_labelled(self):
        workflow = (ROOT / '.github/workflows/release.yml').read_text(encoding='utf-8')
        self.assertIn('dist/*.AppImage', workflow)
        self.assertIn('dist/*-windows-x64.zip', workflow)
        self.assertIn('dist/SHA256SUMS', workflow)
        appimage = (ROOT / 'tools/package-appimage.py').read_text(encoding='utf-8')
        self.assertIn("-linux-x86_64.AppImage", appimage)

    def test_one_root_executable_and_updater_in_subfolder(self):
        finalize = (ROOT / 'tools/finalize-release.py').read_text(encoding='utf-8')
        self.assertIn("root + '/SuperBomberman5Recomp.exe'", finalize)
        self.assertIn("root + '/updater/SuperBomberman5Updater.exe'", finalize)
        self.assertNotIn("root + '/SuperBomberman5Updater.exe'", finalize)

    def test_catalog_rom_matches_identity(self):
        identity = (ROOT / 'rom_identity.txt').read_text(encoding='utf-8')
        sha = re.search(r'expected_sha256\s*=\s*([0-9a-fA-F]{64})', identity)[1].lower()
        doc = (ROOT / 'docs/QUIVER.md').read_text(encoding='utf-8')
        self.assertIn(sha, doc)
        self.assertIn('"destination": "Super Bomberman 5 (USA).sfc"', doc)

    def test_updater_defers_to_quiver(self):
        updater = (ROOT / 'src/updater/sb5_updater.c').read_text(encoding='utf-8')
        self.assertIn('installed-build.txt', updater)


if __name__ == '__main__':
    unittest.main()
