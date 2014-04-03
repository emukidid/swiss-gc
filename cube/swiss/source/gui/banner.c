/****************************************************************************
* banner.c
*
* Functions to load and show a GC game banner using GX.
****************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "banner.h"
#include "dvd.h"
#include "swiss.h"
#include "main.h"
#include "FrameBufferMagic.h"



/*
extern u8 *bannerData;

u32 showBanner(int x, int y, int zoom)
{	
	unsigned int BNROffset = getBannerOffset();
	
	if(!BNROffset) {
		memcpy(bannerData,blankbanner+0x20,0x1800);
	}
	else
	{
		deviceHandler_seekFile(&curFile,BNROffset+0x20,DEVICE_HANDLER_SEEK_SET);
		if(deviceHandler_readFile(&curFile,bannerData,0x1800)!=0x1800) {
			memcpy(bannerData,blankbanner+0x20,0x1800);
		}
	}
	DCFlushRange(bannerData,0x1800);
	DrawImage(TEX_BANNER, x, y, 96*zoom, 32*zoom, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	return BNROffset;
}*/

