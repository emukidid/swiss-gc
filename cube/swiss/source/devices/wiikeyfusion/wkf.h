/**
 * Wiikey Fusion Driver for Gamecube & Wii
 *
 * Written by emu_kidid
**/

#ifndef WKF_H
#define WKF_H

#include <gccore.h>
#include <ogc/disc_io.h>

#define DEVICE_TYPE_GC_WKF	(('W'<<24)|('K'<<16)|('F'<<8)|'!')

extern const DISC_INTERFACE __io_wkf;
void wkfWriteRam(int offset, int data);
void wkfWriteOffset(int offset);
#endif

