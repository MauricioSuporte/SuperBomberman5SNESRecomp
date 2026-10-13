"""Compile authored translation metadata and create a data-only menu mod."""
import argparse
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--header', type=Path)
parser.add_argument('--mod-out', type=Path)
args = parser.parse_args()
data = json.loads((ROOT / 'translations/ptbr/labels.json').read_text(encoding='utf-8'))
allowed = set('ABCDEFGHIJKLMNOPQRSTUVWXYZ ÇÃÕÉ')
lines = ['/* Generated from authored labels.json; no ROM is read. */',
         'static const PtbrLabel kLabels[] = {']
for label in data['labels']:
    text = label['text']
    if set(text) - allowed or len(text)*6-1 > label['width']*8:
        raise SystemExit('Unsupported glyph or overflowing label: ' + label['id'])
    if (label['layer'] not in (0,2) or label['height'] not in (8,16)
        or not 1 <= label['width'] <= 17 or label['column']+label['width'] > 32
        or not 0 <= label['row'] <= 28-label['height']//8
        or len(label['guard']) != 2 or any(not 0 <= v < 1024 for v in label['guard'])):
        raise SystemExit('Invalid label bounds: ' + label['id'])
    ink = 4 if label['layer']==0 else 3 if label['id']=='press_start' else 2
    fields = [label['layer'], label['row'], label['column'], label['width'], label['height'], *label['guard'], ink]
    lines.append('  {' + ', '.join(str(v) for v in fields) + ', ' + json.dumps(text, ensure_ascii=False) + '},')
lines.append('};')
for layer, capacity in ((0,128),(2,192)):
    if sum(l['width']*(l['height']//8) for l in data['labels'] if l['layer']==layer) > capacity:
        raise SystemExit('Overlay tile budget exceeded.')
if args.header:
    args.header.parent.mkdir(parents=True, exist_ok=True)
    args.header.write_text('\n'.join(lines)+'\n', encoding='utf-8')
if args.mod_out:
    args.mod_out.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.mod_out, 'w', zipfile.ZIP_DEFLATED) as out:
        out.write(ROOT / 'mods/preloaded/packages/superbomberman5.ptbr/0.1.0/manifest.toml', 'manifest.toml')
        out.write(ROOT / 'translations/ptbr/labels.json', 'translations/labels.json')
        out.write(ROOT / 'LICENSE', 'LICENSE')
    print('Data-only mod: ' + str(args.mod_out))
