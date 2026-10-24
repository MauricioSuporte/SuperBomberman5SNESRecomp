/* Portable game folders: rom.cfg and ROM lookup must survive copying the
 * whole folder to another place (another PC, drive or directory).
 *
 * The test copies itself into a synthetic game folder
 *   <tmp>/game/rom_identity.txt   (marks the game root)
 *   <tmp>/game/build/<this test>  (where a rebuilt game lives)
 * and runs that copy from an unrelated working directory, then moves the
 * whole folder and runs it again. No game data: the "ROM" is random bytes
 * whose SHA-256 the child computes itself. POSIX only (the framework's
 * executable-path query is what is being exercised). */
#include "host_paths.h"
#include "launcher_cache.h"
#include "rom_image_verify.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

#ifdef _WIN32
int main(void) { puts("portable_paths: skipped on Windows"); return 0; }
#else
#include <sys/stat.h>
#include <unistd.h>

static int write_file(const char *path, const void *data, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(data, 1, n, f) == n;
    return fclose(f) == 0 && ok;
}

static int read_line(const char *path, char *out, size_t cap) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(out, (int)cap, f)) { fclose(f); return 0; }
    fclose(f);
    out[strcspn(out, "\r\n")] = 0;
    return 1;
}

static int run(const char *cmd) { return system(cmd) == 0; }

/* The child: everything below runs as <root>/build/portable_paths_tests. */
static int child(const char *mode, const char *root) {
    char got[1024], expect[1200], line[1024];
    CHECK(snesrecomp_game_root_dir(got, sizeof(got)));
    CHECK(strcmp(got, root) == 0);

    if (strcmp(mode, "first") == 0) {
        snprintf(expect, sizeof(expect), "%s/Game.sfc", root);
        CHECK(snesrecomp_portable_path(expect, got, sizeof(got)));
        CHECK(strcmp(got, "Game.sfc") == 0);
        snprintf(expect, sizeof(expect), "%s/roms/../roms/Other.sfc", root);
        CHECK(snesrecomp_portable_path(expect, got, sizeof(got)));
        CHECK(strcmp(got, "roms/Other.sfc") == 0);
        CHECK(snesrecomp_portable_path("/elsewhere/x.sfc", got, sizeof(got)));
        CHECK(strcmp(got, "/elsewhere/x.sfc") == 0);

        /* rom.cfg stores the relative form and reads back an openable path. */
        snprintf(expect, sizeof(expect), "%s/Game.sfc", root);
        CHECK(snesrecomp_rom_cache_write(expect));
        snprintf(got, sizeof(got), "%s/build/rom.cfg", root);
        CHECK(read_line(got, line, sizeof(line)));
        CHECK(strcmp(line, "Game.sfc") == 0);
        CHECK(snesrecomp_rom_cache_read(got, sizeof(got)));
        CHECK(strcmp(got, expect) == 0);

        /* An absolute path from another machine: found by its name. */
        CHECK(snesrecomp_resolve_stored_path("/home/someone/old/Game.sfc", got, sizeof(got)));
        CHECK(strcmp(got, expect) == 0);
        CHECK(!snesrecomp_resolve_stored_path("/home/someone/old/Missing.sfc", got, sizeof(got)));

        /* Any name, copier header, roms/ subfolder: found by digest. */
        uint8_t digest[32];
        static uint8_t payload[4096];
        snprintf(got, sizeof(got), "%s/roms/renamed.SMC", root);
        FILE *f = fopen(got, "rb");
        CHECK(f && fseek(f, 512, SEEK_SET) == 0 && fread(payload, 1, sizeof(payload), f) == sizeof(payload));
        fclose(f);
        sha256_compute(payload, sizeof(payload), digest);
        CHECK(snesrecomp_rom_locate_sha256(digest, line, sizeof(line)));
        CHECK(strcmp(line, got) == 0);
        digest[0] ^= 1;
        CHECK(!snesrecomp_rom_locate_sha256(digest, line, sizeof(line)));
        return 0;
    }
    /* "moved": the same rom.cfg, the folder now somewhere else. */
    snprintf(expect, sizeof(expect), "%s/Game.sfc", root);
    CHECK(snesrecomp_rom_cache_read(got, sizeof(got)));
    CHECK(strcmp(got, expect) == 0);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 4 && strcmp(argv[1], "--child") == 0) return child(argv[2], argv[3]);

    char tmp[] = "/tmp/sb5-portable-XXXXXX";
    char self[1024], game[1200], path[1400], cmd[4096], other[1200], moved[1400];
    CHECK(mkdtemp(tmp));
    CHECK(snesrecomp_exe_path(self, sizeof(self)));
    snprintf(game, sizeof(game), "%s/game", tmp);
    snprintf(other, sizeof(other), "%s/elsewhere", tmp);
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/build' '%s/roms' '%s'", game, game, other);
    CHECK(run(cmd));
    snprintf(path, sizeof(path), "%s/rom_identity.txt", game);
    CHECK(write_file(path, "game_id = test\n", 15));
    snprintf(path, sizeof(path), "%s/Game.sfc", game);
    CHECK(write_file(path, "rom", 3));
    {
        static uint8_t image[512 + 4096];
        for (size_t i = 0; i < sizeof(image); ++i) image[i] = (uint8_t)(i * 131u + 7u);
        snprintf(path, sizeof(path), "%s/roms/renamed.SMC", game);
        CHECK(write_file(path, image, sizeof(image)));
    }
    snprintf(cmd, sizeof(cmd), "cp '%s' '%s/build/portable_paths_tests'", self, game);
    CHECK(run(cmd));
    unsetenv("APPIMAGE");
    snprintf(cmd, sizeof(cmd), "cd '%s' && '%s/build/portable_paths_tests' --child first '%s'",
             other, game, game);
    CHECK(run(cmd));
    snprintf(moved, sizeof(moved), "%s/moved/copy-of-game", tmp);
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/moved' && mv '%s' '%s'", tmp, game, moved);
    CHECK(run(cmd));
    snprintf(cmd, sizeof(cmd), "cd '%s' && '%s/build/portable_paths_tests' --child moved '%s'",
             other, moved, moved);
    CHECK(run(cmd));
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmp);
    run(cmd);
    puts("portable_paths: ok");
    return 0;
}
#endif
