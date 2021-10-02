/**
 * GCLoader Driver for Gamecube
 *
 * Written by meneerbeer, emu_kidid
**/

#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <ogc/exi.h>
#include <gcloader.h>
#include <malloc.h>
#include "main.h"
#include "swiss.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

static vu32* const gcloader = (vu32*)0xCC006000;

u32 __gcloaderCmdImm(unsigned int cmd, unsigned int p1, unsigned int p2) {
	gcloader[2] = cmd;
	gcloader[3] = p1;
	gcloader[4] = p2;
	gcloader[6] = 0;
	gcloader[8] = 0;
	gcloader[7] = 1;
	int retries = 1000000;
	while(( gcloader[7] & 1) && retries) {
		retries --;						// Wait for IMM command to finish up
	}
	return !retries ? -1 : gcloader[8];
}

u32 gcloaderReadId() {
	return __gcloaderCmdImm(0xB0000000, 0x00000000, 0x00000000);
}

char *gcloaderGetVersion() {
	int len = (__gcloaderCmdImm(0xB1000001, 0x00000000, 0x00000000)+31)&~31;
	if(len <= 0) {
		return NULL;
	}
	char *buffer = memalign(32, len);
	if(!buffer) {
		return NULL;
	}
	gcloader[2] = 0xB1000000;
	gcloader[3] = 0;
	gcloader[4] = 0;
	gcloader[5] = (u32)buffer;
	gcloader[6] = len;
	gcloader[7] = 3; // DMA | START
	DCInvalidateRange(buffer, len);

	while(gcloader[7] & 1);

	buffer[len-1] = '\0';
	return buffer;
}

void gcloaderWriteFrags(u32 discNum, u32 (*fragList)[4], u32 totFrags) {
    int i;

	print_gecko("GCLoader setting up disc %i with %i fragments\r\n", discNum, totFrags);
	__gcloaderCmdImm(0xB3000001, discNum, totFrags);
    
    for(i = 0; i < totFrags; i++) {
		print_gecko("Frag %i: ofs in file: [0x%08X] len [0x%08X] LBA on SD [0x%08X]\r\n",
					i, fragList[i][0], fragList[i][1], fragList[i][3]);
		__gcloaderCmdImm(fragList[i][0], fragList[i][1], fragList[i][3]);
    }
}

void gcloaderWriteDiscNum(u32 discNum) {
	__gcloaderCmdImm(0xB3000002, discNum, 0x00000000);
}
