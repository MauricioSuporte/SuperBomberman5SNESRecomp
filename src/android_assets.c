/* Android first-run setup (built only for __ANDROID__).
 *
 * The desktop host expects its data (mods/, assets/, rom_identity.txt) beside
 * the executable and keeps config.ini, rom.cfg and saves/ there too. On
 * Android the "executable" is app_process and the APK is read-only, so:
 *
 *  1. The game folder is the app's external files directory
 *     (Android/data/<package>/files): writable by the app, reachable with a
 *     file manager or `adb push`, so the player can drop the ROM there.
 *  2. The bundled data listed in the APK asset sb5/files.txt is copied there
 *     whenever the APK version changes (the player's own files -- config,
 *     saves, mod state, ROMs -- are never in that list and are kept).
 *  3. APPIMAGE points at a file name inside that folder. The runner's path
 *     helpers already use $APPIMAGE as "the user-visible file the game folder
 *     is anchored to" (host_paths.c, keybinds.c), which is exactly what is
 *     needed here, without changing the framework. */
#ifdef __ANDROID__

#include "android_assets.h"
#include "android_java.h"

#include <pthread.h>
#include <time.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_system.h>
#include <android/log.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "SB5", __VA_ARGS__)

static int mkdirs_for(const char *path) {
    char tmp[1024];
    if (snprintf(tmp, sizeof tmp, "%s", path) >= (int)sizeof tmp) return 0;
    for (char *p = tmp + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, 0775) != 0 && errno != EEXIST) return 0;
        *p = '/';
    }
    return 1;
}

static char *read_asset(const char *name, size_t *size) {
    SDL_RWops *rw = SDL_RWFromFile(name, "rb");
    if (!rw) return NULL;
    Sint64 n = SDL_RWsize(rw);
    char *buf = n >= 0 ? malloc((size_t)n + 1) : NULL;
    if (buf && SDL_RWread(rw, buf, 1, (size_t)n) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    SDL_RWclose(rw);
    if (buf) {
        buf[n] = '\0';
        *size = (size_t)n;
    }
    return buf;
}

static int write_file(const char *path, const char *data, size_t size) {
    if (!mkdirs_for(path)) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(data, 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

/* First line of files.txt is "version <v>"; the copy is redone only when it
 * differs from the stamp left by the previous extraction. */
static int extract_bundle(const char *root) {
    size_t list_size = 0;
    char *list = read_asset("sb5/files.txt", &list_size);
    if (!list) {
        LOG("no bundled sb5/files.txt in the APK");
        return 0;
    }
    char stamp_path[1024], *stamp = NULL;
    snprintf(stamp_path, sizeof stamp_path, "%s/.sb5-bundle", root);
    FILE *f = fopen(stamp_path, "rb");
    if (f) {
        char line[128] = {0};
        if (fgets(line, sizeof line, f)) stamp = strdup(line);
        fclose(f);
    }
    char *eol = strchr(list, '\n');
    size_t head = eol ? (size_t)(eol - list + 1) : list_size;
    if (stamp && strlen(stamp) == head && memcmp(stamp, list, head) == 0) {
        free(stamp);
        free(list);
        return 1;
    }
    free(stamp);

    int copied = 0, ok = 1;
    for (char *line = eol ? eol + 1 : list + list_size; *line;) {
        char *next = strchr(line, '\n');
        if (next) *next = '\0';
        if (*line) {
            char asset[1024], dst[1024];
            size_t n = 0;
            snprintf(asset, sizeof asset, "sb5/%s", line);
            snprintf(dst, sizeof dst, "%s/%s", root, line);
            char *data = read_asset(asset, &n);
            if (data && write_file(dst, data, n)) {
                ++copied;
            } else {
                LOG("could not extract %s", line);
                ok = 0;
            }
            free(data);
        }
        if (!next) break;
        line = next + 1;
    }
    if (ok) write_file(stamp_path, list, head);
    LOG("extracted %d bundled files to %s", copied, root);
    free(list);
    return ok;
}

/* recomp-net's browser handoff (rnet_open_url.c), linked into libmain.so. */
int rnet_open_url(const char *url);

/* Self-test (CI emulator only, tools/android-emulator-test.sh): runs the
 * lines of selftest.txt in the game folder and logs each result.
 *   http METHOD URL [BODY]   HTTPS through SB5Activity (Boss Rush, updates)
 *   openurl URL              Discord sign-in browser handoff (recomp-net)
 *   install URL              in-place update through the system installer
 * Never present in normal use. */
static char s_selftest[4096];
static void *selftest_thread(void *arg) {
    (void)arg;
    struct timespec ts = {6, 0};   /* let the launcher come up first */
    nanosleep(&ts, NULL);
    for (char *line = strtok(s_selftest, "\n"); line; line = strtok(NULL, "\n")) {
        char method[16] = "", url[1024] = "";
        static char out[1 << 16];
        int used = 0;
        if (sscanf(line, "http %15s %1023s %n", method, url, &used) >= 2) {
            const char *body = used > 0 && line[used] ? line + used : NULL;
            const int st = Sb5AndroidHttp(method, url, body, out, sizeof(out));
            LOG("selftest http %s status=%d bytes=%zu body=%.80s", method, st, strlen(out), out);
        } else if (sscanf(line, "openurl %1023s", url) == 1) {
            LOG("selftest openurl ok=%d", rnet_open_url(url));
        } else if (sscanf(line, "install %1023s", url) == 1) {
            char msg[320] = "";
            const bool ok = Sb5AndroidInstallApk(url, msg, sizeof(msg));
            LOG("selftest install started=%d msg=%s", ok, msg);
        }
        ts.tv_sec = 3;
        nanosleep(&ts, NULL);
    }
    return NULL;
}

static void start_selftest(const char *root) {
    char path[1024];
    snprintf(path, sizeof path, "%s/selftest.txt", root);
    FILE *f = fopen(path, "rb");
    if (!f) return;
    const size_t n = fread(s_selftest, 1, sizeof(s_selftest) - 1, f);
    fclose(f);
    s_selftest[n] = 0;
    LOG("selftest: %zu bytes of commands", n);
    pthread_t t;
    if (pthread_create(&t, NULL, selftest_thread, NULL) == 0) pthread_detach(t);
}

void Sb5AndroidPrepare(void) {
    /* Before anything initialises SDL's joystick layer (the launcher does it
     * first): the accelerometer must not appear as a controller. */
    SDL_SetHintWithPriority(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0", SDL_HINT_OVERRIDE);
    /* Log what SDL lists as joysticks (logcat tag SB5): a phone's sensors
     * have shown up here and been taken for controllers. */
    const char *ext = SDL_AndroidGetExternalStoragePath();
    char report[1024];
    FILE *rf = NULL;
    if (ext) {
        snprintf(report, sizeof report, "%s/joysticks.txt", ext);
        rf = fopen(report, "w");
    }
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) == 0) {
        const int n = SDL_NumJoysticks();
        LOG("joysticks: %d", n);
        if (rf) fprintf(rf, "joysticks: %d\n", n);
        for (int i = 0; i < n; ++i) {
            SDL_Joystick *js = SDL_JoystickOpen(i);
            LOG("joystick #%d '%s' controller=%d buttons=%d axes=%d hats=%d", i,
                SDL_JoystickNameForIndex(i) ? SDL_JoystickNameForIndex(i) : "?",
                SDL_IsGameController(i), js ? SDL_JoystickNumButtons(js) : -1,
                js ? SDL_JoystickNumAxes(js) : -1, js ? SDL_JoystickNumHats(js) : -1);
            if (rf)
                fprintf(rf, "#%d '%s' controller=%d buttons=%d axes=%d hats=%d\n", i,
                        SDL_JoystickNameForIndex(i) ? SDL_JoystickNameForIndex(i) : "?",
                        SDL_IsGameController(i), js ? SDL_JoystickNumButtons(js) : -1,
                        js ? SDL_JoystickNumAxes(js) : -1, js ? SDL_JoystickNumHats(js) : -1);
            if (js) SDL_JoystickClose(js);
        }
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    }
    if (rf) fclose(rf);
    const char *root = SDL_AndroidGetExternalStoragePath();
    if (!root) {
        LOG("no external storage path: %s", SDL_GetError());
        return;
    }
    extract_bundle(root);
    /* recomp-ui's launcher resolves its fonts/images against the internal
     * files folder (launcher_imgui.cpp asset()), so the bundle goes there too. */
    const char *internal = SDL_AndroidGetInternalStoragePath();
    if (internal) extract_bundle(internal);
    char anchor[1024];
    snprintf(anchor, sizeof anchor, "%s/SuperBomberman5Recomp", root);
    setenv("APPIMAGE", anchor, 1);
    if (chdir(root) != 0) LOG("chdir(%s) failed", root);
    start_selftest(root);
}

#endif /* __ANDROID__ */
