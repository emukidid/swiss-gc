#ifndef __BNR_H
#define __BNR_H

// Banner is 96 cols * 32 lines in RGB5A3 fmt
#define BNR_PIXELDATA_LEN (96*32*2)
#define BNR_SHORT_TEXT_LEN 0x20
#define BNR_FULL_TEXT_LEN 0x40
#define BNR_DESC_LEN 0x80

typedef struct {
	char gameName[BNR_SHORT_TEXT_LEN];
	char company[BNR_SHORT_TEXT_LEN];
	char fullGameName[BNR_FULL_TEXT_LEN];
	char fullCompany[BNR_FULL_TEXT_LEN];
	char description[BNR_DESC_LEN];
} BNRDesc;

typedef struct {
	char magic[4]; // 'BNR1' or 'BNR2'
	u8 padding[0x1C];
	u8 pixelData[BNR_PIXELDATA_LEN];
	BNRDesc desc[5];
} BNR;

#endif