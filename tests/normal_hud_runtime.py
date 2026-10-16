"""Local Windows/LLE Normal HUD regression; requires the user's ROM.
Captures and fresh SRAM remain under ignored .bootstrap/normal-hud-runtime.
"""
from pathlib import Path
import argparse
import tempfile
import normal_character_cancel_runtime as harness

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe',type=Path,default=root/'build-lle/SuperBomberman5SNESRecomp.exe')
parser.add_argument('--rom',type=Path,default=root/'Super Bomberman 5 (USA).sfc')
parser.add_argument('--english',action='store_true',help='Run with PT-BR disabled.')
args=parser.parse_args()
args.exe=args.exe.resolve();args.rom=args.rom.resolve();args.minimal=False
out=root/'.bootstrap/normal-hud-runtime';out.mkdir(parents=True,exist_ok=True)
folder=Path(tempfile.mkdtemp(prefix='two-color-en-' if args.english else 'two-color-ptbr-',dir=out))
harness.setup(args,folder,not args.english)
p=folder/'mods/preloaded/state.toml'
p.write_text(p.read_text(encoding='utf-8')+'\n[[package]]\nid = "superbomberman5.hud-faces"\nversion = "0.1.0"\n[[feature]]\npackage_id = "superbomberman5.hud-faces"\nid = "hud_faces"\nenabled = true\n',encoding='utf-8')
route=('turbo on\nwait 900\npress start 6\nwait 180\npress start 6\nwait 180\npress start 6\nwait 180\npress down 6\nwait 25\npress start 6\nwait 480\npress a 6\nwait 360\npress a 6\nwait 360\nuntil cb8 == 8e 2000\nwait 30\npress right 6\nwait 20\npress r 6\nwait 20\npress a 6\nwait 20\npress p2:down 6\nwait 20\npress p2:r 6\nwait 20\npress p2:a 6\nwait 600\npress start 6\nwait 700\ndump alive\npress a 2\nwait 180\ndump fuse\nwait 40\ndump dying\nwait 100\ndump dead\npress start 6\nwait 180\ndump rejoin\npress p2:a 2\nwait 180\ndump p2-dying\nwait 160\ndump p2-dead\nquit\n')
harness.execute(folder,args.rom,route,'run')
frames={tag:(folder/(tag+'.wram.bin')).read_bytes() for tag in
        ('alive','dying','dead','rejoin','p2-dead')}
for tag,r in frames.items():
    assert r[0x1031]==13,(tag,'wrong stage')
    assert r[0x35c5]!=r[0x35c6],(tag,'shared face object')
    assert r[0x16c2:0x16c4]==bytes((1,6)),(tag,'wrong characters')
    assert r[0x16da:0x16dc]==bytes((9,50)),(tag,'wrong selected colors')
for seat in range(2):
    alive=frames['alive'];o=alive[0x35c5+seat];a=alive[0x79f+o]
    dead=frames['dead' if seat==0 else 'p2-dead']
    assert dead[0x15a2+seat]&128,('not dead',seat)
    assert dead[0x79f+dead[0x35c5+seat]]!=a,('unchanged defeated face',seat)
r=frames['rejoin'];assert not r[0x15a2]&128
assert r[0x79f+r[0x35c5]]==frames['alive'][0x79f+frames['alive'][0x35c5]],'rejoin kept defeated face'
print('Normal HUD P1/P2 colors, deaths and P1 rejoin passed:',folder)
