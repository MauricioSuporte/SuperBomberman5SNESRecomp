"""Local cartridge integration tests; all captures stay under ignored .bootstrap/."""
from pathlib import Path
import os,shutil,subprocess,sys
import argparse, tempfile
parser=argparse.ArgumentParser(description='Windows/LLE Louie fixtures; requires SB5_COOP_INPUT_FIXTURE=ON.')
parser.add_argument('--exe',type=Path,required=True)
parser.add_argument('--rom',type=Path,required=True)
parser.add_argument('--case',choices=['red','yellow','black','chain','five'],required=True)
parser.add_argument('--out',type=Path)
args=parser.parse_args()
kind={'red':0,'yellow':3,'black':6,'chain':3,'five':6}[args.case]
root=Path(__file__).resolve().parents[1]
output=args.out or root/'.bootstrap/louie-validation'
output.mkdir(parents=True,exist_ok=True)
out=Path(tempfile.mkdtemp(prefix=args.case+'-',dir=output)).resolve()
shutil.copy2(args.exe.resolve(),out/'game.exe');shutil.copytree(root/'mods',out/'mods',dirs_exist_ok=True)
(out/'mods/preloaded/state.toml').write_text('format_version = 1\n[[package]]\nid = "superbomberman5.louie-improvements"\nversion = "0.1.0"\n[[feature]]\npackage_id = "superbomberman5.louie-improvements"\nid = "louie_improvements"\nenabled = true\n',encoding='utf8')
if args.case=='five':
 with (out/'mods/preloaded/state.toml').open('a',encoding='utf8') as f:
  for package,feature in [('normal-coop','five_players'),('ptbr','menus_ptbr')]:
   f.write(f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\nenabled = true\n')
(out/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nWindowScale=1\nNewRenderer=0\n[Sound]\nEnableAudio=1\n',encoding='utf8')
route='turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\npress start 2\nwait 180\npress start 2\nwait 180\npress a 2\nwait 90\npress a 2\nwait 180\npress a 2\nwait 700\npress start 2\nwait 800\ndump stage\npoke 0x146b 15\npoke 0x1063 10\npoke 0x1163 80\nwait 5\ndump egg\npress right 16\nwait 120\ndump mount\npress y 2\nwait 20\ndump action\nquit\n'
route=route.replace('poke 0x146b',f'poke 0x1fff0 {kind+1:02x}\npoke 0x146b')
if args.case=='five':
 route=route.replace('press start 2\nwait 180\npress a 2\nwait 90',('press down 2\nwait 25\n'*4)+'press start 2\nwait 180\npress a 2\nwait 90')
 route=route.replace('poke 0x146b 15','poke 0x14c1 15').replace('poke 0x1063 10','poke 0x10b9 10').replace('poke 0x1163 80','poke 0x11b9 80')
 route=route.replace('press right 16\n','wait 16\n')
# Controlled open corridor; all movement and hits still execute native routines.
fixture=''
for row in (3,4,5):
 fixture+=f'poke {0x1052+row*16+2:#x} 000000000000000000\npoke {0x1152+row*16+2:#x} 000000000000000000\n'
fixture+='poke 0x567 3800\npoke 0x5c7 4400\npoke 0x182a 28\npoke 0x1842 00\npoke 0x1872 28\npoke 0x188a 00\nwait 3\npoke 0x1632 01\n'
if kind==0:
 fixture+='poke 0x55f 5000\npoke 0x5bf 4400\npoke 0x182f 40\npoke 0x1847 00\npoke 0x1877 28\npoke 0x188f 00\nwait 1\ndump red_before\npress y 2\nwait 4\ndump red_hit\nwait 100\ndump red_held\nwait 100\ndump red_end\n'
elif kind==3:
 fixture+='press a 2\nwait 3\npoke 0x567 2800\npoke 0x182a 18\npoke 0x1632 01\nwait 2\npoke 0x1632 01\ndump yellow_before\npress y 2\nwait 4\ndump yellow_hit\nwait 20\ndump yellow_roll\n'
else:
 fixture+='poke 0x55f 5800\npoke 0x5bf 4400\npoke 0x182f 48\npoke 0x1847 00\npoke 0x1877 28\npoke 0x188f 00\nwait 1\ndump black_before\npress y 2\nwait 8\ndump black_hit\nwait 25\ndump black_end\n'
if args.case=='chain':
 fixture=fixture.replace('press a 2\nwait 3\npoke 0x567', 'poke 0x4ebd 03\npress a 2\nwait 3\npoke 0x567 6800\nwait 2\npress a 2\nwait 3\npoke 0x567 c000\nwait 2\npress a 2\nwait 3\npoke 0x108b 30\npoke 0x567')
 fixture=fixture.replace('dump yellow_before','poke 0x1188 00\npoke 0x118d 00\ndump yellow_before')
 fixture+='wait 20\ndump chain1\nwait 20\ndump chain2\nwait 20\ndump chain3\n'
if args.case=='five':
 for old,new in [('0x567','0x56f'),('0x5c7','0x5cf'),('0x182a','0x182e'),('0x1842','0x1846'),('0x1872','0x1876'),('0x188a','0x188e'),('0x1632','0x1636'),('0x182f','0x1834'),('0x1847','0x184c'),('0x1877','0x187c'),('0x188f','0x1894')]:fixture=fixture.replace(old,new)
 fixture=fixture.replace('press y 2','poke 0x1ffe4 0200\nwait 2\npoke 0x1ffe4 0000')
route=route[:route.index('press y 2')]+fixture+'quit\n'
(out/'route.input').write_text(route,encoding='utf8')
env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(out))
with (out/'run.log').open('w',encoding='utf8') as log:
 p=subprocess.run([str(out/'game.exe'),'--no-launcher','--config','headless.ini','--script','route.input',str(args.rom.resolve())],cwd=out,env=env,stdout=log,stderr=log,timeout=180)
assert p.returncode==0,(p.returncode,out)
log=(out/'run.log').read_text(encoding='utf8',errors='replace').lower()
for error in ['interp_cap','unresolved-abandon','architectural brk','fatal','watchdog tripped','dispatch_abandon']:
 assert error not in log,(error,out)
def ram(tag):return (out/f'{tag}.wram.bin').read_bytes()
def pos(r,a=0):return (r[0x182a+a]|r[0x1842+a]<<8,r[0x1872+a]|r[0x188a+a]<<8)
seat_index=4 if args.case=='five' else 0
m=ram('mount');assert m[0x1782+seat_index]&0x80,(args.case,'mount not attached',out)
assert m[0x16c2+m[0x1cbf]+seat_index]==kind,(args.case,'wrong egg type',out)
if args.case=='red':
 hit,held,end=map(ram,['red_hit','red_held','red_end'])
 assert pos(hit,5)==pos(held,5) and pos(end,5)!=pos(held,5),out
 assert hit[0x1fff1]==ram('red_before')[0x1fff1]+1,out
 assert hit[0x800+hit[0x17fa+5]]==0 and end[0x800+end[0x17fa+5]]==1,out
elif args.case in ('yellow','chain'):
 before,hit,roll=map(ram,['yellow_before','yellow_hit','yellow_roll'])
 assert hit[0xe90]&0x10 and roll[0xe7c]>before[0xe7c],out
 if args.case=='chain':
  end=ram('chain3');assert before[0xe40+1] and before[0xe40+2],out
  assert end[0xe7c+1]>before[0xe7c+1] and end[0xe7c+2]>before[0xe7c+2],out
  assert end[0x1052+0x39]!=0x30,out
else:
 before,hit,end=map(ram,['black_before','black_hit','black_end'])
 assert m[0x1fff4]==1,('native Black acquisition command',out)
 assert hit[0x1fff5]==before[0x1fff5]+1,('native dash command',out)
 assert end[0x1fff5]==hit[0x1fff5],('dash sound repeated',out)
 enemy=10 if args.case=='five' else 5
 assert pos(end,enemy)[0]>=pos(before,enemy)[0]+16,out
 assert hit[0x15a2+seat_index]&4 and end[0x1782+seat_index]&0x80,out
 if args.case=='five':assert end[0x1cbf]==5 and end[0x1d25]==31,out
print(f'{args.case}: runtime assertions passed; captures in {out}')
