/* Release metadata, version order, checksum lookup and the files an update
 * must never overwrite. */
#define SB5_UPDATER_NO_MAIN 1
#include "../src/updater/sb5_updater.c"
#define CHECK(v) do { if (!(v)) { fprintf(stderr, "line %d\n", __LINE__); return 1; } } while (0)
int main(void) {
    Release r;
    char d[65];
    const char *json =
        "{\"url\":\"x\",\"tag_name\": \"v1.2.0\",\"assets\":[{\"name\":\"SHA256SUMS\",\"browser_download_url\":"
        "\"https://github.com/o/r/releases/download/v1.2.0/SHA256SUMS\"},{\"browser_download_url\":"
        "\"https://github.com/o/r/releases/download/v1.2.0/superbomberman5-1.2.0-linux-x86_64.AppImage\"},"
        "{\"browser_download_url\":\"https://github.com/o/r/releases/download/v1.2.0/superbomberman5-1.2.0-windows-x64.zip\"}]}";
    const char *sums =
        "1111111111111111111111111111111111111111111111111111111111111111  superbomberman5-1.2.0-linux-x86_64.AppImage\n"
        "2222222222222222222222222222222222222222222222222222222222222222 *superbomberman5-1.2.0-windows-x64.zip\r\n";
    CHECK(Sb5ParseRelease(json, &r));
    CHECK(!strcmp(r.tag, "v1.2.0"));
    CHECK(strstr(r.sums_url, "/SHA256SUMS"));
#if defined(_WIN32)
    CHECK(!strcmp(r.package_name, "superbomberman5-1.2.0-windows-x64.zip"));
#else
    CHECK(!strcmp(r.package_name, "superbomberman5-1.2.0-linux-x86_64.AppImage"));
#endif
    CHECK(!Sb5ParseRelease("{\"tag_name\":\"v1\"}", &r));
    CHECK(Sb5ExpectedDigest(sums, "superbomberman5-1.2.0-windows-x64.zip", d) && d[0] == '2' && strlen(d) == 64);
    CHECK(Sb5ExpectedDigest(sums, "superbomberman5-1.2.0-linux-x86_64.AppImage", d) && d[0] == '1');
    CHECK(!Sb5ExpectedDigest(sums, "superbomberman5-1.2.0-windows-x64.zi", d));
    CHECK(Sb5VersionNewer("v1.2.0", "1.1.2") && Sb5VersionNewer("v1.10.0", "1.9.9") && Sb5VersionNewer("2.0", "1.99.99"));
    CHECK(!Sb5VersionNewer("v1.2.0", "1.2.0") && !Sb5VersionNewer("v1.1.9", "1.2.0") && Sb5VersionNewer("v1.2.1", "dev") == 1);
    CHECK(Sb5PlayerOwned("rom.cfg") && Sb5PlayerOwned("saves/save.srm") && Sb5PlayerOwned("build/x.exe"));
    CHECK(Sb5PlayerOwned("Super Bomberman 5 (USA).sfc") && Sb5PlayerOwned("mods/preloaded/state.toml"));
    CHECK(Sb5PlayerOwned("src/gen/") && Sb5PlayerOwned("recomp/funcs.h") && Sb5PlayerOwned("config.ini"));
    CHECK(!Sb5PlayerOwned("SuperBomberman5Recomp.exe") && !Sb5PlayerOwned("src/game_rtl.c"));
    CHECK(!Sb5PlayerOwned("mods/preloaded/packages/x/0.1.0/manifest.toml") && !Sb5PlayerOwned("VERSION"));
    {
        /* Asset sizes come from the same asset as the download URL. */
        const char *sized =
            "{\"tag_name\":\"v2.3.0\",\"assets\":[{\"name\":\"SHA256SUMS\",\"uploader\":{\"id\":1},\"size\":214,"
            "\"browser_download_url\":\"https://x/SHA256SUMS\"},{\"name\":\"a.AppImage\",\"size\": 14813688,"
            "\"browser_download_url\":\"https://x/superbomberman5-2.3.0-linux-x86_64.AppImage\"},{\"name\":\"b.zip\","
            "\"size\":19020449,\"browser_download_url\":\"https://x/superbomberman5-2.3.0-windows-x64.zip\"}]}";
        char line[160], t[24];
        CHECK(Sb5ParseRelease(sized, &r));
#if defined(_WIN32)
        CHECK(r.package_size == 19020449);
#else
        CHECK(r.package_size == 14813688);
#endif
        CHECK(Sb5ParseRelease(json, &r) && r.package_size == 0); /* no size: unknown */
        Sb5FormatDuration(5, t, sizeof(t)); CHECK(!strcmp(t, "0:05"));
        Sb5FormatDuration(125.4, t, sizeof(t)); CHECK(!strcmp(t, "2:05"));
        Sb5FormatDuration(3725, t, sizeof(t)); CHECK(!strcmp(t, "1:02:05"));
        Sb5FormatDuration(-1, t, sizeof(t)); CHECK(!strcmp(t, "--:--"));
        /* Half of 20 MB in 5 s: 2 MB/s, 5 s left. */
        Sb5FormatProgress(line, sizeof(line), "Downloading", 10485760, 20971520, 5.0, 1, NULL);
        CHECK(strstr(line, "[############------------]") && strstr(line, " 50%"));
        CHECK(strstr(line, "10.0/20.0 MB") && strstr(line, "2.0 MB/s") && strstr(line, "ETA 0:05"));
        Sb5FormatProgress(line, sizeof(line), "Downloading", 0, 20971520, 0.1, 1, NULL);
        CHECK(strstr(line, "  0%") && strstr(line, "ETA --:--") && !strstr(line, "MB/s"));
        Sb5FormatProgress(line, sizeof(line), "Installing", 1626, 1626, 8.0, 0, "files");
        CHECK(strstr(line, "[########################] 100%") && strstr(line, "1626/1626 files") &&
              strstr(line, "done in 0:08"));
        Sb5FormatProgress(line, sizeof(line), "Installing", 400, 1600, 2.0, 0, "files");
        CHECK(strstr(line, " 25%") && strstr(line, "ETA 0:06") && !strstr(line, "MB"));
        Sb5FormatProgress(line, sizeof(line), "Installing", 2000, 1600, 2.0, 0, "files"); /* clamped */
        CHECK(strstr(line, "100%") && strstr(line, "1600/1600"));
        Sb5FormatProgress(line, sizeof(line), "Downloading", 3145728, 0, 2.0, 1, NULL); /* size unknown */
        CHECK(!strchr(line, '[') && strstr(line, "3.0 MB") && strstr(line, "1.5 MB/s"));
        Sb5FormatProgress(line, sizeof(line), "Unpacking", 0, 0, 3.0, 0, ""); /* timer only */
        CHECK(strstr(line, "Unpacking") && strstr(line, "...") && strstr(line, "0:03") && !strchr(line, '['));
        CHECK(strlen(line) < 80);
        Sb5FormatProgress(line, sizeof(line), "Downloading", 19020449, 19020449, 4000.0, 1, NULL);
        CHECK(strlen(line) < 100);
    }
    puts("Updater release parsing, versions, checksums, player files and progress passed.");
    return 0;
}
