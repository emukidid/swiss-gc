/*
*
*   Swiss - The Gamecube IPL replacement
*
*	frag.c
*		- Wrap normal read requests around a fragmented file table
*/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

extern void dcache_flush_icache_inv(void*, u32);
extern void do_read(void*, u32, u32, u32);

// Returns the amount read from the given offset until a frag is hit
u32 read_frag(void *dst, u32 len, u32 offset) {

	u32 *fragList = (u32*)VAR_FRAG_LIST;
	int isDisc2 = (*(u32*)(VAR_DISC_2_LBA)) == (*(u32*)VAR_CUR_DISC_LBA);
	int maxFrags = (*(u32*)(VAR_DISC_2_LBA)) ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0, j = 0;
	int fragTableStart = isDisc2 ? (maxFrags*4) : 0;
	int amountToRead = len;
	int adjustedOffset = offset;
	
	// Locate this offset in the fat table and read as much as we can from a single fragment
	for(i = 0; i < maxFrags; i++) {
		int fragOffset = fragList[(i*3)+0];
		int fragSize = fragList[(i*3)+1];
		int fragSector = fragList[(i*3)+2];
		int fragOffsetEnd = fragOffset + fragSize;
		
		// Find where our read starts and read as much as we can in this frag before returning
		if(offset >= fragOffset && offset <= fragOffsetEnd) {
			// Does our read get cut off early?
			if(offset + len > fragOffsetEnd) {
				amountToRead = fragOffsetEnd - offset;
			}
			if(fragOffset != 0) {
				adjustedOffset = offset - fragOffset;
			}
			do_read(dst, amountToRead, adjustedOffset, fragSector);
			break;
		}
	}
	return amountToRead;
}

void device_frag_read(void *dst, u32 len, u32 offset)
{
	void *oDst = dst;
	u32 oLen = len;
		
	while(len != 0) {
		int amountRead = read_frag(dst, len, offset);
		len-=amountRead;
		dst+=amountRead;
		offset+=amountRead;
	}
	
	dcache_flush_icache_inv(oDst, oLen);
}