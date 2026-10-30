/* Boss Rush ranking over a Discord webhook. See boss_rush_net.h. */
#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE 1
#endif
#include "boss_rush_net.h"
#include "host_paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/* ---- configuration ---------------------------------------------------- */
static bool cfg_read;
static char cfg_webhook[512], cfg_board[32];

static void trim(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = 0;
    size_t lead = strspn(s, " \t");
    memmove(s, s + lead, strlen(s + lead) + 1);
}
static bool valid_webhook(const char *u) {
    if (!strncmp(u, "https://discord.com/api/" "webhooks/", 33) ||
        !strncmp(u, "https://discordapp.com/api/" "webhooks/", 36) ||
        !strncmp(u, "https://ptb.discord.com/api/" "webhooks/", 37) ||
        !strncmp(u, "https://canary.discord.com/api/" "webhooks/", 40))
        return !strpbrk(u, " \"'\\\r\n?#");
    /* Local test server only (tests/boss_rush_ranking_runtime.py). */
    return getenv("SB5_BOSS_RUSH_TEST_SERVER") && !strncmp(u, "http://127.0.0.1:", 17) && !strpbrk(u, " \"'\\\r\n?#");
}
static bool valid_id(const char *s) {
    size_t n = strlen(s);
    return n >= 5 && n < sizeof(cfg_board) && strspn(s, "0123456789") == n;
}
static void read_config(void) {
    if (cfg_read) return;
    cfg_read = true;
    char path[1024], line[700];
    const char *w = getenv("SB5_BOSS_RUSH_WEBHOOK"), *b = getenv("SB5_BOSS_RUSH_BOARD");
    if (w && b) {
        snprintf(cfg_webhook, sizeof(cfg_webhook), "%s", w);
        snprintf(cfg_board, sizeof(cfg_board), "%s", b);
    } else if (snesrecomp_game_root_dir(path, sizeof(path))) {
        size_t n = strlen(path);
        snprintf(path + n, sizeof(path) - n, "%sboss_rush_ranking.cfg", n && (path[n - 1] == '/' || path[n - 1] == '\\') ? "" : "/");
        FILE *f = fopen(path, "rb");
        if (f) {
            while (fgets(line, sizeof(line), f)) {
                trim(line);
                if (!strncmp(line, "webhook=", 8)) { snprintf(cfg_webhook, sizeof(cfg_webhook), "%s", line + 8); trim(cfg_webhook); }
                else if (!strncmp(line, "board=", 6)) { snprintf(cfg_board, sizeof(cfg_board), "%s", line + 6); trim(cfg_board); }
            }
            fclose(f);
        }
    }
    size_t n = strlen(cfg_webhook);
    while (n && cfg_webhook[n - 1] == '/') cfg_webhook[--n] = 0;
    if (!valid_webhook(cfg_webhook) || !valid_id(cfg_board)) cfg_webhook[0] = cfg_board[0] = 0;
    fprintf(stderr, "[boss-rush] online ranking %s\n", cfg_webhook[0] ? "configured" : "not configured");
}
bool BrNetConfigured(void) { read_config(); return cfg_webhook[0] != 0; }

/* ---- lock ------------------------------------------------------------- */
#if defined(_WIN32)
static INIT_ONCE lock_once = INIT_ONCE_STATIC_INIT;
static CRITICAL_SECTION lock_cs;
static BOOL CALLBACK lock_init(PINIT_ONCE o, PVOID p, PVOID *c) { (void)o; (void)p; (void)c; InitializeCriticalSection(&lock_cs); return TRUE; }
static void lock(void) { InitOnceExecuteOnce(&lock_once, lock_init, NULL, NULL); EnterCriticalSection(&lock_cs); }
static void unlock(void) { LeaveCriticalSection(&lock_cs); }
#else
static pthread_mutex_t lock_mx = PTHREAD_MUTEX_INITIALIZER;
static void lock(void) { pthread_mutex_lock(&lock_mx); }
static void unlock(void) { pthread_mutex_unlock(&lock_mx); }
#endif

/* ---- curl ------------------------------------------------------------- */
static void temp_path(char *out, size_t cap, const char *tag) {
    static unsigned counter;
#if defined(_WIN32)
    /* Short (8.3) form: curl.exe reads its arguments in the system code
     * page, and a user folder like C:\\Users\\Jo\xc3\xa3o would not survive it. */
    char dir[MAX_PATH], shortdir[MAX_PATH];
    DWORD n = GetTempPathA(sizeof(dir), dir);
    if (!n || n >= sizeof(dir)) strcpy(dir, ".\\");
    n = GetShortPathNameA(dir, shortdir, sizeof(shortdir));
    if (n && n < sizeof(shortdir)) strcpy(dir, shortdir);
    snprintf(out, cap, "%ssb5br_%lu_%u_%s", dir, (unsigned long)GetCurrentProcessId(), ++counter, tag);
#else
    const char *dir = getenv("TMPDIR");
    snprintf(out, cap, "%s/sb5br_%ld_%u_%s", dir && dir[0] ? dir : "/tmp", (long)getpid(), ++counter, tag);
#endif
}
/* Runs one request; returns the HTTP status (0 on failure) and the body. */
static int http(const char *method, const char *url, const char *body, char *out, size_t cap) {
    char cfg_path[1024], body_path[1024];
    out[0] = 0;
    temp_path(cfg_path, sizeof(cfg_path), "req.txt");
    body_path[0] = 0;
    FILE *f;
    if (body) {
        temp_path(body_path, sizeof(body_path), "body.json");
        f = fopen(body_path, "wb");
        if (!f) return 0;
        fwrite(body, 1, strlen(body), f);
        fclose(f);
    }
    /* The URL (with the webhook token) goes through a config file, never the
     * command line, so it does not show in process listings. */
    f = fopen(cfg_path, "wb");
    if (!f) { if (body) remove(body_path); return 0; }
    fprintf(f, "url = \"%s\"\nrequest = \"%s\"\nsilent\nshow-error\nmax-time = 12\nconnect-timeout = 6\n"
               "header = \"User-Agent: DiscordBot (https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp, 1)\"\nwrite-out = \"\\n%%{http_code}\"\n",
            url, method);
    if (body) {
        fprintf(f, "header = \"Content-Type: application/json\"\ndata-binary = \"@");
        for (const char *p = body_path; *p; ++p) fputs(*p == '\\' ? "\\\\" : (char[2]){*p, 0}, f);
        fputs("\"\n", f);
    }
    fclose(f);
    size_t used = 0;
#if defined(_WIN32)
    {
        SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
        HANDLE rd = NULL, wr = NULL;
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char cmd[1200];
        DWORD got;
        if (CreatePipe(&rd, &wr, &sa, 0)) {
            SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = wr;
            si.hStdError = NULL;
            si.hStdInput = NULL;
            snprintf(cmd, sizeof(cmd), "curl.exe -K \"%s\"", cfg_path);
            if (CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(wr); wr = NULL;
                while (used + 1 < cap && ReadFile(rd, out + used, (DWORD)(cap - 1 - used), &got, NULL) && got) used += got;
                WaitForSingleObject(pi.hProcess, 30000);
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            if (wr) CloseHandle(wr);
            CloseHandle(rd);
        }
    }
#else
    {
        int fds[2];
        if (pipe(fds) == 0) {
            pid_t child = fork();
            if (child == 0) {
                dup2(fds[1], 1);
                close(fds[0]);
                close(fds[1]);
                execlp("curl", "curl", "-K", cfg_path, (char *)NULL);
                _exit(127);
            }
            close(fds[1]);
            if (child > 0) {
                ssize_t got;
                while (used + 1 < cap && (got = read(fds[0], out + used, cap - 1 - used)) > 0) used += (size_t)got;
                int status;
                waitpid(child, &status, 0);
            }
            close(fds[0]);
        }
    }
#endif
    out[used] = 0;
    remove(cfg_path);
    if (body) remove(body_path);
    /* write-out appends "\n<status>". */
    char *nl = strrchr(out, '\n');
    if (!nl) return 0;
    int code = atoi(nl + 1);
    *nl = 0;
    return code;
}

/* ---- jobs ------------------------------------------------------------- */
static BrBoard board;
static int board_state;          /* 1 ok, 0 none, -1 failed */
static bool fetching;
static time_t fetched_at;
static bool submit_pending, submitting;
static BrEntry submit_entry;
static int submit_state = BR_SUBMIT_IDLE;
static unsigned submit_rank;
static bool worker_running;

static char *answer(void) {
    static char *buf;
    if (!buf) buf = malloc(1 << 17);
    return buf;
}
static bool fetch_board(BrBoard *out) {
    char url[640];
    char *buf = answer();
    if (!buf) return false;
    snprintf(url, sizeof(url), "%s/messages/%s", cfg_webhook, cfg_board);
    int code = http("GET", url, NULL, buf, 1 << 17);
    if (code != 200) {
        fprintf(stderr, "[boss-rush] ranking fetch failed (HTTP %d)\n", code);
        return false;
    }
    /* A board that was never written yet has no data: start empty. */
    if (!BrParseBoard(buf, out)) memset(out, 0, sizeof(*out));
    return true;
}
static void run_submit(const BrEntry *e) {
    BrBoard b;
    int state = BR_SUBMIT_FAILED;
    unsigned rank = 0;
    if (fetch_board(&b)) {
        rank = BrInsert(&b, e);
        if (!rank) {
            state = BR_SUBMIT_NOT_TOP;
        } else {
            static char payload[1 << 15];
            char url[640];
            char *buf = answer();
            int code = 0;
            if (BrBoardPayload(&b, payload, sizeof(payload))) {
                snprintf(url, sizeof(url), "%s/messages/%s", cfg_webhook, cfg_board);
                code = http("PATCH", url, payload, buf, 1 << 17);
            }
            if (code == 200) {
                state = BR_SUBMIT_RANKED;
                if (BrNoticePayload(&b, e->players, rank, payload, sizeof(payload))) {
                    snprintf(url, sizeof(url), "%s?wait=true", cfg_webhook);
                    int n = http("POST", url, payload, buf, 1 << 17);
                    if (n != 200) fprintf(stderr, "[boss-rush] notification failed (HTTP %d)\n", n);
                }
            } else {
                fprintf(stderr, "[boss-rush] ranking update failed (HTTP %d)\n", code);
            }
        }
        lock();
        if (state != BR_SUBMIT_FAILED) { board = b; board_state = 1; fetched_at = time(NULL); }
        unlock();
    }
    lock();
    submit_state = state;
    submit_rank = state == BR_SUBMIT_RANKED ? rank : 0;
    unlock();
    fprintf(stderr, "[boss-rush] ranking submit: %s (rank %u)\n",
            state == BR_SUBMIT_RANKED ? "top 5" : state == BR_SUBMIT_NOT_TOP ? "not top 5" : "failed", rank);
}

#if defined(_WIN32)
static DWORD WINAPI worker(LPVOID arg)
#else
static void *worker(void *arg)
#endif
{
    (void)arg;
    for (;;) {
        bool do_fetch = false, do_submit = false;
        BrEntry e;
        lock();
        if (submit_pending) { submit_pending = false; submitting = true; e = submit_entry; do_submit = true; }
        else if (fetching) do_fetch = true;
        else { worker_running = false; unlock(); break; }
        unlock();
        if (do_submit) {
            run_submit(&e);
            lock(); submitting = false; unlock();
        } else if (do_fetch) {
            BrBoard b;
            bool ok = fetch_board(&b);
            lock();
            if (ok) { board = b; board_state = 1; } else board_state = -1;
            fetched_at = time(NULL);
            fetching = false;
            unlock();
        }
    }
#if defined(_WIN32)
    return 0;
#else
    return NULL;
#endif
}
/* Called with the lock held. */
static void wake_worker(void) {
    if (worker_running) return;
    worker_running = true;
#if defined(_WIN32)
    HANDLE t = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (t) CloseHandle(t); else worker_running = false;
#else
    pthread_t t;
    if (pthread_create(&t, NULL, worker, NULL) == 0) pthread_detach(t); else worker_running = false;
#endif
}

void BrNetRefresh(bool force) {
    if (!BrNetConfigured()) return;
    lock();
    time_t now = time(NULL);
    if (!fetching && !submitting && !submit_pending && (force || !fetched_at || now - fetched_at >= 60)) {
        fetching = true;
        wake_worker();
    }
    unlock();
}
int BrNetBoard(BrBoard *out) {
    if (!BrNetConfigured()) return -1;
    lock();
    int s = board_state;
    if (s == 1 && out) *out = board;
    unlock();
    return s;
}
void BrNetSubmit(const BrEntry *e) {
    if (!BrNetConfigured()) { submit_state = BR_SUBMIT_FAILED; return; }
    lock();
    submit_entry = *e;
    submit_pending = true;
    submit_state = BR_SUBMIT_BUSY;
    submit_rank = 0;
    wake_worker();
    unlock();
}
int BrNetSubmitState(unsigned *rank) {
    lock();
    int s = submit_state;
    if (rank) *rank = submit_rank;
    unlock();
    return s;
}
void BrNetSubmitClear(void) {
    lock();
    if (submit_state != BR_SUBMIT_BUSY) { submit_state = BR_SUBMIT_IDLE; submit_rank = 0; }
    unlock();
}
