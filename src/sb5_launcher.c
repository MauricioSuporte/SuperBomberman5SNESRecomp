/* Super Bomberman 5 launcher additions: community links (Discord), the
 * updater button and the netplay policy. Transport, lobby, Discord sign-in
 * and the netplay barrier stay the framework's (snesrecomp + recomp-ui). */
#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE 1
#endif
#include "sb5_launcher.h"
#include "host_paths.h"
#include "mod_runtime.h"
#include "sb4_rom.h"
#include "discord_presence.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define SB5_UPDATER_NAME "SuperBomberman5Updater.exe"
#else
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#define SB5_UPDATER_NAME "SuperBomberman5Updater"
#endif

#if defined(RECOMP_LAUNCHER)
#include "recomp_launcher.h"
#endif

const char kSb5DiscordInvite[] = "https://discord.gg/kH54NCDQB4";
const char kSb5ReleasesUrl[] = "https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases";

void Sb5ForceDelaySync(void) {
    /* Lockstep (delay-sync) netplay. Rollback restores snapshots and replays
     * frames; with this LLE port two loopback peers forked (PPU/VRAM) after
     * forced mispredicts, while delay-sync stayed byte-identical through
     * Battle with every mod on. An explicit SNES_NET_MODE still wins. */
    /* Host-authoritative delay-sync: the host never waits for anyone and
     * sends each frame's inputs to the guests, who lag by their latency
     * instead of slowing the match (3+ player internet matches ran at
     * ~28 fps in plain lockstep). Every SB5 peer sets it. */
    if (!getenv("SNES_NET_HOST_AUTHORITY")) {
#if defined(_WIN32)
        _putenv_s("SNES_NET_HOST_AUTHORITY", "1");
#else
        setenv("SNES_NET_HOST_AUTHORITY", "1", 0);
#endif
    }
    if (getenv("SNES_NET_MODE")) return;
#if defined(_WIN32)
    _putenv_s("SNES_NET_MODE", "delay");
#else
    setenv("SNES_NET_MODE", "delay", 0);
#endif
}

int Sb5NetplayReady(char *reason, size_t cap) {
    if (snes_mod_runtime_feature_enabled_c("superbomberman5.bomber-kings", "bomber_kings") && !Sb4Rom()) {
        snprintf(reason, cap,
                 "Bomber Kings needs your own Super Bomberman 4 ROM: select it on the Mods page "
                 "(every player supplies their own copy).");
        return 0;
    }
    return 1;
}

int Sb5PrepareNetplay(int from_lobby, char *reason, size_t cap) {
    (void)from_lobby;
    Sb5ForceDelaySync();
    return Sb5NetplayReady(reason, cap);
}

/* ---- updater ----------------------------------------------------------- */

static int is_file(const char *p) {
#if defined(_WIN32)
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(p, &st) == 0 && S_ISREG(st.st_mode);
#endif
}

static void dir_of(char *path) {
    char *a = strrchr(path, '/'), *b = strrchr(path, '\\');
    char *s = a > b ? a : b;
    if (s) *s = 0;
}

/* The updater sits in updater/ at the install root (or at the root itself),
 * beside VERSION: next to the setup
 * host, one level above the rebuilt game in build/, or in the working
 * directory the setup host hands off with. */
int Sb5FindUpdater(char *updater, size_t cap, char *root, size_t root_cap) {
    char exe[2048], dirs[3][2048];
    int i;
    if (!snesrecomp_exe_path(exe, sizeof(exe))) exe[0] = 0;
    dir_of(exe);
    snprintf(dirs[0], sizeof(dirs[0]), "%s", exe);
    snprintf(dirs[1], sizeof(dirs[1]), "%s", exe);
    dir_of(dirs[1]);
#if defined(_WIN32)
    if (!GetCurrentDirectoryA(sizeof(dirs[2]), dirs[2])) dirs[2][0] = 0;
#else
    if (!getcwd(dirs[2], sizeof(dirs[2]))) dirs[2][0] = 0;
#endif
    for (i = 0; i < 3; ++i) {
        char candidate[2100], version[2100];
        if (!dirs[i][0]) continue;
        snprintf(version, sizeof(version), "%s/VERSION", dirs[i]);
        snprintf(candidate, sizeof(candidate), "%s/updater/%s", dirs[i], SB5_UPDATER_NAME);
        if (!is_file(candidate))
            snprintf(candidate, sizeof(candidate), "%s/%s", dirs[i], SB5_UPDATER_NAME);
        if (is_file(candidate) && is_file(version)) {
            snprintf(updater, cap, "%s", candidate);
            snprintf(root, root_cap, "%s", dirs[i]);
            return 1;
        }
    }
    return 0;
}

int Sb5StartUpdater(char *msg, size_t cap) {
    char updater[2100], root[2048];
    if (!Sb5FindUpdater(updater, sizeof(updater), root, sizeof(root))) {
        snprintf(msg, cap, "The updater was not found beside the game. Download the newest release from %s",
                 kSb5ReleasesUrl);
        return 0;
    }
#if defined(_WIN32)
    {
        char cmd[4600];
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        snprintf(cmd, sizeof(cmd), "\"%s\" --wait-pid %lu --dir \"%s\" --launch", updater,
                 (unsigned long)GetCurrentProcessId(), root);
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, root, &si, &pi)) {
            snprintf(msg, cap, "Could not start the updater (error %lu).", (unsigned long)GetLastError());
            return 0;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
#else
    {
        /* AppRun hides $APPIMAGE from the game (it anchored the game folder to the
         * .AppImage's folder) and passes the file as $SB5_APPIMAGE instead. */
        const char *appimage = getenv("SB5_APPIMAGE");
        if (!appimage || !appimage[0]) appimage = getenv("APPIMAGE");
        char pid[32];
        pid_t child;
        snprintf(pid, sizeof(pid), "%ld", (long)getpid());
        if (!appimage || !appimage[0]) {
            snprintf(msg, cap, "Updating needs the AppImage. Download the newest release from %s", kSb5ReleasesUrl);
            return 0;
        }
        child = fork();
        if (child < 0) {
            snprintf(msg, cap, "Could not start the updater.");
            return 0;
        }
        if (child == 0) {
            setsid();
            execl(updater, updater, "--wait-pid", pid, "--dir", root, "--appimage", appimage, "--launch",
                  (char *)NULL);
            _exit(127);
        }
    }
#endif
    snprintf(msg, cap, "Updater started; the game closes now.");
    return 1;
}

#if defined(RECOMP_LAUNCHER) && defined(RECOMP_LAUNCHER_HAS_GAME_LINKS)
static const RecompLauncherCGameLink kLinks[] = {
    {"Discord: online matches & news", kSb5DiscordInvite,
     "Find online matches and news about Super Bomberman 5 Recompiled.", 1},
    {"Release notes & downloads (GitHub)", kSb5ReleasesUrl, NULL, 0},
};

static int update_action(char *msg, size_t cap) { return Sb5StartUpdater(msg, cap); }
static int quiver_action(char *msg, size_t cap) {
    snprintf(msg, cap, "Close the game and use Update in Quiver Launcher's library.");
    return 0;
}
#endif

/* ---- version label and update check ------------------------------------ */

/* Read every frame by the launcher (GameInfo pointers), filled here and by
 * the background check. Each write is a whole short string into a buffer
 * that is never freed; a frame that reads mid-write shows one stale line. */
static char g_version_label[48];
static char g_action_note[320];
static char g_netplay_notice[320];
static char g_local_version[32];
static char g_check_updater[2100];
static char g_check_root[2048];

static void read_local_version(const char *root) {
    char path[2100];
    FILE *f;
    g_local_version[0] = 0;
    snprintf(path, sizeof(path), "%s/VERSION", root);
    f = fopen(path, "rb");
    if (f) {
        if (fgets(g_local_version, sizeof(g_local_version), f))
            g_local_version[strcspn(g_local_version, "\r\n ")] = 0;
        fclose(f);
    }
}

/* Runs `updater --check` (no window, no install) and parses its
 * "local=<v>\nlatest=v<x>" answer; exit code 10 means a newer release. */
static int run_update_check(char *latest, size_t cap) {
    char out[512];
    size_t used = 0;
    int code = -1;
    latest[0] = 0;
    out[0] = 0;
#if defined(_WIN32)
    {
        SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
        HANDLE rd = NULL, wr = NULL;
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char cmd[4400];
        DWORD got, exit_code = 1;
        if (!CreatePipe(&rd, &wr, &sa, 0)) return -1;
        SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = wr;
        si.hStdError = wr;
        si.hStdInput = NULL;
        snprintf(cmd, sizeof(cmd), "\"%s\" --check --dir \"%s\"", g_check_updater, g_check_root);
        if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(rd);
            CloseHandle(wr);
            return -1;
        }
        CloseHandle(wr);
        while (used + 1 < sizeof(out) &&
               ReadFile(rd, out + used, (DWORD)(sizeof(out) - 1 - used), &got, NULL) && got)
            used += got;
        out[used] = 0;
        CloseHandle(rd);
        WaitForSingleObject(pi.hProcess, 60000);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        code = (int)exit_code;
    }
#else
    {
        int fds[2], status = 0;
        pid_t child;
        ssize_t got;
        if (pipe(fds) != 0) return -1;
        child = fork();
        if (child < 0) { close(fds[0]); close(fds[1]); return -1; }
        if (child == 0) {
            dup2(fds[1], 1);
            dup2(fds[1], 2);
            close(fds[0]);
            close(fds[1]);
            execl(g_check_updater, g_check_updater, "--check", "--dir", g_check_root, (char *)NULL);
            _exit(127);
        }
        close(fds[1]);
        while (used + 1 < sizeof(out) && (got = read(fds[0], out + used, sizeof(out) - 1 - used)) > 0)
            used += (size_t)got;
        out[used] = 0;
        close(fds[0]);
        if (waitpid(child, &status, 0) == child && WIFEXITED(status)) code = WEXITSTATUS(status);
    }
#endif
    {
        const char *p = strstr(out, "latest=");
        if (p) {
            p += 7;
            if (*p == 'v' || *p == 'V') ++p;
            snprintf(latest, cap, "%.*s", (int)strcspn(p, "\r\n "), p);
        }
    }
    return code;
}

static void apply_check_result(int code, const char *latest) {
    const char *v = g_local_version[0] ? g_local_version : "?";
    if (code == 10 && latest[0]) {
        snprintf(g_action_note, sizeof(g_action_note),
                 "Installed: v%s. New version v%s is available! Update to play online "
                 "(online rooms only match players on the same version).", v, latest);
        snprintf(g_netplay_notice, sizeof(g_netplay_notice),
                 "You have v%s, but v%s is out: online players are on the new version, so their "
                 "rooms do not appear here and yours would not appear to them. Hosting is disabled "
                 "until you update (main page: Update game / Atualizar jogo).", v, latest);
    } else if (code == 0) {
        snprintf(g_action_note, sizeof(g_action_note),
                 "Installed: v%s (up to date). Online play needs the same version on every player.", v);
    } else {
        snprintf(g_action_note, sizeof(g_action_note),
                 "Installed: v%s. Could not check for updates. Online play needs the same version on "
                 "every player.", v);
    }
}

#if defined(_WIN32)
static DWORD WINAPI update_check_thread(LPVOID arg) {
#else
static void *update_check_thread(void *arg) {
#endif
    char latest[64];
    int code;
    (void)arg;
    code = run_update_check(latest, sizeof(latest));
    apply_check_result(code, latest);
#if defined(_WIN32)
    return 0;
#else
    return NULL;
#endif
}

static void start_update_check(void) {
    if (getenv("SB5_NO_UPDATE_CHECK")) return;
#if defined(_WIN32)
    {
        HANDLE t = CreateThread(NULL, 0, update_check_thread, NULL, 0, NULL);
        if (t) CloseHandle(t);
    }
#else
    {
        pthread_t t;
        if (pthread_create(&t, NULL, update_check_thread, NULL) == 0) pthread_detach(t);
    }
#endif
}

void Sb5ConfigureLauncher(struct RecompLauncherCGameInfo *gi) {
    Sb5DiscordConfigureLauncher(gi); /* Rich Presence + Discord invites */
#if defined(RECOMP_LAUNCHER) && defined(RECOMP_LAUNCHER_HAS_GAME_LINKS)
    char updater[2100], root[2048];
    const int have_updater = Sb5FindUpdater(updater, sizeof(updater), root, sizeof(root));
    gi->game_links = kLinks;
    gi->num_game_links = (int)(sizeof(kLinks) / sizeof(kLinks[0]));
    if (have_updater) read_local_version(root);
#if defined(SNES_GAME_VERSION)
    if (!g_local_version[0]) snprintf(g_local_version, sizeof(g_local_version), "%s", SNES_GAME_VERSION);
#endif
#if defined(RECOMP_LAUNCHER_HAS_NETPLAY_NOTICE)
    if (g_local_version[0]) {
        snprintf(g_version_label, sizeof(g_version_label), "v%s", g_local_version);
        gi->game_version_label = g_version_label;
    }
    gi->netplay_notice = g_netplay_notice;
#endif
    if (have_updater) {
        char quiver[2100];
        snprintf(quiver, sizeof(quiver), "%s/installed-build.txt", root);
        if (is_file(quiver)) {
            /* Quiver Launcher owns this install's updates. */
            gi->game_action_label = "Update with Quiver Launcher";
            gi->game_action = quiver_action;
        } else {
            gi->game_action_label = "Update game / Atualizar jogo";
            gi->game_action = update_action;
        }
        snprintf(g_action_note, sizeof(g_action_note),
                 "Installed: v%s. Checking for updates...", g_local_version[0] ? g_local_version : "?");
        gi->game_action_note = g_action_note;
        snprintf(g_check_updater, sizeof(g_check_updater), "%s", updater);
        snprintf(g_check_root, sizeof(g_check_root), "%s", root);
        start_update_check();
    }
#else
    (void)gi;
#endif
}
