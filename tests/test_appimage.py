from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

@unittest.skipUnless(os.name == 'posix' and shutil.which('flock'), 'Linux AppRun test')
class AppRunTests(unittest.TestCase):
    def test_first_launch_preserves_local_build_on_repeat(self):
        with tempfile.TemporaryDirectory(prefix='sb5-launch-') as temp:
            root = Path(temp)
            appdir = root/'App Dir'
            setup = appdir/'setup'
            setup.mkdir(parents=True)
            (setup/'VERSION').write_text('0.4.4\n')
            host = setup/'SuperBomberman5SNESRecomp'
            host.write_text('#!/bin/sh\nprintf "setup:%s" "$1"\n')
            host.chmod(0o755)
            env = {**os.environ, 'APPDIR': str(appdir), 'XDG_DATA_HOME': str(root/'User Data')}
            command = ['bash', str(ROOT/'scripts/AppRun'), 'argument with spaces']
            self.assertEqual(subprocess.check_output(command, env=env, text=True), 'setup:argument with spaces')
            dest = root/'User Data/superbomberman5/0.4.4'
            (dest/'build').mkdir()
            game = dest/'build/SuperBomberman5SNESRecomp'
            game.write_text('#!/bin/sh\nprintf "rebuilt:%s" "$1"\n')
            game.chmod(0o755)
            self.assertEqual(subprocess.check_output(command, env=env, text=True), 'rebuilt:argument with spaces')
            self.assertTrue((dest/'.appimage-ready').is_file())

    def test_incomplete_existing_directory_is_not_overwritten(self):
        with tempfile.TemporaryDirectory(prefix='sb5-launch-') as temp:
            root = Path(temp)
            (root/'app/setup').mkdir(parents=True)
            (root/'app/setup/VERSION').write_text('0.4.4')
            dest = root/'data/superbomberman5/0.4.4'
            dest.mkdir(parents=True)
            (dest/'keep').write_text('user data')
            result = subprocess.run(['bash', str(ROOT/'scripts/AppRun')],
                env={**os.environ, 'APPDIR': str(root/'app'), 'XDG_DATA_HOME': str(root/'data')}, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((dest/'keep').read_text(), 'user data')

if __name__ == '__main__':
    unittest.main()
