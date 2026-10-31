/* Boss Rush ranking screens (presentation; no WRAM writes).
 *
 *  - Player-count menu of the title's Boss Rush: the five best times of the
 *    category under the cursor, top right, once the ranking has been read.
 *  - End of the rush (Terrorin beaten): the final time and the category's
 *    ranking. Online (netplay room) the host sends the time with every
 *    player's room name and character. Offline, when the ranking could be
 *    read and the time enters the top 5, each player types a name and then
 *    chooses whether to send it.
 *
 * Without internet, or without the ranking configuration, nothing here shows
 * up beyond the time, and the game never waits: every request runs in the
 * background (boss_rush_net.c) and is only polled here. Input is taken from
 * the game only while a name or the send question is on screen, and never in
 * a netplay room (the peers must keep running the same machine). */
#include "boss_rush_results.h"
#include "boss_rush_mod.h"
#include "boss_rush_net.h"
#include "boss_rush_ranking.h"
#include "common_rtl.h"
#include "ptbr_mod.h"
#include "snes_osd.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined(SNESRECOMP_NET)
#include "snes_netplay.h"
#endif
#if defined(SNES_HOST_HAS_INGAME_CHAT)
#include "host_chat.h"
#endif

enum { PAD_B = 0x0001, PAD_START = 0x0008, PAD_UP = 0x0010, PAD_DOWN = 0x0020,
       PAD_LEFT = 0x0040, PAD_RIGHT = 0x0080, PAD_A = 0x0100 };
enum { SEAT_CHOICE = 0x1f828 };
enum { RES_NONE, RES_WAIT, RES_NAME, RES_ASK, RES_SENDING, RES_DONE };
enum { BR_NAME_MAX = 12, GRID_COLS = 10, GRID_ROWS = 5, WAIT_FRAMES = 60 * 20, SHOW_FRAMES = 60 * 12 };
static const char kGrid[4][GRID_COLS + 1] = {"ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ0123", "456789-.!?"};

static bool was_finished, was_active;
static int state = RES_NONE;
static BrEntry entry;
static bool online, host;
static unsigned seat_index;          /* name being typed */
static char name[BR_NAME_MAX + 1];
static int gx, gy;                   /* grid cursor */
static bool ask_yes = true;
static unsigned timer, rank;
static int outcome;                  /* BR_SUBMIT_* once known */
static uint16_t prev_pads, held[5];
static bool masked;

static bool pt(void) { return PtbrEnabled(); }
static bool netplay(void) {
#if defined(SNESRECOMP_NET)
    return snes_netplay_active() != 0;
#else
    return false;
#endif
}
static bool netplay_host(void) {
#if defined(SNESRECOMP_NET)
    return snes_netplay_is_host() != 0;
#else
    return false;
#endif
}
static const char *seat_name(unsigned seat) {
#if defined(SNES_HOST_HAS_INGAME_CHAT)
    const char *n = host_chat_seat_name((int)seat);
    if (n && n[0]) return n;
#endif
    (void)seat;
    return NULL;
}

/* The roster, characters and names of the finished run. */
static void capture_entry(void) {
    memset(&entry, 0, sizeof(entry));
    entry.frames = BossRushFrames();
    entry.online = online;
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    if (t) strftime(entry.date, sizeof(entry.date), "%Y-%m-%d", t);
    uint8_t mask = BossRushRoster();
    if (!mask) mask = 1;
    for (unsigned seat = 0; seat < 5 && entry.players < BR_SEATS; ++seat) {
        if (!(mask >> seat & 1)) continue;
        BrPlayer *p = &entry.player[entry.players++];
        unsigned extra = g_ram[SEAT_CHOICE + seat], stock = g_ram[0x16c2 + seat] + 1u;
        p->choice = (uint8_t)(extra >= 11 && extra <= 19 ? extra : stock <= 10 ? stock : 0);
        const char *n = online ? seat_name(seat) : NULL;
        char fallback[8];
        snprintf(fallback, sizeof(fallback), "P%u", seat + 1);
        BrSetName(p->name, n ? n : fallback);
    }
}

static void begin(void) {
    online = netplay();
    host = online && netplay_host();
    capture_entry();
    timer = 0; rank = 0; outcome = -1;
    if (!BrNetConfigured()) { state = RES_DONE; return; }
    if (online) {
        if (host) { BrNetSubmit(&entry); state = RES_SENDING; }
        else { BrNetRefresh(true); state = RES_WAIT; }
        return;
    }
    BrNetRefresh(true);
    state = RES_WAIT;
}

/* ---- input (offline name entry only) ---------------------------------- */
static bool capturing(void) { return !online && (state == RES_NAME || state == RES_ASK); }

void BossRushResultsBeforeFrame(void) {
    bool active = BossRushActive();
    if (active && !was_active)
        snes_osd_push(pt() ? "Boss Rush: save states desativados ate o fim" : "Boss Rush: save states are off until it ends", 4000);
    was_active = active;
    bool finished = BossRushFinished();
    if (finished && !was_finished) begin();
    was_finished = finished;
    if (!BossRushActive() && state != RES_NONE && !finished) state = RES_NONE; /* back on the title */

    uint16_t pads = 0;
    for (int i = 0; i < 5; ++i) pads |= RtlGetPadState(i);
    uint16_t pressed = (uint16_t)(pads & ~prev_pads);
    prev_pads = pads;
    masked = false;
    if (capturing()) {
        /* The game (and the other mods) see no buttons while typing. */
        for (int i = 0; i < 5; ++i) { held[i] = RtlGetPadState(i); RtlSetPadState(i, 0); }
        masked = true;
    }

    switch (state) {
    case RES_WAIT: {
        BrBoard b;
        int s = BrNetBoard(&b);
        if (s == 1) {
            rank = BrRankOf(&b, &entry);
            if (online || !rank) state = RES_DONE;
            else { state = RES_NAME; seat_index = 0; name[0] = 0; gx = gy = 0; }
        } else if (s == -1 || ++timer > WAIT_FRAMES) {
            state = RES_DONE; /* offline: only the time */
        }
        break;
    }
    case RES_NAME: {
        size_t len = strlen(name);
        if (pressed & PAD_UP) gy = (gy + GRID_ROWS - 1) % GRID_ROWS;
        if (pressed & PAD_DOWN) gy = (gy + 1) % GRID_ROWS;
        if (pressed & PAD_LEFT) gx = (gx + GRID_COLS - 1) % GRID_COLS;
        if (pressed & PAD_RIGHT) gx = (gx + 1) % GRID_COLS;
        bool done = false;
        if (pressed & PAD_A) {
            if (gy < 4) { if (len < BR_NAME_MAX) { name[len] = kGrid[gy][gx]; name[len + 1] = 0; } }
            else if (gx < 3) { if (len && len < BR_NAME_MAX) { name[len] = ' '; name[len + 1] = 0; } }  /* space */
            else if (gx < 7) { if (len) name[len - 1] = 0; }                                        /* delete */
            else done = true;                                                                     /* OK */
        }
        if (pressed & PAD_B) { if (len) name[len - 1] = 0; }
        if (pressed & PAD_START) done = true;
        if (done && name[0]) {
            BrSetName(entry.player[seat_index].name, name);
            if (++seat_index < entry.players) { name[0] = 0; gx = gy = 0; }
            else { state = RES_ASK; ask_yes = true; }
        }
        break;
    }
    case RES_ASK:
        if (pressed & (PAD_LEFT | PAD_RIGHT)) ask_yes = !ask_yes;
        if (pressed & PAD_B) { ask_yes = false; pressed |= PAD_A; }
        if (pressed & (PAD_A | PAD_START)) {
            if (ask_yes) { BrNetSubmit(&entry); state = RES_SENDING; }
            else { outcome = BR_SUBMIT_IDLE; state = RES_DONE; timer = 0; }
        }
        break;
    case RES_SENDING: {
        unsigned r = 0;
        int s = BrNetSubmitState(&r);
        if (s != BR_SUBMIT_BUSY && s != BR_SUBMIT_IDLE) {
            outcome = s; rank = r; state = RES_DONE; timer = 0;
            if (s == BR_SUBMIT_RANKED || s == BR_SUBMIT_NOT_TOP) BrNetRefresh(true);
            BrNetSubmitClear();
        }
        break;
    }
    case RES_DONE:
        if (++timer > SHOW_FRAMES || (!online && timer > 30 && (pressed & (PAD_A | PAD_START)))) state = RES_NONE;
        break;
    default: break;
    }
}
void BossRushResultsAfterFrame(void) {
    if (!masked) return;
    for (int i = 0; i < 5; ++i) RtlSetPadState(i, held[i]);
    masked = false;
}

/* ---- drawing ---------------------------------------------------------- */
enum { WHITE = 0xf8f8f8, YELLOW = 0xf8e040, GREY = 0xa0a0a0, GREEN = 0x60f060, RED = 0xf87060 };

static void centered(int y, const char *s, uint32_t rgb) { BossRushText(128 - BossRushTextWidth(s) / 2, y, s, rgb); }

/* "1 12:34.56 NAME+2" fitted to `width` pixels. */
static void ranking_line(char *out, size_t cap, unsigned pos, const BrEntry *e, int width) {
    char t[16], who[64];
    BrFormatTime(e->frames, t, sizeof(t));
    snprintf(who, sizeof(who), "%s", e->player[0].name);
    for (char *p = who; *p; ++p) if ((unsigned char)*p >= 0x80) *p = '?';
    if (e->players > 1) { size_t n = strlen(who); snprintf(who + n, sizeof(who) - n, "+%u", e->players - 1u); }
    snprintf(out, cap, "%u %s %s", pos, t, who);
    while (strlen(out) > 3 && BossRushTextWidth(out) > width) out[strlen(out) - 1] = 0;
}
static void draw_top(int x0, int y0, int x1, unsigned category, const BrBoard *b, unsigned highlight) {
    char line[64];
    unsigned n = b->count[category - 1];
    BossRushShade(x0, y0, x1, y0 + 14 + 10 * (int)(n ? n : 1));
    snprintf(line, sizeof(line), pt() ? "TOP 5 - %uP" : "TOP 5 - %uP", category);
    BossRushText(x0 + 4, y0 + 3, line, YELLOW);
    if (!n) BossRushText(x0 + 4, y0 + 13, pt() ? "SEM TEMPOS" : "NO TIMES YET", GREY);
    for (unsigned i = 0; i < n; ++i) {
        ranking_line(line, sizeof(line), i + 1, &b->entry[category - 1][i], x1 - x0 - 8);
        BossRushText(x0 + 4, y0 + 13 + 10 * (int)i, line, i + 1 == highlight ? GREEN : WHITE);
    }
}

static void draw_name_entry(void) {
    char line[64];
    snprintf(line, sizeof(line), pt() ? "NOME DO JOGADOR %u" : "PLAYER %u NAME", seat_index + 1);
    centered(92, line, YELLOW);
    snprintf(line, sizeof(line), "%s_", name);
    centered(104, line, WHITE);
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < GRID_COLS; ++c) {
            char ch[2] = {kGrid[r][c], 0};
            BossRushText(68 + c * 12, 118 + r * 11, ch, r == gy && c == gx ? GREEN : WHITE);
        }
    const char *opts[3] = {pt() ? "ESPACO" : "SPACE", pt() ? "APAGAR" : "DELETE", "OK"};
    int xs[3] = {64, 112, 168};
    int sel = gy == 4 ? (gx < 3 ? 0 : gx < 7 ? 1 : 2) : -1;
    for (int k = 0; k < 3; ++k) BossRushText(xs[k], 164, opts[k], k == sel ? GREEN : GREY);
}

void BossRushResultsDraw(void) {
    /* The player-count menu: top right, once the ranking has been read. */
    unsigned category = BossRushPlayerMenuCategory();
    if (category && BrNetConfigured()) {
        BrNetRefresh(false);
        BrBoard b;
        if (BrNetBoard(&b) == 1) draw_top(132, 4, 252, category, &b, 0);
        return;
    }
    if (state == RES_NONE) return;

    char line[64], t[16];
    BossRushShade(20, 28, 236, 200);
    centered(34, pt() ? "BOSS RUSH CONCLUIDO!" : "BOSS RUSH CLEAR!", YELLOW);
    BrFormatTime(entry.frames, t, sizeof(t));
    snprintf(line, sizeof(line), pt() ? "TEMPO %s - %uP" : "TIME %s - %uP", t, entry.players);
    centered(46, line, WHITE);

    switch (state) {
    case RES_WAIT:
        if (BrNetConfigured()) centered(62, pt() ? "LENDO O RANKING..." : "READING THE RANKING...", GREY);
        break;
    case RES_NAME:
        snprintf(line, sizeof(line), pt() ? "NOVO TOP 5! POSICAO %u" : "NEW TOP 5! RANK %u", rank);
        centered(62, line, GREEN);
        centered(74, pt() ? "A: LETRA  B: APAGAR  START: OK" : "A: LETTER  B: DELETE  START: OK", GREY);
        draw_name_entry();
        break;
    case RES_ASK:
        snprintf(line, sizeof(line), pt() ? "NOVO TOP 5! POSICAO %u" : "NEW TOP 5! RANK %u", rank);
        centered(62, line, GREEN);
        centered(100, pt() ? "ENVIAR AO DISCORD?" : "SEND TO DISCORD?", WHITE);
        BossRushText(88, 116, pt() ? "SIM" : "YES", ask_yes ? GREEN : GREY);
        BossRushText(148, 116, pt() ? "NAO" : "NO", ask_yes ? GREY : GREEN);
        break;
    case RES_SENDING:
        centered(62, pt() ? "ENVIANDO AO DISCORD..." : "SENDING TO DISCORD...", GREY);
        break;
    case RES_DONE: {
        int y = 62;
        if (outcome == BR_SUBMIT_RANKED) {
            snprintf(line, sizeof(line), pt() ? "ENVIADO! POSICAO %u" : "SENT! RANK %u", rank);
            centered(y, line, GREEN);
        } else if (outcome == BR_SUBMIT_NOT_TOP || (outcome < 0 && BrNetConfigured() && BrNetBoard(NULL) == 1 && !rank)) {
            centered(y, pt() ? "FORA DO TOP 5" : "NOT IN THE TOP 5", GREY);
        } else if (outcome == BR_SUBMIT_FAILED) {
            centered(y, pt() ? "NAO FOI POSSIVEL ENVIAR" : "COULD NOT SEND", RED);
        } else if (outcome < 0 && online && rank) {
            snprintf(line, sizeof(line), pt() ? "TOP 5! POSICAO %u" : "TOP 5! RANK %u", rank);
            centered(y, line, GREEN);
        }
        BrBoard b;
        if (BrNetConfigured() && BrNetBoard(&b) == 1)
            draw_top(52, 76, 204, entry.players, &b, outcome == BR_SUBMIT_RANKED ? rank : 0);
        break;
    }
    default: break;
    }
}

void BossRushResultsReset(void) {
    state = RES_NONE; was_finished = was_active = false; masked = false; prev_pads = 0;
}
