#ifndef SB5_ROM_SIGNATURE_H
#define SB5_ROM_SIGNATURE_H
/* Recognises a Super Bomberman 5 image by its contents, not by a fixed digest.
 * tools/check-rom.py is the same check for the generation step. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Sb5RomSignature {
    bool is_game;      /* title, HiROM map, size and checksum pair all fit */
    bool checksum_ok;  /* stored checksum equals the data: false for patched images */
} Sb5RomSignature;

/* `image` has any copier header already removed. Returns sig->is_game. */
bool Sb5RomSignatureCheck(const uint8_t *image, size_t len, Sb5RomSignature *sig);
#endif
