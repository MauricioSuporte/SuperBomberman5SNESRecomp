import importlib.util
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def load(name, file):
    spec = importlib.util.spec_from_file_location(name, ROOT / file)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


check = load('check_rom', 'tools/check-rom.py')
probes = load('gen_rom_probes', 'tools/gen-rom-probes.py')


def synthetic(title=b'Hu SUPER BOMBERMAN 5 ', map_mode=0x31):
    image = bytearray(b'\x5a' * 0x200000)
    image[0xffc0:0xffc0 + len(title)] = title
    image[0xffd5] = map_mode
    image[0xffdc:0xffe0] = b'\xff\xff\x00\x00'
    total = check.checksum(bytes(image))
    complement = total ^ 0xffff
    image[0xffdc:0xffe0] = bytes([complement & 255, complement >> 8, total & 255, total >> 8])
    return bytes(image)


class ContentCheckTests(unittest.TestCase):
    def test_accepts_the_game_and_its_variants(self):
        self.assertIsNone(check.problem(synthetic()))
        self.assertIsNone(check.problem(synthetic(b'Hu SUPER BOMBERMAN 5G')))

    def test_refuses_other_files(self):
        self.assertIn('title', check.problem(synthetic(b'Hu SUPER BOMBERMAN 4 ')))
        self.assertIn('HiROM', check.problem(synthetic(map_mode=0x30)))
        self.assertIn('small', check.problem(b'PK\x03\x04' + b'\0' * 1024))

    def test_copier_header_is_removed(self):
        self.assertEqual(check.image_of(b'\0' * 512 + b'\1' * 1024), b'\1' * 1024)


class ProbeTableTests(unittest.TestCase):
    def test_every_plugin_with_a_table_is_registered(self):
        source = ''.join(p.read_text(encoding='utf-8', errors='replace') for p in (ROOT / 'src').glob('*.c'))
        registered = set(re.findall(r'SB5_ROM_GATED\("([^"]+)"', source))
        self.assertLessEqual(set(probes.PLUGINS), registered)

    def test_table_has_no_rom_bytes_only_addresses_and_digests(self):
        table = (ROOT / 'src/rom_probes.h').read_text(encoding='utf-8')
        self.assertEqual(set(probes.PLUGINS), set(re.findall(r'\{"([^"]+)", kSites', table)))


if __name__ == '__main__':
    unittest.main()
