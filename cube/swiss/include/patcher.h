#ifndef PATCHER_H
#define PATCHER_H

#include "devices/deviceHandler.h"

typedef struct FuncPattern
{
	u32 Length;
	u32 Loads;
	u32 Stores;
	u32 FCalls;
	u32 Branch;
	u32 Moves;
	u8 *Patch;
	u32 PatchLength;
	char *Name;
} FuncPattern;

/* the patches */
extern u8 slot_a_hdd[];
extern u32 slot_a_hdd_size;
extern u8 slot_b_hdd[];
extern u32 slot_b_hdd_size;
extern u8 slot_a_sd[];
extern u32 slot_a_sd_size;
extern u8 slot_b_sd[];
extern u32 slot_b_sd_size;

extern u8 DVDRead[];
extern u32 DVDRead_length;
extern u8 DVDReadInt[];
extern u32 DVDReadInt_length;
extern u8 DVDReadAsync[];
extern u32 DVDReadAsync_length;
extern u8 DVDReadAsyncInt[];
extern u32 DVDReadAsyncInt_length;

#define LO_RESERVE 0x80001800
#define HI_RESERVE 0x817F8000

/* Function jump locations */
#define READ_TYPE1_V1_OFFSET (base_addr)
#define READ_TYPE1_V2_OFFSET (base_addr | 0x04)
#define READ_TYPE1_V3_OFFSET (base_addr | 0x08)
#define READ_TYPE2_V1_OFFSET (base_addr | 0x0C)
#define ID_JUMP_OFFSET (base_addr | 0x10)


extern u8 _Read_original[46];
extern u8 _Read_original_2[38];
extern u8 _Read_original_3[46];
extern u32 _DVDLowReadDiskID_original[8];
extern u32 _DVDLowSeek_original[9];
extern u32 _DVDLowSeek_original_v2[9];
extern u32 _AIResetStreamSampleCount_original[9];
extern u32 _getTiming_v2[28];
extern u32 _getTiming_v1[25];
extern u32 sig_fwrite[8];
extern u32 sig_fwrite_alt[8];

int dvd_patchDVDRead(void *addr, u32 len);
void dvd_patchDVDReadID(void *addr, u32 len);
void dvd_patchAISCount(void *addr, u32 len);
void dvd_patchDVDLowSeek(void *addr, u32 len);
void install_code();
void patch_VIInit(void *addr, u32 len);
void dvd_patchVideoMode(void *addr, u32 len,int mode);
void dvd_patchfwrite(void *addr, u32 len);
void dvd_patchreset(void *addr,u32 len);
int check_dol(file_handle *disc, unsigned int *sig, int size);
void set_base_addr(int useHi);
u32 get_base_addr();
void patchZeldaWW(void *addr, u32 len,int mode);
int applyPatches(u8 *data, u32 length, u32 disableInterrupts);

#endif

