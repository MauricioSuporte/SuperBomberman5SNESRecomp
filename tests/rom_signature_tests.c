#include "../src/rom_signature.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SIZE = 0x200000, HEADER = 0xffc0 };

static void seal(uint8_t *rom, size_t size) {
    uint8_t *h = rom + HEADER;
    h[0x1c] = 0xff; h[0x1d] = 0xff; h[0x1e] = 0; h[0x1f] = 0;
    uint32_t sum = 0;
    for (size_t i = 0; i < size; ++i) sum += rom[i];
    sum &= 0xffff;
    h[0x1e] = sum & 255; h[0x1f] = sum >> 8;
    h[0x1c] = (~sum) & 255; h[0x1d] = (~sum >> 8) & 255;
}
static void build(uint8_t *rom, const char *title) {
    memset(rom, 0x5a, SIZE);
    memcpy(rom + HEADER, title, strlen(title));
    rom[HEADER + 0x15] = 0x31;
    seal(rom, SIZE);
}

int main(void) {
    uint8_t *rom = malloc(SIZE);
    Sb5RomSignature sig;
    if (!rom) return 99;
    build(rom, "Hu SUPER BOMBERMAN 5 ");
    if (!Sb5RomSignatureCheck(rom, SIZE, &sig) || !sig.checksum_ok) return 1;
    build(rom, "Hu SUPER BOMBERMAN 5G");
    if (!Sb5RomSignatureCheck(rom, SIZE, NULL)) return 2;
    rom[0x1000] ^= 1; /* Patched: still the game, checksum reported as wrong. */
    if (!Sb5RomSignatureCheck(rom, SIZE, &sig) || sig.checksum_ok) return 3;
    build(rom, "Hu SUPER BOMBERMAN 4 ");
    if (Sb5RomSignatureCheck(rom, SIZE, &sig) || sig.is_game) return 4;
    build(rom, "Hu SUPER BOMBERMAN 5 ");
    rom[HEADER + 0x15] = 0x30; /* LoROM */
    if (Sb5RomSignatureCheck(rom, SIZE, NULL)) return 5;
    build(rom, "Hu SUPER BOMBERMAN 5 ");
    rom[HEADER + 0x1c] ^= 1; /* Broken checksum pair. */
    if (Sb5RomSignatureCheck(rom, SIZE, NULL)) return 6;
    build(rom, "Hu SUPER BOMBERMAN 5 ");
    if (Sb5RomSignatureCheck(rom, 0x100000, NULL) || Sb5RomSignatureCheck(NULL, SIZE, NULL)) return 7;
    free(rom);
    puts("rom signature tests passed");
    return 0;
}
