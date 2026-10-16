"""Regress Normal's two-face HUD with three to five co-op seats.
Requires a local host built with SB5_COOP_INPUT_FIXTURE=ON and the user ROM.
All saves and captures stay under ignored .bootstrap/normal-hud-coop.
"""
from pathlib import Path
import argparse
import tempfile
import tomllib
import normal_character_cancel_runtime as harness
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe',type=Path,required=True)
parser.add_argument('--rom',type=Path,default=root/'Super Bomberman 5 (USA).sfc')
parser.add_argument('--players',type=int,choices=[1,3,4,5],default=5)
parser.add_argument('--english',action='store_true')
args=parser.parse_args();args.exe=args.exe.resolve();args.rom=args.rom.resolve();args.minimal=False
out=root/'.bootstrap/normal-hud-coop';out.mkdir(exist_ok=True,parents=True)
folder=Path(tempfile.mkdtemp(prefix=f'{args.players}p-',dir=out))
harness.setup(args,folder,not args.english)
p=folder/'mods/preloaded/state.toml'
state=p.read_text(encoding='utf-8')
for package in ['hud-faces','gold-battle','pause-menu','louie-improvements','infinite-time']:
    manifest=tomllib.loads((folder/f'mods/preloaded/packages/superbomberman5.{package}/0.1.0/manifest.toml').read_text(encoding='utf-8'))
    state+=f'[[package]]\nid = "{manifest["id"]}"\nversion = "0.1.0"\n'
    for feature in manifest['feature']:
        state+=f'[[feature]]\npackage_id = "{manifest["id"]}"\nid = "{feature["id"]}"\nenabled = true\n'
p.write_text(state,encoding='utf-8')
cfg=folder/'headless.ini';cfg.write_text(cfg.read_text().replace('NewRenderer=0','NewRenderer=1'))
route=('turbo on\nwait 900\npress start 6\nwait 180\npress start 6\nwait 180\npress start 6\nwait 180\n'+'press down 6\nwait 25\n'*(args.players-1)+'press start 6\nwait 480\npress a 6\nwait 360\npress a 6\nwait 360\nuntil cb8 == 8e 2000\nwait 30\n')
route+='press right 6\nwait 20\n'*5+'press a 6\nwait 40\npress p2:down 6\nwait 20\npress p2:r 6\nwait 20\npress p2:a 6\nwait 40\n'
for seat in range(3,args.players+1):
    addr=0x1ffe0+(seat-3)*2
    directions=[0x20,0x80,0x80] if seat==3 else [0x40]*(seat-1)
    for bits in directions:
        route+=f'poke {addr:x} {bits.to_bytes(2,"little").hex()}\nwait 6\npoke {addr:x} 0000\nwait 20\n' 
    addr=0x1ffe0+(seat-3)*2
    route+=f'poke {addr:x} 0001\nwait 6\npoke {addr:x} 0000\nwait 180\n'
route+='wait 600\npress start 6\nwait 350\ndump alive\n'
# Model a stale Battle pointer in P3 and a wrong living-face animation.
# P1's object id is 8 + 2*players in this deterministic route.
p1_face=8+args.players*2
route+=f'poke 35c7 {p1_face:02x}\npoke {0x79f+p1_face:x} 1d\nwait 20\ndump repaired\n'
route+='poke 1ffe7 04\nwait 30\npoke 1ba5 04\npoke 1ffe0 0800\nwait 6\npoke 1ffe0 0000\nwait 120\ndump rejoin3\n'
route+='poke 1ffe7 02\nwait 30\ndump dead2\npress p2:start 6\nwait 120\ndump rejoin2\nquit\n'
if args.players==1:
    route=route.replace('press p2:down 6\nwait 20\npress p2:r 6\nwait 20\npress p2:a 6\nwait 40\n','')
    route=route[:route.index('dump alive')]+ 'dump alive\nwait 120\ndump repaired\ndump rejoin3\ndump dead2\ndump rejoin2\nquit\n'
harness.execute(folder,args.rom,route,'run')
for tag in ['alive','repaired','rejoin3','dead2','rejoin2']:
    ram=(folder/(tag+'.wram.bin')).read_bytes()
    assert ram[0x1031]==13,(tag,'not in stage')
    for seat in range(min(args.players,2)):
        o=ram[0x35c5+seat]
        assert o<0x60 and ram[0x4ff+o]==6 and ram[0x4a0+o]==0xf0,(tag,seat,'missing face')
    if tag!='dead2':
        assert ram[0x79f+ram[0x35c5]]==20,(tag,'P3 changed P1 animation')
        if args.players>1:assert ram[0x79f+ram[0x35c6]]==28,(tag,'wrong P2 face')
    oam=(folder/(tag+'.oam.bin')).read_bytes()
    top=[(oam[i*4],oam[i*4+1]) for i in range(128) if oam[i*4+1]==3]
    assert top.count((24,3))==1 and top.count((160,3))==(args.players>1),(tag,top)
print('P1 present, P2 absent:' if args.players==1 else f'{args.players} players: P1/P2 HUD, repair and extra-seat reentry passed:',folder)
