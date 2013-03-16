/*
	This file is used from both C code and Assembly 
	- Make sure both sections in here are kept in sync !
	
	It contains hardcoded locations where Swiss and the patch 
	codes will store/retrieve certain values that are stashed away.
	
	Offsets are all relative to top of Main RAM on GameCube, 0x81800000.
*/

#ifdef _LANGUAGE_ASSEMBLY
.set VAR_AREA,			0x8180
.set VAR_AREA_SIZE,		0x200	# Size of our variables block
.set VAR_FRAG_SIZE,		0xE0	# Size of frag array in bytes
.set VAR_FRAG_LIST,		-0x200	# 0xE0 of fragments (18 frags max) (u32 sector, u32 size)
.set VAR_READ_DVDSTRUCT,-0x120	# 0x40 of ptr to DVD structs
.set VAR_DISC_1_LBA, 	-0x100	# is the base file sector for disk 1
.set VAR_DISC_2_LBA, 	-0xFC	# is the base file sector for disk 2
.set VAR_CUR_DISC_LBA, 	-0xF8	# is the currently selected disk sector
.set VAR_EXI_BUS_SPD, 	-0xF4	# is the EXI bus speed (192 = 16mhz vs 208 = 32mhz)
.set VAR_SD_TYPE, 		-0xF0	# is the Card Type (SDHC=0, SD=1)
.set VAR_EXI_FREQ, 		-0xDC	# is the EXI frequency (4 = 16mhz, 5 = 32mhz)
.set VAR_EXI_SLOT, 		-0xD8	# is the EXI slot (0 = slot a, 1 = slot b)
.set VAR_TMP1,  		-0xD4	# space for a variable if required
.set VAR_TMP2,  		-0xD0	# space for a variable if required
.set VAR_TMP3,  		-0xCC	# space for a variable if required
.set VAR_TMP4,  		-0xC8	# space for a variable if required
.set VAR_DOL_SECTOR,	-0xC4	# In-Game-Reset DOL address
.set VAR_DOL_SIZE,		-0xC0	# In-Game-Reset DOL size
.set VAR_PROG_MODE,		-0xB8	# data to overwrite GXRMode obj with
.set VAR_FLOAT9_16,		-0x90	# constant 9/16
.set VAR_FLOAT1_6,		-0x8C	# constant 1/6
.set VAR_FLOAT3_4,		-0x88	# constant 3/4
.set VAR_FLOATM_1,		-0x84	# constant -1
.set VAR_MEMCARD_LBA,	-0x18	# Memory card file base on SD
.set VAR_MEMCARD_WORK,	-0x14	# Memory card work area 40960 bytes big
.set VAR_MEMCARD_RESULT,-0x10	# Last memory card status from a CARD func
.set VAR_MC_CB_ADDR,	-0x0C	# memcard callback addr
.set VAR_MC_CB_ARG1,	-0x08	# memcard callback r3
.set VAR_MC_CB_ARG2,	-0x04	# memcard callback r4

#else

#define VAR_AREA 			(0x81800000)		// Base location of our variables
#define VAR_AREA_SIZE		(0x200)				// Size of our variables block
#define VAR_FRAG_SIZE		(0xE0)				// Size of frag array in bytes
#define VAR_FRAG_LIST		(VAR_AREA-0x200)	// 0xE0 of fragments (18 frags max) (u32 sector, u32 size)
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
#define VAR_DOL_SECTOR		(VAR_AREA-0xC4)		// In-Game-Reset DOL address
#define VAR_DOL_SIZE		(VAR_AREA-0xC0)		// In-Game-Reset DOL size
#define VAR_PROG_MODE		(VAR_AREA-0xB8)		// data to overwrite GXRMode obj with
#define VAR_FLOAT9_16		(VAR_AREA-0x90)		// constant 9/16
#define VAR_FLOAT1_6		(VAR_AREA-0x8C)		// constant 1/6
#define VAR_FLOAT3_4		(VAR_AREA-0x88)		// constant 3/4
#define VAR_FLOATM_1		(VAR_AREA-0x84)		// constant -1
#define VAR_MEMCARD_LBA		(VAR_AREA-0x18)		// Memory card file base on SD
#define VAR_MEMCARD_WORK	(VAR_AREA-0x14)		// Memory card work area 40960 bytes big
#define VAR_MEMCARD_RESULT	(VAR_AREA-0x10)		// Last memory card status from a CARD func
#define VAR_MC_CB_ADDR		(VAR_AREA-0x0C)		// memcard callback addr
#define VAR_MC_CB_ARG1		(VAR_AREA-0x08)		// memcard callback r3
#define VAR_MC_CB_ARG2		(VAR_AREA-0x04)		// memcard callback r4

#endif

