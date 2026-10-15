"""Picker defaults and shared BG palette regression; raw evidence stays in .bootstrap/."""
from pathlib import Path
import shutil,subprocess,os
import argparse
import tempfile
parser=argparse.ArgumentParser(description='Isolated picker color regression using a user-owned JPN ROM.')
parser.add_argument('--exe',type=Path,required=True)
parser.add_argument('--rom',type=Path,required=True)
parser.add_argument('--out',type=Path,default=Path('.bootstrap/selection-color-fix'))
args=parser.parse_args()
root=Path(__file__).resolve().parents[1];base=args.out.resolve();base.mkdir(parents=True,exist_ok=True)
for mode in ['normal','battle']:
 d=Path(tempfile.mkdtemp(prefix=mode+'-',dir=base));shutil.copy2(args.exe.resolve(),d/'game.exe');shutil.copytree(root/'mods',d/'mods',dirs_exist_ok=True)
 state='format_version = 1\n'
 for package,feature in [('normal-characters','character_select'),('ptbr','menus_ptbr'),('selection-colors','color_controls')]:
  state+=f'[[package]]\nid = "superbomberman5.{package}"\nversion = "0.1.0"\n[[feature]]\npackage_id = "superbomberman5.{package}"\nid = "{feature}"\nenabled = true\n'
 (d/'mods/preloaded/state.toml').write_text(state,encoding='utf-8')
 (d/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=0\n[Sound]\nEnableAudio=1\n')
 old='turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\npress start 2\nwait 180\npress start 2\nwait 180\npress a 2\nwait 90\npress a 2\nwait 180\npress a 2\nwait 700\n' if mode=='normal' else 'turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\npress down 2\npress start 2\nwait 700\npress a 2\nwait 500\npress a 2\nwait 500\npress down 2\nwait 20\npress left 2\nwait 20\npress down 2\nwait 20\npress left 2\nwait 20\npress down 2\nwait 20\npress left 2\nwait 20\npress down 2\nwait 20\npress left 2\nwait 20\nwait 60\npress a 2\nwait 500\n'
 route=old+'dump initial\n'
 for cell in range(5):
  route+=f'dump cell{cell}-base\n'
  for c in range(6): route+=f'press r 2\nwait 24\ndump cell{cell}-color{(c+1)%6}\n'
  route+='press right 2\nwait 40\n'
 route+='quit\n';(d/'route.input').write_text(route)
 env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(d))
 with (d/'run.log').open('w') as log:
  result=subprocess.run([str(d/'game.exe'),'--no-launcher','--config',str(d/'headless.ini'),'--script',str(d/'route.input'),str(args.rom.resolve())],cwd=d,env=env,stdout=log,stderr=log,timeout=240)
 assert result.returncode==0,(mode,result.returncode)
 log=(d/'run.log').read_text(errors='replace').lower()
 for bad in ['interp_cap','unresolved-abandon','architectural brk','fatal','failed to activate']:
  assert bad not in log,(mode,bad)
 initial=(d/'initial.wram.bin').read_bytes()
 assert initial[0x1031]==4 and initial[0x6264:0x6269]==bytes(range(5)),mode
 print(mode,result.returncode)
 for cell in range(5):
  paths=[d/f'cell{cell}-base.cgram.bin']+[d/f'cell{cell}-color{c}.cgram.bin' for c in range(6)]
  pos=0x38+cell*16
  assert (d/f'cell{cell}-base.wram.bin').read_bytes()[0x625f]==cell+1
  for c in range(6):
   ram=(d/f'cell{cell}-color{c}.wram.bin').read_bytes()
   assert ram[0x625f]==cell+1 and ram[0x6264]==c,(mode,cell,c)
  baseline=paths[0].read_bytes()
  for capture in paths[1:]:
   palette=capture.read_bytes()
   # Lower strip and its shared index 8 must remain byte-identical.
   assert palette[(pos-8)*2:(pos+1)*2]==baseline[(pos-8)*2:(pos+1)*2],(mode,cell,capture)
  print('column',cell+1,'all six variants preserve the lower strip')
 print('initial colors',(d/'initial.wram.bin').read_bytes()[0x6264:0x6269].hex())
