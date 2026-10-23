/* Discord Rich Presence and netplay invites (Discord's local RPC pipe).
 *
 * Shows what the player is doing (launcher, online room, Normal Game, Battle)
 * and, while seated in an online room, publishes the room as a joinable
 * party: friends use Discord's "Join" / "Ask to Join" / "Invite to play" and
 * their launcher opens Online Netplay and joins that room. No Discord SDK:
 * the client speaks the documented IPC framing over \\.\pipe\discord-ipc-N
 * (Windows) or $XDG_RUNTIME_DIR/discord-ipc-N (Linux). Discord not running
 * means nothing happens. SB5_DISCORD=0 turns it off. */
#pragma once
#include <stddef.h>

struct RecompLauncherCGameInfo;

/* Wires the launcher hooks (external join poll, also the launcher tick). */
void Sb5DiscordConfigureLauncher(struct RecompLauncherCGameInfo *gi);
/* Once per emulated frame (game_rtl.c); rate-limited internally. */
void Sb5DiscordFrame(void);

/* Room invite pasted in any chat: "...Room: sb5:<version>:<lobby id>".
 * Format returns 0 for a LAN / empty room; Parse finds the token anywhere in
 * the text and returns 0 when there is none. Exposed for tests. */
int Sb5RoomInviteFormat(const char *version, const char *lobby_id, char *out, size_t cap);
int Sb5RoomInviteParse(const char *text, char *lobby_id, size_t cap);
