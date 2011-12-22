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

/* Pre-patcher magic */
#define PRE_PATCHER_MAGIC "Pre-Patched by Swiss v0.2"

#define LO_RESERVE 0x80001800
#define HI_RESERVE 0x817F8000

/* Function jump locations */
#define READ_TYPE1_V1_OFFSET (base_addr)
#define READ_TYPE1_V2_OFFSET (base_addr | 0x04)
#define READ_TYPE1_V3_OFFSET (base_addr | 0x08)
#define READ_TYPE2_V1_OFFSET (base_addr | 0x0C)
#define ID_JUMP_OFFSET (base_addr | 0x10)

int Patch_DVDHighLevelRead(u8 *data, u32 length);
int Patch_DVDLowLevelRead(void *addr, u32 length);
int Patch_480pVideo(u8 *data, u32 length);
int Patch_DVDAudioStreaming(u8 *data, u32 length);
void Patch_Fwrite(void *addr, u32 length);
void Patch_DVDReset(void *addr,u32 length);
void Patch_DVDLowReadDiskId(void *addr, u32 length);
void Patch_GXSetVATZelda(void *addr, u32 length,int mode);
void install_code();
u32 get_base_addr();
void set_base_addr(int useHi);

#endif

