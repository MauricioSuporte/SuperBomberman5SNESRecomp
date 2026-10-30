/* Boss Rush ranking: entries, merging, Discord payloads and reading them back. */
#include "../src/boss_rush_ranking.c"
#include <assert.h>

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

static BrEntry make(uint32_t frames, unsigned players, const char *first, unsigned choice) {
    BrEntry e;
    memset(&e, 0, sizeof(e));
    e.frames = frames; e.players = (uint8_t)players; e.online = players > 1;
    strcpy(e.date, "2026-10-29");
    for (unsigned k = 0; k < players && k < BR_SEATS; ++k) {  /* a bad count still has five seats */
        char n[32];
        snprintf(n, sizeof(n), "%s%u", first, k + 1);
        BrSetName(e.player[k].name, k ? n : first);
        e.player[k].choice = (uint8_t)(choice + k);
    }
    return e;
}

int main(void) {
    char name[BR_NAME_BYTES + 1];
    /* Names: trimmed, control bytes dropped, cut on a UTF-8 boundary. */
    BrSetName(name, "  Jo\xc3\xa3o\n ");
    CHECK(!strcmp(name, "Jo\xc3\xa3o"));
    BrSetName(name, "");
    CHECK(!strcmp(name, "?"));
    BrSetName(name, "ABCDEFGHIJKLMNO\xc3\xa3");        /* 15 + 2 bytes: the 2-byte char does not fit */
    CHECK(!strcmp(name, "ABCDEFGHIJKLMNO"));
    BrSetName(name, "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86\xe3\x81\x88\xe3\x81\x8a\xe3\x81\x8b"); /* 6 x 3 bytes */
    CHECK(strlen(name) == 15);

    char t[16];
    BrFormatTime(0, t, sizeof(t)); CHECK(!strcmp(t, "00:00.00"));
    BrFormatTime(60 * 75 + 30, t, sizeof(t)); CHECK(!strcmp(t, "01:15.50"));

    /* Merging: ascending, ties keep the earlier, five per category. */
    BrBoard b;
    memset(&b, 0, sizeof(b));
    BrEntry e = make(6000, 1, "A", 15);
    CHECK(BrInsert(&b, &e) == 1);
    e = make(5000, 1, "B", 1); CHECK(BrInsert(&b, &e) == 1);
    e = make(6000, 1, "C", 2); CHECK(BrInsert(&b, &e) == 3);       /* tie: after A */
    e = make(7000, 1, "D", 3); CHECK(BrInsert(&b, &e) == 4);
    e = make(8000, 1, "E", 4); CHECK(BrInsert(&b, &e) == 5);
    e = make(9000, 1, "F", 5); CHECK(BrRankOf(&b, &e) == 0 && BrInsert(&b, &e) == 0);
    e = make(100, 1, "G", 6); CHECK(BrInsert(&b, &e) == 1);
    CHECK(b.count[0] == 5 && !strcmp(b.entry[0][0].player[0].name, "G") && b.entry[0][4].frames == 7000);
    e = make(0, 1, "Z", 1); CHECK(BrRankOf(&b, &e) == 0);          /* no time, no rank */
    e = make(100, 6, "Z", 1); CHECK(BrRankOf(&b, &e) == 0);        /* bad category */
    /* Other categories are separate. */
    e = make(12345, 3, "Ana \"o,k;~|%\\", 11); CHECK(BrInsert(&b, &e) == 1);
    e = make(9999, 5, "\xe5\xa4\xaa\xe9\x83\x8e", 13); CHECK(BrInsert(&b, &e) == 1);

    /* Board payload -> read back (the payload has the same embeds/footer
     * structure Discord returns). */
    static char payload[1 << 15];
    size_t n = BrBoardPayload(&b, payload, sizeof(payload));
    CHECK(n > 0 && strstr(payload, "\"embeds\":[") && strstr(payload, "sb5br1 c=5"));
    CHECK(strstr(payload, "Jet") != NULL);                         /* character names in the text */
    CHECK(strstr(payload, "\\\"o") != NULL);                        /* quote escaped for JSON */
    BrBoard back;
    CHECK(BrParseBoard(payload, &back));
    CHECK(back.count[0] == 5 && back.count[1] == 0 && back.count[2] == 1 && back.count[4] == 1);
    CHECK(!memcmp(&back.entry[0][0], &b.entry[0][0], sizeof(BrEntry)));
    CHECK(!strcmp(back.entry[2][0].player[0].name, "Ana \"o,k;~|%\\"));
    CHECK(back.entry[2][0].player[2].choice == 13 && back.entry[2][0].online == 1);
    CHECK(!strcmp(back.entry[4][0].player[0].name, "\xe5\xa4\xaa\xe9\x83\x8e"));
    CHECK(!strcmp(back.entry[4][0].date, "2026-10-29"));

    /* Discord may send non-ASCII as \u escapes (and surrogate pairs). */
    const char *escaped = "{\"embeds\":[{\"footer\":{\"text\":\"sb5br1 c=1 600,2026-10-30,0,Jo\\u00e3o~15 900,-,0,\\ud83d\\ude00~2\"}}]}";
    CHECK(BrParseBoard(escaped, &back));
    CHECK(back.count[0] == 2 && !strcmp(back.entry[0][0].player[0].name, "Jo\xc3\xa3o"));
    CHECK(back.entry[0][1].frames == 900 && !strcmp(back.entry[0][1].player[0].name, "\xf0\x9f\x98\x80"));
    /* Wrong player count for the category, junk: skipped. */
    CHECK(BrParseBoard("{\"text\":\"sb5br1 c=2 600,-,0,A~1 abc 700,-,1,A~1;B~2\"}", &back));
    CHECK(back.count[1] == 1 && back.entry[1][0].frames == 700);
    CHECK(!BrParseBoard("{\"content\":\"hello\"}", &back));
    /* An empty board message (never written): every category empty. */
    memset(&b, 0, sizeof(b));
    n = BrBoardPayload(&b, payload, sizeof(payload));
    CHECK(n > 0 && BrParseBoard(payload, &back) && !back.count[0] && !back.count[4]);

    /* Notification: the category with the new line in bold. */
    e = make(4321, 2, "Mau", 15); BrInsert(&b, &e);
    n = BrNoticePayload(&b, 2, 1, payload, sizeof(payload));
    CHECK(n > 0 && strstr(payload, "**1. `01:12.01`**") && !strstr(payload, "sb5br1"));
    CHECK(BrNoticePayload(&b, 2, 2, payload, sizeof(payload)) == 0);  /* no such rank */
    CHECK(BrBoardPayload(&b, payload, 64) == 0);                     /* too small: refused, not cut */

    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    puts("boss rush ranking tests passed");
    return 0;
}
