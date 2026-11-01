#include "rom_signature.h"
#include <string.h>

enum { HEADER = 0xffc0, MIN_SIZE = 0x200000 };
static const char kTitle[] = "Hu SUPER BOMBERMAN 5"; /* "5G" is Gold Cartridge */

/* 16-bit byte sum; a size that is not a power of two is mirrored upward. */
static unsigned checksum(const uint8_t *d, size_t n) {
    size_t base = 1;
    uint32_t sum = 0;
    while (base * 2 <= n) base *= 2;
    for (size_t i = 0; i < base; ++i) sum += d[i];
    if (base < n) {
        const size_t rest = n - base;
        for (size_t i = 0; i < base; ++i) sum += d[base + i % rest];
    }
    return sum & 0xffff;
}

bool Sb5RomSignatureCheck(const uint8_t *image, size_t len, Sb5RomSignature *sig) {
    Sb5RomSignature local = {false, false};
    if (image && len >= MIN_SIZE) {
        const uint8_t *h = image + HEADER;
        const unsigned complement = h[0x1c] | h[0x1d] << 8, stored = h[0x1e] | h[0x1f] << 8;
        local.is_game = memcmp(h, kTitle, sizeof(kTitle) - 1) == 0 &&
                        (h[0x15] & 0x0f) == 1 && (complement ^ stored) == 0xffff;
        local.checksum_ok = local.is_game && checksum(image, len) == stored;
    }
    if (sig) *sig = local;
    return local.is_game;
}
