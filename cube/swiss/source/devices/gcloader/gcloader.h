/**
 * GCLoader Driver for Gamecube
 *
 * Written by meneerbeer, emu_kidid
**/

#ifndef GCLOADER_H
#define GCLOADER_H

#include <gccore.h>
#include "deviceHandler.h"

#define MAX_GCLOADER_FRAGS_PER_DISC (40)

u32 gcloaderReadId();
char *gcloaderGetVersion();
void gcloaderWriteFrags(u32 discNum, file_frag *fragList, u32 totFrags);
void gcloaderWriteDiscNum(u32 discNum);

#endif
