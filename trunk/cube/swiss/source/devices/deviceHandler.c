/* deviceHandler.c
	- devices interface (based on fileBrowser from wii64/wiiSX)
	by emu_kidid
 */

#include "deviceHandler.h"

#ifndef NULL
#define NULL 0
#endif

file_handle* deviceHandler_initial;

int  (*deviceHandler_init)(file_handle*) = NULL;
int  (*deviceHandler_readDir)(file_handle*, file_handle**) = NULL;
int  (*deviceHandler_readFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_writeFile)(file_handle*, void*, unsigned int) = NULL;
int  (*deviceHandler_seekFile)(file_handle*, unsigned int, unsigned int) = NULL;
void (*deviceHandler_setupFile)(file_handle*, file_handle*);
int  (*deviceHandler_deinit)() = NULL;
