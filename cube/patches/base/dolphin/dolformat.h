#ifndef DOLFORMAT_H
#define DOLFORMAT_H

#include "common.h"

#define DOL_MAX_TEXT 7
#define DOL_MAX_DATA 11

typedef struct DOLImage {
	u32 textData[DOL_MAX_TEXT];
	u32 dataData[DOL_MAX_DATA];
	void *text[DOL_MAX_TEXT];
	void *data[DOL_MAX_DATA];
	u32 textLen[DOL_MAX_TEXT];
	u32 dataLen[DOL_MAX_DATA];
	void *bss;
	u32 bssLen;
	void (*entry)(void);
	u8 padding[28];
} DOLImage;

#endif /* DOLFORMAT_H */
