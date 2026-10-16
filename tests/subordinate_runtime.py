"""Regress the unlocked yellow-Bomberman cell using isolated local SRAM."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile
from pause_menu_runtime import BATTLE_ENTER, press
ROOT=Path(__file__).resolve().parents[1]

def run(args,kind,unlocked,subordinate,honey):
    # The Super Bomberman 5 Extra Characters mod carries Subordinate Bomber
    # and Honey/Kotetsu together; the cases differ only by enabling it.
    honey=subordinate
    folder=Path(tempfile.mkdtemp(prefix=kind+'-',dir=args.out))
    shutil.copy2(args.exe,folder/'game.exe');shutil.copytree(ROOT/'mods',folder/'mods')
    state='format_version = 1\n'
    mods=[('ptbr','menus_ptbr'),('selection-colors','color_controls'),('gold-battle','gold_battle')]
    if subordinate:mods.append(('extra-characters','extra_characters'))
    if kind=='normal':mods.append(('normal-characters','character_select'))
    for mod,feature in mods:
        state+=f'[[package]]\nid="superbomberman5.{mod}"\nversion="0.1.0"\n[[feature]]\npackage_id="superbomberman5.{mod}"\nid="{feature}"\nenabled=true\n'
    (folder/'mods/preloaded/state.toml').write_text(state)
    (folder/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=1\n[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    (folder/'saves').mkdir()
    sram=bytearray(args.seed.read_bytes());sram[0x1e9]=int(unlocked)
    (folder/'saves/save.srm').write_bytes(sram)
    if kind=='normal':
        route='turbo on\nwait 900\n'+press('start',180)*4+press('a',90)+press('a',180)+press('a',900)
    else:
        route=BATTLE_ENTER.split('press a 2\nwait 180\npress p2:a',1)[0]
    route+='dump picker\n'+press('r')*5+'dump yellow_standard\n'+press('right')*4+press('down')+'dump subordinate\n'
    if kind=='normal':
        route+=press('a',1200)+press('start',600)+'dump stage\nquit\n'
    else:
        route+=press('a',180)+press('p2:a',180)+press('a',180)+press('p2:a',180)+press('a',180)+'wait 500\n'+press('a',500)+press('a',600)+'dump stage\nquit\n'
    (folder/'route.input').write_text(route)
    env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/'run.log').open('w') as log:
        result=subprocess.run([str(folder/'game.exe'),'--no-launcher','--config','headless.ini','--script','route.input',str(args.rom)],cwd=folder,env=env,stdout=log,stderr=log,timeout=240)
    assert result.returncode==0,(folder,result.returncode)
    ram=lambda tag:(folder/f'{tag}.wram.bin').read_bytes()
    cells=0x1f800 if honey else 0x62c9
    assert ram('picker')[cells+9]==(10 if subordinate else 1),(folder,'cell',ram('picker')[cells+9])
    assert ram('yellow_standard')[0x6264]==5,(folder,'standard yellow')
    assert ram('stage')[0x16c2]==(9 if subordinate else 0),(folder,'stage character',ram('stage')[0x16c2])
    assert ram('stage')[0x1031]==(13 if kind=='normal' else 116),(folder,'stage mode')
    for bad in ['interp_cap','unresolved-abandon','architectural brk','fatal','failed to activate']:
        assert bad not in (folder/'run.log').read_text().lower(),(folder,bad)
    assert (folder/'subordinate.sram.bin').read_bytes()[0x1e9]==int(unlocked)
    print(f'{kind}: unlocked={unlocked}, Subordinate={subordinate}, Honey/Kotetsu={honey}: passed: {folder}',flush=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--rom',type=Path,required=True)
    parser.add_argument('--seed',type=Path,required=True,help='Local native SRAM dump used only as an isolated seed')
    parser.add_argument('--out',type=Path,default=ROOT/'.bootstrap/subordinate-slot-validation')
    args=parser.parse_args()
    for field in ['exe','rom','seed','out']:setattr(args,field,getattr(args,field).resolve())
    args.out.mkdir(parents=True,exist_ok=True)
    cases=[('battle-unlocked',True,True,True),('battle-locked',False,True,False),('battle-stock',True,False,True),('normal',True,True,True)]
    with ThreadPoolExecutor(max_workers=2) as pool:
        jobs=[pool.submit(run,args,*case) for case in cases]
        for job in jobs:job.result()
if __name__=='__main__':main()
