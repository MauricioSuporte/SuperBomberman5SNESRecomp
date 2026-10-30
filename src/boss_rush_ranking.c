/* Boss Rush ranking: pure logic. See boss_rush_ranking.h. */
#include "boss_rush_ranking.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* Picker choices: 1-10 the cartridge's characters (10 = Subordinate Bomber
 * with its mod), 11-19 the extra characters (extra_characters_mod.h). */
static const char *const kNames[20] = {
    NULL, "Bomberman", "Bomber Woof", "Dave Bomber", "Gary Bomber", "Pirate Bomber",
    "Muscle Bomber", "Iron Mask Bomber", "Baron Bombano", "Plunder Bomber", "Subordinate Bomber",
    "Honey", "Kotetsu", "Bomberman (SB4)", "Hammer Bomber", "Jet Bomber", "Lady Bomber", "Bazooka Bomber",
    "Bomber Great", "Terrorin",
};
const char *BrCharacterName(unsigned choice) {
    return choice < 20 && kNames[choice] ? kNames[choice] : "?";
}

void BrSetName(char out[BR_NAME_BYTES + 1], const char *src) {
    size_t n = 0;
    out[0] = 0;
    for (const unsigned char *p = (const unsigned char *)(src ? src : ""); *p && n < BR_NAME_BYTES;) {
        unsigned len = *p < 0x80 ? 1 : (*p & 0xe0) == 0xc0 ? 2 : (*p & 0xf0) == 0xe0 ? 3 : (*p & 0xf8) == 0xf0 ? 4 : 0;
        if (!len) { ++p; continue; }                     /* stray continuation byte */
        if (len == 1 && (*p < 0x20 || *p == 0x7f)) { ++p; continue; }
        bool whole = true;
        for (unsigned k = 1; k < len; ++k) if ((p[k] & 0xc0) != 0x80) whole = false;
        if (!whole) { ++p; continue; }
        if (n + len > BR_NAME_BYTES) break;
        memcpy(out + n, p, len); n += len; p += len;
    }
    while (n && out[n - 1] == ' ') --n;
    size_t lead = 0;
    while (lead < n && out[lead] == ' ') ++lead;
    memmove(out, out + lead, n - lead); n -= lead;
    out[n] = 0;
    if (!n) strcpy(out, "?");
}

unsigned BrRankOf(const BrBoard *b, const BrEntry *e) {
    if (!e->players || e->players > BR_CATEGORIES || !e->frames) return 0;
    unsigned c = e->players - 1, n = b->count[c], i = 0;
    while (i < n && b->entry[c][i].frames <= e->frames) ++i;
    return i < BR_TOP ? i + 1 : 0;
}
unsigned BrInsert(BrBoard *b, const BrEntry *e) {
    unsigned rank = BrRankOf(b, e);
    if (!rank) return 0;
    unsigned c = e->players - 1, at = rank - 1, n = b->count[c];
    if (n == BR_TOP) --n;
    memmove(&b->entry[c][at + 1], &b->entry[c][at], (n - at) * sizeof(BrEntry));
    b->entry[c][at] = *e;
    b->count[c] = (uint8_t)(n + 1);
    return rank;
}

void BrFormatTime(uint32_t f, char *out, size_t cap) {
    uint32_t cs = f * 100u / 60u;
    unsigned c = cs % 100, s = (cs / 100) % 60, m = cs / 6000;
    snprintf(out, cap, "%02u:%02u.%02u", m > 99 ? 99 : m, s, c);
}

/* ---- output buffer ---------------------------------------------------- */
typedef struct Out { char *p; size_t cap, n; bool full; } Out;
static void put(Out *o, const char *s, size_t len) {
    if (o->full || o->n + len + 1 > o->cap) { o->full = true; return; }
    memcpy(o->p + o->n, s, len); o->n += len; o->p[o->n] = 0;
}
static void puts_(Out *o, const char *s) { put(o, s, strlen(s)); }
static void putf(Out *o, const char *fmt, ...) {
    char tmp[256];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof(tmp)) { o->full = true; return; }
    put(o, tmp, (size_t)n);
}
/* Text for a JSON string body (no quotes). */
static void json_text(Out *o, const char *s) {
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        char e[8];
        if (*p == '"' || *p == '\\') { e[0] = '\\'; e[1] = (char)*p; put(o, e, 2); }
        else if (*p == '\n') put(o, "\\n", 2);
        else if (*p < 0x20) { snprintf(e, sizeof(e), "\\u%04x", *p); put(o, e, 6); }
        else put(o, (const char *)p, 1);
    }
}
/* A player name inside Discord markdown, then as JSON text. */
static void md_name(Out *o, const char *name) {
    char buf[BR_NAME_BYTES * 2 + 1];
    size_t n = 0;
    for (const char *p = name; *p && n + 2 < sizeof(buf); ++p) {
        if (strchr("\\*_~`|>[]()#@:", *p)) buf[n++] = '\\';
        buf[n++] = *p;
    }
    buf[n] = 0;
    json_text(o, buf);
}
/* Footer data: reserved characters of a name as %XX. */
static void data_name(Out *o, const char *name) {
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        if (*p <= ' ' || *p == 0x7f || strchr("%,;~|\"\\", *p)) putf(o, "%%%02X", *p);
        else put(o, (const char *)p, 1);
    }
}

static void entry_line(Out *o, const BrEntry *e, unsigned rank, bool bold) {
    char t[16];
    BrFormatTime(e->frames, t, sizeof(t));
    putf(o, bold ? "**%u. `%s`** " : "**%u.** `%s` ", rank, t);
    for (unsigned k = 0; k < e->players && k < BR_SEATS; ++k) {
        if (k) puts_(o, ", ");
        md_name(o, e->player[k].name);
        if (e->player[k].choice) { puts_(o, " ("); json_text(o, BrCharacterName(e->player[k].choice)); puts_(o, ")"); }
    }
    putf(o, " \xc2\xb7 %s \xc2\xb7 %s\\n", e->online ? "online" : "offline", e->date[0] ? e->date : "?");
}
static void category_embed(Out *o, const BrBoard *b, unsigned c, int bold_rank, bool with_data) {
    static const unsigned kColors[BR_CATEGORIES] = {0xf5c542, 0x42a5f5, 0x66bb6a, 0xab47bc, 0xef5350};
    putf(o, "{\"color\":%u,\"title\":\"Boss Rush \\u2014 %u %s / %u %s\",\"description\":\"", kColors[c], c + 1,
         c ? "jogadores" : "jogador", c + 1, c ? "players" : "player");
    if (!b->count[c]) puts_(o, "Sem tempos ainda / No times yet");
    for (unsigned i = 0; i < b->count[c]; ++i) entry_line(o, &b->entry[c][i], i + 1, (int)i + 1 == bold_rank);
    puts_(o, "\"");
    if (with_data) {
        putf(o, ",\"footer\":{\"text\":\"sb5br1 c=%u", c + 1);
        for (unsigned i = 0; i < b->count[c]; ++i) {
            const BrEntry *e = &b->entry[c][i];
            putf(o, " %u,%s,%u,", (unsigned)e->frames, e->date[0] ? e->date : "-", e->online ? 1u : 0u);
            for (unsigned k = 0; k < e->players && k < BR_SEATS; ++k) {
                if (k) puts_(o, ";");
                data_name(o, e->player[k].name);
                putf(o, "~%u", e->player[k].choice);
            }
        }
        puts_(o, "\"}");
    }
    puts_(o, "}");
}

size_t BrBoardPayload(const BrBoard *b, char *out, size_t cap) {
    Out o = {out, cap, 0, false};
    if (!cap) return 0;
    out[0] = 0;
    puts_(&o, "{\"content\":\"\\ud83c\\udfc6 **Boss Rush \\u2014 ranking**\",\"allowed_mentions\":{\"parse\":[]},\"embeds\":[");
    for (unsigned c = 0; c < BR_CATEGORIES; ++c) {
        if (c) puts_(&o, ",");
        category_embed(&o, b, c, -1, true);
    }
    puts_(&o, "]}");
    return o.full ? 0 : o.n;
}

size_t BrNoticePayload(const BrBoard *b, unsigned category, unsigned rank, char *out, size_t cap) {
    Out o = {out, cap, 0, false};
    if (!cap || category < 1 || category > BR_CATEGORIES || rank < 1 || rank > b->count[category - 1]) return 0;
    out[0] = 0;
    const BrEntry *e = &b->entry[category - 1][rank - 1];
    char t[16];
    BrFormatTime(e->frames, t, sizeof(t));
    putf(&o, "{\"content\":\"\\ud83c\\udfc6 **Novo top 5 no Boss Rush / New Boss Rush top 5** \\u2014 %u %s: **#%u** `%s`\","
             "\"allowed_mentions\":{\"parse\":[]},\"embeds\":[",
         category, category > 1 ? "jogadores / players" : "jogador / player", rank, t);
    category_embed(&o, b, category - 1, (int)rank, false);
    puts_(&o, "]}");
    return o.full ? 0 : o.n;
}

/* ---- reading the board back ------------------------------------------- */
static int hexval(int c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}
static size_t utf8(uint32_t cp, char *o) {
    if (cp < 0x80) { o[0] = (char)cp; return 1; }
    if (cp < 0x800) { o[0] = (char)(0xc0 | cp >> 6); o[1] = (char)(0x80 | (cp & 63)); return 2; }
    if (cp < 0x10000) { o[0] = (char)(0xe0 | cp >> 12); o[1] = (char)(0x80 | (cp >> 6 & 63)); o[2] = (char)(0x80 | (cp & 63)); return 3; }
    o[0] = (char)(0xf0 | cp >> 18); o[1] = (char)(0x80 | (cp >> 12 & 63)); o[2] = (char)(0x80 | (cp >> 6 & 63)); o[3] = (char)(0x80 | (cp & 63));
    return 4;
}
static uint32_t hex4(const char *p) {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) { int h = hexval(p[i]); if (h < 0) return 0xffffffffu; v = v << 4 | (uint32_t)h; }
    return v;
}
/* The JSON string starting at `p` (just after its opening quote), unescaped. */
static size_t json_string(const char *p, char *out, size_t cap) {
    size_t n = 0;
    while (*p && *p != '"' && n + 5 < cap) {
        if (*p != '\\') { out[n++] = *p++; continue; }
        ++p;
        char c = *p ? *p++ : 0;
        switch (c) {
        case 'n': out[n++] = '\n'; break;
        case 't': out[n++] = '\t'; break;
        case 'r': case 'b': case 'f': break;
        case 'u': {
            uint32_t cp = hex4(p);
            if (cp == 0xffffffffu) return n;
            p += 4;
            if (cp >= 0xd800 && cp < 0xdc00 && p[0] == '\\' && p[1] == 'u') {
                uint32_t lo = hex4(p + 2);
                if (lo >= 0xdc00 && lo < 0xe000) { cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00); p += 6; }
            }
            n += utf8(cp, out + n);
            break;
        }
        default: if (c) out[n++] = c; break;
        }
    }
    out[n] = 0;
    return n;
}
static void undata_name(const char *s, size_t len, char out[BR_NAME_BYTES + 1]) {
    char tmp[BR_NAME_BYTES * 3 + 1];
    size_t n = 0;
    for (size_t i = 0; i < len && n + 1 < sizeof(tmp); ++i) {
        if (s[i] == '%' && i + 2 < len && hexval(s[i + 1]) >= 0 && hexval(s[i + 2]) >= 0) {
            tmp[n++] = (char)(hexval(s[i + 1]) << 4 | hexval(s[i + 2])); i += 2;
        } else tmp[n++] = s[i];
    }
    tmp[n] = 0;
    BrSetName(out, tmp);
}
static bool parse_entry(const char *s, size_t len, BrEntry *e) {
    memset(e, 0, sizeof(*e));
    char buf[1024];
    if (len >= sizeof(buf)) return false;
    memcpy(buf, s, len); buf[len] = 0;
    char *f1 = strchr(buf, ','), *f2 = f1 ? strchr(f1 + 1, ',') : NULL, *f3 = f2 ? strchr(f2 + 1, ',') : NULL;
    if (!f3) return false;
    *f1 = *f2 = *f3 = 0;
    char *end;
    unsigned long frames = strtoul(buf, &end, 10);
    if (*end || !frames || frames > 99u * 60u * 60u) return false;
    e->frames = (uint32_t)frames;
    if (strlen(f1 + 1) == 10) memcpy(e->date, f1 + 1, 10);
    e->online = f2[1] == '1';
    for (char *p = f3 + 1; *p && e->players < BR_SEATS;) {
        char *semi = strchr(p, ';');
        size_t plen = semi ? (size_t)(semi - p) : strlen(p);
        char *tilde = memchr(p, '~', plen);
        if (!tilde) return false;
        BrPlayer *pl = &e->player[e->players++];
        undata_name(p, (size_t)(tilde - p), pl->name);
        unsigned long ch = strtoul(tilde + 1, NULL, 10);
        pl->choice = ch < 20 ? (uint8_t)ch : 0;
        if (!semi) break;
        p = semi + 1;
    }
    return e->players > 0;
}
bool BrParseBoard(const char *json, BrBoard *b) {
    memset(b, 0, sizeof(*b));
    bool any = false;
    for (const char *p = json ? strstr(json, "sb5br1 c=") : NULL; p; p = strstr(p + 1, "sb5br1 c=")) {
        static char text[8192];
        json_string(p, text, sizeof(text));
        unsigned c = (unsigned)(text[9] - '0');
        if (c < 1 || c > BR_CATEGORIES || (text[10] && text[10] != ' ')) continue;
        BrBoard one; memset(&one, 0, sizeof(one));
        for (const char *q = text + 10; *q;) {
            while (*q == ' ') ++q;
            if (!*q) break;
            const char *sp = strchr(q, ' ');
            size_t len = sp ? (size_t)(sp - q) : strlen(q);
            BrEntry e;
            if (parse_entry(q, len, &e) && e.players == c) BrInsert(&one, &e);
            q += len;
        }
        b->count[c - 1] = one.count[c - 1];
        memcpy(b->entry[c - 1], one.entry[c - 1], sizeof(one.entry[c - 1]));
        any = true;
    }
    return any;
}
