"""Exercise native PT-BR font families against identical untranslated menus.
All ROMs, saves and diagnostic images stay in ignored .bootstrap/.
"""
from pathlib import Path
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
import shutil
import struct
import subprocess
import tempfile
from pause_menu_runtime import BATTLE_ENTER
ROOT=Path(__file__).resolve().parents[1]
BASE='turbo on\nwait 900\npress start 2\nwait 180\npress start 2\nwait 180\n'

def route():
    p=BASE+'dump title\nwait 16\ndump title_blink\npress start 2\nwait 180\ndump players\npress start 2\nwait 700\ndump normal\npress a 2\nwait 500\ndump files\nreset\n'
    options=BASE+'press down 2\nwait 20\npress down 2\nwait 20\npress start 2\nwait 600\ndump options\n'
    p+=options+'press a 2\nwait 400\ndump password\nreset\n'
    p+=options+'press down 2\nwait 20\npress down 2\nwait 20\npress a 2\nwait 400\ndump soundtest\nreset\n'
    battle=BATTLE_ENTER.split('dump selection')[0];parts=battle.split('wait 500\n')
    p+= ''.join(part+(f'wait 500\ndump battle_menu{i+1}\n' if i<len(parts)-1 else '') for i,part in enumerate(parts))
    p+='dump arena\n'
    for n in range(8):p+=f'press right 2\nwait 1\ndump transition{n}\n'
    return p+'quit\n'

def run(args,enabled):
    folder=Path(tempfile.mkdtemp(prefix='on-' if enabled else 'off-',dir=args.out))
    shutil.copy2(args.exe,folder/'game.exe');shutil.copytree(ROOT/'mods',folder/'mods')
    (folder/'mods/preloaded/state.toml').write_text('format_version=1\n[[package]]\nid="superbomberman5.ptbr"\nversion="0.1.0"\n[[feature]]\npackage_id="superbomberman5.ptbr"\nid="menus_ptbr"\nenabled='+str(enabled).lower()+'\n')
    (folder/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=1\n[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    (folder/'route.input').write_text(route());env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/'run.log').open('w') as log:
        result=subprocess.run([str(folder/'game.exe'),'--no-launcher','--config','headless.ini','--script','route.input',str(args.rom)],cwd=folder,env=env,stdout=log,stderr=log,timeout=360)
    assert result.returncode==0,(folder,result.returncode)
    log=(folder/'run.log').read_text().lower()
    assert not any(b in log for b in ['interp_cap','unresolved-abandon','architectural brk','fatal']),folder
    return folder

def compare(on,off):
    labels=json.loads((ROOT/'translations/ptbr/labels.json').read_text(encoding='utf8'))['labels'];seen=set();count=0
    for shot in on.glob('*.fb.bgrx'):
        tag=shot.name.removesuffix('.fb.bgrx');count+=1
        for suffix in ['vram.bin','oam.bin','cgram.bin','wram.bin','sram.bin']:
            assert (on/f'{tag}.{suffix}').read_bytes()==(off/f'{tag}.{suffix}').read_bytes(),(tag,suffix)
        v=struct.unpack('<32768H',(on/f'{tag}.vram.bin').read_bytes())
        for l in labels:
            a=0x3000+l['layer']*0x800+l['row']*32+l['column'];ctx=l.get('context',[0,0,0])
            if [v[a]&1023,v[a+1]&1023]==l['guard'] and (not ctx[2] or v[0x3000+l['layer']*0x800+ctx[0]*32+ctx[1]]&1023==ctx[2]):seen.add(l['id'])
        pixels=shot.read_bytes()
        if any(pixels[i:i+3]!=b'\0\0\0' for i in range(0,len(pixels),4)):
            assert pixels!=(off/shot.name).read_bytes(),tag
    required={'normal_game','options_header','select_file','normal_mode','password_heading','sound_test_header','character_select','rule_settings','cpu_level'}
    assert required<=seen,('missing contexts',required-seen)
    print(f'{count} frames; {len(seen)} translated label contexts; guest VRAM/OAM/CGRAM/WRAM/SRAM identical with translation off: passed')

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--rom',type=Path,required=True);parser.add_argument('--out',type=Path,default=ROOT/'.bootstrap/ptbr-typography-validation');args=parser.parse_args();args.exe=args.exe.resolve();args.rom=args.rom.resolve();args.out=args.out.resolve();args.out.mkdir(exist_ok=True,parents=True)
    with ThreadPoolExecutor(max_workers=2) as pool:
        on,off=list(pool.map(lambda enabled:run(args,enabled),[True,False]))
    compare(on,off);print(on,off)
if __name__=='__main__':main()
