/* In-game room chat (snesrecomp/runner/src/desktop/host_chat.c): ring
 * tracking, five-second pop-ups, the text bar and the overlay raster. */
#include "desktop/host_chat.c"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static HostChatLine ring[80];
static int ring_n;
static char sent[HOST_CHAT_TEXT_LEN];
static int sends, send_rc;

static int src_send(void *ctx, const char *t) { (void)ctx; snprintf(sent, sizeof(sent), "%s", t); sends++; return send_rc; }
static int src_count(void *ctx) { (void)ctx; return ring_n; }
static int src_get(void *ctx, int i, HostChatLine *out) {
  (void)ctx;
  if (i < 0 || i >= ring_n) return 0;
  *out = ring[i];
  return 1;
}
static int src_hidden(void *ctx, const HostChatLine *l) { (void)ctx; return strcmp(l->account, "muted") == 0; }

static void push(const char *from, const char *text, int system, const char *account) {
  HostChatLine *l = &ring[ring_n];
  memset(l, 0, sizeof(*l));
  snprintf(l->from, sizeof(l->from), "%s", from);
  snprintf(l->text, sizeof(l->text), "%s", text);
  snprintf(l->account, sizeof(l->account), "%s", account ? account : "");
  l->is_system = system;
  l->seq = (uint32_t)(ring_n + 1);
  ring_n++;
}

int main(int argc, char **argv) {
  const HostChatSource src = { NULL, src_send, src_count, src_get, src_hidden };
  const uint32_t *px; int x, y, w, h; uint32_t gen, gen2;
  const char *font = argc > 1 ? argv[1] : NULL;

  /* Not in a match: nothing opens. */
  host_chat_tick(&src, 0, 0.0);
  CHECK(!host_chat_active() && !host_chat_open(0.0));

  /* Lines already in the room are history, not pop-ups. */
  push("", "Voce entrou na sala Suporte.", 1, NULL);
  push("MauricioSuporte", "alo teste", 0, "acc1");
  host_chat_tick(&src, 1, 10.0);
  CHECK(host_chat_active() && host_chat_line_count() == 2);
  CHECK(!host_chat_line_popup(0, 10.0) && !host_chat_line_popup(1, 10.0));

  /* A new line pops up for 5 s (+ fade), then goes. Ignored senders are hidden. */
  push("FelipebKof", "bora", 0, "acc2");
  push("Troll", "spam", 0, "muted");
  host_chat_tick(&src, 1, 20.0);
  CHECK(host_chat_line_count() == 3);
  CHECK(host_chat_line_popup(2, 20.0) && host_chat_line_popup(2, 24.9));
  CHECK(!host_chat_line_popup(2, 20.0 + HOST_CHAT_VISIBLE_SECONDS + HOST_CHAT_FADE_SECONDS + 0.01));
  host_chat_tick(&src, 1, 21.0); /* unchanged ring keeps its arrival time */
  CHECK(host_chat_line_popup(2, 25.2));

  /* T opens the bar; typing, UTF-8 backspace, control chars dropped. */
  CHECK(host_chat_open(30.0) && host_chat_typing());
  host_chat_text("ol\xC3\xA1\x01!");
  CHECK(strcmp(host_chat_input(), "ol\xC3\xA1!") == 0);
  CHECK(host_chat_key(HOST_CHAT_KEY_BACKSPACE, 30.1));
  CHECK(host_chat_key(HOST_CHAT_KEY_BACKSPACE, 30.1));
  CHECK(strcmp(host_chat_input(), "ol") == 0);
  host_chat_text("  oi  ");
  CHECK(host_chat_key(HOST_CHAT_KEY_ENTER, 30.2));
  CHECK(!host_chat_typing() && sends == 1 && strcmp(sent, "ol  oi") == 0);

  /* Empty Enter and Escape only close; a refused send leaves a notice. */
  CHECK(host_chat_open(31.0)); host_chat_text("   ");
  CHECK(host_chat_key(HOST_CHAT_KEY_ENTER, 31.0) && sends == 1 && !host_chat_typing());
  CHECK(host_chat_open(32.0)); host_chat_text("x");
  CHECK(host_chat_key(HOST_CHAT_KEY_ESCAPE, 32.0) && sends == 1 && !host_chat_typing());
  send_rc = -1;
  CHECK(host_chat_open(33.0)); host_chat_text("lan");
  CHECK(host_chat_key(HOST_CHAT_KEY_ENTER, 33.0) && sends == 2 && g_notice_at == 33.0);
  send_rc = 0;

  /* Length cap on a code-point boundary. */
  CHECK(host_chat_open(34.0));
  for (int i = 0; i < 200; ++i) host_chat_text("\xC3\xA9");
  CHECK(strlen(host_chat_input()) == 254);
  host_chat_close();

  /* Raster: needs the launcher's Lato (path passed by the test runner). */
  if (font && host_chat_load_font(font)) {
    CHECK(!host_chat_render(1920, 1080, 100.0, &px, &x, &y, &w, &h, &gen)); /* all popped out */
    push("MauricioSuporte", "uma linha bem comprida para quebrar em varias linhas dentro do painel do chat do jogo", 0, "acc1");
    host_chat_tick(&src, 1, 100.0);
    CHECK(host_chat_render(1920, 1080, 100.5, &px, &x, &y, &w, &h, &gen));
    CHECK(x > 0 && x < 40 && y + h <= 1080 && y + h > 1040 && w <= 1920 / 2 && h >= 2 * 40);
    {
      int opaque = 0;
      for (int i = 0; i < w * h; ++i) opaque += (px[i] >> 24) > 0;
      CHECK(opaque > w * 4);
    }
    CHECK(host_chat_render(1920, 1080, 100.6, &px, &x, &y, &w, &h, &gen2) && gen2 == gen);
    /* Typing shows history up to the upper quarter of the screen. */
    CHECK(host_chat_open(101.0));
    CHECK(host_chat_render(1920, 1080, 101.0, &px, &x, &y, &w, &h, &gen2) && gen2 != gen);
    CHECK(y >= 1080 / 4 && h > 5 * 40);
    host_chat_close();
    CHECK(host_chat_render(1280, 720, 101.5, &px, &x, &y, &w, &h, &gen) && y + h <= 720);
    /* Name tags: placed from frame pixels through the viewport, 75% opacity. */
    {
      const int vp[4] = { 240, 0, 1440, 1080 }; /* 4:3 field pillarboxed in 1920x1080 */
      const HostOverlayQuad *q;
      int n, max_a = 0;
      host_chat_set_seat_name(0, "MauricioSuporte");
      host_chat_set_tag(0, 1, 48, 27);
      n = host_chat_overlay(1920, 1080, vp, 256, 224, 150.0, &q);
      CHECK(n == 1 && q[0].slot == 1);
      CHECK(q[0].y + q[0].h == (int)lroundf(27.0f * 1080.0f / 224.0f));
      CHECK(abs(q[0].x + q[0].w / 2 - (240 + (int)lroundf(48.0f * 1440.0f / 256.0f))) <= 1);
      for (int i = 0; i < q[0].w * q[0].h; ++i)
        if ((int)(q[0].px[i] >> 24) > max_a) max_a = (int)(q[0].px[i] >> 24);
      CHECK(max_a == 191);
      gen = q[0].generation;
      host_chat_set_tag(0, 1, 60, 40); /* moving reuses the bitmap */
      n = host_chat_overlay(1920, 1080, vp, 256, 224, 150.1, &q);
      CHECK(n == 1 && q[0].generation == gen);
      host_chat_set_tag(0, 0, 60, 40); /* dead / off the field */
      CHECK(host_chat_overlay(1920, 1080, vp, 256, 224, 150.2, &q) == 0);
    }
  } else {
    printf("ingame_chat_tests: no font given, raster checks skipped\n");
  }

  /* Leaving the match forgets the session. */
  host_chat_tick(&src, 0, 200.0);
  CHECK(!host_chat_active() && host_chat_line_count() == 0 && !host_chat_typing());
  printf("ingame_chat_tests: ok\n");
  return 0;
}
