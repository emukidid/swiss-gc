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

extern DEVICEHANDLER_INTERFACE __device_card_a;
extern DEVICEHANDLER_INTERFACE __device_card_b;

int initialize_card(int slot);
void setCopyGCIMode(bool _isCopyGCIMode);
void setGCIInfo(void *buffer);
char getGCIRegion(const char *gameID);
#endif

