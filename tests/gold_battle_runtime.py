"""Gold Battle checks against a local user ROM; all captures stay in .bootstrap."""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from pause_menu_runtime import BATTLE_ENTER, press, pc
ROOT=Path(__file__).resolve().parents[1]

def rules_route():
    # The existing two-human Battle fixture stops at Rule Settings before A.
    return BATTLE_ENTER.rsplit('press a 2\nwait 500\ndump selection\n',1)[0]

def run(args,arena,translated=True):
    folder=Path(tempfile.mkdtemp(prefix=f'arena{arena}-',dir=args.out))
    shutil.copy2(args.exe,folder/'game.exe');shutil.copytree(ROOT/'mods',folder/'mods')
    state='format_version = 1\n'
    for mod,feature in [('gold-battle','gold_battle')]+([('ptbr','menus_ptbr')] if translated else []):
        state+=f'[[package]]\nid="superbomberman5.{mod}"\nversion="0.1.0"\n[[feature]]\npackage_id="superbomberman5.{mod}"\nid="{feature}"\nenabled=true\n'
    (folder/'mods/preloaded/state.toml').write_text(state,encoding='utf-8')
    (folder/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=1\n[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    route=rules_route()+press('down')*4+'dump super\n'+press('right',30)+'dump hyper\n'
    route+=press('left',30)+'dump left_super\n'+press('left',30)+'dump on\n'+press('left',30)+'dump off\n'
    route+=press('right',30)*3+'dump hyper_again\n'+press('a',500)+'dump selection\n'
    route+=press('right',20)*(arena-1)+'dump arena\n'+press('a',600)+'dump stage\nwait 300\ndump settled\nquit\n'
    (folder/'route.input').write_text(route)
    env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/'run.log').open('w') as log:
        result=subprocess.run([str(folder/'game.exe'),'--no-launcher','--config','headless.ini','--script','route.input',str(args.rom)],cwd=folder,env=env,stdout=log,stderr=log,timeout=240)
    assert result.returncode==0,(folder,result.returncode)
    log=(folder/'run.log').read_text(errors='replace').lower()
    for bad in ['interp_cap','unresolved-abandon','dispatch_abandon','architectural brk','fatal','failed to activate']:
        assert bad not in log,(folder,bad)
    def ram(tag):return (folder/f'{tag}.wram.bin').read_bytes()
    for tag,value in [('super',16),('hyper',80),('left_super',16),('on',8),('off',0),('hyper_again',80)]:
        assert ram(tag)[0x625d]&0x58==value,(folder,tag,hex(ram(tag)[0x625d]))
    assert ram('selection')[0x6245]==13
    assert ram('arena')[0x625a]==arena,(folder,ram('arena')[0x625a])
    for tag in ['stage','settled']:
        assert ram(tag)[0x1031]==0x74+4*(arena-1) and ram(tag)[0x625a]==arena,(folder,tag)
    print(f'Arena {arena}, Hyper rule cycle, PT-BR={translated}: passed: {folder}',flush=True)
    return folder

def revenge(args,enabled=True):
    folder=Path(tempfile.mkdtemp(prefix='revenge-' if enabled else 'stock-super-',dir=args.out))
    shutil.copy2(args.exe,folder/'game.exe');shutil.copytree(ROOT/'mods',folder/'mods')
    state='format_version = 1\n'
    for mod,feature in ([('gold-battle','gold_battle')] if enabled else [])+[('ptbr','menus_ptbr'),('pause-menu','pause_menu')]:
        state+=f'[[package]]\nid="superbomberman5.{mod}"\nversion="0.1.0"\n[[feature]]\npackage_id="superbomberman5.{mod}"\nid="{feature}"\nenabled=true\n'
    (folder/'mods/preloaded/state.toml').write_text(state)
    (folder/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=1\n[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    # Controlled contact fixture: native damage/death/cart/rejoin still execute.
    # Seat 2 stands at $BD in arena 1; mark its flame as owned by seat 1.
    route=BATTLE_ENTER+f'poke 0x625d {"74" if enabled else "34"}\n'
    route+='poke 0x15a2 59\npoke 0x1662 00\npoke 0x1782 00\nwait 160\ndump cart\n'
    route+='poke 0x4ebe 04\npoke 0x17cb 05\npoke 0x15eb 03\npoke 0x15d3 00\npoke 0x4ed6 0c\n'
    route+='poke 0x110f 01\npoke 0x500a 01\npoke 0x1663 00\npoke 0x1783 00\npoke 0x15a3 59\nwait 4\ndump death\nwait 100\ndump rejoin\n'+press('start',30)+press('down')+press('start',300)+'dump draw\nwait 900\ndump next\nquit\n'
    (folder/'route.input').write_text(route)
    env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/'run.log').open('w') as log:
        result=subprocess.run([str(folder/'game.exe'),'--no-launcher','--config','headless.ini','--script','route.input',str(args.rom)],cwd=folder,env=env,stdout=log,stderr=log,timeout=240)
    assert result.returncode==0,(folder,result.returncode)
    ram=lambda tag:(folder/f'{tag}.wram.bin').read_bytes()
    assert ram('cart')[0x1ba3]&1 and ram('death')[0x1ba3]&2
    assert not ram('rejoin')[0x15a2]&0x80 and ram('rejoin')[0x15a3]&0x80
    assert ram('draw')[0x1031]==10 and ram('next')[0x1031]==116,(folder,'pause draw/restart')
    actual=[ram('rejoin')[a] for a in [0x4ebd,0x17ca,0x15ea,0x15d2,0x4ed5]]
    assert actual==([4,5,3,0,12] if enabled else [1,2,1,0,0]),(folder,actual)
    for bad in ['interp_cap','unresolved-abandon','architectural brk','fatal']:
        assert bad not in (folder/'run.log').read_text().lower(),(folder,bad)
    print(f'Hyper={enabled}: native cart kill/rejoin and equipment passed: {folder}',flush=True)
    return folder

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--rom',type=Path,required=True)
    parser.add_argument('--out',type=Path,default=ROOT/'.bootstrap/gold-battle-validation')
    parser.add_argument('--revenge-only',action='store_true')
    parser.add_argument('--arena',type=int,choices=[1,11,12,13]);parser.add_argument('--english',action='store_true')
    args=parser.parse_args();args.exe=args.exe.resolve();args.rom=args.rom.resolve();args.out=args.out.resolve();args.out.mkdir(parents=True,exist_ok=True)
    with ThreadPoolExecutor(max_workers=2) as pool:
        jobs=([pool.submit(revenge,args,e) for e in [True,False]] if args.revenge_only else
              [pool.submit(run,args,a,not args.english) for a in ([args.arena] if args.arena else [11,12,13])])
        for job in jobs:job.result()
if __name__=='__main__':main()
