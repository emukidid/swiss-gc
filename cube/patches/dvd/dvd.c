/***************************************************************************
# DVD patch code helper functions for Swiss
# emu_kidid 2013
#**************************************************************************/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

// Perform a debug spinup/etc instead of a hard dvd reset
void handle_disc_swap()
{

	asm("mfmsr	5");
	asm("rlwinm	5,5,0,17,15");
	asm("mtmsr	5");
	
	volatile u32* dvd = (volatile u32*)0xCC006000;
	// Unlock
	// Part 1 with 'MATSHITA'
	dvd[0] |= 0x00000014;
	dvd[1] = dvd[1];
	dvd[2] = 0xFF014D41;
	dvd[3] = 0x54534849;
	dvd[4] = 0x54410200;
	dvd[7] = 1;
	while (!(dvd[0] & 0x14));
	// Part 2 with 'DVD-GAME'
	dvd[0] |= 0x00000014;
	dvd[1] = dvd[1];
	dvd[2] = 0xFF004456;
	dvd[3] = 0x442D4741;
	dvd[4] = 0x4D450300;
	dvd[7] = 1;
	while (!(dvd[0] & 0x14));
	
	// Enable drive extensions
	dvd[0] |= 0x14;
	dvd[1] = dvd[1];
	dvd[2] = 0x55010000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[7] = 1;
	while (!(dvd[0] & 0x14));
	
	// Enable motor/laser
	dvd[0] |= 0x14;
	dvd[1] = dvd[1];
	dvd[2] = 0xfe114100;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[7] = 1;
	while (!(dvd[0] & 0x14));
	
	// Set the status
	dvd[0] |= 0x14;
	dvd[1] = dvd[1];
	dvd[2] = 0xee060300;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[7] = 1;
	while (!(dvd[0] & 0x14));
	
	asm("mfmsr	5");
	asm("ori	5,5,0x8000");
	asm("mtmsr	5");
}

int is_frag_read(unsigned int offset, unsigned int len) {
	u32 *fragList = (u32*)VAR_FRAG_LIST;
	int maxFrags = (VAR_FRAG_SIZE/12), i = 0, j = 0;
	
	// If we locate that this read lies in our frag area, return true
	for(i = 0; i < maxFrags; i++) {
		int fragOffset = fragList[(i*3)+0];
		int fragSize = fragList[(i*3)+1];
		int fragSector = fragList[(i*3)+2];
		int fragOffsetEnd = fragOffset + fragSize;
		
		if(offset >= fragOffset && offset < fragOffsetEnd) {
			// Does our read get cut off early?
			if(offset + len > fragOffsetEnd) {
				return 2; // TODO Disable DVD Interrupts, perform a read for the overhang, clear interrupts.
			}
			return 1;
		}
	}
	return 0;
}

#ifdef DEBUG
extern void print_read(void* dst, u32 len, u32 ofs);
#endif 

void dvd_read_patched_section() {
	// Check if this read offset+size lies in our patched area, if so, we write a 0xE000 cmd to the drive and read from SDGecko.
	volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
	u32 dst = dvd[5];
	u32 len = dvd[4];
	u32 offset = (dvd[3]<<2);
	int ret = 0;

#ifdef DEBUG
	usb_sendbuffer_safe("New Read: ",10);
	print_read((void*)dst,len,offset);
#endif

	// If it doesn't let the value about to be passed to the DVD drive be DVD_DMA | DVD_START
	if((ret=is_frag_read(offset, len))) {
#ifdef DEBUG
		if(ret == 2) {
			usb_sendbuffer_safe("SPLIT FRAG\r\n",12);
		}
		usb_sendbuffer_safe("FRAG READ!\r\n",12);
#endif
		read_entire(dst | 0x80000000, len, offset);
		dvd[3] = 0;
		dvd[4] = 0x20;
		dvd[5] = 0;
		dvd[6] = 0x20;
		dvd[7] = 3;
	}
	else {
#ifdef DEBUG
		usb_sendbuffer_safe("NORM READ!\r\n",12);
#endif
		dvd[7] = 3;	// DVD_DMA | DVD_START
	}
}
