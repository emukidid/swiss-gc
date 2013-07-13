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

void process_queue() {

	DVDQueuedRead *store = (DVDQueuedRead*)(VAR_READ_DVDSTRUCT);
	if(store->len) {
		// read a bit
		int audioPlaying = ((*(volatile u16*)0xCC00503A) & 0x7FFF);
		int amountToRead = store->len > 0x800 ? (audioPlaying ? 0x800:store->len) : store->len;
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
