"""Validate all Battle names with native glyphs, including every sampled transition frame."""
from pathlib import Path
import argparse
import json
import os
import shutil
import struct
import subprocess
import tempfile
from pause_menu_runtime import BATTLE_ENTER
ROOT=Path(__file__).resolve().parents[1]

def glyph(vram,c):
    accent={"ç":("c",[(3,14),(2,15)]),"ã":("a",[(2,1),(3,0),(4,1),(5,0)]),
            "á":("a",[(4,1),(3,2)]),"é":("e",[(4,1),(3,2)]),"í":("i",[(4,1),(3,2)]),
            "ó":("o",[(4,1),(3,2)]),"ú":("u",[(4,1),(3,2)])}
    base,marks=accent.get(c,(c,[]));out=[[0]*8 for _ in range(16)]
    if base==' ':return out,6
    tile=(2+2*(ord(base)-65) if 'A'<=base<='Z' else 0x36+2*(ord(base)-97) if 'a'<=base<='z' else {'!':0x6e,'?':0x70,'-':0x7c}[base])
    for y in range(16):
        for x in range(8):
            at=0x6000+(tile+y//8)*16+y%8
            out[y][x]=sum(((vram[at+(p//2)*8]>>(7-x+(p%2)*8))&1)<<p for p in range(4))
    for x,y in marks:
        for dy in [-1,0,1]:
            for dx in [-1,0,1]:
                xx,yy=x+dx,y+dy
                if 0<=xx<8 and 0<=yy<16 and not out[yy][xx]:out[yy][xx]=1
    for x,y in marks:out[y][x]=4
    width=4 if base in 'ilI' else 6 if base in 'rtf' else 3 if base=='!' else 6 if base=='-' else 7
    return out,width

def expected(vram,text):
    canvas=[[0]*208 for _ in range(16)];left=0
    for c in text:
        source,width=glyph(vram,c)
        for y in range(16):
            for x in range(8):
                if not canvas[y][left+x]:canvas[y][left+x]=source[y][x]
        left+=width
    return canvas,left

def check(folder,tags):
    labels=json.loads((ROOT/'translations/ptbr/labels.json').read_text(encoding='utf-8'))['battle_arenas']
    seen=set();last=0
    for tag in tags:
        o=struct.unpack('<256H',(folder/f'{tag}.oam.bin').read_bytes()[:512])
        for slot in [3,12,13]:
            if o[slot*2]!=171*256+138:continue
            tile=o[slot*2+1]&511
            for a in range(1,14):
                n=10+2*(a-1)
                if tile==0xc0+(n//16)*32+n%16:last=a;break
        assert last,(folder,tag,'no native number')
        vram=struct.unpack('<32768H',(folder/f'{tag}.vram.bin').read_bytes())
        cg=struct.unpack('<256H',(folder/f'{tag}.cgram.bin').read_bytes())
        fb=(folder/f'{tag}.fb.bgrx').read_bytes();canvas,width=expected(vram,labels[last-1]);left=(256-width)//2
        def bgr(c):
            return bytes(((v<<3)|(v>>2)) for v in [(c>>10)&31,(c>>5)&31,c&31])
        # Check actual output against the native glyph ink. This catches both
        # missing translation and a one-frame native-language flash.
        color=bgr(cg[148]);want=set()
        for y in range(16):
            for x in range(208):
                if canvas[y][x]==4:want.add((left+x,195+y))
        actual={(x,y) for y in range(195,211) for x in range(16,240) if fb[(y*256+x)*4:(y*256+x)*4+3]==color}
        assert want==actual,(folder,tag,last,'native-font translated ink mismatch',len(want),len(actual),len(want^actual))
        seen.add(last)
    return seen

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--rom',type=Path,required=True)
    parser.add_argument('--without-gold',action='store_true')
    parser.add_argument('--out',type=Path,default=ROOT/'.bootstrap/battle-translation-validation')
    args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    folder=Path(tempfile.mkdtemp(prefix='stock-' if args.without_gold else 'gold-',dir=args.out.resolve()))
    shutil.copy2(args.exe,folder/'game.exe');shutil.copytree(ROOT/'mods',folder/'mods')
    state='format_version=1\n'
    for mod,feature in [('ptbr','menus_ptbr')]+([] if args.without_gold else [('gold-battle','gold_battle')]):
        state+=f'[[package]]\nid="superbomberman5.{mod}"\nversion="0.1.0"\n[[feature]]\npackage_id="superbomberman5.{mod}"\nid="{feature}"\nenabled=true\n'
    (folder/'mods/preloaded/state.toml').write_text(state)
    (folder/'headless.ini').write_text('[Graphics]\nOutputMethod=SDL-Software\nNewRenderer=1\n[Sound]\nEnableAudio=1\n[GamepadMap]\nEnableGamepad1=0\nEnableGamepad2=0\n')
    route=BATTLE_ENTER.split('dump selection')[0];tags=['arena1'];route+='dump arena1\n'
    count=10 if args.without_gold else 13
    for arena in range(2,count+1):
        route+='press right 2\n'
        for frame in range(8):
            tag=f'move{arena}_{frame}';tags.append(tag);route+=f'dump {tag}\nwait 1\n'
        route+=f'wait 20\ndump arena{arena}\n';tags.append(f'arena{arena}')
    route+='quit\n';(folder/'route.input').write_text(route)
    env=os.environ.copy();env.update(SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SNESRECOMP_DUMP_DIR=str(folder))
    with (folder/'run.log').open('w') as log:
        result=subprocess.run([str(folder/'game.exe'),'--no-launcher','--config','headless.ini','--script','route.input',str(args.rom.resolve())],cwd=folder,env=env,stdout=log,stderr=log,timeout=240)
    assert result.returncode==0,(folder,result.returncode)
    seen=check(folder,tags);assert seen==set(range(1,count+1)),(folder,seen)
    log=(folder/'run.log').read_text().lower()
    assert not any(b in log for b in ['interp_cap','unresolved-abandon','architectural brk','fatal'])
    print(f'{len(tags)} rendered frames, arenas 1-{count}, native-font PT-BR with no source-language flashes: passed: {folder}')
if __name__=='__main__':main()
