"""tools/boss-rush-ranking.py: the release ZIP gets the ranking configuration
only from the secrets, and the source tree never tracks it."""
import importlib.util
import io
import os
import pathlib
import tempfile
import unittest
import zipfile
from contextlib import redirect_stdout
from unittest import mock

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('boss_rush_ranking', ROOT / 'tools/boss-rush-ranking.py')
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
audit_spec = importlib.util.spec_from_file_location('audit', ROOT / 'tools/audit-publication.py')
audit = importlib.util.module_from_spec(audit_spec)
audit_spec.loader.exec_module(audit)

# Built in pieces so no webhook-shaped URL is ever tracked (test_discord_release_post).
FAKE = 'https://discord.com/api/' + 'webhooks/' + '123456789/abc-DEF_token'


def package(tmp: pathlib.Path) -> pathlib.Path:
    path = tmp / 'superbomberman5-9.9.9-windows-x64.zip'
    with zipfile.ZipFile(path, 'w') as archive:
        archive.writestr('superbomberman5-9.9.9/README.txt', 'x')
    return path


class BossRushRankingToolTest(unittest.TestCase):
    def test_adds_config_from_secrets(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = package(pathlib.Path(tmp))
            with mock.patch.dict(os.environ, {'BOSS_RUSH_WEBHOOK': FAKE + '/', 'BOSS_RUSH_BOARD': '987654321'}), \
                    redirect_stdout(io.StringIO()) as out:
                self.assertEqual(tool.add_to_zip(path), 0)
            self.assertNotIn('abc-DEF_token', out.getvalue())
            with zipfile.ZipFile(path) as archive:
                text = archive.read('superbomberman5-9.9.9/boss_rush_ranking.cfg').decode()
            self.assertIn('webhook=' + FAKE + '\n', text)
            self.assertIn('board=987654321\n', text)
            self.assertEqual(audit.audit_zip(path), [])  # allowed in a release package

    def test_without_secrets_the_package_is_untouched(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = package(pathlib.Path(tmp))
            env = {k: v for k, v in os.environ.items() if not k.startswith('BOSS_RUSH_')}
            for bad in ({}, {'BOSS_RUSH_WEBHOOK': 'https://example.com/hook', 'BOSS_RUSH_BOARD': '987654321'},
                        {'BOSS_RUSH_WEBHOOK': FAKE, 'BOSS_RUSH_BOARD': 'abc'}):
                with mock.patch.dict(os.environ, {**env, **bad}, clear=True), redirect_stdout(io.StringIO()):
                    self.assertEqual(tool.add_to_zip(path), 0)
            with zipfile.ZipFile(path) as archive:
                self.assertEqual(archive.namelist(), ['superbomberman5-9.9.9/README.txt'])

    def test_config_is_never_tracked(self):
        self.assertIsNotNone(audit.forbidden('boss_rush_ranking.cfg'))
        self.assertIsNone(audit.forbidden('superbomberman5-9.9.9/boss_rush_ranking.cfg', release=True))
        self.assertIn('boss_rush_ranking.cfg', (ROOT / '.gitignore').read_text())


if __name__ == '__main__':
    unittest.main()
