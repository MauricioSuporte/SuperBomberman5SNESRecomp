/* Android config repair (src/android_config_fix.c): v3.0.1 fresh installs lost
 * NewRenderer/NoSpriteLimits; every other config.ini stays byte-identical. */
#include "../src/android_config_fix.c"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static char out[1 << 14];

/* Case-insensitive prefix test (strncasecmp is not in the Windows CRT). */
static int prefix_nocase(const char *s, const char *p) {
    for (; *p; ++s, ++p)
        if (tolower((unsigned char)*s) != tolower((unsigned char)*p)) return 0;
    return 1;
}

/* Leaves out NUL-terminated for strstr. */
static long fix(const char *in) {
    long n = Sb5ConfigAddRendererKeys(in, strlen(in), out, sizeof out - 1);
    out[n > 0 ? n : 0] = 0;
    return n;
}

/* Value of key in [Graphics] of out[0..n), or -1 when absent: a minimal
 * reader with the host's rules (case-insensitive names, '#' comments). */
static int graphics_value(long n, const char *key) {
    char copy[sizeof out + 1];
    memcpy(copy, out, (size_t)n);
    copy[n] = 0;
    int in_graphics = 0, value = -1;
    for (char *line = strtok(copy, "\n"); line; line = strtok(NULL, "\n")) {
        while (*line == ' ' || *line == '\t') ++line;
        if (*line == '[') { in_graphics = prefix_nocase(line, "[Graphics]"); continue; }
        if (!in_graphics || *line == '#') continue;
        if (prefix_nocase(line, key) && strchr(line, '=')) value = atoi(strchr(line, '=') + 1);
    }
    return value;
}

/* The config.ini a fresh v3.0.1 install produced (pulled from a phone). */
static const char kBroken301[] =
    "[Sound]\nAudioSamples = 2048\n\nEnableAudio = 1\nAudioFreq = 32040\nVolume = 100\n"
    "[Rewind]\nEnabled = 0\n\n[Graphics]\nWindowScale = 2\nFullscreen = 0\nDisplayAspect = 4:3\n"
    "OutputMethod = SDL\nLinearFiltering = 0\nShader = \nWidescreen = 0\nFrameBlend = 0\nVSync = 1\n"
    "Renderer = auto\n\n[GamepadMap]\nEnableGamepad1 = true\nEnableGamepad2 = true\n"
    "GamepadDeadzone = 3277\n\n[General]\nSkipLauncher = 0\nRunAhead = 0\n";

/* The shape of the host's default (what v3.0.0 and fresh installs get). */
static const char kDefault300[] =
    "[General]\n# Automatically save state on quit and reload on start\nAutosave = 0\n\n"
    "[Graphics]\n# Window size (Auto or WidthxHeight)\nWindowSize = Auto\n\n"
    "# Use the optimized SNES PPU implementation\nNewRenderer = 1\n\n"
    "# Remove the sprite limits per scan line\nNoSpriteLimits = 1\n\n"
    "[Sound]\nEnableAudio = 1\nAudioSamples = 512\n";

int main(void) {
    long n;
    /* The broken v3.0.1 file gains both keys, everything else unchanged. */
    n = fix(kBroken301);
    CHECK(n > 0);
    CHECK(graphics_value(n, "NewRenderer") == 1 && graphics_value(n, "NoSpriteLimits") == 1);
    CHECK((size_t)n == strlen(kBroken301) + strlen("NewRenderer = 1\nNoSpriteLimits = 1\n"));
    CHECK(strstr(out, "[Graphics]\nNewRenderer = 1\nNoSpriteLimits = 1\nWindowScale = 2\n"));
    CHECK(strstr(out, "AudioSamples = 2048\n") && strstr(out, "[Rewind]\nEnabled = 0\n"));
    /* A second pass finds nothing to add. */
    {
        char again[sizeof out];
        memcpy(again, out, (size_t)n);
        CHECK(Sb5ConfigAddRendererKeys(again, (size_t)n, out, sizeof out) == 0);
    }
    /* The default (v3.0.0) config is not touched. */
    CHECK(fix(kDefault300) == 0);
    /* A value the player chose is kept, whatever its case or spacing. */
    CHECK(fix("[graphics]\n  newrenderer=0\nNOSPRITELIMITS = 0\n") == 0);
    /* Only the missing key is added. */
    n = fix("[Graphics]\nNewRenderer = 0\n[Sound]\nVolume = 50\n");
    CHECK(n > 0 && graphics_value(n, "NewRenderer") == 0 && graphics_value(n, "NoSpriteLimits") == 1);
    CHECK(!strstr(out, "NewRenderer = 1"));
    /* Keys in another section or commented out do not count. */
    n = fix("[Sound]\nNewRenderer = 0\n[Graphics]\n# NewRenderer = 0\nNoSpriteLimits = 1\n");
    CHECK(n > 0 && graphics_value(n, "NewRenderer") == 1);
    /* No [Graphics] section, last line without newline. */
    n = fix("[Sound]\nVolume = 50");
    CHECK(n > 0 && !memcmp(out, "[Sound]\nVolume = 50\n\n[Graphics]\n", 32));
    CHECK(graphics_value(n, "NewRenderer") == 1 && graphics_value(n, "NoSpriteLimits") == 1);
    /* Header on the last line without newline. */
    n = fix("[Sound]\nVolume = 50\n[Graphics]");
    CHECK(n > 0 && graphics_value(n, "NewRenderer") == 1 && graphics_value(n, "NoSpriteLimits") == 1);
    /* Too small an output buffer is reported, not truncated silently. */
    CHECK(Sb5ConfigAddRendererKeys(kBroken301, strlen(kBroken301), out, 16) == -1);
    puts("android_config_fix_tests: ok");
    return 0;
}
