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
#include "main.h"
#include "swiss.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

static vu32* const gcloader = (vu32*)0xCC006000;

static bool __gcloader_startup(void)
{
    return true;
}

static bool __gcloader_isInserted(void)
{
    return true;
}

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

static bool __gcloader_readSectors(u32 sector, u32 numSectors, void *buffer)
{
	//print_gecko("GCLoader read %08X, %i sectors\r\n", sector, numSectors);
    while(numSectors > 0) {

        u32 len = 0;
        if(numSectors > 16) {
            len = 16*512;
            numSectors = numSectors - 16;
        }
        else {
            len = numSectors * 512;
            numSectors = 0;
        }

        gcloader[2] = 0xB2000000;
        gcloader[3] = sector;
        gcloader[4] = len;
        gcloader[5] = (u32)buffer;
        gcloader[6] = len;
        gcloader[7] = 3; // DMA | START
        DCInvalidateRange(buffer, len);

        while(gcloader[7] & 1);

        buffer += len;
        sector = sector + 16;

    }

    return true;
}

static bool __gcloader_writeSectors(u32 sector, u32 numSectors, void *buffer)
{
	return false;
}

static bool __gcloader_clearStatus(void)
{
	return true;
}

static bool __gcloader_shutdown(void)
{
	return true;
}

u32 gcloaderReadId() {
	return __gcloaderCmdImm(0xB0000000, 0x00000000, 0x00000000);
}

void gcloaderWriteFrags(u32 discNum, vu32 *fragList, u32 totFrags) {
    int i;

	print_gecko("GCLoader setting up disc %i with %i fragments\r\n", discNum, totFrags);
	__gcloaderCmdImm(0xB3000001, discNum, totFrags);
    
    for(i = 0; i < totFrags; i++) {
		print_gecko("Frag %i: ofs in file: [0x%08X] len [0x%08X] LBA on SD [0x%08X]\r\n", 
					i, fragList[(i*3)+0], fragList[(i*3)+1], fragList[(i*3)+2]);
	    __gcloaderCmdImm(fragList[(i*3)+0], fragList[(i*3)+1], fragList[(i*3)+2]);
    }
}


void gcloaderWriteDiscNum(u32 discNum) {
	__gcloaderCmdImm(0xB3000002, discNum, 0x00000000);
}


const DISC_INTERFACE __io_gcloader = {
	DEVICE_TYPE_GC_GCLOADER,
	FEATURE_MEDIUM_CANREAD | FEATURE_GAMECUBE_DVD,
	(FN_MEDIUM_STARTUP)&__gcloader_startup,
	(FN_MEDIUM_ISINSERTED)&__gcloader_isInserted,
	(FN_MEDIUM_READSECTORS)&__gcloader_readSectors,
	(FN_MEDIUM_WRITESECTORS)&__gcloader_writeSectors,
	(FN_MEDIUM_CLEARSTATUS)&__gcloader_clearStatus,
	(FN_MEDIUM_SHUTDOWN)&__gcloader_shutdown
} ;
