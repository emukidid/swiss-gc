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
//	void*	cb;
};

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
	store->dst = dst;
	store->len = len;
	store->ofs = ofs;
#ifdef DEBUG
	usb_sendbuffer_safe("New Read!\r\n",11);
	print_read(dst, len, ofs);
#endif
}

#define SAMPLE_SIZE_32KHZ_1MS 128
#define SAMPLE_SIZE_32KHZ_1MS_SHIFT 7

void process_queue() {

	DVDQueuedRead *store = (DVDQueuedRead*)(VAR_READ_DVDSTRUCT);
	if(store->len) {
		// read a bit
		int amountToRead;
		// Assume 32KHz, ~128 bytes @ 32KHz is going to play for 1000us
		int dmaBytesLeft = (((*(volatile u16*)0xCC00503A) & 0x7FFF)<<5);
#ifdef DEBUG
		usb_sendbuffer_safe("\r\n\r\ndmal: ",10);
		print_int_hex(dmaBytesLeft);
#endif
		int usecToPlay = ((dmaBytesLeft >> SAMPLE_SIZE_32KHZ_1MS_SHIFT) << 10) >> 2;
		// Is there enough audio playing at least to make up for 1000us?
		if(usecToPlay > *(u32*)VAR_DEVICE_SPEED) {
			// TODO Thresholds via if/else for amountAllowedToRead
			u32 amountAllowedToRead = (int)((usecToPlay / *(u32*)VAR_DEVICE_SPEED) * 1024); // lets only take up 1/4 the time reading.
			amountToRead = store->len > amountAllowedToRead ? amountAllowedToRead:store->len;
#ifdef DEBUG
			usb_sendbuffer_safe("\r\nusec: ",8);
			print_int_hex(usecToPlay);
			usb_sendbuffer_safe("\r\nCut Read: ",12);
			print_int_hex(amountAllowedToRead);
#endif
		}
		else if(dmaBytesLeft != 0)
			return;	// Tiny slice of audio is playing, don't try to read here.
		else
			amountToRead = store->len > 0x2000 ? 0x2000:store->len;	// No audio playing, read 0x2000
		
			
#ifdef DEBUG
		usb_sendbuffer_safe("Start Read:",11);
		print_read(store->dst, amountToRead, store->ofs);
#endif
		device_frag_read(store->dst, amountToRead, store->ofs);
#ifdef DEBUG
		usb_sendbuffer_safe("End Read\r\n",10);
#endif
		store->dst += amountToRead;
		store->ofs += amountToRead;
		store->len -= amountToRead;
		
		if(!store->len) {
#ifdef DEBUG
			usb_sendbuffer_safe("FinishedA\r\n",11);
#endif
			*(volatile unsigned long*)VAR_FAKE_IRQ_SET = 1;
#ifdef DEBUG
			usb_sendbuffer_safe("FinishedB\r\n",11);
#endif
		}
	}
}
