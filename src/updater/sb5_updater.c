/*
 * Super Bomberman 5 Recompiled — updater.
 *
 * Reads the latest release of MauricioSuporte/SuperBomberman5SNESRecomp on
 * GitHub, and when it is newer than this install's VERSION, downloads the
 * setup package for this platform, checks it against the release's
 * SHA256SUMS and installs it in place:
 *
 *   Windows  the ZIP is unpacked over the install folder. Player files are
 *            never touched: ROMs, rom.cfg, config/keybinds, saves/, the
 *            rebuilt game in build/, generated sources and the Mods page
 *            selections (mods/preloaded/state.toml). The new setup host is
 *            then newer than build/, so the launcher asks for one Generate &
 *            rebuild, which compiles the new version from the player's ROM.
 *   Linux    the AppImage named by $APPIMAGE (or --appimage) is replaced.
 *
 * No ROM or game data is downloaded: release packages are ROM-free setup
 * packs. Tools used: curl (built into Windows 10 1803+), tar (bsdtar, also
 * built in) to read the ZIP.
 *
 *   SuperBomberman5Updater [--check] [--dir DIR] [--wait-pid PID]
 *                          [--appimage PATH] [--launch] [--yes] [--force]
 *
 * --check exits 0 when up to date and 10 when an update is available,
 * printing "latest=<version>". Exit 1 means it could not find out.
 */
#include "sha256.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <io.h>
#define PATH_SEP '\\'
#define mkdir_one(p) _mkdir(p)
#else
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define PATH_SEP '/'
#define mkdir_one(p) mkdir((p), 0755)
#endif

#if defined(__GNUC__)
#define SB5_UNUSED __attribute__((unused))
#else
#define SB5_UNUSED
#endif
#define SB5_REPO "MauricioSuporte/SuperBomberman5SNESRecomp"
#define SB5_SETUP_EXE "SuperBomberman5Recomp.exe"
#define PATH_CAP 2048

static FILE *g_log;
static char g_dir[PATH_CAP] SB5_UNUSED;
static char g_tmp[PATH_CAP] SB5_UNUSED;

static void say(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
    if (g_log) {
        va_start(ap, fmt);
        vfprintf(g_log, fmt, ap);
        va_end(ap);
        fprintf(g_log, "\n");
        fflush(g_log);
    }
}

static void join(char *out, size_t cap, const char *a, const char *b) {
    size_t n = strlen(a);
    snprintf(out, cap, "%s%s%s", a, (n && (a[n - 1] == '/' || a[n - 1] == '\\')) ? "" : (PATH_SEP == '/' ? "/" : "\\"), b);
}

static int file_exists(const char *p) {
#if defined(_WIN32)
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(p, &st) == 0;
#endif
}

static int is_dir(const char *p) {
#if defined(_WIN32)
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static void mkdirs(const char *path) {
    char buf[PATH_CAP];
    size_t i;
    snprintf(buf, sizeof(buf), "%s", path);
    for (i = 1; buf[i]; ++i) {
        if (buf[i] == '/' || buf[i] == '\\') {
            char c = buf[i];
            if (i == 2 && buf[1] == ':') continue;
            buf[i] = 0;
            mkdir_one(buf);
            buf[i] = c;
        }
    }
    mkdir_one(buf);
}

static unsigned char *read_file(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    unsigned char *data;
    long n;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    data = (unsigned char *)malloc((size_t)n + 1);
    if (!data) { fclose(f); return NULL; }
    if (fread(data, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(data); return NULL; }
    fclose(f);
    data[n] = 0;
    *len = (size_t)n;
    return data;
}

/* Runs a command line, waits, returns its exit code (-1 if it could not run). */
SB5_UNUSED static int run(const char *cmdline) {
#if defined(_WIN32)
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD code = 1;
    char *cmd = _strdup(cmdline);
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    if (!cmd || !CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        free(cmd);
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    free(cmd);
    return (int)code;
#else
    int rc = system(cmdline);
    if (rc == -1) return -1;
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
#endif
}

SB5_UNUSED static int download(const char *url, const char *dest) {
    char cmd[PATH_CAP * 2];
    snprintf(cmd, sizeof(cmd),
             "curl -fsSL --retry 3 --connect-timeout 20 -H \"User-Agent: SuperBomberman5Updater\" -o \"%s\" \"%s\"",
             dest, url);
    return run(cmd) == 0 && file_exists(dest);
}

/* ---- release metadata ------------------------------------------------- */

static int json_string_after(const char *text, const char *key, const char *from, char *out, size_t cap,
                             const char **end) {
    char pat[64];
    const char *p, *q;
    size_t n;
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    p = strstr(from ? from : text, pat);
    if (!p) return 0;
    p = strchr(p + strlen(pat), ':');
    if (!p) return 0;
    while (*++p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {}
    if (*p != '"') return 0;
    q = strchr(++p, '"');
    if (!q) return 0;
    n = (size_t)(q - p);
    if (n >= cap) n = cap - 1;
    memcpy(out, p, n);
    out[n] = 0;
    if (end) *end = q + 1;
    return 1;
}

static void parse_version(const char *s, int v[3]) {
    int i = 0;
    v[0] = v[1] = v[2] = 0;
    while (*s && !isdigit((unsigned char)*s)) ++s;
    while (*s && i < 3) {
        v[i++] = atoi(s);
        while (isdigit((unsigned char)*s)) ++s;
        if (*s != '.') break;
        ++s;
    }
}

int Sb5VersionNewer(const char *latest, const char *local) {
    int a[3], b[3], i;
    parse_version(latest, a);
    parse_version(local, b);
    for (i = 0; i < 3; ++i)
        if (a[i] != b[i]) return a[i] > b[i];
    return 0;
}

typedef struct {
    char tag[64];
    char package_url[1024];
    char package_name[1024];
    char sums_url[1024];
} Release;

static int asset_matches(const char *name) {
#if defined(_WIN32)
    const char *suffix = "-windows-x64.zip";
#else
    const char *suffix = "-linux-x86_64.AppImage";
#endif
    size_t n = strlen(name), m = strlen(suffix);
    return n > m && strcmp(name + n - m, suffix) == 0;
}

int Sb5ParseRelease(const char *json, Release *r) {
    const char *p = json;
    char url[1024];
    memset(r, 0, sizeof(*r));
    if (!json_string_after(json, "tag_name", NULL, r->tag, sizeof(r->tag), NULL)) return 0;
    while (json_string_after(json, "browser_download_url", p, url, sizeof(url), &p)) {
        const char *base = strrchr(url, '/');
        base = base ? base + 1 : url;
        if (asset_matches(base)) {
            snprintf(r->package_url, sizeof(r->package_url), "%s", url);
            snprintf(r->package_name, sizeof(r->package_name), "%s", base);
        } else if (!strcmp(base, "SHA256SUMS")) {
            snprintf(r->sums_url, sizeof(r->sums_url), "%s", url);
        }
    }
    return r->package_url[0] && r->sums_url[0];
}

/* The expected digest of `name` from a SHA256SUMS text ("<hex>  <name>"). */
int Sb5ExpectedDigest(const char *sums, const char *name, char out[65]) {
    const char *line = sums;
    while (line && *line) {
        const char *end = strchr(line, '\n');
        size_t len = end ? (size_t)(end - line) : strlen(line);
        if (len > 66) {
            const char *file = line + 64;
            size_t fl;
            while (file < line + len && (*file == ' ' || *file == '*')) ++file;
            fl = (size_t)(line + len - file);
            while (fl && (file[fl - 1] == '\r' || file[fl - 1] == ' ')) --fl;
            if (fl == strlen(name) && !memcmp(file, name, fl)) {
                memcpy(out, line, 64);
                out[64] = 0;
                return 1;
            }
        }
        line = end ? end + 1 : NULL;
    }
    return 0;
}

SB5_UNUSED static int digest_file(const char *path, char out[65]) {
    size_t len = 0;
    unsigned char *data = read_file(path, &len), dig[32];
    int i;
    if (!data) return 0;
    sha256_compute(data, len, dig);
    free(data);
    for (i = 0; i < 32; ++i) sprintf(out + 2 * i, "%02x", dig[i]);
    return 1;
}

/* ---- installing -------------------------------------------------------- */

/* Player files: never overwritten by an update (relative, '/' separated). */
int Sb5PlayerOwned(const char *rel) {
    static const char *const exact[] = {
        "rom.cfg", "config.ini", "keybinds.ini", "logging.ini", "mods/preloaded/state.toml",
        "last_run_report.json", "netplay_identity.txt", "netplay_lan_lobby.txt", "installed-build.txt",
        "version.txt", NULL};
    static const char *const prefixes[] = {"saves/", "build/", "build-", "src/gen/", ".cache/", ".tools/",
                                           "screenshots/", NULL};
    const char *dot;
    int i;
    for (i = 0; exact[i]; ++i)
        if (!strcmp(rel, exact[i])) return 1;
    for (i = 0; prefixes[i]; ++i)
        if (!strncmp(rel, prefixes[i], strlen(prefixes[i]))) return 1;
    dot = strrchr(rel, '.');
    if (dot && (!strcmp(dot, ".sfc") || !strcmp(dot, ".smc") || !strcmp(dot, ".srm") || !strcmp(dot, ".sav")))
        return 1;
    return !strcmp(rel, "recomp/funcs.h");
}

static int copy_file(const char *from, const char *to) {
    size_t len = 0;
    unsigned char *data = read_file(from, &len);
    FILE *f;
    char tmp[PATH_CAP];
    if (!data) return 0;
    snprintf(tmp, sizeof(tmp), "%s.sb5new", to);
    f = fopen(tmp, "wb");
    if (!f) { free(data); return 0; }
    if (fwrite(data, 1, len, f) != len) { fclose(f); free(data); remove(tmp); return 0; }
    fclose(f);
    free(data);
#if defined(_WIN32)
    if (!MoveFileExA(tmp, to, MOVEFILE_REPLACE_EXISTING)) {
        /* A file still in use (an old updater): keep the new copy beside it. */
        return 0;
    }
#else
    {
        struct stat st;
        if (stat(from, &st) == 0) chmod(tmp, st.st_mode & 0777);
    }
    if (rename(tmp, to) != 0) return 0;
#endif
    return 1;
}

static int g_copied, g_kept, g_failed;

void Sb5CopyTree(const char *src, const char *dst, const char *rel) {
#if defined(_WIN32)
    char pattern[PATH_CAP];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    join(pattern, sizeof(pattern), src, "*");
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        const char *name = fd.cFileName;
        int dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    DIR *d = opendir(src);
    struct dirent *e;
    if (!d) return;
    while ((e = readdir(d))) {
        const char *name = e->d_name;
        char probe[PATH_CAP];
        int dir;
        join(probe, sizeof(probe), src, name);
        dir = is_dir(probe);
#endif
        char s[PATH_CAP], t[PATH_CAP], r[PATH_CAP];
        if (!strcmp(name, ".") || !strcmp(name, "..")) continue;
        join(s, sizeof(s), src, name);
        join(t, sizeof(t), dst, name);
        snprintf(r, sizeof(r), "%s%s%s", rel, rel[0] ? "/" : "", name);
        if (dir) {
            char rd[PATH_CAP + 2];
            snprintf(rd, sizeof(rd), "%s/", r);
            if (Sb5PlayerOwned(rd)) { ++g_kept; continue; }
            mkdirs(t);
            Sb5CopyTree(s, t, r);
        } else if (Sb5PlayerOwned(r) && file_exists(t)) {
            ++g_kept;
        } else if (copy_file(s, t)) {
            ++g_copied;
        } else {
            ++g_failed;
            say("  could not replace %s", r);
        }
#if defined(_WIN32)
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    }
    closedir(d);
#endif
}

SB5_UNUSED static void wait_for_pid(long pid) {
    if (pid <= 0) return;
    say("Waiting for the game to close...");
#if defined(_WIN32)
    {
        HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
        if (h) {
            WaitForSingleObject(h, 60000);
            CloseHandle(h);
        }
    }
#else
    {
        int i;
        for (i = 0; i < 600 && kill((pid_t)pid, 0) == 0; ++i) usleep(100000);
    }
#endif
}

SB5_UNUSED static void exe_dir(char *out, size_t cap) {
#if defined(_WIN32)
    char *slash;
    GetModuleFileNameA(NULL, out, (DWORD)cap);
    slash = strrchr(out, '\\');
    if (slash) *slash = 0;
#else
    ssize_t n = readlink("/proc/self/exe", out, cap - 1);
    char *slash;
    out[n > 0 ? n : 0] = 0;
    slash = strrchr(out, '/');
    if (slash) *slash = 0;
#endif
}

SB5_UNUSED static void launch(const char *path) {
#if defined(_WIN32)
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmd[PATH_CAP + 4];
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    snprintf(cmd, sizeof(cmd), "\"%s\"", path);
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, g_dir, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
#else
    if (fork() == 0) {
        setsid();
        execl(path, path, (char *)NULL);
        _exit(127);
    }
#endif
}

SB5_UNUSED static void pause_if_interactive(int launched) {
#if defined(_WIN32)
    if (!launched) {
        printf("\nPress Enter to close.");
        fflush(stdout);
        (void)getchar();
    }
#else
    (void)launched;
#endif
}

#ifndef SB5_UPDATER_NO_MAIN
int main(int argc, char **argv) {
    int check = 0, do_launch = 0, force = 0, from_temp = 0, i;
    long wait_pid = 0;
    const char *appimage = getenv("APPIMAGE");
    char path[PATH_CAP], json_path[PATH_CAP], sums_path[PATH_CAP], pkg_path[PATH_CAP], log_path[PATH_CAP];
    char local[64] = "0", expected[65], actual[65];
    unsigned char *json, *sums;
    size_t len;
    Release rel;

    exe_dir(g_dir, sizeof(g_dir));
    {
        /* Shipped in <install>/updater/: the install root is the folder with VERSION. */
        char probe[PATH_CAP];
        join(probe, sizeof(probe), g_dir, "VERSION");
        if (!file_exists(probe)) {
            char *a = strrchr(g_dir, '/'), *b = strrchr(g_dir, '\\');
            char *cut = a > b ? a : b;
            if (cut) *cut = 0;
        }
    }
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--check")) check = 1;
        else if (!strcmp(argv[i], "--launch")) do_launch = 1;
        else if (!strcmp(argv[i], "--force")) force = 1;
        else if (!strcmp(argv[i], "--yes")) {}
        else if (!strcmp(argv[i], "--from-temp")) from_temp = 1;
        else if (!strcmp(argv[i], "--dir") && i + 1 < argc) snprintf(g_dir, sizeof(g_dir), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--wait-pid") && i + 1 < argc) wait_pid = atol(argv[++i]);
        else if (!strcmp(argv[i], "--appimage") && i + 1 < argc) appimage = argv[++i];
        else {
            printf("usage: %s [--check] [--dir DIR] [--wait-pid PID] [--appimage PATH] [--launch] [--force]\n", argv[0]);
            return 2;
        }
    }
#if defined(_WIN32)
    {
        char t[MAX_PATH];
        GetTempPathA(sizeof(t), t);
        snprintf(g_tmp, sizeof(g_tmp), "%ssb5-update-%lu", t, (unsigned long)GetCurrentProcessId());
    }
#else
    snprintf(g_tmp, sizeof(g_tmp), "%s/sb5-update-%ld", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", (long)getpid());
#endif
    mkdirs(g_tmp);
#if defined(_WIN32)
    /* This executable is part of the package it installs, and Windows cannot
     * replace a running program: run the rest from a temporary copy. */
    if (!check && !from_temp) {
        char self[PATH_CAP], copy[PATH_CAP], cmd[PATH_CAP * 3];
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        int k;
        GetModuleFileNameA(NULL, self, sizeof(self));
        join(copy, sizeof(copy), g_tmp, "SuperBomberman5Updater.exe");
        if (CopyFileA(self, copy, FALSE)) {
            snprintf(cmd, sizeof(cmd), "\"%s\" --from-temp --dir \"%s\"", copy, g_dir);
            for (k = 1; k < argc; ++k) {
                if (!strcmp(argv[k], "--dir")) { ++k; continue; }
                strncat(cmd, " \"", sizeof(cmd) - strlen(cmd) - 1);
                strncat(cmd, argv[k], sizeof(cmd) - strlen(cmd) - 1);
                strncat(cmd, "\"", sizeof(cmd) - strlen(cmd) - 1);
            }
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                return 0;
            }
        }
    }
#else
    (void)from_temp;
#endif
    if (!check) {
        join(log_path, sizeof(log_path), g_dir, "updater.log");
        g_log = fopen(log_path, "a");
        if (g_log) {
            time_t now = time(NULL);
            fprintf(g_log, "---- %s", ctime(&now));
        }
        say("Super Bomberman 5 Recompiled updater");
    }

    join(path, sizeof(path), g_dir, "VERSION");
    if ((json = read_file(path, &len))) {
        snprintf(local, sizeof(local), "%s", (char *)json);
        local[strcspn(local, "\r\n ")] = 0;
        free(json);
    }
    join(json_path, sizeof(json_path), g_tmp, "latest.json");
    if (!download("https://api.github.com/repos/" SB5_REPO "/releases/latest", json_path) ||
        !(json = read_file(json_path, &len))) {
        say("Could not reach GitHub. Check your internet connection and try again.");
        if (!check) pause_if_interactive(0);
        return 1;
    }
    if (!Sb5ParseRelease((const char *)json, &rel)) {
        free(json);
        say("The latest release has no package for this platform.");
        if (!check) pause_if_interactive(0);
        return 1;
    }
    free(json);
    if (check) {
        printf("local=%s\nlatest=%s\n", local, rel.tag);
        return Sb5VersionNewer(rel.tag, local) ? 10 : 0;
    }
    say("Installed: %s    Latest: %s", local, rel.tag);
    if (!Sb5VersionNewer(rel.tag, local) && !force) {
        say("You already have the newest version.");
        if (do_launch) {
#if defined(_WIN32)
            join(path, sizeof(path), g_dir, SB5_SETUP_EXE);
            launch(path);
#else
            if (appimage) launch(appimage);
#endif
        }
        pause_if_interactive(do_launch);
        return 0;
    }
#if defined(_WIN32)
    join(path, sizeof(path), g_dir, ".git");
    if (is_dir(path) && !force) {
        say("This folder is a Git checkout; update it with git instead (or pass --force).");
        pause_if_interactive(0);
        return 1;
    }
#else
    if (!appimage) {
        say("Run the updater from the AppImage (or pass --appimage PATH).");
        return 1;
    }
#endif
    wait_for_pid(wait_pid);

    join(sums_path, sizeof(sums_path), g_tmp, "SHA256SUMS");
    join(pkg_path, sizeof(pkg_path), g_tmp, rel.package_name);
    say("Downloading %s...", rel.package_name);
    if (!download(rel.sums_url, sums_path) || !download(rel.package_url, pkg_path) ||
        !(sums = read_file(sums_path, &len))) {
        say("The download failed. Nothing was changed.");
        pause_if_interactive(0);
        return 1;
    }
    if (!Sb5ExpectedDigest((const char *)sums, rel.package_name, expected) || !digest_file(pkg_path, actual) ||
        strcmp(expected, actual)) {
        free(sums);
        say("The download does not match the release checksum. Nothing was changed.");
        pause_if_interactive(0);
        return 1;
    }
    free(sums);
    say("Checksum OK (%.16s...).", actual);

#if defined(_WIN32)
    {
        char stage[PATH_CAP], cmd[PATH_CAP * 2], top[PATH_CAP];
        char *dot;
        join(stage, sizeof(stage), g_tmp, "package");
        mkdirs(stage);
        snprintf(cmd, sizeof(cmd), "tar -xf \"%s\" -C \"%s\"", pkg_path, stage);
        if (run(cmd) != 0) {
            say("Could not unpack the package (tar). Nothing was changed.");
            pause_if_interactive(0);
            return 1;
        }
        /* superbomberman5-<v>-windows-x64.zip unpacks into one folder of that name. */
        snprintf(top, sizeof(top), "%s", rel.package_name);
        dot = strrchr(top, '.');
        if (dot) *dot = 0;
        join(path, sizeof(path), stage, top);
        if (!is_dir(path)) snprintf(path, sizeof(path), "%s", stage);
        say("Installing into %s...", g_dir);
        Sb5CopyTree(path, g_dir, "");
        say("%d files updated, %d player files kept, %d failed.", g_copied, g_kept, g_failed);
        if (g_failed) {
            say("Some files are in use. Close every game window and run the updater again.");
            pause_if_interactive(0);
            return 1;
        }
        say("Updated to %s. The launcher will ask for one Generate & rebuild to build the new version "
            "from your ROM.", rel.tag);
        join(path, sizeof(path), g_dir, SB5_SETUP_EXE);
        if (do_launch) launch(path);
        pause_if_interactive(do_launch);
    }
#else
    {
        if (!copy_file(pkg_path, appimage)) {
            say("Could not replace %s.", appimage);
            return 1;
        }
        chmod(appimage, 0755);
        say("Updated to %s. The launcher will ask for one Generate & rebuild to build the new version "
            "from your ROM.", rel.tag);
        if (do_launch) launch(appimage);
    }
#endif
    if (g_log) fclose(g_log);
    return 0;
}
#endif
