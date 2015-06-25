/***************************************************************************
* Read code for GC/Wii via Wiikey Fusion/WASP
* emu_kidid 2015
***************************************************************************/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

void wkfWriteOffset(u32 offset) {
	// TODO mask this cmd
	static volatile unsigned int* const wkf = (unsigned int*)0xCC006000;
	wkf[2] = 0xDE000000;
	wkf[3] = offset;
	wkf[4] = 0x5A000000;
	wkf[6] = 0;
	wkf[8] = 0;
	wkf[7] = 1;
	while( wkf[7] & 1);
}

void __wkfReadSectors(void* dst, unsigned int len, u32 offset) {
	// TODO mask this read
	static volatile unsigned int* const wkf = (unsigned int*)0xCC006000;
	wkf[2] = 0xA8000000;
	wkf[3] = offset >> 2;
	wkf[4] = len;
	wkf[5] = (u32)dst&0x01FFFFFF;
	wkf[6] = len;
	wkf[7] = 3; // DMA | START
	while(wkf[7] & 1);
}

void* mymemcpy(void* dest, const void* src, u32 count)
{
	char* tmp = (char*)dest,* s = (char*)src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

void do_read(void *dst, u32 size, u32 offset, u32 sectorLba) {
	// TODO read into a temporary buffer and handle alignment.
	wkfWriteOffset(sectorLba);
	__wkfReadSectors(dst, size, offset);
}
