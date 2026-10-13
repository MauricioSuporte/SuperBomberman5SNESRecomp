from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

class ReleaseEncodingTests(unittest.TestCase):
    def test_file_output_preserves_accents_and_historical_version(self):
        local=ROOT/'.bootstrap'; local.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='release-utf8-',dir=local) as directory:
            out=Path(directory)/'notes.md'
            subprocess.run([sys.executable,str(ROOT/'tools/release-notes.py'),'--output',str(out),'--version','0.1.0'],check=True)
            text=out.read_bytes().decode('utf-8',errors='strict')
            self.assertIn('Português',text)
            self.assertIn('Protótipo não oficial',text)
            self.assertIn('v0.1.0 —',text)
            self.assertNotIn('\ufffd',text)

if __name__=='__main__': unittest.main()
