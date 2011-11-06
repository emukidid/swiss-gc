/* deviceHandler-CARD.h
	- device interface for Memory Card (CARD)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_CARD_H
#define DEVICE_HANDLER_CARD_H

#include "../deviceHandler.h"
#include <ogc/card.h>

typedef struct {
	u8 gamecode[4];
	u8 company[2];
	u8 reserved01;	/*** Always 0xff ***/
	u8 banner_fmt;
	u8 filename[CARD_FILENAMELEN];
	u32 time;
	u32 icon_addr;
	u16 icon_fmt;
	u16 icon_speed;
	u8 unknown1;	/*** Permission key? ***/
	u8 unknown2;	/*** Again, unknown ***/
	u16 index;		/*** Ignore - and throw away ***/
	u16 filesize8;	/*** File size / 8192 ***/
	u16 reserved02;	/*** Always 0xffff ***/
	u32 comment_addr;
} __attribute__((__packed__)) GCI;

extern file_handle initial_CARDA;
extern file_handle initial_CARDB;

int  deviceHandler_CARD_readDir(file_handle*, file_handle**, unsigned int);
int  deviceHandler_CARD_readFile(file_handle*, void*, unsigned int);
int  deviceHandler_CARD_writeFile(file_handle*, void*, unsigned int);
int  deviceHandler_CARD_seekFile(file_handle*, unsigned int, unsigned int);
void deviceHandler_CARD_setupFile(file_handle* file, file_handle* file2);
int  deviceHandler_CARD_init(file_handle* file);
int  deviceHandler_CARD_deinit(file_handle* file);
int deviceHandler_CARD_deleteFile(file_handle* file);

int initialize_card(int slot);

#endif

