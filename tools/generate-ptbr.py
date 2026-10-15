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
allowed = set('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ÇÃÕÉÁÍÓÚÂÊÔ!?%')
offsets = {}
slots = {}
slot_sizes = {}
for label in data['labels']:
    bank = 2 if label['layer']==2 else 0
    slot = (label['screen'], bank, label['row'], label['column'])
    slot_sizes[slot] = max(slot_sizes.get(slot,0), label['width']*(label['height']//8))
shared_title_tiles = sum(l['width']*(l['height']//8) for l in data['labels'] if l['id']=='press_start')
lines = ['/* Generated from authored labels.json; no ROM is read. */',
         'static const PtbrLabel kLabels[] = {']
for label in data['labels']:
    text = label['text']
    if not text or set(text) - allowed or len(text)*(8 if label['style']=='title' else 18 if label['style']=='poster' else 6 if label['style']=='compact' else 7)-1 > label['width']*8:
        raise SystemExit('Unsupported glyph or overflowing label: ' + label['id'])
    if (label['layer'] not in (0,1,2) or label['height'] not in (16,32)
        or not 1 <= label['width'] <= 28 or not 0 <= label['column'] <= 32-label['width']
        or not 0 <= label['row'] <= 28-label['height']//8
        or len(label['guard']) != 2 or any(not 0 <= v < 1024 for v in label['guard'])):
        raise SystemExit('Invalid label bounds: ' + label['id'])
    context = label.get('context', [0,0,0])
    if (len(context)!=3 or not 0 <= context[0] < 28 or not 0 <= context[1] < 32
        or not 0 <= context[2] < 1024
        or label['style'] not in ('body','title','banner','poster','compact')
        or (label['style']=='poster') != (label['height']==32)):
        raise SystemExit('Invalid label style/context: ' + label['id'])
    ink = label.get('ink', 4 if label['layer']!=2 else 3 if label['id']=='press_start' else 2)
    if not 1 <= ink <= (3 if label['layer']==2 else 15):
        raise SystemExit('Invalid ink: ' + label['id'])
    key = (label['screen'], 2 if label['layer']==2 else 0)
    slot = (*key, label['row'], label['column'])
    if slot not in slots:
        offset = offsets.get(key, shared_title_tiles if key[1]==2 and key[0]!='title' else 0)
        slots[slot] = (0x220 if label['layer']!=2 else 0x140) + offset
        offsets[key] = offset + slot_sizes[slot]
    tile = slots[slot]
    style = {'body':0, 'title':1, 'banner':2, 'poster':3, 'compact':5}[label['style']]
    fields = [label['layer'], label['row'], label['column'], label['width'], label['height'], *label['guard'], ink, tile, style, *context]
    lines.append('  {' + ', '.join(str(v) for v in fields) + ', ' + json.dumps(text, ensure_ascii=False) + '},')
lines.append('};')
for key, text in data['completion_text'].items():
    if not key.isidentifier() or not text or set(text)-allowed or len(text)>24:
        raise SystemExit('Invalid completion text: ' + key)
    lines.append('static const char kCompletion_' + key + '[] = ' + json.dumps(text, ensure_ascii=False) + ';')
lines.append('static const PtbrEnding kEndings[] = {')
endings = data.get('endings', [])
if len(endings) != 33 or [e['id'] for e in endings] != list(range(33)):
    raise SystemExit('Expected all 33 ordered completion results')
for ending in endings:
    if (len(ending['guard']) != 24 or any(not 0 <= v < 1024 for v in ending['guard'])
        or not 1 <= len(ending['tips']) <= 3
        or any(not t or set(t)-allowed or len(t)>24 for t in [ending['title'], *ending['tips']])):
        raise SystemExit('Invalid completion result: ' + str(ending['id']))
    tips = ending['tips'] + [''] * (3-len(ending['tips']))
    lines.append('  {{' + ','.join(map(str, ending['guard'])) + '}, ' +
                 json.dumps(ending['title'], ensure_ascii=False) + ', {' +
                 ','.join(json.dumps(t, ensure_ascii=False) for t in tips) + '}},')
lines.append('};')
lines.append('static const PtbrObjectLabel kObjectLabels[] = {')
for label in data.get('objects',[]):
    if (not label['text'] or set(label['text'])-allowed or len(label['text'])*7-1 > label['count']*16
        or not 1 <= label['count'] <= 14 or not 0 <= label['slot'] <= 128-label['count']
        or not 0 <= label['x'] <= 256-label['count']*16 or not 0 <= label['y'] <= 208
        or not 0 <= label['guard'] < 256 or label['guard']%16
        or label['guard']+((label['count']-1)//8)*32+((label['count']-1)%8)*2+17 > 255):
        raise SystemExit('Invalid object label: ' + label['id'])
    fields=[label['slot'],label['count'],label['x'],label['y'],label['guard']]
    lines.append('  {' + ', '.join(str(v) for v in fields) + ', ' + json.dumps(label['text'],ensure_ascii=False) + '},')
lines.append('};')
for (screen, layer), count in offsets.items():
    if count > (224 if layer!=2 else 192):
        raise SystemExit('Overlay tile budget exceeded: ' + screen)
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
