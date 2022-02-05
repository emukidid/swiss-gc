/*
	This file is used from both C code and Assembly 
	- Make sure both sections in here are kept in sync !
	
	It contains hardcoded locations where Swiss and the patch 
	codes will store/retrieve certain values that are stashed away.
	
	Offsets are all relative to top of Main RAM reserved area on GameCube, 0x80001000-0x80003000.
*/

#ifdef _LANGUAGE_ASSEMBLY
#include "asm.h"
.set VAR_AREA,				0x8000

.set VAR_JUMP_TABLE,		0x00A0	# Dolphin OS jump table
.set VAR_TVMODE,			0x00CC	# TV format

.set VAR_CURRENT_DISC,		0x09B0	# current disc number
.set VAR_SECOND_DISC,		0x09B1	# second disc present
.set VAR_DRIVE_PATCHED,		0x09B2	# disc drive patched
.set VAR_EMU_READ_SPEED,	0x09B3	# emulate read speed
.set VAR_EXI_REGS,			0x09B4	# pointer to EXI registers
.set VAR_EXI_SLOT,			0x09B8	# is the EXI slot (0 = slot a, 1 = slot b)
.set VAR_EXI_FREQ,			0x09B9	# is the EXI frequency (4 = 16mhz, 5 = 32mhz)
.set VAR_SD_SHIFT,			0x09BA	# is the SD Card shift amount when issueing read cmds
.set VAR_ATA_LBA48,			0x09BA	# Is the HDD in use a 48 bit LBA supported HDD?
.set VAR_IGR_EXIT_TYPE,		0x09BB	# IGR exit type
.set VAR_FRAG_LIST,			0x09BC	# pointer to fragments (u32 offset, u32 size, u32 rawsector)

.set VAR_DISC_1_ID,			0x09C0	# disc 1 header
.set VAR_DISC_2_ID,			0x09CA	# disc 2 header

.set VAR_SERVER_MAC,		0x09C0	# server MAC address
.set VAR_CLIENT_MAC,		0x09C6	# client MAC address
.set VAR_SERVER_IP,			0x09CC	# server IPv4 address
.set VAR_CLIENT_IP,			0x09D0	# client IPv4 address
.set VAR_SERVER_PORT,		0x09D4	# server UDP port

.set VAR_FLOAT1_6,			0x09E0	# constant 1/6
.set VAR_FLOAT9_16,			0x09E4	# constant 9/16
.set VAR_FLOAT3_4,			0x09E8	# constant 3/4
.set VAR_FLOATM_1,			0x09EC	# constant -1
.set VAR_VFILTER_ON,		0x09F0	# vertical filter on
.set VAR_VFILTER,			0x09F1	# vertical filter
.set VAR_SAR_WIDTH,			0x09F8	# sample aspect ratio width
.set VAR_SAR_HEIGHT,		0x09FA	# sample aspect ratio height
.set VAR_NEXT_FIELD,		0x09FB	# next video field
.set VAR_CURRENT_FIELD,		0x09FC	# current video field
.set VAR_TRIGGER_LEVEL,		0x09FD	# digital trigger level
.set VAR_CARD_IDS,			0x09FE	# emulated memory cards
.set VAR_CARD_A_ID,			0x09FE	# emulated memory card a
.set VAR_CARD_B_ID,			0x09FF	# emulated memory card b

.set VAR_SECTOR_BUF,		0x0A00	# 0x200 of read data

.set VAR_PATCHES_BASE,		0x3000	# Patches get copied to below this area.

.set VAR_RMODE,				0x30F8	# render mode

# Cheat Engine + Cheats buffer	(0x817FE000)
.set	WIIRD_ENGINE_SPACE,	(0x2000)
.set	WIIRD_ENGINE,		(0x81800000-WIIRD_ENGINE_SPACE)

# IGR Types
.set IGR_OFF,			0
.set IGR_HARDRESET,		1
.set IGR_BOOTBIN,		2
.set IGR_USBGKOFLASH,	3

#else

extern char VAR_AREA[];

extern char VAR_JUMP_TABLE[0x20];	// Dolphin OS jump table
extern char VAR_TVMODE[4];			// TV format

extern char VAR_CURRENT_DISC[1];	// current disc number
extern char VAR_SECOND_DISC[1];		// second disc present
extern char VAR_DRIVE_PATCHED[1];	// disc drive patched
extern char VAR_EMU_READ_SPEED[1];	// emulate read speed
extern char VAR_EXI_REGS[4];		// pointer to EXI registers
extern char VAR_EXI_SLOT[1];		// is the EXI slot (0 = slot a, 1 = slot b)
extern char VAR_EXI_FREQ[1];		// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
extern char VAR_SD_SHIFT[1];		// is the SD Card shift amount when issueing read cmds
extern char VAR_ATA_LBA48[1];		// Is the HDD in use a 48 bit LBA supported HDD?
extern char VAR_IGR_TYPE[1];		// IGR exit type
extern char VAR_FRAG_LIST[4];		// pointer to fragments (u32 offset, u32 size, u32 rawsector)

extern char VAR_DISC_1_ID[10];		// disc 1 header
extern char VAR_DISC_2_ID[10];		// disc 2 header

extern char VAR_SERVER_MAC[6];		// server MAC address
extern char VAR_CLIENT_MAC[6];		// client MAC address
extern char VAR_SERVER_IP[4];		// server IPv4 address
extern char VAR_CLIENT_IP[4];		// client IPv4 address
extern char VAR_SERVER_PORT[2];		// server UDP port

extern char VAR_FLOAT1_6[4];		// constant 1/6
extern char VAR_FLOAT9_16[4];		// constant 9/16
extern char VAR_FLOAT3_4[4];		// constant 3/4
extern char VAR_FLOATM_1[4];		// constant -1
extern char VAR_VFILTER_ON[1];		// vertical filter on
extern char VAR_VFILTER[7];			// vertical filter
extern char VAR_SAR_WIDTH[2];		// sample aspect ratio width
extern char VAR_SAR_HEIGHT[1];		// sample aspect ratio height
extern char VAR_NEXT_FIELD[1];		// next video field
extern char VAR_CURRENT_FIELD[1];	// current video field
extern char VAR_TRIGGER_LEVEL[1];	// digital trigger level
extern char VAR_CARD_IDS[2];		// emulated memory cards
extern char VAR_CARD_A_ID[1];		// emulated memory card a
extern char VAR_CARD_B_ID[1];		// emulated memory card b

extern char VAR_SECTOR_BUF[0x200];	// 0x200 of read data

extern char VAR_PATCHES_BASE[];		// Patches get copied to below this area.

extern char VAR_RMODE[4];			// render mode

// Cheat Engine + Cheats buffer (0x817FE000)
#define WIIRD_ENGINE_SPACE	(0x2000)
#define WIIRD_ENGINE		(0x81800000-WIIRD_ENGINE_SPACE)

// IGR Types
#define IGR_OFF			0
#define IGR_HARDRESET	1
#define IGR_BOOTBIN		2
#define IGR_USBGKOFLASH	3

// FAT Fragments
#define FRAGS_NULL		(char)-1
#define FRAGS_DISC_1	0
#define FRAGS_DISC_2	1
#define FRAGS_BOOT_GCM	2
#define FRAGS_APPLOADER	3
#define FRAGS_CARD(n)	(4 + (n))
#define FRAGS_CARD_A	FRAGS_CARD(0)
#define FRAGS_CARD_B	FRAGS_CARD(1)
#endif

