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
	u32 offsetFoundAt;
} FuncPattern;

/* the SDGecko/IDE-EXI patches */
extern u8 hdd_bin[];
extern u32 hdd_bin_size;
extern u8 sd_bin[];
extern u32 sd_bin_size;
extern u8 usbgecko_bin[];
extern u32 usbgecko_bin_size;

/* SDK patches */
extern u8 DVDCancelAsync[];
extern u32 DVDCancelAsync_length;
extern u8 DVDCancel[];
extern u32 DVDCancel_length;
extern u8 DVDReadInt[];
extern u32 DVDReadInt_length;
extern u8 DVDReadAsyncInt[];
extern u32 DVDReadAsyncInt_length;
extern u8 DVDGetDriveStatus[];
extern u32 DVDGetDriveStatus_length;
extern u8 DVDGetCommandBlockStatus[];
extern u32 DVDGetCommandBlockStatus_length;
extern u8 DVDCompareDiskId[];
extern u32 DVDCompareDiskId_length;
extern u8 ForceProgressive[];
extern u32 ForceProgressive_length;
extern u8 ForceProgressive576p[];
extern u32 ForceProgressive576p_length;

/* SDK CARD library patches */
extern u8 __CARDSync[];
extern u32 __CARDSync_length;
extern u8 CARDCheck[];
extern u32 CARDCheck_length;
extern u8 CARDCheckAsync[];
extern u32 CARDCheckAsync_length;
extern u8 CARDCheckEx[];
extern u32 CARDCheckEx_length;
extern u8 CARDCheckExAsync[];
extern u32 CARDCheckExAsync_length;
extern u8 CARDClose[];
extern u32 CARDClose_length;
extern u8 CARDCreate[];
extern u32 CARDCreate_length;
extern u8 CARDCreateAsync[];
extern u32 CARDCreateAsync_length;
extern u8 CARDDelete[];
extern u32 CARDDelete_length;
extern u8 CARDDeleteAsync[];
extern u32 CARDDeleteAsync_length;
extern u8 CARDFastOpen[];
extern u32 CARDFastOpen_length;
extern u8 CARDFreeBlocks[];
extern u32 CARDFreeBlocks_length;
extern u8 CARDGetEncoding[];
extern u32 CARDGetEncoding_length;
extern u8 CARDGetMemSize[];
extern u32 CARDGetMemSize_length;
extern u8 CARDGetResultCode[];
extern u32 CARDGetResultCode_length;
extern u8 CARDGetSerialNo[];
extern u32 CARDGetSerialNo_length;
extern u8 CARDGetStatus[];
extern u32 CARDGetStatus_length;
extern u8 CARDMount[];
extern u32 CARDMount_length;
extern u8 CARDMountAsync[];
extern u32 CARDMountAsync_length;
extern u8 CARDOpen[];
extern u32 CARDOpen_length;
extern u8 CARDProbe[];
extern u32 CARDProbe_length;
extern u8 CARDProbeEX[];
extern u32 CARDProbeEX_length;
extern u8 CARDRead[];
extern u32 CARDRead_length;
extern u8 CARDReadAsync[];
extern u32 CARDReadAsync_length;
extern u8 CARDSetStatus[];
extern u32 CARDSetStatus_length;
extern u8 CARDSetStatusAsync[];
extern u32 CARDSetStatusAsync_length;
extern u8 CARDWrite[];
extern u32 CARDWrite_length;
extern u8 CARDWriteAsync[];
extern u32 CARDWriteAsync_length;

/* Patch code variables block */
#define VAR_AREA 			(0x81800000)		// Base location of our variables
#define VAR_AREA_SIZE		(0x200)				// Size of our variables block
#define VAR_READ_DVDSTRUCT	(VAR_AREA-0x120)	// 0x40 of ptr to DVD structs
#define VAR_DISC_1_LBA 		(VAR_AREA-0x100)	// is the base file sector for disk 1
#define VAR_DISC_2_LBA 		(VAR_AREA-0xFC)		// is the base file sector for disk 2
#define VAR_CUR_DISC_LBA 	(VAR_AREA-0xF8)		// is the currently selected disk sector
#define VAR_EXI_BUS_SPD 	(VAR_AREA-0xF4)		// is the EXI bus speed (16mhz vs 32mhz)
#define VAR_SD_TYPE 		(VAR_AREA-0xF0)		// is the Card Type (SDHC=0, SD=1)
#define VAR_EXI_FREQ 		(VAR_AREA-0xDC)		// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
#define VAR_EXI_SLOT 		(VAR_AREA-0xD8)		// is the EXI slot (0 = slot a, 1 = slot b)
#define VAR_TMP1  			(VAR_AREA-0xD4)		// space for a variable if required
#define VAR_TMP2  			(VAR_AREA-0xD0)		// space for a variable if required
#define VAR_TMP3  			(VAR_AREA-0xCC)		// space for a variable if required
#define VAR_TMP4  			(VAR_AREA-0xC8)		// space for a variable if required
#define VAR_PROG_MODE		(VAR_AREA-0xB8)		// data/code to overwrite GXRMode obj with for 480p forcing
#define VAR_ASPECT_FLOAT	(VAR_AREA-0x1C)		// Aspect ratio multiply float (4 bytes)
#define VAR_MEMCARD_LBA		(VAR_AREA-0x18)		// Memory card file base on SD
#define VAR_MEMCARD_WORK	(VAR_AREA-0x14)		// Memory card work area 40960 bytes big
#define VAR_MEMCARD_RESULT	(VAR_AREA-0x10)		// Last memory card status from a CARD func
#define VAR_MC_CB_ADDR		(VAR_AREA-0x0C)		// memcard callback addr
#define VAR_MC_CB_ARG1		(VAR_AREA-0x08)		// memcard callback r3
#define VAR_MC_CB_ARG2		(VAR_AREA-0x04)		// memcard callback r4

/* Pre-patcher magic */
#define PRE_PATCHER_MAGIC "Pre-Patched by Swiss v0.2"

#define LO_RESERVE 0x80001800
#define HI_RESERVE (0x817FE800-VAR_AREA_SIZE)

/* Function jump locations in our patch code */
#define READ_TYPE1_V1_OFFSET 	(base_addr)
#define READ_TYPE1_V2_OFFSET 	(base_addr | 0x04)
#define READ_TYPE1_V3_OFFSET 	(base_addr | 0x08)
#define READ_TYPE2_V1_OFFSET 	(base_addr | 0x0C)
#define OS_RESTORE_INT_OFFSET 	(base_addr | 0x10)
#define CARD_OPEN_OFFSET 		(base_addr | 0x14)
#define CARD_FASTOPEN_OFFSET 	(base_addr | 0x18)
#define CARD_CLOSE_OFFSET 		(base_addr | 0x1C)	
#define CARD_CREATE_OFFSET 		(base_addr | 0x20)
#define CARD_DELETE_OFFSET 		(base_addr | 0x24)
#define CARD_READ_OFFSET 		(base_addr | 0x28)
#define CARD_WRITE_OFFSET 		(base_addr | 0x2C)
#define CARD_GETSTATUS_OFFSET 	(base_addr | 0x30)
#define CARD_SETSTATUS_OFFSET 	(base_addr | 0x34)
#define CARD_SETUP_OFFSET 		(base_addr | 0x38)

/* Types of files we may patch */
#define PATCH_DOL		0
#define PATCH_ELF		1
#define PATCH_OTHER		2

int Patch_DVDHighLevelRead(u8 *data, u32 length);
int Patch_DVDLowLevelRead(void *addr, u32 length);
int Patch_ProgVideo(u8 *data, u32 length);
int Patch_WideAspect(u8 *data, u32 length);
int Patch_DVDAudioStreaming(u8 *data, u32 length);
int Patch_DVDStatusFunctions(u8 *data, u32 length);
void Patch_Fwrite(void *addr, u32 length);
void Patch_DVDReset(void *addr,u32 length);
int Patch_DVDCompareDiskId(u8 *data, u32 length);
void Patch_GXSetVATZelda(void *addr, u32 length,int mode);
int Patch_OSRestoreInterrupts(void *addr, u32 length);
int Patch_CARDFunctions(u8 *data, u32 length);
int Patch_CheatsHook(u8 *data, u32 length, u32 type);
void install_code();
u32 get_base_addr();
void set_base_addr(int useHi);

#endif

