"""Build the ROM-free package for the native character color controls plugin."""
import argparse
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, default=root / 'dist/superbomberman5-selection-colors-0.1.0.snesmod')
args = parser.parse_args()
args.out.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(args.out, 'w', zipfile.ZIP_DEFLATED) as archive:
    archive.write(root / 'mods/preloaded/packages/superbomberman5.selection-colors/0.1.0/manifest.toml', 'manifest.toml')
    archive.write(root / 'LICENSE', 'LICENSE')
print(args.out)
