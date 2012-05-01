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

static inline u32 bswap32(u32 val) {
	u32 tmp = val;
	return __lwbrx(&tmp,0);
}

// Enable single file mode on the PC
void usbgecko_single_file_mode(char *filename, int enable) {

}

// Returns 1 if the PC side is ready, 0 otherwise
int usbgecko_pc_ready() {
	return 0;
}

// Read from the remote file, returns amount read
int usbgecko_read_file(u8 *buffer, int length, u32 offset, char* filename) {
	return 0;
}

// Write to the remote file, returns amount written
int usbgecko_write_file(u8 *buffer, int length, u32 offset, char* filename) {
	return 0;
}

// Opens a directory on the PC end
int usbgecko_open_dir(char *filename) {
	return 0;
}

// Returns the next file in the directory opened with usbgecko_open_dir, NULL on end.
file_handle* usbgecko_get_entry() {
	return NULL;
}

