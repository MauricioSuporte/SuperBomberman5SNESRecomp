/*
 * The framework checks one digest, so it is given the right one before it
 * starts. A generated binary stays locked to the image its code came from
 * (g_program_module.rom_sha256). A setup host has no code yet and takes the
 * first file that reads as this game (rom_signature.c). rom_identity.txt's dump
 * leaves everything as it was.
 */
#include "rom_identity_resolve.h"
#include "crc32.h"
#include "host_paths.h"
#include "launcher_cache.h"
#include "program_module.h"
#include "rom_probe.h"
#include "rom_signature.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

enum { MAX_ROM_FILE = 16 * 1024 * 1024, PATH_CAP = 1200 };

static char s_sha256[65], s_crc32[9];

typedef struct Image { uint8_t *raw; const uint8_t *data; size_t len; char sha256[65]; } Image;

static void to_hex(const uint8_t *d, size_t n, char *out) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; ++i) { out[i * 2] = digits[d[i] >> 4]; out[i * 2 + 1] = digits[d[i] & 15]; }
    out[n * 2] = '\0';
}

static bool image_load(const char *path, Image *image) {
    memset(image, 0, sizeof(*image));
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    long size = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
    if (size > 0 && size <= MAX_ROM_FILE && fseek(f, 0, SEEK_SET) == 0 && (image->raw = malloc((size_t)size)) &&
        fread(image->raw, 1, (size_t)size, f) == (size_t)size) {
        const size_t header = size % 1024 == 512 ? 512 : 0;
        uint8_t digest[32];
        image->data = image->raw + header;
        image->len = (size_t)size - header;
        sha256_compute(image->data, image->len, digest);
        to_hex(digest, 32, image->sha256);
    }
    fclose(f);
    return image->data != NULL;
}
static void image_free(Image *image) { free(image->raw); image->raw = NULL; image->data = NULL; }

static bool rom_like(const char *name) {
    static const char *const kExt[] = {".sfc", ".smc", ".swc", ".fig"};
    const size_t n = strlen(name);
    if (n < 5) return false;
    for (unsigned i = 0; i < 4; ++i) {
        bool same = true;
        for (unsigned k = 0; k < 4 && same; ++k) {
            char c = name[n - 4 + k];
            same = (c >= 'A' && c <= 'Z' ? (char)(c + 32) : c) == kExt[i][k];
        }
        if (same) return true;
    }
    return false;
}

/* Decides on one file: returns true when the search can stop. */
typedef struct Search {
    const char *want;      /* a generated binary's locked digest, or NULL */
    const char *supported; /* rom_identity.txt's digest */
    bool found, is_supported;
    Image best;
    char best_name[PATH_CAP];
} Search;

static void adopt(Search *s, Image *image, const char *path, bool is_supported) {
    image_free(&s->best);
    s->best = *image;
    image->raw = NULL;
    snprintf(s->best_name, sizeof(s->best_name), "%s", path);
    s->found = true;
    s->is_supported = is_supported;
}

/* `ordered`: explicit choices win over the folder scan's name order. */
static bool consider(Search *s, const char *path, bool ordered) {
    Image image;
    if (!image_load(path, &image)) return false;
    bool stop = false;
    if (s->want) {
        if (strcmp(image.sha256, s->want) == 0) { adopt(s, &image, path, false); stop = true; }
    } else if (strcmp(image.sha256, s->supported) == 0) {
        adopt(s, &image, path, true);
        stop = true;
    } else if (Sb5RomSignatureCheck(image.data, image.len, NULL) &&
               (ordered || !s->found || strcmp(path, s->best_name) < 0)) {
        adopt(s, &image, path, false);
        stop = ordered;
    }
    image_free(&image);
    return stop;
}

static bool scan_dir(Search *s, const char *dir) {
    char path[PATH_CAP];
#ifdef _WIN32
    char pattern[PATH_CAP];
    WIN32_FIND_DATAA fd;
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool stop = false;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || !rom_like(fd.cFileName)) continue;
        if (snprintf(path, sizeof(path), "%s/%s", dir, fd.cFileName) < (int)sizeof(path))
            stop = consider(s, path, false);
    } while (!stop && FindNextFileA(h, &fd));
    FindClose(h);
    return stop;
#else
    DIR *d = opendir(dir);
    if (!d) return false;
    bool stop = false;
    for (struct dirent *e; !stop && (e = readdir(d)) != NULL;) {
        if (!rom_like(e->d_name)) continue;
        if (snprintf(path, sizeof(path), "%s/%s", dir, e->d_name) < (int)sizeof(path))
            stop = consider(s, path, false);
    }
    closedir(d);
    return stop;
#endif
}

static void search_all(Search *s, int argc, char **argv) {
    for (int i = 1; i < argc; ++i)
        if (rom_like(argv[i]) && consider(s, argv[i], true)) return;
    char cached[1024], stored[1024];
    if (snesrecomp_rom_cache_read(stored, sizeof(stored)) &&
        snesrecomp_resolve_stored_path(stored, cached, sizeof(cached)) && consider(s, cached, true))
        return;
    char dirs[SNESRECOMP_SEARCH_DIR_MAX][SNESRECOMP_SEARCH_PATH_LEN];
    const int n = snesrecomp_rom_search_dirs(dirs, SNESRECOMP_SEARCH_DIR_MAX);
    for (int i = 0; i < n; ++i)
        if (scan_dir(s, dirs[i])) return;
}

static void use_image(SnesDesktopHostGame *game, const Image *image) {
    Sb5RomSignature sig;
    snprintf(s_sha256, sizeof(s_sha256), "%s", image->sha256);
    snprintf(s_crc32, sizeof(s_crc32), "%08x", (unsigned)crc32_compute(image->data, image->len));
    game->expected_sha256_hex = s_sha256;
    game->expected_crc32_hex = s_crc32;
    game->region = "";
    Sb5RomSetSupported(false);
    Sb5RomProbesEvaluate(image->data, image->len);
    Sb5RomSignatureCheck(image->data, image->len, &sig);
    printf("[rom] unsupported dump %.12s...: running by content%s\n", image->sha256,
           sig.checksum_ok ? "" : " (its checksum does not match: patched or a bad dump)");
}

void Sb5ResolveRomIdentity(SnesDesktopHostGame *game, int argc, char **argv) {
    Search search;
    memset(&search, 0, sizeof(search));
    search.supported = game->expected_sha256_hex;
    const SnesProgramModule *module = snes_program_module_find("main");
    char locked[65] = "";
    if (module) {
        to_hex(module->rom_sha256, 32, locked);
        if (strcmp(locked, search.supported) == 0) return;
        search.want = locked;
    }
    search_all(&search, argc, argv);
    if (search.found && !search.is_supported) {
        use_image(game, &search.best);
    } else if (module) {
        /* Its ROM isn't where we looked. Stay locked to it; the launcher can't
         * confirm a CRC it doesn't have, and mods stay off without the image. */
        snprintf(s_sha256, sizeof(s_sha256), "%s", locked);
        snprintf(s_crc32, sizeof(s_crc32), "00000000");
        game->expected_sha256_hex = s_sha256;
        game->expected_crc32_hex = s_crc32;
        game->region = "";
        Sb5RomSetSupported(false);
        Sb5RomProbesFailAll();
    }
    image_free(&search.best);
}
