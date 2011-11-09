/****************************************************************************
* bitmap.c
*
* libogc example to display a Windows bitmap.
*
* NOTE: Windows BMP must comply to the following:
*	24-bit uncompressed (99.99% of Windows Editor output)
*	Be less than or equal to 640 x 480(MPAL/NTSC) 640x528(PAL)
*
* This example decodes a BMP file onto the linear framebuffer.
****************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "bnr2yuv.h"
#include "banner.h"
#include "dvd.h"
#include "swiss.h"
#include "main.h"
#include "FrameBufferMagic.h"

/*void drawBitmap(const unsigned long* image, int scrX, int scrY, int w, int h)
{
	int i, j;
	int current = 0;

	for (j = 0 + scrY; j < h + scrY; j++)
	{
		if((j < vmode->xfbHeight)&&(j < vmode->efbHeight)&&(j < vmode->viHeight)) {
			for (i = 0; i < w/2; i++)
			{
				if(image[current]!=0xFF80FF80)
					xfb[whichfb][320 * j + i + scrX] = image[current];
				current++;
			}
		}
	}
}*/

/*void menuDrawBanner(u32 *curFrameBuffer,u32 *banner ,int scrX, int scrY, int iZoom)
{
	int w = 96;
	int h = 32;
	int bufX = 0;
	int bufY = 0;
  int x, y, i, j, zx, zy, offset;

  scrX /= 2;
  w /= 2;
  for (y = scrY; y < scrY + h; y++) {
	  for (x = scrX; x < scrX + w; x++) {
	    offset = x - scrX + bufX + ((y - scrY + bufY) * 96) /2;
		  zx = scrX + (x - scrX) * iZoom;
		  zy = scrY + (y - scrY) * iZoom;
		  for (i = 0; i < iZoom; i++) {
		    for (j = 0; j < iZoom; j++) {
		      curFrameBuffer[(zx + i) + 640 * (zy + j) /2] = banner[offset];
		    }
		  }
	  }
  }
}*/

//Lets parse the entire game FST in search for the banner
unsigned int getBannerOffset() {
	unsigned int   	buffer[2]; 
	unsigned long   fst_offset=0,filename_offset=0,entries=0,string_table_offset=0,fst_size=0; 
	unsigned long   i,offset; 
	char   filename[256]; 
	
  // lets just quickly check if this is a valid GCM (contains magic)
  deviceHandler_seekFile(&curFile,0x1C,DEVICE_HANDLER_SEEK_SET);
  if((deviceHandler_readFile(&curFile,(unsigned char*)buffer,sizeof(int))!=sizeof(int)) || (buffer[0]!=0xC2339F3D)) {
    return 0;
  }

  // get FST offset and size
  deviceHandler_seekFile(&curFile,0x424,DEVICE_HANDLER_SEEK_SET);
  if(deviceHandler_readFile(&curFile,(unsigned char*)buffer,sizeof(int)*2)!=sizeof(int)*2) {
    return 0;
  }  
  
  fst_offset=buffer[0];
  fst_size	=buffer[1];

  if((!fst_offset) || (!fst_size)) {
    return 0;
  }
      
  // allocate space for FST table
  char* FST = (char*)memalign(32,fst_size);		//malloc dies on NTSC for some reason.. bad libogc! 
  if(!FST) {
  	return 0;	//didn't fit, doesn't matter it's just for a banner
	}
	
  // read the FST
  deviceHandler_seekFile(&curFile,fst_offset,DEVICE_HANDLER_SEEK_SET);
  if(deviceHandler_readFile(&curFile,FST,fst_size)!=fst_size) {
    free(FST);
    return 0;
  }
  
  // number of entries and string table location
  entries = *(unsigned int*)&FST[8];
  string_table_offset=12*entries; 
    
  // go through every entry
	for (i=1;i<entries;i++) 
	{ 
		offset=i*0x0c; 
		if(FST[offset]==0) //skip directories
		{ 
			filename_offset=(unsigned int)FST[offset+1]*256*256+(unsigned int)FST[offset+2]*256+(unsigned int)FST[offset+3]; 
			memset(filename,0,256);
			strcpy(filename,&FST[string_table_offset+filename_offset]); 
			unsigned int loc = 0;
			memcpy(&loc,&FST[offset+4],4);
			if((!stricmp(filename,"opening.bnr")) || (!stricmp(filename,"OPENING.BNR"))) 
			{
				free(FST);
				return loc;
			}		
		} 
	}
	free(FST);
	return 0;
}

/* Courtesy of Maximillionop for GCOS 1.x */

/*static void RGB5A1TORGB888(u16 RGB5A1, RGBAPoint *RGBA, RGBPoint bgColor)
{
	u16 WorkPoint = RGB5A1;
	double Alpha, bgAlpha;

	RGBA->Alpha = (((WorkPoint & (0x8000)) >> 15) == 1); // Bit 16
	if(RGBA->Alpha)
	{
		RGBA->RGB.B = (WorkPoint & 0x1F); // Bits 1 to 5
		RGBA->RGB.B = ((RGBA->RGB.B << 3) | (RGBA->RGB.B >> 2));

		RGBA->RGB.G = (WorkPoint & 0x03E0) >> 5; // Bits 6 to 10
		RGBA->RGB.G = ((RGBA->RGB.G << 3) | (RGBA->RGB.G >> 2));

		RGBA->RGB.R = (WorkPoint & 0x7C00) >> 10; // Bits 11 to 15
		RGBA->RGB.R = ((RGBA->RGB.R << 3) | (RGBA->RGB.R >> 2));
	} else {
		Alpha = (WorkPoint & 0x7000) >> 12; // Bits 13 to 15
		Alpha = (Alpha / 7.0);
		bgAlpha = 1.0 - Alpha;

		RGBA->RGB.B = (WorkPoint & 0x0F); // Bits 1 to 4
		RGBA->RGB.B = (u8)((double)(16 * RGBA->RGB.B) * Alpha + (double)(bgColor.B) * bgAlpha);

		RGBA->RGB.G = (WorkPoint & 0xF0) >> 4; // Bits 5 to 8
		RGBA->RGB.G = (u8)((double)(16 * RGBA->RGB.G) * Alpha + (double)(bgColor.G) * bgAlpha);

		RGBA->RGB.R = (WorkPoint & 0x0F00) >> 8; // Bits 9 to 12
		RGBA->RGB.R = (u8)((double)(16 * RGBA->RGB.R) * Alpha + (double)(bgColor.R) * bgAlpha);
	}
}

static void RGB888TOYCBYCr(RGBAPoint RGBA1, RGBAPoint RGBA2, YCbYCrPoint *YCbYCr)
{
	long tR,tG,tB;
	tR = (RGBA1.RGB.R+RGBA2.RGB.R)/2;
	tG = (RGBA1.RGB.G+RGBA2.RGB.G)/2;
	tB = (RGBA1.RGB.B+RGBA2.RGB.B)/2;
	YCbYCr->Y1 = (u8)((0.29900 * RGBA1.RGB.R) + (0.58700 * RGBA1.RGB.G) + (0.11400 * RGBA1.RGB.B));
	YCbYCr->Y2 = (u8)((0.29900 * RGBA2.RGB.R) + (0.58700 * RGBA2.RGB.G) + (0.11400 * RGBA2.RGB.B));
	YCbYCr->Cb = (u8)((-0.16874 * tR) - (0.33126 * tG) + (0.50000 * tB) + 0x80);
	YCbYCr->Cr = (u8)((0.50000 * tR) - (0.41869 * tB) - (0.08131 * tB) + 0x80);
}

void ConvertBanner(BannerData RGB5A1Array, RGBPoint BgColor,int x, int y, int zoom)
{
	RGBAPoint RGBAArray[32][96];
	YCbYCrPoint YCbYCrArray[32][48];
	unsigned int bannerYUV[sizeof(YCbYCrArray)];
	int X, Y; // Image coordinates
	int TileX, TileY; // In-Tile coordinates
	memset(YCbYCrArray,0,sizeof(YCbYCrArray));
	memset(RGBAArray,0,sizeof(RGBAArray));
	for(Y = 0; Y < 32; Y++)
	{
		for(X = 0; X < 96; X++)
		{
			TileX = X % 4;
			TileY = Y % 4;
			RGB5A1TORGB888(RGB5A1Array[(Y - TileY) / 4][(X - TileX) / 4][TileY][TileX], &RGBAArray[Y][X], BgColor);
			if(X % 2) //every odd pixel
			{
				RGB888TOYCBYCr(RGBAArray[Y][X-1], RGBAArray[Y][X], &YCbYCrArray[Y][(X - 1) / 2]);		
			}
		}
	}
	memcpy(bannerYUV,YCbYCrArray,sizeof(YCbYCrArray));
	// Draw YCbYCrArray, it´s a 48*32 array containing the whole image
	menuDrawBanner(xfb[whichfb],bannerYUV,x,y,zoom);
}*/

extern u8 *bannerData;

void showBanner(int x, int y, int zoom)
{	
	unsigned int BNROffset = getBannerOffset();
	if(!BNROffset) {
		memcpy(bannerData,blankbanner+0x20,0x1800);
	}
	else
	{
		deviceHandler_seekFile(&curFile,BNROffset+0x20,DEVICE_HANDLER_SEEK_SET);
		if(deviceHandler_readFile(&curFile,(unsigned char*)&bannerData,0x1800)!=0x1800) {
			memcpy(bannerData,blankbanner+0x20,0x1800);
		}
	}
	DCFlushRange(bannerData,0x1800);
	DrawImage(TEX_BANNER, x, y, 96*zoom, 32*zoom, 0, 0.0f, 1.0f, 0.0f, 1.0f);

/*	BannerData Banner;
	RGBPoint BackgroundColor;
	unsigned int BNROffset = getBannerOffset();
	if(!BNROffset) {
		memcpy(Banner,blankbanner+0x20,0x1800);
	}
	else
	{
		deviceHandler_seekFile(&curFile,BNROffset+0x20,DEVICE_HANDLER_SEEK_SET);
		if(deviceHandler_readFile(&curFile,(unsigned char*)&Banner,0x1800)!=0x1800) {
			memcpy(Banner,blankbanner+0x20,0x1800);
		}
	}

	BackgroundColor.R = 0;
	BackgroundColor.G = 0;
	BackgroundColor.B = 0;
	ConvertBanner(Banner, BackgroundColor,x,y,zoom);*/
}

