#ifndef SB5_ANDROID_CONFIG_FIX_H
#define SB5_ANDROID_CONFIG_FIX_H

#include <stddef.h>

/* Repair for config.ini files written by v3.0.1 on a fresh Android install:
 * that release created config.ini (with only [Sound] and [Rewind]) before the
 * host wrote its default, so the default's "NewRenderer = 1" and
 * "NoSpriteLimits = 1" were never written and both read as off.
 *
 * Copies `in` (len bytes) to `out`, adding "NewRenderer = 1" and/or
 * "NoSpriteLimits = 1" right after the [Graphics] header when the section has
 * no such key (an existing value, 0 or 1, is kept), or appending a [Graphics]
 * section with both when there is none, using the file's line ending (CRLF
 * when it has one). Returns the output length when keys were added, 0 when
 * nothing was missing (out is then unspecified), and -1 when the result does
 * not fit in cap bytes. Pure: no file access. */
long Sb5ConfigAddRendererKeys(const char *in, size_t len, char *out, size_t cap);

#endif
