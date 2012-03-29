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
extern u8 sd_bin[];
extern u32 sd_bin_size;

extern u8 DVDRead[];
extern u32 DVDRead_length;
extern u8 DVDReadInt[];
extern u32 DVDReadInt_length;
extern u8 DVDReadAsync[];
extern u32 DVDReadAsync_length;
extern u8 DVDReadAsyncInt[];
extern u32 DVDReadAsyncInt_length;

/* Patch code variables block */
#define VAR_AREA 			(0x81800000)		// Base location of our variables
#define VAR_AREA_SIZE		(0x100)				// Size of our variables block
#define VAR_DISC_1_LBA 		(VAR_AREA-0x100)	// is the base file sector for disk 1
#define VAR_DISC_2_LBA 		(VAR_AREA-0xF0)		// is the base file sector for disk 2
#define VAR_CUR_DISC_LBA 	(VAR_AREA-0xE0)		// is the currently selected disk sector
#define VAR_EXI_BUS_SPD 	(VAR_AREA-0xD0)		// is the EXI bus speed (16mhz vs 32mhz)
#define VAR_SD_TYPE 		(VAR_AREA-0xCC)		// is the Card Type (SDHC=0, SD=1)
#define VAR_EXI_FREQ 		(VAR_AREA-0xC8)		// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
#define VAR_EXI_SLOT 		(VAR_AREA-0xC4)		// is the EXI slot (0 = slot a, 1 = slot b)
#define VAR_DISC_1_ID 		(VAR_AREA-0xC0)		// is the Disc ID of the first disk
#define VAR_DISC_2_ID 		(VAR_AREA-0xA0)		// is the Disc ID of the second disk
#define VAR_32B_BUF			(VAR_AREA-0x80)		// is a 32 byte area to redirect reads to
#define VAR_TMP1  			(VAR_AREA-0x60)		// space for a variable if required
#define VAR_TMP2  			(VAR_AREA-0x5C)		// space for a variable if required
#define VAR_TMP3  			(VAR_AREA-0x58)		// space for a variable if required
#define VAR_TMP4  			(VAR_AREA-0x54)		// space for a variable if required
#define VAR_CB_ADDR			(VAR_AREA-0x50)		// high level read callback addr
#define VAR_CB_ARG1			(VAR_AREA-0x4C)		// high level read callback r3
#define VAR_CB_ARG2			(VAR_AREA-0x48)		// high level read callback r4

/* Pre-patcher magic */
#define PRE_PATCHER_MAGIC "Pre-Patched by Swiss v0.2"

#define LO_RESERVE 0x80001800
#define HI_RESERVE (0x817FE800-VAR_AREA_SIZE)

/* Function jump locations */
#define READ_TYPE1_V1_OFFSET (base_addr)
#define READ_TYPE1_V2_OFFSET (base_addr | 0x04)
#define READ_TYPE1_V3_OFFSET (base_addr | 0x08)
#define READ_TYPE2_V1_OFFSET (base_addr | 0x0C)
#define ID_JUMP_OFFSET (base_addr | 0x10)
#define OS_RESTORE_INT_OFFSET (base_addr | 0x14)

int Patch_DVDHighLevelRead(u8 *data, u32 length);
int Patch_DVDLowLevelRead(void *addr, u32 length);
int Patch_480pVideo(u8 *data, u32 length);
int Patch_DVDAudioStreaming(u8 *data, u32 length);
int Patch_DVDStatusFunctions(u8 *data, u32 length);
void Patch_Fwrite(void *addr, u32 length);
void Patch_DVDReset(void *addr,u32 length);
void Patch_DVDLowReadDiskId(void *addr, u32 length);
void Patch_GXSetVATZelda(void *addr, u32 length,int mode);
int Patch_OSRestoreInterrupts(void *addr, u32 length);
void install_code();
u32 get_base_addr();
void set_base_addr(int useHi);

#endif

