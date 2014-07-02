/***************************************************************************
* Second level (crappy!) DVD Queue code for GC/Wii
* emu_kidid 2012
*
***************************************************************************/

#include "../../reservedarea.h"

int usb_sendbuffer_safe(const void *buffer,int size);

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct DVDQueuedRead DVDQueuedRead;
//typedef void (*DVDCallback)(s32 result, DVDFileInfo* fileInfo);
struct DVDQueuedRead
{
	void*	dst;
    u32		len;
    u32		ofs;
	void*	oldDst;
	u32		oldLen;
	u32		oldOfs;
};

void flush_queue(void);

#ifdef DEBUG
void print_read(void* dst, u32 len, u32 ofs) {
	usb_sendbuffer_safe("dst[",4);
	print_int_hex((u32)dst);
	usb_sendbuffer_safe("] ofs[",6);
	print_int_hex(ofs);
	usb_sendbuffer_safe("] len[",6);
	print_int_hex(len);
	usb_sendbuffer_safe("]\r\n",3);
}
#endif

void add_read_to_queue(void* dst, u32 len, u32 ofs) {
	DVDQueuedRead *store = (DVDQueuedRead*)(VAR_READ_DVDSTRUCT);
	store->dst = store->oldDst = dst;
	store->len = store->oldLen = len;
	store->ofs = store->oldOfs = ofs;
#ifdef DEBUG
	usb_sendbuffer_safe("New Read: ",10);
	print_read(dst, len, ofs);
#endif
}

#define SAMPLE_SIZE_32KHZ_1MS 128
#define SAMPLE_SIZE_32KHZ_1MS_SHIFT 7

void process_queue(void) {

	DVDQueuedRead *store = (DVDQueuedRead*)(VAR_READ_DVDSTRUCT);
	if(store->len) {
		// read a bit
		int amountToRead = 0;
		// Assume 32KHz, ~128 bytes @ 32KHz is going to play for 1000us
		int dmaBytesLeft = (((*(volatile u16*)0xCC00503A) & 0x7FFF)<<5);

		int usecToPlay = ((dmaBytesLeft >> SAMPLE_SIZE_32KHZ_1MS_SHIFT) << 10) >> 1;
		u32 amountAllowedToRead;
		// Is there enough audio playing at least to make up for 1024 bytes to be read?
		if(usecToPlay > *(u32*)VAR_DEVICE_SPEED) {
			// Thresholds based on SD speed tests on 5 different cards (EXI limit reached before SPI)
			amountAllowedToRead = usecToPlay<<1;
		}
		if(dmaBytesLeft == 0) {
			amountAllowedToRead = 8192;
		}
		if(dmaBytesLeft != 0 && amountToRead == 0) {
			*(volatile u32*)VAR_INTERRUPT_TIMES += 1;
			if((*(volatile u32*)VAR_INTERRUPT_TIMES) < 4)
				return;
			*(volatile u32*)VAR_INTERRUPT_TIMES = 0;
			amountAllowedToRead = 8192;
		}
		amountToRead = store->len > amountAllowedToRead ? amountAllowedToRead:store->len;
		
		if(amountToRead == 0)
			return;
		
		device_frag_read(store->dst, amountToRead, store->ofs);

		store->dst += amountToRead;
		store->ofs += amountToRead;
		store->len -= amountToRead;
		
		if(!store->len) {
			*(volatile unsigned long*)VAR_FAKE_IRQ_SET = 1;
#ifdef DEBUG
			usb_sendbuffer_safe("Finished Read ",14);
			print_read(store->oldDst, store->oldLen, store->oldOfs);
#endif
		}
	}
}

// Reads immediately, no pseudo queuing.
void read_entire(void* dst, u32 len, u32 ofs) {
	device_frag_read(dst, len, ofs);
	dcache_flush_icache_inv(dst, len);
}	
