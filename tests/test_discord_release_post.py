import importlib.util
import pathlib
import subprocess
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('discord_post', ROOT / 'tools' / 'discord-release-post.py')
discord_post = importlib.util.module_from_spec(spec)
spec.loader.exec_module(discord_post)


class DiscordReleasePostTests(unittest.TestCase):
    def notes(self):
        out = subprocess.run([sys.executable, str(ROOT / 'tools' / 'release-notes.py')],
                             capture_output=True, check=True)
        return out.stdout.decode('utf-8')

    def test_payload_fits_discord_limits(self):
        url = 'https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v9.9.9'
        notes = self.notes() + '\n## Long\n\n' + ('x' * 90 + '\n') * 200
        payload = discord_post.build_payload('9.9.9', notes, url)
        self.assertIn(url, payload['content'])
        self.assertLessEqual(len(payload['content']), 2000)
        self.assertTrue(payload['embeds'])
        self.assertLessEqual(len(payload['embeds']), 10)
        total = 0
        for embed in payload['embeds']:
            self.assertLessEqual(len(embed['description']), 4096)
            total += len(embed['title']) + len(embed['description'])
        self.assertLessEqual(total, 6000)
        self.assertEqual(payload['allowed_mentions'], {'parse': []})

    def test_sections_keep_both_languages(self):
        titles = [h for h, _ in discord_post.split_sections(self.notes())]
        self.assertIn('English', titles)
        self.assertIn('Português', titles)

    def test_no_webhook_is_tracked(self):
        tracked = subprocess.run(['git', 'ls-files'], cwd=ROOT, capture_output=True, text=True, check=True).stdout
        for name in tracked.splitlines():
            path = ROOT / name
            if not path.is_file() or path.stat().st_size > 2_000_000:
                continue
            data = path.read_bytes()
            for host in (b'discord', b'discordapp'):
                self.assertNotIn(host + b'.com/api/' + b'webhooks/', data, name)


if __name__ == '__main__':
    unittest.main()
