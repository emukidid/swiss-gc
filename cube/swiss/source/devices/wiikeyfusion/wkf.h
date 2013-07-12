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
int wkfSpiRead(unsigned char *buf, unsigned int addr, int len);
void wkfWriteFlash(unsigned char *menuImg, unsigned char *firmwareImg);
char *wkfGetSerial();
void wkfRead(void* dst, int len, u64 offset);
unsigned int __wkfSpiReadId();
void wkfReinit();
void __wkfReset();
#endif

