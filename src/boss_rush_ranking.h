/* Boss Rush ranking: the five best times for each player count (1-5),
 * kept in one Discord message that the game edits through a webhook.
 *
 * This file is the pure part (no I/O): entries, merging, the Discord JSON
 * payloads and reading the board back from Discord's JSON. The network side
 * is boss_rush_net.c, the in-game screens boss_rush_results.c. */
#ifndef SB5_BOSS_RUSH_RANKING_H
#define SB5_BOSS_RUSH_RANKING_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { BR_CATEGORIES = 5, BR_TOP = 5, BR_SEATS = 5, BR_NAME_BYTES = 16 };

typedef struct BrPlayer {
    char name[BR_NAME_BYTES + 1]; /* UTF-8, cut on a character boundary */
    uint8_t choice;               /* picker choice 1-19 (0 = unknown) */
} BrPlayer;

typedef struct BrEntry {
    uint32_t frames;  /* run time in frames (60 per second) */
    char date[11];    /* YYYY-MM-DD (UTC) */
    uint8_t online;   /* 1 = netplay room, 0 = one machine */
    uint8_t players;  /* 1-5 */
    BrPlayer player[BR_SEATS];
} BrEntry;

typedef struct BrBoard {
    uint8_t count[BR_CATEGORIES];
    BrEntry entry[BR_CATEGORIES][BR_TOP];
} BrBoard;

/* Character name of a picker choice (English, as the game's mods name them). */
const char *BrCharacterName(unsigned choice);

/* Copy `src` into a player name: control characters dropped, at most
 * BR_NAME_BYTES bytes without cutting a UTF-8 sequence. Empty -> "?". */
void BrSetName(char out[BR_NAME_BYTES + 1], const char *src);

/* Position (1-5) the entry would take in its category, 0 when it does not
 * enter the top 5. Equal times keep the earlier entry ahead. */
unsigned BrRankOf(const BrBoard *b, const BrEntry *e);
/* Insert into category e->players; returns the position (1-5) or 0. */
unsigned BrInsert(BrBoard *b, const BrEntry *e);

/* Discord payloads (UTF-8 JSON). The board message: one embed per category;
 * each embed's footer carries the machine-readable data ("sb5br1 c=N ...")
 * the game reads back. The notification: a new top-5 time and its category.
 * Return the length written, 0 if `cap` is too small. */
size_t BrBoardPayload(const BrBoard *b, char *out, size_t cap);
size_t BrNoticePayload(const BrBoard *b, unsigned category, unsigned rank, char *out, size_t cap);

/* Read the board from a Discord message JSON (GET .../messages/<id>).
 * Categories without data stay empty. Returns false if no category was
 * found at all (not a board message). */
bool BrParseBoard(const char *json, BrBoard *b);

/* "MM:SS.CC" from frames (as the in-game timer). */
void BrFormatTime(uint32_t frames, char *out, size_t cap);
#endif
