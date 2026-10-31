#pragma once
#include <stddef.h>
struct RecompLauncherCGameInfo;
extern const char kSb5DiscordInvite[];
extern const char kSb5ReleasesUrl[];
/* Netplay runs as delay-sync (lockstep) unless SNES_NET_MODE says otherwise. */
void Sb5ForceDelaySync(void);
int Sb5NetplayReady(char *reason, size_t cap);
int Sb5PrepareNetplay(int from_lobby, char *reason, size_t cap);
/* Every netplay session runs without AOT dispatch (crossplay with Android). */
void Sb5NetplayInterpreterOnly(void);
/* Discord / releases links and the Update button in the launcher. */
void Sb5ConfigureLauncher(struct RecompLauncherCGameInfo *gi);
int Sb5FindUpdater(char *updater, size_t cap, char *root, size_t root_cap);
/* Starts the updater for this install; 1 = the game should close now. */
int Sb5StartUpdater(char *msg, size_t cap);
