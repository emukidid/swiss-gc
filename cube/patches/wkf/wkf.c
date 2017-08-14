/***************************************************************************
* Wiikey Fusion related patches
* emu_kidid 2015
***************************************************************************/

#include "../../reservedarea.h"
#include "../base/common.h"

extern void print_int_hex(unsigned int num);

void wkfWriteOffset(u32 offset) {
	volatile u32* wkf = (volatile u32*)0xCC006000;
	wkf[2] = 0xDE000000;
	wkf[3] = offset;
	wkf[4] = 0x5A000000;
	wkf[6] = 0;
	wkf[8] = 0;
	wkf[7] = 1;
	while( wkf[7] & 1);
}


void wkfRead(void* dst, int len, u32 offset) {
	volatile u32* dvd = (volatile u32*)0xCC006000;
	dvd[2] = 0xA8000000;
	dvd[3] = offset >> 2;
	dvd[4] = len;
	dvd[5] = (unsigned long)dst;
	dvd[6] = len;
	dvd[7] = 3; // enable reading!
}

// Adjusts the offset on the WKF for fragmented reads
void adjust_read() {
	volatile u32* dvd = (volatile u32*)0xCC006000;
	
	u32 dst = dvd[5];
	u32 len = dvd[4];
	u32 offset = (dvd[3]<<2);
	
	u32 *fragList = (u32*)VAR_FRAG_LIST;
	int isDisc2 = (*(u32*)(VAR_DISC_2_LBA)) == (*(u32*)VAR_CUR_DISC_LBA);
	int maxFrags = (*(u32*)(VAR_DISC_2_LBA)) ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0, j = 0;
	int fragTableStart = isDisc2 ? (maxFrags*3) : 0;
	u32 adjustedOffset = offset;

	// Locate this offset in the fat table
	for(i = 0; i < maxFrags; i++) {
		u32 fragOffset = fragList[(i*3)+0];
		u32 fragSize = fragList[(i*3)+1] & 0x7FFFFFFF;
		u32 fragSector = fragList[(i*3)+2];
		u32 fragOffsetEnd = fragOffset + fragSize;
		u32 isPatchFrag = fragList[(i*3)+1] >> 31;
		
		// Find where our read starts
		if(offset >= fragOffset && offset < fragOffsetEnd) {
			if(isPatchFrag) {
#ifdef DEBUG
				usb_sendbuffer_safe("FRAG INFO: ofs: ",16);
				print_int_hex(fragOffset);
				usb_sendbuffer_safe(" len: ",6);
				print_int_hex(fragSize);
				usb_sendbuffer_safe(" sec: ",6);
				print_int_hex(fragSector);
				usb_sendbuffer_safe(" end: ",6);
				print_int_hex(fragOffsetEnd);
				usb_sendbuffer_safe(" frg? ",6);
				print_int_hex(isPatchFrag);
				usb_sendbuffer_safe("\r\n",2);

				usb_sendbuffer_safe("PATCH READ: dst: ",17);
				print_int_hex(dst);
				usb_sendbuffer_safe(" len: ",6);
				print_int_hex(len);
				usb_sendbuffer_safe(" ofs: ",6);
				print_int_hex(offset);
				usb_sendbuffer_safe("\r\n",2);
#endif
				device_frag_read((void*)(dst | 0x80000000), len, offset);
				dcache_flush_icache_inv((void*)(dst | 0x80000000), len);
#ifdef DEBUG				
				usb_sendbuffer_safe("data: \r\n",8);
				print_int_hex(*(u32*)((dst+0)| 0x80000000));
				print_int_hex(*(u32*)((dst+4)| 0x80000000));
				print_int_hex(*(u32*)((dst+len-4)| 0x80000000));
#endif
				dvd[3] = 0;
				dvd[4] = 0x20;
				dvd[5] = 0;
				dvd[6] = 0x20;
				dvd[7] = 3;
				break;
			}
			else {
#ifdef DEBUG
				usb_sendbuffer_safe("NORM READ!\r\n",12);
#endif
				if(fragOffset != 0) {
					adjustedOffset = offset - fragOffset;
				}
				fragSector = fragSector + (adjustedOffset>>9);
				if(*(volatile unsigned int*)VAR_TMP1 != fragSector) {
					wkfWriteOffset(fragSector);
					*(volatile unsigned int*)VAR_TMP1 = fragSector;
				}
				wkfRead((void*)dst, len, adjustedOffset & 511);
				break;
			}
		}
	}
}

void wkfReinit(void) {
	asm("mfmsr	5");
	asm("rlwinm	5,5,0,17,15");
	asm("mtmsr	5");
	// TODO re-init WKF
}

//void swap_disc() {
//	int isDisc1 = (*(u32*)(VAR_DISC_1_LBA)) == (*(u32*)VAR_CUR_DISC_LBA);
//	*(u32*)VAR_CUR_DISC_LBA = isDisc1 ? *(u32*)(VAR_DISC_2_LBA) : *(u32*)(VAR_DISC_1_LBA);
//}

