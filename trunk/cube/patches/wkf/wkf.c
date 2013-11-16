/*
*
*   Swiss - The Gamecube IPL replacement
*
*	wkf.c
*		- Wiikey Fusion related patches
*/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

void wkfWriteOffset(unsigned int offset) {
	static volatile unsigned int* const wkf = (unsigned int*)0xCC006000;
	wkf[2] = 0xDE000000;
	wkf[3] = offset;
	wkf[4] = 0x5A000000;
	wkf[5] = 0;
	wkf[6] = 0;
	wkf[8] = 0;
	wkf[7] = 1;
	while( wkf[7] & 1);
}

void do_read(u32 offset, u32 len, u32 dst) {
	volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
	dvd[2] = 0xA8000000;
	dvd[3] = offset >> 2;
	dvd[4] = len;
	dvd[5] = (unsigned long)dst;
	dvd[6] = len;
	dvd[7] = 3; // enable reading!
}
/*
// Look at Patcher.c for the cleaner version of this with less magic numbers
void Patch_DVDLowLevelReadForWKF(void *addr, u32 length) {
	void *addr_start = addr;
	void *addr_end = addr+length;	

	while(addr_start<addr_end) 
	{
		// Patch Read to adjust the offset for fragmented files
		if(*(u32*)(addr_start) == 0x3C60A800) 
		{
			int i = 0;
			// Patch the DI start
			while((*(u32*)(addr_start + i)) != 0x9004001C) i+=4;
			
			u32 newval = (u32)(0x80001800 - ((u32)addr_start + i));
			newval&= 0x03FFFFFC;
			newval|= 0x48000001;
			*(u32*)(addr_start + i) = newval;
			usb_sendbuffer_safe("patched\r\n", 9);
			break;
		}	
		addr_start += 4;
	}
}*/

// Adjusts the offset on the WKF for fragmented reads
void adjust_read() {
	volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
	
	u32 dst = dvd[5];
	u32 len = dvd[4];
	u32 offset = (dvd[3]<<2);
	
	u32 *fragList = (u32*)VAR_FRAG_LIST;
	int isDisc2 = (*(u32*)(VAR_DISC_2_LBA)) == (*(u32*)VAR_CUR_DISC_LBA);
	int maxFrags = (*(u32*)(VAR_DISC_2_LBA)) ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0, j = 0;
	int fragTableStart = isDisc2 ? (maxFrags*4) : 0;
	u32 adjustedOffset = offset;

	// Locate this offset in the fat table
	for(i = 0; i < maxFrags; i++) {
		int fragOffset = fragList[(i*3)+0];
		int fragSize = fragList[(i*3)+1];
		int fragSector = fragList[(i*3)+2];
		int fragOffsetEnd = fragOffset + fragSize;
		
		// Find where our read starts
		if(offset >= fragOffset && offset <= fragOffsetEnd) {
			if(fragOffset != 0) {
				adjustedOffset = offset - fragOffset;
			}
			fragSector = fragSector + (adjustedOffset>>9);
			if(*(volatile unsigned int*)VAR_TMP4 != fragSector) {
				wkfWriteOffset(fragSector);
				*(volatile unsigned int*)VAR_TMP4 = fragSector;
			}
			do_read(adjustedOffset & 511, len, dst);
			break;
		}
	}
}

void swap_disc() {
	int isDisc1 = (*(u32*)(VAR_DISC_1_LBA)) == (*(u32*)VAR_CUR_DISC_LBA);
	*(u32*)VAR_CUR_DISC_LBA = isDisc1 ? *(u32*)(VAR_DISC_2_LBA) : *(u32*)(VAR_DISC_1_LBA);
}
