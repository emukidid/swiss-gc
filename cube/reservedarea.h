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
.set VAR_IGR_DOL_SIZE,		0x09BC	# IGR DOL Size
.set VAR_DISC_1_ID,			0x09C0	# disc 1 header
.set VAR_DISC_2_ID,			0x09E0	# disc 2 header
.set VAR_SECTOR_BUF,		0x0A00	# 0x200 of read data

.set VAR_SERVER_MAC,		0x09C0	# server MAC address
.set VAR_CLIENT_MAC,		0x09C6	# client MAC address
.set VAR_SERVER_IP,			0x09CC	# server IPv4 address
.set VAR_CLIENT_IP,			0x09D0	# client IPv4 address
.set VAR_SERVER_PORT,		0x09D4	# server UDP port
.set VAR_FSP_KEY,			0x09D6	# FSP session key

.set VAR_DISC_1_FNLEN,		0x0A00	# disc 1 filename length
.set VAR_DISC_1_FN,			0x0A01	# disc 1 filename
.set VAR_DISC_2_FNLEN,		0x0B00	# disc 2 filename length
.set VAR_DISC_2_FN,			0x0B01	# disc 2 filename

.set VAR_PATCHES_BASE,		0x2E00	# Patches get copied to below this area.

.set VAR_FLOAT1_6,			0x2E00	# constant 1/6
.set VAR_FLOAT9_16,			0x2E04	# constant 9/16
.set VAR_FLOAT3_4,			0x2E08	# constant 3/4
.set VAR_FLOATM_1,			0x2E0C	# constant -1
.set VAR_VFILTER_ON,		0x2E10	# vertical filter on
.set VAR_VFILTER,			0x2E11	# vertical filter
.set VAR_SAR_WIDTH,			0x2E18	# sample aspect ratio width
.set VAR_SAR_HEIGHT,		0x2E1A	# sample aspect ratio height
.set VAR_NEXT_FIELD,		0x2E1B	# next video field
.set VAR_CURRENT_FIELD,		0x2E1C	# current video field
.set VAR_FRAG_LIST,			0x2E20	# 0x1E0 of fragments (40 frags max) (u32 offset, u32 size, u32 rawsector)

.set VAR_RMODE,				0x30F8	# render mode

# execD replacement lives here (0x817FA000)	- if this is changed, be sure to update the patch Makefile
.set	EXECD_RUNNER_SPACE,  (0x1000)
.set	EXECD_RUNNER,	(WIIRD_ENGINE-EXECD_RUNNER_SPACE)

# Cheat Engine + Cheats buffer	(0x817FB000)
.set	WIIRD_ENGINE_SPACE,  (0x2E00)
.set	WIIRD_ENGINE,	(DECODED_BUFFER_0-WIIRD_ENGINE_SPACE)

# Audio Streaming buffers	(these live above ArenaHi...)
.set 	BUFSIZE, 			0xE00
.set 	CHUNK_48, 			0x400
.set 	CHUNK_48to32, 		0x600
.set 	DECODE_WORK_AREA,	(0x81800000-CHUNK_48to32)
.set 	DECODED_BUFFER_0,	(DECODE_WORK_AREA-(BUFSIZE*2))
.set 	DECODED_BUFFER_1,	(DECODE_WORK_AREA-(BUFSIZE*1))

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
extern char VAR_IGR_EXIT_TYPE[1];	// IGR exit type
extern char VAR_IGR_DOL_SIZE[4];	// IGR DOL Size
extern char VAR_DISC_1_ID[0x20];	// disc 1 header
extern char VAR_DISC_2_ID[0x20];	// disc 2 header
extern char VAR_SECTOR_BUF[0x200];	// 0x200 of read data

extern char VAR_SERVER_MAC[6];		// server MAC address
extern char VAR_CLIENT_MAC[6];		// client MAC address
extern char VAR_SERVER_IP[4];		// server IPv4 address
extern char VAR_CLIENT_IP[4];		// client IPv4 address
extern char VAR_SERVER_PORT[2];		// server UDP port
extern char VAR_FSP_KEY[2];			// FSP session key

extern char VAR_DISC_1_FNLEN[1];	// disc 1 filename length
extern char VAR_DISC_1_FN[0xFF];	// disc 1 filename
extern char VAR_DISC_2_FNLEN[1];	// disc 2 filename length
extern char VAR_DISC_2_FN[0xFF];	// disc 2 filename

extern char VAR_PATCHES_BASE[];		// Patches get copied to below this area.

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
extern char VAR_FRAG_LIST[0x1E0];	// 0x1E0 of fragments (40 frags max) (u32 offset, u32 size, u32 rawsector)

extern char VAR_RMODE[4];			// render mode

// execD replacement lives here (0x817FA000) - if this is changed, be sure to update the patch Makefile
#define EXECD_RUNNER_SPACE	(0x1000)
#define EXECD_RUNNER		(WIIRD_ENGINE-EXECD_RUNNER_SPACE)

// Cheat Engine + Cheats buffer (0x817FB000)
#define WIIRD_ENGINE_SPACE  (0x2E00)
#define WIIRD_ENGINE		(DECODED_BUFFER_0-WIIRD_ENGINE_SPACE)

// Audio Streaming buffers	(these live above ArenaHi...)
#define BUFSIZE 			0xE00
#define CHUNK_48 			0x400
#define CHUNK_48to32 		0x600
#define DECODE_WORK_AREA 	(0x81800000-CHUNK_48to32)
#define DECODED_BUFFER_0 	(DECODE_WORK_AREA-(BUFSIZE*2))
#define DECODED_BUFFER_1 	(DECODE_WORK_AREA-(BUFSIZE*1))

// IGR Types
#define IGR_OFF			0
#define IGR_HARDRESET	1
#define IGR_BOOTBIN		2
#define IGR_USBGKOFLASH	3

// FAT Fragments
#define FRAGS_DISC_1	0x00000000
#define FRAGS_DISC_2	0x80000000
#define FRAGS_IGR_DOL	0xE0000000
#define FRAGS_CARD_A	0x7E000000
#define FRAGS_CARD_B	0xFE000000
#endif

