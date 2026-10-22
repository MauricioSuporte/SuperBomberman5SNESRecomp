"""Announce a published release in the Discord #patch-notes channel.

The webhook URL is a credential: it is read only from the environment
variable DISCORD_WEBHOOK_URL (a GitHub Actions secret), never from a tracked
file or an argument, and it is never printed.
"""
from pathlib import Path
import argparse
import json
import os
import sys
import urllib.request

EMBED_DESCRIPTION_LIMIT = 4096
EMBED_TOTAL_LIMIT = 6000
COLOR = 0x7C4DFF


def split_sections(notes):
    """Return [(heading, body)] for the top-level `## ` sections of the notes."""
    sections, heading, lines = [], None, []
    for line in notes.splitlines():
        if line.startswith('## '):
            if heading is not None or any(l.strip() for l in lines):
                sections.append((heading, '\n'.join(lines).strip()))
            heading, lines = line[3:].strip(), []
        elif line.startswith('# ') and heading is None and not lines:
            continue
        else:
            lines.append(line)
    if heading is not None or any(l.strip() for l in lines):
        sections.append((heading, '\n'.join(lines).strip()))
    return [(h, b) for h, b in sections if b]


def clip(text, limit, url):
    if len(text) <= limit:
        return text
    tail = f'\n\n… [full notes]({url})'
    cut = text[:limit - len(tail)]
    cut = cut[:cut.rfind('\n')] if '\n' in cut else cut
    return cut.rstrip() + tail


def build_payload(version, notes, url):
    title = f'Super Bomberman 5 Recompiled v{version}'
    content = f'**{title}** — {url}'
    embeds, used = [], 0
    for heading, body in split_sections(notes):
        name = heading or 'Release notes'
        budget = min(EMBED_DESCRIPTION_LIMIT, EMBED_TOTAL_LIMIT - used - len(name) - 16)
        if budget < 200:
            break
        description = clip(body, budget, url)
        embeds.append({'title': name, 'url': url, 'description': description, 'color': COLOR})
        used += len(name) + len(description)
        if len(embeds) == 10:
            break
    return {'content': content, 'embeds': embeds, 'allowed_mentions': {'parse': []}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--notes', type=Path, required=True)
    parser.add_argument('--url', required=True, help='Public release page URL.')
    parser.add_argument('--dry-run', action='store_true', help='Print the payload instead of sending it.')
    args = parser.parse_args()

    payload = build_payload(args.version, args.notes.read_text(encoding='utf-8'), args.url)
    if args.dry_run:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 0
    webhook = os.environ.get('DISCORD_WEBHOOK_URL', '').strip()
    if not webhook:
        print('DISCORD_WEBHOOK_URL is not set; skipping the Discord announcement.')
        return 0
    request = urllib.request.Request(
        webhook + ('&' if '?' in webhook else '?') + 'wait=true',
        data=json.dumps(payload, ensure_ascii=False).encode('utf-8'),
        headers={'Content-Type': 'application/json; charset=utf-8',
                 'User-Agent': 'SuperBomberman5SNESRecomp-release (github.com/MauricioSuporte)'},
        method='POST')
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            print(f'Discord announcement posted (HTTP {response.status}).')
    except urllib.error.HTTPError as error:
        print(f'Discord announcement failed: HTTP {error.code} {error.read()[:300]!r}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
