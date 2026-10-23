/* Discord Rich Presence client against a fake Discord IPC socket (POSIX).
 *
 * The test plays Discord: it listens on $XDG_RUNTIME_DIR/discord-ipc-0,
 * checks the handshake, answers READY, expects the two SUBSCRIBEs and a
 * SET_ACTIVITY with the room as a joinable party, then sends ACTIVITY_JOIN
 * and checks the launcher poll hands out the room id. Lobby state is stubbed. */
#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define SNESRECOMP_NET 1
#define RECOMP_LAUNCHER 1
#define SNES_GAME_VERSION "9.9.9"

/* ---- stubs for the lobby / netplay the module reads ---- */
#include "recomp_net/lobby_client.h"
static RNetLobbyJoinInfo g_ji;
static int g_in_lobby, g_members = 2, g_pw;
int rnet_lobby_in_lobby(void) { return g_in_lobby; }
const RNetLobbyJoinInfo *rnet_lobby_join_info(void) { return &g_ji; }
int rnet_lobby_member_count(void) { return g_members; }
int rnet_lobby_local_is_spectator(void) { return 0; }
int rnet_lobby_list_count(void) { return 1; }
int rnet_lobby_list_get(int i, RNetLobbyRow *out)
{
    (void)i;
    memset(out, 0, sizeof(*out));
    snprintf(out->lobby_id, sizeof(out->lobby_id), "%s", g_ji.lobby_id);
    out->has_password = g_pw;
    return 1;
}
const char *rnet_lobby_game_version(void) { return "9.9.9"; }
int snes_netplay_active(void) { return 0; }
unsigned char g_ram[0x20000];

#include "../src/discord_presence.c"

static int g_fail;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); g_fail++; } } while (0)

static void sleep_ms(int ms) { struct timespec ts = {0, ms * 1000000L}; nanosleep(&ts, NULL); }

static int read_frame(int fd, unsigned *op, char *out, size_t cap)
{
    unsigned char h[8];
    size_t got = 0;
    unsigned len;
    for (int tries = 0; got < 8 && tries < 2000; ++tries) {
        ssize_t n = recv(fd, h + got, 8 - got, MSG_DONTWAIT);
        if (n > 0) got += (size_t)n; else sleep_ms(1);
    }
    if (got < 8) return -1;
    *op = h[0] | h[1] << 8 | h[2] << 16 | (unsigned)h[3] << 24;
    len = h[4] | h[5] << 8 | h[6] << 16 | (unsigned)h[7] << 24;
    if (len + 1 > cap) return -1;
    got = 0;
    for (int tries = 0; got < len && tries < 2000; ++tries) {
        ssize_t n = recv(fd, out + got, len - got, MSG_DONTWAIT);
        if (n > 0) got += (size_t)n; else sleep_ms(1);
    }
    out[got] = 0;
    return got == len ? 0 : -1;
}

static void send_frame(int fd, unsigned op, const char *json)
{
    unsigned len = (unsigned)strlen(json);
    unsigned char h[8] = {op, op >> 8, op >> 16, op >> 24, len, len >> 8, len >> 16, len >> 24};
    (void)!write(fd, h, 8);
    (void)!write(fd, json, len);
}

/* Drive the module's launcher tick until `fd` has a frame or time runs out. */
static int tick_until_frame(int fd, unsigned *op, char *buf, size_t cap)
{
    char id[48];
    for (int i = 0; i < 40; ++i) {
        unsigned char peek;
        (void)poll_external_join(NULL, id, sizeof(id));
        if (recv(fd, &peek, 1, MSG_PEEK | MSG_DONTWAIT) == 1)
            return read_frame(fd, op, buf, cap);
        sleep_ms(260);
    }
    return -1;
}

int main(void)
{
    char dir[] = "/tmp/sb5dcXXXXXX", path[128], buf[4096], id[48];
    struct sockaddr_un addr;
    int ls, fd = -1, i;
    unsigned op = 0;

    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    setenv("XDG_RUNTIME_DIR", dir, 1);
    snprintf(path, sizeof(path), "%s/discord-ipc-0", dir);
    ls = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);
    if (bind(ls, (struct sockaddr *)&addr, sizeof(addr)) || listen(ls, 1)) { perror("bind"); return 1; }
    fcntl(ls, F_SETFL, O_NONBLOCK);

    g_in_lobby = 1;
    snprintf(g_ji.lobby_id, sizeof(g_ji.lobby_id), "room-42");
    g_ji.max_slots = 5;
    g_ji.player_count = 2;

    /* The first tick connects and sends the handshake. */
    for (i = 0; i < 20 && fd < 0; ++i) {
        (void)poll_external_join(NULL, id, sizeof(id));
        fd = accept(ls, NULL, NULL);
        if (fd < 0) sleep_ms(50);
    }
    CHECK(fd >= 0);
    if (fd < 0) return 1;
    CHECK(read_frame(fd, &op, buf, sizeof(buf)) == 0);
    CHECK(op == 0 && strstr(buf, "\"client_id\":\"" SB5_DISCORD_APP_ID "\"") && strstr(buf, "\"v\":1"));

    send_frame(fd, 1, "{\"cmd\":\"DISPATCH\",\"evt\":\"READY\",\"data\":{\"v\":1}}");
    CHECK(tick_until_frame(fd, &op, buf, sizeof(buf)) == 0);
    CHECK(op == 1 && strstr(buf, "\"cmd\":\"SUBSCRIBE\"") && strstr(buf, "\"evt\":\"ACTIVITY_JOIN\""));
    CHECK(read_frame(fd, &op, buf, sizeof(buf)) == 0);
    CHECK(strstr(buf, "\"evt\":\"ACTIVITY_JOIN_REQUEST\"") != NULL);

    CHECK(tick_until_frame(fd, &op, buf, sizeof(buf)) == 0);
    printf("activity: %s\n", buf);
    CHECK(strstr(buf, "\"cmd\":\"SET_ACTIVITY\"") != NULL);
    CHECK(strstr(buf, "\"details\":\"Online room\"") != NULL);
    CHECK(strstr(buf, "\"party\":{\"id\":\"sb5-room-42\",\"size\":[2,5]}") != NULL);
    CHECK(strstr(buf, "\"secrets\":{\"join\":\"sb5:9.9.9:room-42\"}") != NULL);
    CHECK(strstr(buf, "\"large_image\":\"logo\"") != NULL);

    /* Ask to Join while joinable -> accepted. */
    send_frame(fd, 1, "{\"cmd\":\"DISPATCH\",\"evt\":\"ACTIVITY_JOIN_REQUEST\",\"data\":"
                      "{\"user\":{\"id\":\"1234567\",\"username\":\"friend\"}}}");
    CHECK(tick_until_frame(fd, &op, buf, sizeof(buf)) == 0);
    CHECK(strstr(buf, "\"cmd\":\"SEND_ACTIVITY_JOIN_INVITE\"") && strstr(buf, "\"user_id\":\"1234567\""));

    /* A friend's accepted invite -> the launcher poll returns the room once. */
    send_frame(fd, 1, "{\"cmd\":\"DISPATCH\",\"evt\":\"ACTIVITY_JOIN\",\"data\":"
                      "{\"secret\":\"sb5:9.9.9:room-77\"}}");
    {
        int got = 0;
        for (i = 0; i < 20 && !got; ++i) {
            got = poll_external_join(NULL, id, sizeof(id));
            if (!got) sleep_ms(260);
        }
        CHECK(got && !strcmp(id, "room-77"));
        CHECK(!poll_external_join(NULL, id, sizeof(id)));
    }

    /* Leaving the room drops the party and the secret. */
    g_in_lobby = 0;
    CHECK(tick_until_frame(fd, &op, buf, sizeof(buf)) == 0);
    CHECK(strstr(buf, "\"details\":\"In the launcher\"") && !strstr(buf, "\"secrets\"") &&
          !strstr(buf, "\"party\""));

    /* Discord going away is survived. */
    close(fd);
    for (i = 0; i < 5; ++i) { (void)poll_external_join(NULL, id, sizeof(id)); sleep_ms(260); }
    CHECK(!g_dc.connected);

    close(ls);
    unlink(path);
    rmdir(dir);

    /* Room invite pasted in a chat: format, then find it inside chat noise. */
    {
        char msg[512], got[48];
        CHECK(Sb5RoomInviteFormat("2.2.1", "abc-123_X", msg, sizeof(msg)) == 1);
        CHECK(strstr(msg, "sb5:2.2.1:abc-123_X") != NULL);
        CHECK(Sb5RoomInviteParse(msg, got, sizeof(got)) == 1 && !strcmp(got, "abc-123_X"));
        CHECK(Sb5RoomInviteParse("> Room: `sb5:2.2.1:Zz9`\n", got, sizeof(got)) == 1 &&
              !strcmp(got, "Zz9"));
        CHECK(Sb5RoomInviteFormat("2.2.1", "lan:1.2.3.4:5", msg, sizeof(msg)) == 0);
        CHECK(Sb5RoomInviteParse("hello there", got, sizeof(got)) == 0);
        CHECK(Sb5RoomInviteParse("sb5::abc sb5:1.0", got, sizeof(got)) == 0);
        /* Accepting queues the room for the launcher's external join. */
        g_dc.pending_join[0] = '\0';
        CHECK(room_invite_accept(NULL, "Room: sb5:2.2.1:room42") == 1);
        CHECK(poll_external_join(NULL, id, sizeof(id)) == 1 && !strcmp(id, "room42"));
        CHECK(room_invite_accept(NULL, "nothing here") == 0);
        /* Copy Invite follows the room the player sits in. */
        g_in_lobby = 1;
        snprintf(g_ji.lobby_id, sizeof(g_ji.lobby_id), "%s", "room77");
        CHECK(room_invite_text(NULL, msg, sizeof(msg)) == 1 && strstr(msg, "sb5:9.9.9:room77"));
        g_in_lobby = 0;
        CHECK(room_invite_text(NULL, msg, sizeof(msg)) == 0);
    }
    if (g_fail) { fprintf(stderr, "discord_presence_tests: %d failure(s)\n", g_fail); return 1; }
    printf("discord_presence_tests: ok\n");
    return 0;
}
