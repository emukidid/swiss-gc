/**
 * USB Gecko Driver for Gamecube & Wii
 *
 * Communicates with a client on the PC which serves file data over USB Gecko
 * Written by emu_kidid
**/

#include <stdio.h>
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <ogc/usbgecko.h>
#include <ogc/exi.h>
#include "main.h"
#include "swiss.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "deviceHandler.h"
#include "usbgecko.h"

u8 ASK_READY = 0x15;
u8 ASK_OPENPATH = 0x16;
u8 ASK_GETENTRY = 0x17;
u8 ASK_SERVEFILE = 0x18;
u8 ASK_LOCKFILE = 0x19;
u8 ANS_READY = 0x25;
u8 *replybuffer = NULL; 
file_handle filehndl;

char served_file[1024];		// The file we're currently being served by the PC

typedef struct {
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size to read
} usb_data_req;

static inline u32 bswap32(u32 val) {
	u32 tmp = val;
	return __lwbrx(&tmp,0);
}

u8 *get_buffer() {
	if(replybuffer == NULL) {
		replybuffer = (u8*)memalign(32,2048);
	}
	return replybuffer;
}

// Returns 1 if the PC side is ready, 0 otherwise
int usbgecko_pc_ready() {
	// Ask PC, are you ready?
	usb_sendbuffer_safe(1, &ASK_READY, 1);
	
	// Try get a reply
	usb_recvbuffer_safe(1, get_buffer(), 1);
	if(get_buffer()[0] == ANS_READY) {
		return 1;
	}
	return 0;
}

void usbgecko_lock_file(int lock) {
	
	if(!lock) {	// Send a zero byte read to end locked mode
		usb_data_req req;
		req.offset = 0;
		req.size = 0;
		usb_sendbuffer_safe(1, &req, sizeof(usb_data_req));
	}
	else {
		// Tell PC, I'm ready to read from a file
		usb_sendbuffer_safe(1, &ASK_LOCKFILE, 1);
		// Try get a reply
		usb_recvbuffer_safe(1, get_buffer(), 1);
		if(get_buffer()[0] == ANS_READY) {
			// all good
		}
	}
}

void usbgecko_served_file(char *filename) {
	if(!strcmp((const char*)served_file, filename)) {
		return;
	}
	else {
		usbgecko_lock_file(0);
		// Tell PC, I'm ready to read from a file
		usb_sendbuffer_safe(1, &ASK_SERVEFILE, 1);
		// Try get a reply
		usb_recvbuffer_safe(1, get_buffer(), 1);
		if(get_buffer()[0] == ANS_READY) {
			strcpy(served_file, filename);
			usb_sendbuffer_safe(1, served_file, 1024);
		}
		usbgecko_lock_file(1);
	}
}

// Read from the remote file, returns amount read
int usbgecko_read_file(void *buffer, u32 length, u32 offset, char* filename) {
	if(!length) return 0;
	usbgecko_served_file(filename);

	usb_data_req req;
	req.offset = bswap32(offset);
	req.size = bswap32(length);
	usb_sendbuffer_safe(1, &req, sizeof(usb_data_req));
	usb_recvbuffer_safe(1, buffer, length);
	return length;
}

// Write to the remote file, returns amount written
int usbgecko_write_file(void *buffer, u32 length, u32 offset, char* filename) {
	return 0;
}

// Opens a directory on the PC end
int usbgecko_open_dir(char *filename) {
	usbgecko_lock_file(0);
	// Tell PC, I'm ready to send a path name
	usb_sendbuffer_safe(1, &ASK_OPENPATH, 1);
	
	// Try get a reply
	usb_recvbuffer_safe(1, get_buffer(), 1);
	if(get_buffer()[0] == ANS_READY) {
		usb_sendbuffer_safe(1, filename, 4096);
		return 1;
	}
	return 0;
}

// Returns the next file in the directory opened with usbgecko_open_dir, NULL on end.
file_handle* usbgecko_get_entry() {
	// Tell PC, I'm ready to receive a file_handle
	usb_sendbuffer_safe(1, &ASK_GETENTRY, 1);
	
	// Try get a reply
	usb_recvbuffer_safe(1, get_buffer(), 1);
	if(get_buffer()[0] == ANS_READY) {
		usb_recvbuffer_safe(1, get_buffer(), sizeof(file_handle));
		if(!get_buffer()[0]) {
			return NULL;
		}
		else {
			memset(&filehndl, 0, sizeof(file_handle));
			memcpy(&filehndl, get_buffer(), sizeof(file_handle));
			filehndl.size = bswap32(filehndl.size);
			filehndl.fileAttrib = bswap32(filehndl.fileAttrib);
			return &filehndl;
		}
	}
	return NULL;
}

