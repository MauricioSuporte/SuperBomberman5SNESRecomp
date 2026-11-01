#!/usr/bin/env python3
"""Print a ROM's crc32 and sha256 if it reads as Super Bomberman 5.

The same content check as src/rom_signature.c, for the generation step. Exits 1
with the reason otherwise.
"""
from __future__ import annotations

import hashlib
import pathlib
import sys
import zlib

TITLE = b'Hu SUPER BOMBERMAN 5'
HEADER = 0xffc0
MIN_SIZE = 0x200000


def image_of(raw: bytes) -> bytes:
    return raw[512:] if len(raw) % 1024 == 512 else raw


def checksum(image: bytes) -> int:
    base = 1
    while base * 2 <= len(image):
        base *= 2
    total = sum(image[:base])
    if base < len(image):
        tail = image[base:]
        total += sum(tail) * (base // len(tail)) + sum(tail[:base % len(tail)])
    return total & 0xffff


def problem(image: bytes) -> str | None:
    """Why this image isn't the game, or None."""
    if len(image) < MIN_SIZE:
        return f'too small for this game ({len(image)} bytes)'
    header = image[HEADER:HEADER + 0x40]
    if header[:len(TITLE)] != TITLE:
        return f'internal title is {bytes(header[:21])!r}'
    if header[0x15] & 0x0f != 1:
        return 'not a HiROM image'
    if (header[0x1c] | header[0x1d] << 8) ^ (header[0x1e] | header[0x1f] << 8) != 0xffff:
        return 'the header checksum pair is invalid'
    return None


def main() -> int:
    if len(sys.argv) not in (2, 3):
        print(f'usage: {sys.argv[0]} <rom> [write the bare image here]', file=sys.stderr)
        return 2
    image = image_of(pathlib.Path(sys.argv[1]).read_bytes())
    reason = problem(image)
    if reason:
        print(f'{sys.argv[1]}: not Super Bomberman 5: {reason}', file=sys.stderr)
        return 1
    if checksum(image) != (image[HEADER + 0x1e] | image[HEADER + 0x1f] << 8):
        print(f'{sys.argv[1]}: warning: checksum does not match: patched or a bad dump', file=sys.stderr)
    if len(sys.argv) == 3:
        pathlib.Path(sys.argv[2]).write_bytes(image)
    print(f'{zlib.crc32(image):08x} {hashlib.sha256(image).hexdigest()}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
