/* deviceHandler-CARD.h
	- device interface for Memory Card (CARD)
	by emu_kidid
 */


#ifndef DEVICE_HANDLER_CARD_H
#define DEVICE_HANDLER_CARD_H

#include "../deviceHandler.h"

extern file_handle initial_CARDA;
extern file_handle initial_CARDB;

int  deviceHandler_CARD_readDir(file_handle*, file_handle**);
int  deviceHandler_CARD_readFile(file_handle*, void*, unsigned int);
int  deviceHandler_CARD_writeFile(file_handle*, void*, unsigned int);
int  deviceHandler_CARD_seekFile(file_handle*, unsigned int, unsigned int);
void deviceHandler_CARD_setupFile(file_handle* file, file_handle* file2);
int  deviceHandler_CARD_init(file_handle* file);
int  deviceHandler_CARD_deinit(file_handle* file);
int deviceHandler_CARD_deleteFile(file_handle* file);

int initialize_card(int slot);

#endif

