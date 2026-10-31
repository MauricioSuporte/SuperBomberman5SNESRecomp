#ifndef SB5_ANDROID_ASSETS_H
#define SB5_ANDROID_ASSETS_H

/* Android only: extract the bundled data into the app's storage, anchor the
 * game folder there and make it the working directory (android_assets.c).
 * Call first thing in SDL_main, before the desktop host starts. */
void Sb5AndroidPrepare(void);

#endif
