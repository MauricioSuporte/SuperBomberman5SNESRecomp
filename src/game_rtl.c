/*
 * Super Bomberman 5 — per-game runtime glue.
 *
 * THIS FILE IS THE PORT. Everything else the scaffold produced is layout,
 * build wiring, and packaging; this is where the actual work happens.
 *
 * The framework does not, and cannot, drive an arbitrary SNES game on its
 * own. Two responsibilities always land on the game host:
 *
 *   1. Deciding what "one frame" means for THIS title, and returning from
 *      run_frame() at that boundary. A real ROM's reset vector never
 *      returns — it enters a main loop that waits on vblank — so somebody
 *      has to choose the yield point.
 *   2. Delivering NMI/IRQ at the hardware edge (see
 *      snesrecomp/docs/FRAME_MODEL_HOSTS.md).
 *
 * A new port starts on the framework's beam-aligned driver
 * (runner/src/beam_frame_driver.h): one frame is one PPU field, cut at the
 * beam's vblank edge; interrupts -- raster splits and a coprocessor's IRQ
 * line -- are taken where the beam latches them; parked time still runs the
 * beam, APU and coprocessors; and the field is rasterized from the raster
 * journal with real HDMA. It is the framework's, so a fix to it reaches this
 * port on a submodule pull.
 *
 * This file used to hold its own copy of a simpler driver: frames measured
 * from wherever the CPU last stopped, NMI delivered at an arbitrary beam
 * position, the field drawn with end-of-frame register state. A game that
 * uploads in vblank between INIDISP=$80 and $0F flickered black, and one
 * that chains raster splits fell apart. Replace these calls only for what
 * the driver does not do; MetalWarriorsSNESRecomp's src/mw_rtl.c and
 * GundamWingEndlessDuelSNESRecomp's src/game_rtl.c are developed examples of
 * title-specific drivers.
 */

#include "game_rtl.h"

#include "beam_frame_driver.h"
#include "common_rtl.h"
#include "ptbr_mod.h"
#include "normal_character_mod.h"
#include "normal_coop_mod.h"
#include "infinite_time_mod.h"
#include "pause_menu_mod.h"
#include "portal_labels_mod.h"
#include "selection_color_mod.h"
#include "subordinate_mod.h"
#include "honey_kotetsu_mod.h"
#include "gold_battle_mod.h"
#include <stdbool.h>

static void GameInitialize(void)
{
    /* Standard five-player layout: P1 on port 1, four seats on port 2. */
    RtlSetMultitap(0, 0);
    RtlSetMultitap(1, 1);
    snes_beam_frame_driver_reset();
}

void GameRunOneFrame(void)
{
#ifdef SB5_COOP_INPUT_FIXTURE
    extern void CoopInputFixture(void);
    CoopInputFixture();
#endif
    SubordinateBeforeFrame();
    HoneyKotetsuBeforeFrame();
    NormalCharacterBeforeFrame();
    SelectionColorBeforeFrame();
    NormalCoopBeforeFrame();
    InfiniteTimeBeforeFrame();
    PauseMenuBeforeFrame();
    GoldBattleBeforeFrame();
    snes_beam_frame_driver_run_frame();
    PauseMenuAfterFrame();
    NormalCoopAfterFrame();
    SelectionColorAfterFrame();
    NormalCharacterFrame();
}

/* Render-only overlays draw in this order on line 1 and restore in the
 * reverse order on line 224, so each one puts back exactly what it saw. */
static void ObserveOverlays(const struct Ppu *ppu, unsigned line, void *context)
{
    if (line == 224) {
        PauseMenuObserve(ppu, line, context);
        PortalLabelsObserve(ppu, line, context);
        HoneyKotetsuObserve(ppu, line, context);
        SubordinateObserve(ppu, line, context);
        SelectionColorObserve(ppu, line, context);
    } else {
        SelectionColorObserve(ppu, line, context);
        SubordinateObserve(ppu, line, context);
        HoneyKotetsuObserve(ppu, line, context);
        PortalLabelsObserve(ppu, line, context);
        PauseMenuObserve(ppu, line, context);
    }
}

void GameDrawPpuFrame(void)
{
    bool overlays = SelectionColorEnabled() || SubordinateEnabled() || HoneyKotetsuEnabled() ||
                    PauseMenuEnabled() || PortalLabelsEnabled();
    PtbrDrawPpuFrameObserved(overlays ? ObserveOverlays : NULL, NULL);
}

void GameSessionReset(void)
{
    /* Rematch / soft-return: clear anything that must not survive a new
     * session. recomp-ai-rules/NETPLAY.md §3 — sticky state that "has always
     * been fine" is the usual desync culprit, because single-player never
     * re-enters the boot path twice in one process. */
    snes_beam_frame_driver_reset();
    GoldBattleReset();
    PauseMenuReset();
    NormalCharacterReset();
    InfiniteTimeReset();
    NormalCoopReset();
    SelectionColorReset();
}

static void GameHardwareReset(void)
{
    snes_beam_frame_driver_reset();
    GoldBattleReset();
    PauseMenuReset();
    NormalCharacterReset();
    InfiniteTimeReset();
    NormalCoopReset();
    SelectionColorReset();
}

static void GameSaveExtra(struct SaveLoadInfo *sli) {
    NormalCharacterSave(sli);SelectionColorSave(sli);NormalCoopSave(sli);PauseMenuSave(sli);GoldBattleSave(sli);
}
static void GameLoadExtra(struct SaveLoadInfo *sli,uint32_t version) {
    NormalCharacterLoad(sli,version);SelectionColorLoad(sli,version);NormalCoopLoad(sli,version);PauseMenuLoad(sli,version);GoldBattleLoad(sli,version);
}

const RtlGameInfo kGameInfo = {
    .title = "superbomberman5",
    .initialize = &GameInitialize,
    .run_frame = &GameRunOneFrame,
    .draw_ppu_frame = &GameDrawPpuFrame,
    .save_name_prefix = "save",
    .session_reset = &GameSessionReset,
    .hardware_reset = &GameHardwareReset,
    .state_save_extra = &GameSaveExtra,
    .state_load_extra = &GameLoadExtra,
    .on_state_loaded = &NormalCharacterLoaded,
};
