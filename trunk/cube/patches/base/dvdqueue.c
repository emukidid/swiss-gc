/***************************************************************************
* Memory Card emulation code for GC/Wii
* emu_kidid 2012
*
***************************************************************************/

#include "../../reservedarea.h"

int usb_sendbuffer_safe(const void *buffer,int size);

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct DVDDiskID DVDDiskID;

struct DVDDiskID
{
    char      gameName[4];
    char      company[2];
    u8        diskNumber;
    u8        gameVersion;
    u8        streaming;
    u8        streamingBufSize; // 0 = default
    u8        padding[22];      // 0's are stored

};

typedef struct DVDCommandBlock DVDCommandBlock;

typedef void (*DVDCBCallback)(s32 result, DVDCommandBlock* block);

struct DVDCommandBlock
{
    DVDCommandBlock* next;
    DVDCommandBlock* prev;
    u32          command;
    s32          state;
    u32          offset;
    u32          length;
    void*        addr;
    u32          currTransferSize;
    u32          transferredSize;
    DVDDiskID*   id;
    DVDCBCallback callback;
    void*        userData;
};

typedef struct DVDFileInfo  DVDFileInfo;
typedef void (*DVDCallback)(s32 result, DVDFileInfo* fileInfo);

struct DVDFileInfo
{
	DVDCommandBlock cb;

    u32             startAddr;      // disk address of file
    u32             length;         // file size in bytes

    DVDCallback     callback;
};

typedef struct StructQueue
{
	DVDFileInfo *block[8];
} StructQueue;
/*
void print_struct(DVDFileInfo* ptr) {
	if(ptr->cb.state == 0 || ptr->cb.state == 1) {
		usb_sendbuffer_safe(ptr->cb.state == 0 ? "DONE":"BUSY",4);
	}
	else {
		usb_sendbuffer_safe("UNK",3);
	}
	usb_sendbuffer_safe(": dst[",6);
	print_int_hex((u32)ptr->cb.addr);
	usb_sendbuffer_safe("] ofs[",6);
	print_int_hex(ptr->cb.offset);
	usb_sendbuffer_safe("] len[",6);
	print_int_hex(ptr->cb.length);
	usb_sendbuffer_safe("] base[",7);
	print_int_hex(ptr->startAddr);
	usb_sendbuffer_safe("] size[",7);
	print_int_hex(ptr->length);
	usb_sendbuffer_safe("]\r\n",3);
}*/

extern void process_read_queue();

void add_read_to_queue(DVDFileInfo* ptr) {
	//usb_sendbuffer_safe("ADD:",4);
	//usb_sendbuffer_safe(ptr->callback ? "CB\r\n":"NO\r\n",4);
	//print_struct(ptr);
	StructQueue *queue = (StructQueue*)(VAR_READ_DVDSTRUCT);
	int i;
	for(i = 0; i < 8; i++) {
		if(!queue->block[i] || queue->block[i] == ptr) {
			queue->block[i] = ptr;
			return;					// Found a spot in our queue for this, we're ok
		}
	}
	
	while(queue->block[0]) {
		process_read_queue();		// Queue overwhelmed, process it all.. NOW!
	}
	queue->block[0] = ptr;
}

DVDFileInfo* get_queued_read() {
	StructQueue *queue = (StructQueue*)(VAR_READ_DVDSTRUCT);
	int i, emptyslot = 0;
	
	// Clear any which are complete or have been marked as cancelled
	for(i = 0; i < 8; i++) {
		if(queue->block[i]) {
			if(!queue->block[i]->cb.state || queue->block[i]->cb.state == 10 ) {
				/*usb_sendbuffer_safe("CLR:",4);
				usb_sendbuffer_safe(queue->block[i]->callback ? "CB\r\n":"NO\r\n",4);
				print_struct(queue->block[i]);*/
				queue->block[i] = 0;
			}
		}
	}

	// return first available struct
	for(i = 0; i < 8; i++) {
		if(queue->block[i]) {
			/*usb_sendbuffer_safe("RET:",4);
			usb_sendbuffer_safe(queue->block[i]->callback ? "CB\r\n":"NO\r\n",4);
			print_struct(queue->block[i]);*/
			return queue->block[i];
		}
	}
	return 0;
}

