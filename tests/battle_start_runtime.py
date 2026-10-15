"""Confirm translated Battle Start, native blink and arena labels with state parity."""
from pathlib import Path
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
import shutil
import struct
import subprocess
import tempfile
from pause_menu_runtime import BATTLE_ENTER
from battle_translation_runtime import check
ROOT=Path(__file__).resolve().parents[1]

def run(args,arena,translated):
    folder=Path(tempfile.mkdtemp(prefix=f'arena{arena}-'+('on-' if translated else 'off-'),dir=args.out))
    shutil.copy2(args.exe,folder/'game.exe');shutil.copytree(ROOT/'mods',folder/'mods')
    state='format_version=1\n'
    mods=([('ptbr','menus_ptbr')] if translated else [])+([('gold-battle','gold_battle')] if arena>10 else [])
    for mod,feature in mods:state+=f'[[package]]\nid="superbomberman5.{mod}"\nversion="0.1.0"\n[[feature]]\npackage_id="superbomberman5.{mod}"\nid="{feature}"\nenabled=true\n'
    (folder/'mods/preloaded/state.toml').write_text(state)
    (folder/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=1\n[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    route=BATTLE_ENTER.split('dump selection')[0]+('press right 2\nwait 20\n'*(arena-1))+'dump selection\npress a 2\n'
    tags=['selection']
    for frame in range(32):route+=f'dump confirm{frame}\nwait 1\n';tags.append(f'confirm{frame}')
    previous=32
    for frame in [80,120,160,200]:route+=f'wait {frame-previous}\ndump confirm{frame}\n';tags.append(f'confirm{frame}');previous=frame
    route+='wait 450\ndump stage\nquit\n';(folder/'route.input').write_text(route)
    env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/'run.log').open('w') as log:
        result=subprocess.run([str(folder/'game.exe'),'--no-launcher','--config','headless.ini','--script','route.input',str(args.rom)],cwd=folder,env=env,stdout=log,stderr=log,timeout=300)
    assert result.returncode==0,(folder,result.returncode)
    log=(folder/'run.log').read_text().lower();assert not any(x in log for x in ['interp_cap','unresolved-abandon','architectural brk','fatal']),folder
    assert (folder/'stage.wram.bin').read_bytes()[0x1031]==0x74+4*(arena-1),folder
    return folder,tags

def compare(on,off,tags,arena):
    visible=hidden=0
    for tag in tags+['stage']:
        for suffix in ['wram.bin','sram.bin','vram.bin','oam.bin','cgram.bin']:
            assert (on/f'{tag}.{suffix}').read_bytes()==(off/f'{tag}.{suffix}').read_bytes(),(arena,tag,suffix)
        if tag.startswith('confirm'):
            o=struct.unpack('<256H',(off/f'{tag}.oam.bin').read_bytes()[:512]);a=(on/f'{tag}.fb.bgrx').read_bytes();b=(off/f'{tag}.fb.bgrx').read_bytes()
            region=lambda frame:b''.join(frame[(y*256+56)*4:(y*256+200)*4] for y in range(18,36))
            if o[0]==19*256+56 and o[1] in [0x22a2,0x2080]:
                assert region(a)!=region(b),(tag,'missing translated heading')
                if o[1]==0x22a2:visible+=1
            else:assert region(a)==region(b),(tag,'native blink must stay blank');hidden+=1
    assert visible and hidden,(visible,hidden)
    assert check(on,tags)=={arena}
    assert (on/'stage.fb.bgrx').read_bytes()==(off/'stage.fb.bgrx').read_bytes(),'Overlay leaked into gameplay'
    print(f'Arena {arena}: {len(tags)} frames, translated heading {visible} visible/{hidden} blink-off, Portuguese names and exact guest restoration passed: {on}',flush=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,default=ROOT/'.bootstrap/battle-start-validation');args=p.parse_args();args.exe=args.exe.resolve();args.rom=args.rom.resolve();args.out=args.out.resolve();args.out.mkdir(parents=True,exist_ok=True)
    with ThreadPoolExecutor(max_workers=4) as pool:
        jobs={arena:[pool.submit(run,args,arena,t) for t in [True,False]] for arena in [1,13]}
        for arena,pair in jobs.items():
            (on,tags),(off,other)=[j.result() for j in pair];assert tags==other;compare(on,off,tags,arena)
if __name__=='__main__':main()
