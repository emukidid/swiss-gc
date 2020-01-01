/**
 * GCLoader Driver for Gamecube
 *
 * Written by meneerbeer, emu_kidid
**/

#ifndef GCLOADER_H
#define GCLOADER_H

#include <gccore.h>
#include <ogc/disc_io.h>

#define DEVICE_TYPE_GC_GCLOADER    (('G'<<24)|('C'<<16)|('O'<<8)|'D')

#define MAX_GCLOADER_FRAGS_PER_DISC (40)

extern const DISC_INTERFACE __io_gcloader;
u32 gcloaderReadId();
void gcloaderWriteFrags(u32 discNum, vu32 *fragList, u32 totFrags);
void gcloaderWriteDiscNum(u32 discNum);

#endif
