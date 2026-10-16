"""Regress selected characters/colors through native Game Over -> Continue.
Run with a local Windows host and user ROM; all saves/captures stay ignored.
Use --single, --english, or --skins (Honey/Subordinate, co-op disabled).
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
parser.add_argument('--skins',action='store_true',help='Honey/Subordinate with co-op disabled.')
parser.add_argument('--single',action='store_true')
args=parser.parse_args()
args.exe=args.exe.resolve();args.rom=args.rom.resolve();args.minimal=False
out=root/'.bootstrap/normal-hud-runtime';out.mkdir(parents=True,exist_ok=True)
folder=Path(tempfile.mkdtemp(prefix='two-color-en-' if args.english else 'two-color-ptbr-',dir=out))
harness.setup(args,folder,not args.english)
p=folder/'mods/preloaded/state.toml'
p.write_text(p.read_text(encoding='utf-8')+'\n[[package]]\nid = "superbomberman5.hud-faces"\nversion = "0.1.0"\n[[feature]]\npackage_id = "superbomberman5.hud-faces"\nid = "hud_faces"\nenabled = true\n',encoding='utf-8')
route=('turbo on\nwait 900\npress start 6\nwait 180\npress start 6\nwait 180\npress start 6\nwait 180\npress down 6\nwait 25\npress start 6\nwait 480\npress a 6\nwait 360\npress a 6\nwait 360\nuntil cb8 == 8e 2000\nwait 30\npress right 6\nwait 20\npress r 6\nwait 20\npress a 6\nwait 20\npress p2:down 6\nwait 20\npress p2:r 6\nwait 20\npress p2:a 6\nwait 600\npress start 6\nwait 700\ndump alive\npoke 0x1d23 00\npress a+p2:a 2\nwait 900\ndump gameover\npress a 6\nwait 1200\ndump map\npress start 6\nwait 700\ndump continued\nquit\n')
if args.skins:
    p.write_text(p.read_text(encoding='utf-8').replace('id = "five_players"\nenabled = true','id = "five_players"\nenabled = false'),encoding='utf-8')
    route=route.replace('press right 6\nwait 20\npress r 6','press right 6\nwait 20\n'*5+'press r 6',1)
    route=route.replace('press p2:down 6\nwait 20\npress p2:r','press p2:down 6\nwait 20\n'+'press p2:right 6\nwait 20\n'*3+'press p2:r')
if args.single:
    route=route.replace('press down 6\nwait 25\npress start 6','press start 6',1)
    route=route.replace('press p2:down 6\nwait 20\npress p2:r 6\nwait 20\npress p2:a 6\n','').replace('press a+p2:a 2','press a 2')
harness.execute(folder,args.rom,route,'run')
alive=(folder/'alive.wram.bin').read_bytes()
continued=(folder/'continued.wram.bin').read_bytes()
count=1 if args.single else 2
assert continued[0x1031]==13,'Continue did not reach Zone 1-1'
for field in (0x16c2,0x16da,0x625f,0x6264,0x1f828):
    assert alive[field:field+count]==continued[field:field+count],(folder,hex(field),'lost choice')
for seat in range(count):
    assert not continued[0x17fa+seat]&128,(folder,seat,'missing actor')
    assert not continued[0x15a2+seat]&128,(folder,seat,'still dead')
print('Game Over/Continue retained characters, palettes, skins and roster:',folder)


