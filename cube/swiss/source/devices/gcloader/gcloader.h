/**
 * GCLoader Driver for Gamecube
 *
 * Written by meneerbeer, emu_kidid
**/

#ifndef GCLOADER_H
#define GCLOADER_H

#include <gccore.h>

#define MAX_GCLOADER_FRAGS_PER_DISC (40)

u32 gcloaderReadId();
char *gcloaderGetVersion();
void gcloaderWriteFrags(u32 discNum, u32 (*fragList)[4], u32 totFrags);
void gcloaderWriteDiscNum(u32 discNum);

#endif
