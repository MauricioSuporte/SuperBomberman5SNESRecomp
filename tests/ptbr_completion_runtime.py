"""Isolated LLE PT-BR completion/HUD regressions with a user-owned ROM.

Death/roster and saved completion counts are test fixtures, not a playthrough.
Raw scene captures stay in ignored .bootstrap/.
"""
from pathlib import Path
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MENU = ('turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\n'
        'press start 2\nwait 180\npress start 2\nwait 180\npress a 2\nwait 270\n')


def execute(args, folder, route, name):
    script=folder/(name+'.input');script.write_text(route,encoding='utf-8')
    env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/(name+'.log')).open('w') as log:
        result=subprocess.run([str(folder/'game.exe'),'--no-launcher','--config',str(folder/'headless.ini'),
                               '--script',str(script),str(args.rom)],cwd=folder,env=env,stdout=log,stderr=log,timeout=240)
    assert result.returncode==0,(folder,result.returncode)
    log=(folder/(name+'.log')).read_text(errors='replace').lower()
    for bad in ['interp_cap','unresolved-abandon','dispatch_abandon','architectural brk','fatal','failed to activate']:
        assert bad not in log,(folder,bad)


def run(args, scenario, enabled):
    folder=Path(tempfile.mkdtemp(prefix=scenario+('-on-' if enabled else '-off-'),dir=args.out))
    shutil.copy2(args.exe,folder/'game.exe');shutil.copytree(ROOT/'mods',folder/'mods')
    (folder/'mods/preloaded/state.toml').write_text('format_version = 1\n[[package]]\nid = "superbomberman5.ptbr"\nversion = "0.1.0"\n[[feature]]\npackage_id = "superbomberman5.ptbr"\nid = "menus_ptbr"\nenabled = '+str(enabled).lower()+'\n')
    (folder/'headless.ini').write_text(f'[Graphics]\nOutputMethod=SDL-Software\nNewRenderer={args.renderer}\n[Sound]\nEnableAudio=1\n')
    if scenario in ('continue','erase'):
        # Obtain the native initialized SRAM header at runtime; never embed it.
        execute(args,folder,'turbo on\nwait 900\nquit\n','initialize')
        save=folder/'saves/save.srm';ram=bytearray(save.read_bytes())
        ram[0x42]=0;ram[0x43]=0x0d
        ram[0x30:0x32]=sum(ram[0x32:0x6b]).to_bytes(2,'little');save.write_bytes(ram)
    if scenario in ('continue','erase'):
        route=MENU+('press down 2\nwait 30\n'*(1 if scenario=='continue' else 2))+'press a 2\nwait 180\ndump file\nquit\n'
        tags=['file']
    elif scenario=='hud':
        route=(ROOT/'validation/first-stage.input').read_text().split('press right 30')[0]
        route=route.replace('press start 2\nwait 180\npress a 2','press down 2\nwait 30\npress start 2\nwait 180\npress a 2')
        route+='poke 0x17fa ff\npoke 0x15a2 80\npoke 0x1d23 00\n';tags=[]
        for side in ('left','right'):
            if side=='right':route+='poke 0x17fa 08\npoke 0x15a2 01\npoke 0x17fb ff\npoke 0x15a3 80\n'
            for message,reserve in [('over',0),('start',1)]:
                route+=f'poke 0x1d23 {reserve:02x}\n'
                for n in range(4):
                    tag=f'{side}-{message}-{n}';tags.append(tag);route+=f'wait 16\ndump {tag}\n'
        route+='quit\n'
    else:
        route=(ROOT/'validation/first-stage.input').read_text().split('press right 30')[0]
        if scenario=='perfect':route+='poke 0x2902 '+('ff'*12)+'0f\n'
        route+='poke 0x1d23 00\npoke 0x15a2 80\nwait 1200\ndump over\npress down 2\npress a 2\nwait 600\n'
        if scenario=='perfect':route+='dump reward\npress a 2\nwait 600\n'
        route+='dump certificate\npress a 2\nwait 120\ndump tips\nquit\n'
        tags=['over']+(['reward'] if scenario=='perfect' else [])+['certificate','tips']
    execute(args,folder,route,'run');return folder,tags


def compare(off,on,tags,scenario):
    if scenario=='perfect':
        assert (off/'certificate.wram.bin').read_bytes()[0x103c]==100,'Fixture must reach the native 100% result'
    visible=[]
    for tag in tags:
        for suffix in ['wram.bin','vram.bin','oam.bin','cgram.bin','sram.bin']:
            assert (off/f'{tag}.{suffix}').read_bytes()==(on/f'{tag}.{suffix}').read_bytes(),(scenario,tag,suffix)
        changed=(off/f'{tag}.fb.bgrx').read_bytes()!=(on/f'{tag}.fb.bgrx').read_bytes()
        if tag=='reward':
            a=(off/f'{tag}.fb.bgrx').read_bytes();b=(on/f'{tag}.fb.bgrx').read_bytes()
            points={(i//4%256,i//4//256) for i in range(0,len(a),4) if a[i:i+4]!=b[i:i+4]}
            # LLE framebuffer line zero is SNES scanline one.
            assert all(48<=x<208 and 15<=y<63 for x,y in points),'Reward changed the password or scene'
            for row in [15,31,47]:assert any(row<=y<row+16 for x,y in points),'Missing reward line'
        if changed:visible.append(tag)
        if scenario!='hud':assert changed,(scenario,tag,'missing translation')
    if scenario=='hud':
        for side in ['left','right']:
            for message in ['over','start']:assert any(t.startswith(side+'-'+message) for t in visible),(side,message)
        assert len(visible)<len(tags),'Blink-off frames must stay blank'
    print(scenario,'native-state parity and visible translations passed',on,flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--out',type=Path,default=ROOT/'.bootstrap/ptbr-completion-validation');p.add_argument('--scenarios',nargs='+',default=['continue','erase','ending','perfect','hud']);p.add_argument('--renderer',type=int,choices=[0,1],default=0)
    args=p.parse_args();args.exe=args.exe.resolve();args.rom=args.rom.resolve();args.out.mkdir(parents=True,exist_ok=True)
    with ThreadPoolExecutor(max_workers=4) as pool:
        jobs={s:[pool.submit(run,args,s,e) for e in [False,True]] for s in args.scenarios}
        for s,pair in jobs.items():
            (off,tags),(on,other)=[j.result() for j in pair];assert tags==other;compare(off,on,tags,s)
