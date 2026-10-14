import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('finalizer', ROOT / 'tools/finalize-release.py')
finalizer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(finalizer)


class PackageIntegrityTests(unittest.TestCase):
    def test_embedded_checksums_cover_payload_with_relative_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'bundle.zip'
            payload = {'bundle/app.exe': b'synthetic executable',
                       'bundle/mods/example/manifest.toml': b'name = "Example"'}
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('bundle/mods/', b'')
                for name, data in payload.items():
                    archive.writestr(name, data)
            finalizer.add_file_checksums(path)
            with zipfile.ZipFile(path) as archive:
                lines = archive.read('bundle/FILE_SHA256SUMS').decode().splitlines()
                self.assertEqual(len(lines), len(payload))
                for line in lines:
                    digest, relative = line.split('  ')
                    self.assertEqual(digest, hashlib.sha256(payload['bundle/' + relative]).hexdigest())
                    self.assertNotIn('FILE_SHA256SUMS', relative)
