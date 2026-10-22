/* Netplay name tags: which seats get a nameplate and where (src/name_tags.c). */
#include "../src/name_tags.c"
#include <stdio.h>
#include <string.h>
uint8 g_ram[0x20000];
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static void stage(int battle) {
  memset(g_ram, 0, sizeof(g_ram));
  g_ram[0x0aee] = 1;
  g_ram[0x1031] = battle ? 0x74 : 0x0d;
  g_ram[0x1d1a] = battle ? 0x03 : 0x42;
  g_ram[0x1d3b] = 0xf0; g_ram[0x1d3c] = 0xff; /* scroll x -16 */
  g_ram[0x1d3d] = 0xe4; g_ram[0x1d3e] = 0xff; /* scroll y -28 */
  for (int i = 0; i < 5; ++i) {
    g_ram[0x17fa + i] = (uint8)(2 * i);
    g_ram[0x15a2 + i] = 0x19;
    g_ram[0x6269 + i] = i < 2 ? 1 : 2;
    g_ram[0x182a + i] = (uint8)(16 + 32 * i);
    g_ram[0x1872 + i] = 16;
  }
}

int main(void) {
  int x, y;
  /* Battle, verified dump layout: seats 0/1 human, 2-4 COM. */
  stage(1);
  CHECK(NameTagPlace(g_ram, 0, &x, &y) && x == 32 && y == 16 + 28 - 17);
  CHECK(NameTagPlace(g_ram, 1, &x, &y) && x == 64);
  CHECK(!NameTagPlace(g_ram, 2, &x, &y)); /* COM */
  g_ram[0x6269 + 1] = 0x81;                /* team bit does not matter */
  CHECK(NameTagPlace(g_ram, 1, &x, &y));
  g_ram[0x15a2 + 0] |= 0x80;               /* dead */
  CHECK(!NameTagPlace(g_ram, 0, &x, &y));
  g_ram[0x15a2 + 0] = 0x19;
  g_ram[0x1782 + 0] = 0x80;                /* riding a Louie: 8 lines higher */
  CHECK(NameTagPlace(g_ram, 0, &x, &y) && y == 16 + 28 - 17 - 8);
  g_ram[0x1d1d] = 1;                       /* paused */
  CHECK(!NameTagPlace(g_ram, 0, &x, &y));
  g_ram[0x1d1d] = 0;
  g_ram[0x0aee] = 0;                       /* menus, picker, transitions */
  CHECK(!NameTagPlace(g_ram, 0, &x, &y));
  stage(1);
  g_ram[0x17fa + 0] = 0xff;                /* no character object */
  CHECK(!NameTagPlace(g_ram, 0, &x, &y));
  CHECK(!NameTagPlace(g_ram, 5, &x, &y) && !NameTagPlace(g_ram, -1, &x, &y));

  /* Campaign: participation mask, boss rivals excluded, COM kind ignored. */
  stage(0);
  g_ram[0x1d25] = 0x05;                    /* P1 and P3 */
  CHECK(NameTagPlace(g_ram, 0, &x, &y));
  CHECK(!NameTagPlace(g_ram, 1, &x, &y));
  CHECK(NameTagPlace(g_ram, 2, &x, &y));   /* $6269 = 2 is a Battle notion */
  g_ram[0x101f + 2] = 1;                   /* rival bomber in a boss stage */
  CHECK(!NameTagPlace(g_ram, 2, &x, &y));
  g_ram[0x1031] = 0x74;                    /* not a campaign stage id */
  CHECK(!NameTagPlace(g_ram, 0, &x, &y));

  /* Off-screen positions show nothing. */
  stage(1);
  g_ram[0x1872 + 0] = 0; g_ram[0x188a + 0] = 0;
  g_ram[0x1d3d] = 40; g_ram[0x1d3e] = 0;
  CHECK(!NameTagPlace(g_ram, 0, &x, &y));
  printf("name_tags_tests: ok\n");
  return 0;
}
