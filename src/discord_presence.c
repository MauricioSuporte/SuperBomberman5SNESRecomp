/* Discord Rich Presence and netplay invites over Discord's local RPC pipe.
 * See discord_presence.h. Everything runs on the main thread from two ticks:
 * the launcher's per-frame external-join poll and the emulated frame. I/O is
 * non-blocking; with Discord closed each tick costs one failed open every
 * few seconds. */
#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE 1
#endif
#include "discord_presence.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#if defined(RECOMP_LAUNCHER)
#include "recomp_launcher.h"
#endif
#if defined(SNESRECOMP_NET)
#include "recomp_net/lobby_client.h"
#include "snes_netplay.h"
#endif

/* Discord application "Super Bomberman 5 Recomp" (Developer Portal). The id
 * is public: it is what every Rich Presence client sends. */
#define SB5_DISCORD_APP_ID "1562965215291904100"
/* Rich Presence art asset key uploaded in the Developer Portal. */
#define SB5_DISCORD_LARGE_IMAGE "logo"

#ifndef SNES_GAME_VERSION
#define SNES_GAME_VERSION "dev"
#endif

extern unsigned char g_ram[];

enum { OP_HANDSHAKE = 0, OP_FRAME = 1, OP_CLOSE = 2, OP_PING = 3, OP_PONG = 4 };
enum { RX_CAP = 64 * 1024 };

typedef struct {
    int enabled;          /* -1 unknown, 0 off (SB5_DISCORD=0), 1 on */
#if defined(_WIN32)
    HANDLE pipe;
#else
    int sock;
#endif
    int connected;        /* pipe open */
    int ready;            /* READY dispatch received */
    uint64_t next_try_ms;
    uint64_t last_send_ms;
    unsigned nonce;
    unsigned char rx[RX_CAP];
    size_t rx_len;
    char sent_activity[1536]; /* last activity JSON sent */
    char want_activity[1536];
    uint64_t category_since; /* unix seconds the current category started */
    char category[32];
    char join_secret[128];   /* published secret, "" none */
    char pending_join[48];   /* lobby id from an accepted invite */
    int registered;
    int room_has_password;   /* cached from the lobby list */
    char room_pw_checked[48];
} DiscordState;

static DiscordState g_dc = {
    .enabled = -1,
#if defined(_WIN32)
    .pipe = INVALID_HANDLE_VALUE,
#else
    .sock = -1,
#endif
};

static uint64_t now_ms(void)
{
#if defined(_WIN32)
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
#endif
}

static unsigned long self_pid(void)
{
#if defined(_WIN32)
    return (unsigned long)GetCurrentProcessId();
#else
    return (unsigned long)getpid();
#endif
}

static int dc_enabled(void)
{
    if (g_dc.enabled < 0) {
        const char *v = getenv("SB5_DISCORD");
        g_dc.enabled = (v && v[0] == '0') ? 0 : 1;
    }
    return g_dc.enabled;
}

/* ---- transport ------------------------------------------------------- */

static void dc_close(void)
{
#if defined(_WIN32)
    if (g_dc.pipe != INVALID_HANDLE_VALUE) CloseHandle(g_dc.pipe);
    g_dc.pipe = INVALID_HANDLE_VALUE;
#else
    if (g_dc.sock >= 0) close(g_dc.sock);
    g_dc.sock = -1;
#endif
    if (g_dc.connected)
        fprintf(stderr, "discord: disconnected\n");
    g_dc.connected = 0;
    g_dc.ready = 0;
    g_dc.rx_len = 0;
    g_dc.sent_activity[0] = '\0';
    g_dc.next_try_ms = now_ms() + 10000u;
}

static int dc_write(const void *buf, size_t len)
{
#if defined(_WIN32)
    DWORD wrote = 0;
    if (!WriteFile(g_dc.pipe, buf, (DWORD)len, &wrote, NULL) || wrote != (DWORD)len)
        return -1;
    return 0;
#else
    const unsigned char *p = (const unsigned char *)buf;
    while (len) {
        ssize_t n = send(g_dc.sock, p, len, MSG_NOSIGNAL);
        if (n < 0 && (errno == EAGAIN || errno == EINTR)) continue;
        if (n <= 0) return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
#endif
}

/* Appends what is available without blocking. -1 on a closed pipe. */
static int dc_read_available(void)
{
#if defined(_WIN32)
    DWORD avail = 0, got = 0;
    if (!PeekNamedPipe(g_dc.pipe, NULL, 0, NULL, &avail, NULL)) return -1;
    if (!avail) return 0;
    if (avail > RX_CAP - g_dc.rx_len) avail = (DWORD)(RX_CAP - g_dc.rx_len);
    if (!avail) return -1; /* a frame larger than the buffer: give up */
    if (!ReadFile(g_dc.pipe, g_dc.rx + g_dc.rx_len, avail, &got, NULL)) return -1;
    g_dc.rx_len += got;
    return 0;
#else
    for (;;) {
        ssize_t n;
        if (g_dc.rx_len >= RX_CAP) return -1;
        n = recv(g_dc.sock, g_dc.rx + g_dc.rx_len, RX_CAP - g_dc.rx_len, MSG_DONTWAIT);
        if (n > 0) { g_dc.rx_len += (size_t)n; continue; }
        if (n == 0) return -1;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        if (errno == EINTR) continue;
        return -1;
    }
#endif
}

static int dc_send(uint32_t op, const char *json)
{
    size_t len = strlen(json);
    unsigned char hdr[8];
    hdr[0] = (unsigned char)op; hdr[1] = (unsigned char)(op >> 8);
    hdr[2] = (unsigned char)(op >> 16); hdr[3] = (unsigned char)(op >> 24);
    hdr[4] = (unsigned char)len; hdr[5] = (unsigned char)(len >> 8);
    hdr[6] = (unsigned char)(len >> 16); hdr[7] = (unsigned char)(len >> 24);
    if (dc_write(hdr, 8) || dc_write(json, len)) {
        dc_close();
        return -1;
    }
    return 0;
}

static int dc_open(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
#if defined(_WIN32)
        wchar_t name[64];
        HANDLE h;
        swprintf(name, 64, L"\\\\.\\pipe\\discord-ipc-%d", i);
        h = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            g_dc.pipe = h;
            return 0;
        }
#else
        static const char *const subdirs[] = {"", "app/com.discordapp.Discord/",
                                               "snap.discord/", "snap.discord-canary/"};
        const char *base = getenv("XDG_RUNTIME_DIR");
        size_t d;
        if (!base || !base[0]) base = getenv("TMPDIR");
        if (!base || !base[0]) base = "/tmp";
        for (d = 0; d < sizeof(subdirs) / sizeof(subdirs[0]); ++d) {
            struct sockaddr_un addr;
            int s = socket(AF_UNIX, SOCK_STREAM, 0);
            if (s < 0) return -1;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/%sdiscord-ipc-%d",
                     base, subdirs[d], i);
            if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
                g_dc.sock = s;
                return 0;
            }
            close(s);
        }
#endif
    }
    return -1;
}

/* ---- tiny JSON helpers ------------------------------------------------ */

static void json_escape(char *out, size_t cap, const char *in)
{
    size_t o = 0;
    if (!cap) return;
    for (; in && *in && o + 7 < cap; ++in) {
        unsigned char c = (unsigned char)*in;
        if (c == '"' || c == '\\') { out[o++] = '\\'; out[o++] = (char)c; }
        else if (c < 0x20) o += (size_t)snprintf(out + o, cap - o, "\\u%04x", c);
        else out[o++] = (char)c;
    }
    out[o] = '\0';
}

/* Value of "key":"..." after `from` (unescaped minimally). 1 when found. */
static int json_str(const char *json, const char *key, char *out, size_t cap)
{
    char pat[64];
    const char *p;
    size_t o = 0;
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    p = strstr(json, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ' || *p == ':') ++p;
    if (*p != '"') return 0;
    ++p;
    while (*p && *p != '"' && o + 1 < cap) {
        if (*p == '\\' && p[1]) ++p;
        out[o++] = *p++;
    }
    out[o] = '\0';
    return 1;
}

/* ---- protocol --------------------------------------------------------- */

static void dc_command(const char *cmd, const char *evt, const char *args_json)
{
    char buf[2048];
    ++g_dc.nonce;
    if (evt)
        snprintf(buf, sizeof(buf), "{\"cmd\":\"%s\",\"evt\":\"%s\",\"args\":%s,\"nonce\":\"%u\"}",
                 cmd, evt, args_json ? args_json : "{}", g_dc.nonce);
    else
        snprintf(buf, sizeof(buf), "{\"cmd\":\"%s\",\"args\":%s,\"nonce\":\"%u\"}",
                 cmd, args_json ? args_json : "{}", g_dc.nonce);
    (void)dc_send(OP_FRAME, buf);
}

#if defined(_WIN32)
/* So Discord can start the game when a friend accepts an invite while it is
 * closed: register discord-<app id> to the install's root launcher. */
static void dc_register_launch(void)
{
    wchar_t exe[MAX_PATH], root[MAX_PATH], cmd[MAX_PATH + 8], key[128];
    wchar_t *slash;
    DWORD n = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (!n || n >= MAX_PATH) return;
    wcscpy(root, exe);
    slash = wcsrchr(root, L'\\');
    if (slash) {
        /* build\SuperBomberman5SNESRecomp.exe -> ..\SuperBomberman5Recomp.exe */
        *slash = L'\0';
        slash = wcsrchr(root, L'\\');
        if (slash) {
            wcscpy(slash + 1, L"SuperBomberman5Recomp.exe");
            if (GetFileAttributesW(root) != INVALID_FILE_ATTRIBUTES)
                wcscpy(exe, root);
        }
    }
    swprintf(cmd, MAX_PATH + 8, L"\"%ls\"", exe);
    swprintf(key, 128, L"Software\\Classes\\discord-%hs", SB5_DISCORD_APP_ID);
    RegSetKeyValueW(HKEY_CURRENT_USER, key, NULL, REG_SZ,
                    L"URL:Run game " SB5_DISCORD_APP_ID " protocol",
                    (DWORD)(sizeof(L"URL:Run game " SB5_DISCORD_APP_ID " protocol")));
    RegSetKeyValueW(HKEY_CURRENT_USER, key, L"URL Protocol", REG_SZ, L"", sizeof(L""));
    wcscat(key, L"\\shell\\open\\command");
    RegSetKeyValueW(HKEY_CURRENT_USER, key, NULL, REG_SZ, cmd,
                    (DWORD)((wcslen(cmd) + 1) * sizeof(wchar_t)));
}
#endif

static void dc_on_frame(const char *json)
{
    char evt[48] = "", cmd[48] = "";
    json_str(json, "evt", evt, sizeof(evt));
    json_str(json, "cmd", cmd, sizeof(cmd));
    if (!strcmp(cmd, "DISPATCH") && !strcmp(evt, "READY")) {
        g_dc.ready = 1;
        fprintf(stderr, "discord: connected\n");
        dc_command("SUBSCRIBE", "ACTIVITY_JOIN", "{}");
        dc_command("SUBSCRIBE", "ACTIVITY_JOIN_REQUEST", "{}");
        g_dc.sent_activity[0] = '\0';
        return;
    }
    if (!strcmp(evt, "ACTIVITY_JOIN")) {
        char secret[160] = "";
        const char *id;
        if (json_str(json, "secret", secret, sizeof(secret)) &&
            !strncmp(secret, "sb5:", 4) && (id = strchr(secret + 4, ':')) && id[1]) {
            snprintf(g_dc.pending_join, sizeof(g_dc.pending_join), "%s", id + 1);
            fprintf(stderr, "discord: invite accepted -> room %s (host v%.*s)\n",
                    g_dc.pending_join, (int)(id - secret - 4), secret + 4);
        }
        return;
    }
    if (!strcmp(evt, "ACTIVITY_JOIN_REQUEST")) {
        /* "Ask to Join" from a friend: accept while the room is joinable
         * (same rule as the public room list), otherwise decline. */
        char user[32] = "", args[96];
        const char *u = strstr(json, "\"user\"");
        if (u && json_str(u, "id", user, sizeof(user)) && user[0]) {
            snprintf(args, sizeof(args), "{\"user_id\":\"%s\"}", user);
            dc_command(g_dc.join_secret[0] ? "SEND_ACTIVITY_JOIN_INVITE"
                                           : "CLOSE_ACTIVITY_REQUEST",
                       NULL, args);
        }
        return;
    }
    if (!strcmp(evt, "ERROR")) {
        char msg[160] = "";
        json_str(json, "message", msg, sizeof(msg));
        fprintf(stderr, "discord: error from %s: %s\n", cmd, msg);
    }
}

static void dc_pump(void)
{
    uint64_t t = now_ms();
    if (!dc_enabled()) return;
    if (!g_dc.connected) {
        char hs[96];
        if (t < g_dc.next_try_ms) return;
        g_dc.next_try_ms = t + 5000u;
        if (dc_open() != 0) return;
        g_dc.connected = 1;
        g_dc.rx_len = 0;
#if defined(_WIN32)
        if (!g_dc.registered) { dc_register_launch(); g_dc.registered = 1; }
#endif
        snprintf(hs, sizeof(hs), "{\"v\":1,\"client_id\":\"%s\"}", SB5_DISCORD_APP_ID);
        if (dc_send(OP_HANDSHAKE, hs) != 0) return;
    }
    if (dc_read_available() != 0) { dc_close(); return; }
    while (g_dc.rx_len >= 8) {
        uint32_t op = (uint32_t)g_dc.rx[0] | (uint32_t)g_dc.rx[1] << 8 |
                      (uint32_t)g_dc.rx[2] << 16 | (uint32_t)g_dc.rx[3] << 24;
        uint32_t len = (uint32_t)g_dc.rx[4] | (uint32_t)g_dc.rx[5] << 8 |
                       (uint32_t)g_dc.rx[6] << 16 | (uint32_t)g_dc.rx[7] << 24;
        char *json;
        if (len > RX_CAP - 9) { dc_close(); return; }
        if (g_dc.rx_len < 8u + len) break;
        json = (char *)malloc(len + 1u);
        if (!json) { dc_close(); return; }
        memcpy(json, g_dc.rx + 8, len);
        json[len] = '\0';
        memmove(g_dc.rx, g_dc.rx + 8 + len, g_dc.rx_len - 8 - len);
        g_dc.rx_len -= 8u + len;
        if (op == OP_FRAME) dc_on_frame(json);
        else if (op == OP_PING) (void)dc_send(OP_PONG, json);
        else if (op == OP_CLOSE) {
            char msg[160] = "";
            json_str(json, "message", msg, sizeof(msg));
            fprintf(stderr, "discord: closed by Discord: %s\n", msg);
            free(json);
            dc_close();
            return;
        }
        free(json);
        if (!g_dc.connected) return;
    }
    /* Discord allows about 5 activity updates per 20 s. */
    if (g_dc.ready && g_dc.want_activity[0] && strcmp(g_dc.want_activity, g_dc.sent_activity) &&
        t - g_dc.last_send_ms >= 4000u) {
        char args[1700];
        snprintf(args, sizeof(args), "{\"pid\":%lu,\"activity\":%s}", self_pid(), g_dc.want_activity);
        dc_command("SET_ACTIVITY", NULL, args);
        snprintf(g_dc.sent_activity, sizeof(g_dc.sent_activity), "%s", g_dc.want_activity);
        g_dc.last_send_ms = t;
    }
}

/* ---- what to show ----------------------------------------------------- */

typedef struct {
    int in_room;
    int online_room; /* not a LAN / Direct IP room */
    int players, max_players;
    int spectator;
    char lobby_id[48];
} RoomInfo;

static void room_info(RoomInfo *r, int from_launcher)
{
    memset(r, 0, sizeof(*r));
#if defined(SNESRECOMP_NET)
    if (!rnet_lobby_in_lobby()) return;
    {
        const RNetLobbyJoinInfo *ji = rnet_lobby_join_info();
        if (!ji || !ji->lobby_id[0]) return;
        r->in_room = 1;
        snprintf(r->lobby_id, sizeof(r->lobby_id), "%s", ji->lobby_id);
        r->online_room = strncmp(ji->lobby_id, "lan:", 4) != 0;
        r->players = rnet_lobby_member_count();
        if (r->players < 1) r->players = ji->player_count > 0 ? ji->player_count : 1;
        r->max_players = ji->max_slots >= 2 ? ji->max_slots : 5;
        r->spectator = rnet_lobby_local_is_spectator();
    }
    /* The list is fresh only while the launcher shows it: cache the room's
     * password flag from there. */
    if (from_launcher && strcmp(g_dc.room_pw_checked, r->lobby_id)) {
        int i, n = rnet_lobby_list_count();
        for (i = 0; i < n; ++i) {
            RNetLobbyRow row;
            if (rnet_lobby_list_get(i, &row) && !strcmp(row.lobby_id, r->lobby_id)) {
                g_dc.room_has_password = row.has_password;
                snprintf(g_dc.room_pw_checked, sizeof(g_dc.room_pw_checked), "%s", r->lobby_id);
                break;
            }
        }
    }
#else
    (void)from_launcher;
#endif
}

static const char *game_scene(void)
{
    const int mode = g_ram[0x1031];
    const int not_campaign = g_ram[0x1d1a] & 1;
    if (mode >= 0x74 && not_campaign) return "Battle";
    if (mode >= 0x0d && mode <= 0x73 && !not_campaign) return "Normal Game";
    if (mode == 8 && !not_campaign) return "Normal Game";
    if (mode == 4) return "Choosing a bomber";
    return "In the menus";
}

static void build_activity(int from_launcher)
{
    RoomInfo room;
    char details[96], state[96], party[192] = "", secrets[192] = "", cat[32];
    char esc_d[200], esc_s[200], ver[32];
    const char *v = SNES_GAME_VERSION;
    int netplay = 0;

    room_info(&room, from_launcher);
#if defined(SNESRECOMP_NET)
    netplay = !from_launcher && snes_netplay_active();
    {
        const char *lv = rnet_lobby_game_version();
        if (lv && lv[0]) v = lv;
    }
#endif
    snprintf(ver, sizeof(ver), "%s", v);

    if (from_launcher) {
        snprintf(details, sizeof(details), "%s", room.in_room ? "Online room" : "In the launcher");
        if (room.in_room)
            snprintf(state, sizeof(state), "Waiting for players");
        else
            snprintf(state, sizeof(state), "v%s", ver);
        snprintf(cat, sizeof(cat), "%s", room.in_room ? "room" : "launcher");
    } else {
        snprintf(details, sizeof(details), "%s", game_scene());
        snprintf(state, sizeof(state), "%s",
                 netplay ? (room.spectator ? "Watching online" : "Playing online")
                         : "Playing offline");
        snprintf(cat, sizeof(cat), "%s", netplay ? "online" : "offline");
    }
    if (strcmp(cat, g_dc.category)) {
        snprintf(g_dc.category, sizeof(g_dc.category), "%s", cat);
        g_dc.category_since = (uint64_t)time(NULL);
    }

    g_dc.join_secret[0] = '\0';
    if (room.in_room && room.online_room) {
        snprintf(party, sizeof(party), ",\"party\":{\"id\":\"sb5-%s\",\"size\":[%d,%d]}",
                 room.lobby_id, room.players, room.max_players);
        if (!room.spectator && !g_dc.room_has_password && room.players < room.max_players &&
            strlen(room.lobby_id) < 96) {
            snprintf(g_dc.join_secret, sizeof(g_dc.join_secret), "sb5:%s:%s", ver, room.lobby_id);
            snprintf(secrets, sizeof(secrets), ",\"secrets\":{\"join\":\"%s\"}", g_dc.join_secret);
        }
    }
    json_escape(esc_d, sizeof(esc_d), details);
    json_escape(esc_s, sizeof(esc_s), state);
    snprintf(g_dc.want_activity, sizeof(g_dc.want_activity),
             "{\"details\":\"%s\",\"state\":\"%s\",\"timestamps\":{\"start\":%llu},"
             "\"assets\":{\"large_image\":\"%s\",\"large_text\":\"Super Bomberman 5 Recomp v%s\"}"
             "%s%s,\"instance\":false}",
             esc_d, esc_s, (unsigned long long)g_dc.category_since,
             SB5_DISCORD_LARGE_IMAGE, ver, party, secrets);
}

/* ---- entry points ----------------------------------------------------- */

void Sb5DiscordFrame(void)
{
    static uint64_t last;
    uint64_t t;
    if (!dc_enabled()) return;
    t = now_ms();
    if (t - last < 1000u) return;
    last = t;
    build_activity(0);
    dc_pump();
}

#if defined(RECOMP_LAUNCHER) && defined(RECOMP_LAUNCHER_HAS_EXTERNAL_JOIN)
static int poll_external_join(void *ctx, char *lobby_id, size_t cap)
{
    static uint64_t last;
    uint64_t t;
    (void)ctx;
    t = now_ms();
    /* Discord off still delivers a pasted room invite (room_invite_accept). */
    if (dc_enabled() && t - last >= 250u) {
        last = t;
        build_activity(1);
        dc_pump();
    }
    if (g_dc.pending_join[0]) {
        snprintf(lobby_id, cap, "%s", g_dc.pending_join);
        g_dc.pending_join[0] = '\0';
        return 1;
    }
    return 0;
}
#endif

/* ---- room invites pasted in any chat ---------------------------------- */

/* The invite carries the same token as the Discord join secret,
 * "sb5:<host version>:<lobby id>", inside a message a person can read. */
int Sb5RoomInviteFormat(const char *version, const char *lobby_id, char *out, size_t cap)
{
    int n;
    if (!out || !cap) return 0;
    out[0] = '\0';
    if (!version || !lobby_id || !lobby_id[0] || !strncmp(lobby_id, "lan:", 4)) return 0;
    n = snprintf(out, cap,
                 "Join my Super Bomberman 5 Recomp room! Copy this whole message, "
                 "then in the game open Online Netplay and press Join from Invite.\n"
                 "Room: sb5:%s:%s", version, lobby_id);
    if (n < 0 || (size_t)n >= cap) { out[0] = '\0'; return 0; }
    return 1;
}

static int invite_id_char(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           c == '-' || c == '_' || c == '.';
}

/* Finds "sb5:<version>:<id>" anywhere in text (chat apps add quotes, links,
 * line breaks around it). Writes the lobby id; 0 when there is none. */
int Sb5RoomInviteParse(const char *text, char *lobby_id, size_t cap)
{
    const char *p = text;
    if (!lobby_id || !cap) return 0;
    lobby_id[0] = '\0';
    if (!text) return 0;
    while ((p = strstr(p, "sb5:")) != NULL) {
        const char *v = p + 4, *id;
        size_t n = 0;
        while (*v && (invite_id_char(*v)) && *v != ':') ++v;
        if (*v == ':' && v > p + 4) {
            id = v + 1;
            while (invite_id_char(id[n])) ++n;
            if (n > 0 && n < cap && n < 48) {
                memcpy(lobby_id, id, n);
                lobby_id[n] = '\0';
                return 1;
            }
        }
        p += 4;
    }
    return 0;
}

#if defined(RECOMP_LAUNCHER) && defined(RECOMP_LAUNCHER_HAS_ROOM_INVITE)
static int room_invite_text(void *ctx, char *out, size_t cap)
{
    RoomInfo room;
    const char *v = SNES_GAME_VERSION;
    (void)ctx;
    room_info(&room, 1);
    if (!room.in_room || !room.online_room) return 0;
#if defined(SNESRECOMP_NET)
    {
        const char *lv = rnet_lobby_game_version();
        if (lv && lv[0]) v = lv;
    }
#endif
    return Sb5RoomInviteFormat(v, room.lobby_id, out, cap);
}

static int room_invite_accept(void *ctx, const char *text)
{
    char id[sizeof(g_dc.pending_join)];
    (void)ctx;
    if (!Sb5RoomInviteParse(text, id, sizeof(id))) return 0;
    snprintf(g_dc.pending_join, sizeof(g_dc.pending_join), "%s", id);
    return 1;
}
#endif

void Sb5DiscordConfigureLauncher(struct RecompLauncherCGameInfo *gi)
{
#if defined(RECOMP_LAUNCHER) && defined(RECOMP_LAUNCHER_HAS_EXTERNAL_JOIN)
    gi->poll_external_join = poll_external_join;
    gi->external_join_ctx = NULL;
#if defined(RECOMP_LAUNCHER_HAS_ROOM_INVITE)
    gi->room_invite_text = room_invite_text;
    gi->room_invite_accept = room_invite_accept;
    gi->room_invite_ctx = NULL;
#endif
#else
    (void)gi;
#endif
}
