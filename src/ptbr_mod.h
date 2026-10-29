#pragma once
void PtbrDrawPpuFrame(void);
#include "beam_frame_driver.h"
void PtbrDrawPpuFrameObserved(SnesBeamRasterObserver *observer,void *context);

const char *PtbrPauseLabel(unsigned index);
#include <stdbool.h>
bool PtbrEnabled(void);
