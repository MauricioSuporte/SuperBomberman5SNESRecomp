#ifndef SB5_PORTAL_LABELS_MOD_H
#define SB5_PORTAL_LABELS_MOD_H
#include <stdbool.h>
struct Ppu;
bool PortalLabelsEnabled(void);
void PortalLabelsObserve(const struct Ppu *ppu, unsigned line, void *context);
#endif
