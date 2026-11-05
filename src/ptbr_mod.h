#pragma once
void PtbrDrawPpuFrame(void);
#include "beam_frame_driver.h"
void PtbrDrawPpuFrameObserved(SnesBeamRasterObserver *observer,void *context);

const char *PtbrPauseLabel(unsigned index);
#include <stdbool.h>
bool PtbrEnabled(void);
struct Ppu;
void PtbrDrawBodyText(struct Ppu *ppu,unsigned row,unsigned column,unsigned width,unsigned tile,unsigned attributes,const char *text);
void PtbrBlankMap(struct Ppu *ppu,unsigned row,unsigned column,unsigned rows,unsigned columns,unsigned word);
