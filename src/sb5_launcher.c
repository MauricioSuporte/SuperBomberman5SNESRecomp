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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define SB5_UPDATER_NAME "SuperBomberman5Updater.exe"
#else
#include <sys/stat.h>
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
        const char *appimage = getenv("APPIMAGE");
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

void Sb5ConfigureLauncher(struct RecompLauncherCGameInfo *gi) {
#if defined(RECOMP_LAUNCHER) && defined(RECOMP_LAUNCHER_HAS_GAME_LINKS)
    char updater[2100], root[2048];
    gi->game_links = kLinks;
    gi->num_game_links = (int)(sizeof(kLinks) / sizeof(kLinks[0]));
    if (Sb5FindUpdater(updater, sizeof(updater), root, sizeof(root))) {
        char quiver[2100];
        snprintf(quiver, sizeof(quiver), "%s/installed-build.txt", root);
        if (is_file(quiver)) {
            /* Quiver Launcher owns this install's updates. */
            gi->game_action_label = "Update with Quiver Launcher";
            gi->game_action_note = "Installed by Quiver Launcher: update it there. Online play needs the newest "
                                   "version on every player.";
            gi->game_action = quiver_action;
        } else {
            gi->game_action_label = "Update game / Atualizar jogo";
            gi->game_action_note = "Online play needs the newest version on every player.";
            gi->game_action = update_action;
        }
    }
#else
    (void)gi;
#endif
}
