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
    puts("Updater release parsing, versions, checksums and player files passed.");
    return 0;
}
