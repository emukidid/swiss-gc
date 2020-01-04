/* filemeta.h
	- file meta gathering
	by emu_kidid
 */

#ifndef FILEMETA_H
#define FILEMETA_H

#include <stdint.h>
#include <stdio.h>
#include "deviceHandler.h"

// Banner is 96 cols * 32 lines in RGB5A3 fmt
#define BNR_PIXELDATA_LEN (96*32*2)
#define BNR_SHORT_TEXT_LEN 0x20
#define BNR_FULL_TEXT_LEN 0x40
#define BNR_DESC_LEN 0x80

typedef struct {
	u8 gameName[BNR_SHORT_TEXT_LEN];
	u8 company[BNR_SHORT_TEXT_LEN];
	u8 fullGameName[BNR_FULL_TEXT_LEN];
	u8 fullCompany[BNR_FULL_TEXT_LEN];
	u8 description[BNR_DESC_LEN];
} BNRDesc;

typedef struct {
	char magic[4]; // 'BNR1' or 'BNR2'
	u8 padding[0x1C];
	u8 pixelData[BNR_PIXELDATA_LEN];
	BNRDesc desc[5];
} BNR;

void meta_create_direct_texture(file_meta* meta);
void populate_meta(file_handle *f);
file_handle* meta_find_disk2(file_handle *f);
void meta_free(void* ptr);
#endif

