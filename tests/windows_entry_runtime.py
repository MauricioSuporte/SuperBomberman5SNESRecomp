"""Windows entry integration check with a synthetic, ROM-free child process."""
from pathlib import Path
import subprocess, shutil, tempfile, argparse
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--entry",type=Path,required=True)
p.add_argument("--cc",type=Path,required=True)
args=p.parse_args()
d=Path(tempfile.mkdtemp(prefix="entry espaço ",dir=r/".bootstrap"))
src=d/"child.c";src.write_text('#include <windows.h>\n#include <stdio.h>\nint wmain(int n,wchar_t **v){FILE *f=fopen("received.txt","wb");for(int i=1;i<n;i++){char b[4096];WideCharToMultiByte(CP_UTF8,0,v[i],-1,b,sizeof(b),0,0);fprintf(f,"%s\\n",b);}fclose(f);return 37;}')
subprocess.run([str(args.cc.resolve()),"-municode",str(src),"-o",str(d/"SuperBomberman5SNESRecomp.exe")],check=True)
shutil.copy2(args.entry.resolve(),d/"SuperBomberman5Recomp.exe")
def run(arguments,expected):
 p=subprocess.run([str(d/"SuperBomberman5Recomp.exe"),*arguments],cwd=r,timeout=10)
 assert p.returncode==37,p.returncode
 assert (d/"received.txt").read_text(encoding="utf8").splitlines()==expected
run(["--rom","C:/pasta com espaço/rom japonesa.sfc"],["--rom","C:/pasta com espaço/rom japonesa.sfc"])
run([],["--launcher"])
(d/"build-lle").mkdir();shutil.move(d/"SuperBomberman5SNESRecomp.exe",d/"build-lle/SuperBomberman5SNESRecomp.exe")
run(["--help"],["--help"])
# An existing setup host must not shadow the locally generated game.
other=d/"setup.c";other.write_text(src.read_text().replace("return 37;","return 19;"))
subprocess.run([str(args.cc.resolve()),"-municode",str(other),"-o",str(d/"SuperBomberman5SNESRecomp.exe")],check=True)
run(["--help"],["--help"])
(d/"build").mkdir()
shutil.move(d/"build-lle/SuperBomberman5SNESRecomp.exe",d/"build/SuperBomberman5SNESRecomp.exe")
run(["--help"],["--help"])
print("Native entry: setup/build-lle/build lookup and precedence, Unicode/spaces, working directory, default launcher and exit status passed.")
