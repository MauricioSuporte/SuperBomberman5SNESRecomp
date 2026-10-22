/*
 * Netplay name tags: the room name of the player above the bomber they
 * control, while it is on the field and alive.
 *
 * Presentation only. Nothing is written to WRAM, VRAM or OAM; the host draws
 * the tags at window resolution over the finished frame (host_chat.c), so a
 * peer with tags and one without still run the same machine.
 *
 * Which seats (verified on the JPN image, Windows/Linux LLE dumps):
 *   - a stage is live and not paused: gameplay-ready byte $0AEE == 1, pause
 *     latch $1D1D == 0 and mode $1031 is a stage (campaign $0D-$73, Battle $74+);
 *   - the seat has a character object ($17FA+i != $FF) and is alive
 *     ($15A2+i bit 7 clear);
 *   - a person drives it: Battle seat kind $6269+i == 1 (2 = COM); in the
 *     campaign the participation mask $1D25 has the seat and it is not a
 *     boss's rival bomber ($101F+i).
 * Position: world x/y ($182A/$1842, $1872/$188A) minus the signed camera
 * scroll ($1D3B/$1D3D) is the bomber's feet on screen; the head ends about
 * 17 lines above, a Louie rider ($1782+i bit 7) sits 8 lines higher.
 */
#include "name_tags.h"

#include "types.h"

#if defined(SNES_HOST_HAS_INGAME_CHAT)
#include "host_chat.h"
#endif

extern uint8 g_ram[];

#define NAME_TAG_SEATS 5

static int s16(const uint8_t *ram, int at) {
  return (int16_t)(uint16_t)(ram[at] | ram[at + 1] << 8);
}

int NameTagPlace(const uint8_t *ram, int seat, int *x, int *y) {
  const int mode = ram[0x1031];
  const int battle = ram[0x1d1a] & 1;
  int sx, sy;
  if (seat < 0 || seat >= NAME_TAG_SEATS) return 0;
  if (ram[0x0aee] != 1 || mode < 0x0d) return 0;
  if (ram[0x1d1d]) return 0; /* paused: the pause panel covers the field */
  if (!battle && mode > 0x73) return 0;
  if (ram[0x17fa + seat] & 0x80) return 0;   /* no character object */
  if (ram[0x15a2 + seat] & 0x80) return 0;   /* dead */
  if (battle) {
    if ((ram[0x6269 + seat] & 0x7f) != 1) return 0; /* COM seat */
  } else {
    if (!(ram[0x1d25] >> seat & 1)) return 0;        /* not playing */
    if (ram[0x101f + seat]) return 0;                /* boss rival */
  }
  sx = (ram[0x182a + seat] | ram[0x1842 + seat] << 8) - s16(ram, 0x1d3b);
  sy = (ram[0x1872 + seat] | ram[0x188a + seat] << 8) - s16(ram, 0x1d3d);
  sy -= 17;
  if (ram[0x1782 + seat] & 0x80) sy -= 8; /* riding a Louie */
  if (sx < 0 || sx > 255 || sy < 8 || sy > 223) return 0;
  *x = sx;
  *y = sy;
  return 1;
}

void NameTagsAfterFrame(void) {
#if defined(SNES_HOST_HAS_INGAME_CHAT)
  if (!host_chat_active()) return;
  for (int seat = 0; seat < NAME_TAG_SEATS; ++seat) {
    int x = 0, y = 0;
    const int show = host_chat_seat_name(seat)[0] && NameTagPlace(g_ram, seat, &x, &y);
    host_chat_set_tag(seat, show, x, y);
  }
#endif
}
