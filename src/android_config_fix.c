/* See android_config_fix.h. Built into the Android library and the native
 * tests; it has no platform dependencies. */
#include "android_config_fix.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/* Case-insensitive comparison of [s, s+n) with the NUL-terminated word,
 * matching the host's StringEqualsNoCase on section and key names. */
static bool span_equals(const char *s, size_t n, const char *word) {
    if (strlen(word) != n) return false;
    for (size_t i = 0; i < n; ++i)
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)word[i])) return false;
    return true;
}

static void trim(const char **s, size_t *n) {
    while (*n && isspace((unsigned char)**s)) ++*s, --*n;
    while (*n && isspace((unsigned char)(*s)[*n - 1])) --*n;
}

typedef struct { char *out; size_t cap, used; bool overflow; } Writer;

static void put(Writer *w, const char *s, size_t n) {
    if (w->overflow || w->used + n > w->cap) {
        w->overflow = true;
        return;
    }
    memcpy(w->out + w->used, s, n);
    w->used += n;
}

long Sb5ConfigAddRendererKeys(const char *in, size_t len, char *out, size_t cap) {
    /* Pass 1: find the [Graphics] header and which keys the section has. */
    bool in_graphics = false, has_section = false, has_new = false, has_limits = false;
    size_t header_end = 0; /* offset just past the [Graphics] header line */
    for (size_t pos = 0; pos < len;) {
        const char *line = in + pos;
        const char *nl = memchr(line, '\n', len - pos);
        size_t n = nl ? (size_t)(nl - line) : len - pos;
        size_t next = pos + n + (nl ? 1 : 0);
        const char *t = line;
        size_t tn = n;
        trim(&t, &tn);
        if (tn && t[0] == '[') {
            const char *close = memchr(t, ']', tn);
            in_graphics = close && span_equals(t + 1, (size_t)(close - t - 1), "Graphics");
            if (in_graphics && !has_section) {
                has_section = true;
                header_end = next;
            }
        } else if (in_graphics && tn && t[0] != '#' && t[0] != ';') {
            const char *eq = memchr(t, '=', tn);
            if (eq) {
                const char *k = t;
                size_t kn = (size_t)(eq - t);
                trim(&k, &kn);
                if (span_equals(k, kn, "NewRenderer")) has_new = true;
                if (span_equals(k, kn, "NoSpriteLimits")) has_limits = true;
            }
        }
        pos = next;
    }
    if (has_new && has_limits) return 0;

    /* Keep the file's own line ending: a config edited on Windows uses CRLF. */
    bool crlf = false;
    for (size_t i = 0; i + 1 < len && !crlf; ++i) crlf = in[i] == '\r' && in[i + 1] == '\n';
    const char *eol = crlf ? "\r\n" : "\n";
    const size_t eol_n = crlf ? 2 : 1;
    Writer w = { out, cap, 0, false };
    if (has_section) {
        put(&w, in, header_end);
        /* A header on the file's last line has no newline to end it. */
        if (header_end && in[header_end - 1] != '\n') put(&w, eol, eol_n);
    } else {
        put(&w, in, len);
        if (len && in[len - 1] != '\n') put(&w, eol, eol_n);
        put(&w, eol, eol_n);
        put(&w, "[Graphics]", 10);
        put(&w, eol, eol_n);
    }
    if (!has_new) { put(&w, "NewRenderer = 1", 15); put(&w, eol, eol_n); }
    if (!has_limits) { put(&w, "NoSpriteLimits = 1", 18); put(&w, eol, eol_n); }
    if (has_section) put(&w, in + header_end, len - header_end);
    return w.overflow ? -1 : (long)w.used;
}
