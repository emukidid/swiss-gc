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
	DrawImage(TEX_BANNER, x, y, 96*zoom, 32*zoom, 0, 0.0f, 1.0f, 0.0f, 1.0f);
	return BNROffset;
}

