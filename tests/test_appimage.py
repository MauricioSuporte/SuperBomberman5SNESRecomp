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

    def test_game_folder_not_anchored_beside_appimage(self):
        # The runtime sets $APPIMAGE; the host would anchor config, rom.cfg,
        # mods and the setup wizard's project lookup to its folder, so a setup
        # host showed PLAY and died with "This is a setup build" (v2.2.4 report).
        with tempfile.TemporaryDirectory(prefix='sb5-launch-') as temp:
            root = Path(temp)
            setup = root/'app/setup'
            setup.mkdir(parents=True)
            (setup/'VERSION').write_text('2.2.7\n')
            host = setup/'SuperBomberman5SNESRecomp'
            host.write_text('#!/bin/sh\nprintf "%s|%s|%s" "${APPIMAGE-unset}" "$SB5_APPIMAGE" "$PWD"\n')
            host.chmod(0o755)
            side = root/'Desktop folder'
            (side/'saves').mkdir(parents=True)
            (side/'saves/slot.srm').write_text('save')
            (side/'keybinds.ini').write_text('side keys')
            (side/'netplay_secret').write_text('secret')
            (setup/'keybinds.ini').write_text('shipped keys')
            image = side/'superbomberman5.AppImage'
            image.write_text('')
            env = {**os.environ, 'APPDIR': str(root/'app'), 'XDG_DATA_HOME': str(root/'data'),
                   'APPIMAGE': str(image)}
            out = subprocess.check_output(['bash', str(ROOT/'scripts/AppRun')], env=env, text=True)
            dest = root/'data/superbomberman5/2.2.7'
            self.assertEqual(out, f'unset|{image}|{dest}')
            self.assertEqual((dest/'saves/slot.srm').read_text(), 'save')
            self.assertEqual((dest/'netplay_secret').read_text(), 'secret')
            self.assertEqual((dest/'keybinds.ini').read_text(), 'shipped keys')
            # Once only: a later change beside the AppImage is not copied again.
            (dest/'netplay_secret').unlink()
            subprocess.check_output(['bash', str(ROOT/'scripts/AppRun')], env=env, text=True)
            self.assertFalse((dest/'netplay_secret').exists())

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
