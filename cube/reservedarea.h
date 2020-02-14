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
.set VAR_TVMODE,			0x00CC	# TV format
.set VAR_CLIENT_MAC,		0x09EA	# client MAC address
.set VAR_CLIENT_IP,			0x09F0	# client IPv4 address
.set VAR_SERVER_MAC,		0x09F4	# server MAC address
.set VAR_SERVER_IP,			0x09FA	# server IPv4 address
.set VAR_SERVER_PORT,		0x09FE	# server UDP port
.set VAR_SECTOR_CUR,		0x09FC	# is the currently buffered disk sector
.set VAR_SECTOR_BUF,		0x0A00	# 0x200 of read data
.set VAR_DISC_1_FNLEN,		0x0A00	# disc 1 filename length
.set VAR_DISC_1_FN,			0x0A01	# disc 1 filename
.set VAR_DISC_2_FNLEN,		0x0B00	# disc 2 filename length
.set VAR_DISC_2_FN,			0x0B01	# disc 2 filename
.set VAR_DISC_1_ID,			0x0A00	# disc 1 header
.set VAR_DISC_2_ID,			0x0B00	# disc 2 header
.set VAR_PATCHES_BASE,		0x2D00	# Patches get copied to below this area.
.set VAR_FRAG_LIST,			0x2D00	# 0x1C8 of fragments (38 frags max) (u32 offset, u32 size, u32 rawsector)
.set VAR_TIMER_START,		0x2EC8	# Use this as a timer start (tbu,tb)
.set VAR_DISC_CHANGING,		0x2ED0	# Are we changing discs?
.set VAR_NEXT_FIELD,		0x2ED4	# next video field
.set VAR_IGR_EXIT_TYPE,		0x2ED5	# IGR exit type
.set VAR_IPV4_ID,			0x2ED6	# IPv4 fragment identifier
.set VAR_FSP_KEY,			0x2ED8	# FSP session key
.set VAR_FSP_DATA_LENGTH,	0x2EDA	# FSP payload size
.set VAR_IGR_EXIT_FLAG,		0x2EDC	# IGR exit flag
.set VAR_IGR_DOL_SIZE,		0x2EE0	# IGR DOL Size
.set VAR_READS_IN_AS,		0x2EE4	# How many times have we tried to read while streaming is on?
.set VAR_AS_ENABLED,		0x2EE8	# Is Audio streaming enabled by the user?
.set VAR_RMODE,				0x2EEC	# render mode
.set VAR_VFILTER,			0x2EF0	# vertical filter
.set VAR_VFILTER_ON,		0x2EF7	# vertical filter on
.set VAR_FAKE_IRQ_SET,		0x2EF8	# flag to say we are ready to fake irq.
.set VAR_SAR_WIDTH,			0x2EFC	# sample aspect ratio width
.set VAR_SAR_HEIGHT,		0x2EFE	# sample aspect ratio height
.set VAR_CURRENT_FIELD,		0x2EFF	# current video field
.set VAR_CURRENT_DISC,		0x2F00	# current disc number
.set VAR_SECOND_DISC,		0x2F04	# second disc present
.set VAR_SD_LBA,			0x2F0C	# is the SD Card sector being read
.set VAR_LAST_OFFSET,		0x2F10	# the last offset a read was simulated from
.set VAR_EXECD_OFFSET,		0x2F14	# offset of execD.bin on multi-dol discs
.set VAR_SD_SHIFT, 			0x2F18	# is the SD Card shift amount when issueing read cmds
.set VAR_EXI_FREQ, 			0x2F19	# is the EXI frequency (4 = 16mhz, 5 = 32mhz)
.set VAR_EXI_SLOT, 			0x2F1A	# is the EXI slot (0 = slot a, 1 = slot b)
.set VAR_ATA_LBA48,			0x2F1B	# Is the HDD in use a 48 bit LBA supported HDD?
.set VAR_TMP1,  			0x2F1C  # space for a variable if required
.set VAR_TMP2,  			0x2F20  # space for a variable if required
.set VAR_FLOAT9_16,			0x2F24  # constant 9/16
.set VAR_FLOAT1_6,			0x2F28  # constant 1/6
.set VAR_FLOAT3_4,			0x2F2C  # constant 3/4
.set VAR_FLOATM_1,			0x2F30  # constant -1
.set VAR_DI_REGS,			0x2F34  # DI Regs are mapped to here...
.set VAR_INTERRUPT_TIMES,	0x2F58	# how many times have we called the dvd queue
.set VAR_DEVICE_SPEED,		0x2F5C	# How long in usec does it take to read 1024 bytes
.set VAR_STREAM_START,		0x2F60	# AS Start
.set VAR_STREAM_CUR,		0x2F64	# AS Current Location
.set VAR_STREAM_END,		0x2F68	# AS End
.set VAR_STREAM_SIZE,		0x2F6C	# AS Size
.set VAR_STREAM_UPDATE,		0x2F70	# AS Update Request
.set VAR_STREAM_ENDING,		0x2F71	# AS Ending
.set VAR_STREAM_LOOPING,	0x2F72	# AS Looping
.set VAR_AS_SAMPLECNT,		0x2F73	# AS Sample Counter
.set VAR_STREAM_CURBUF,		0x2F74	# AS Current Main Buffer
.set VAR_STREAM_DI,			0x2F75	# AS DI Status
.set VAR_STREAM_BUFLOC,		0x2F78	# AS Buffer Location
.set VAR_AS_HIST_0,			0x2F7C
.set VAR_AS_HIST_1,			0x2F80
.set VAR_AS_HIST_2,			0x2F84
.set VAR_AS_HIST_3,			0x2F88
.set VAR_AS_TMP_LSAMPLE,	0x2F8C
.set VAR_AS_TMP_RSAMPLE,	0x2F8E
.set VAR_AS_OUTL,			0x2F90
.set VAR_AS_OUTR,			0x2FC8

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
extern char VAR_TVMODE[4];			// TV format
extern char VAR_CLIENT_MAC[6];		// client MAC address
extern char VAR_CLIENT_IP[4];		// client IPv4 address
extern char VAR_SERVER_MAC[6];		// server MAC address
extern char VAR_SERVER_IP[4];		// server IPv4 address
extern char VAR_SERVER_PORT[2];		// server UDP port
extern char VAR_SECTOR_CUR[4];		// is the currently buffered disk sector
extern char VAR_SECTOR_BUF[0x200];	// 0x200 of read data
extern char VAR_DISC_1_FNLEN[1];	// disc 1 filename length
extern char VAR_DISC_1_FN[0xFF];	// disc 1 filename
extern char VAR_DISC_2_FNLEN[1];	// disc 2 filename length
extern char VAR_DISC_2_FN[0xFF];	// disc 2 filename
extern char VAR_DISC_1_ID[0x20];	// disc 1 header
extern char VAR_DISC_2_ID[0x20];	// disc 2 header
extern char VAR_PATCHES_BASE[];		// Patches get copied to below this area.
extern char VAR_FRAG_LIST[0x1C8];	// 0x1C8 of fragments (40 frags max) (u32 offset, u32 size, u32 rawsector)
extern char VAR_TIMER_START[8];		// Use this as a timer start (tbu,tb)
extern char VAR_DISC_CHANGING[4];	// Are we changing discs?
extern char VAR_NEXT_FIELD[1];		// next video field
extern char VAR_IGR_EXIT_TYPE[1];	// IGR exit type
extern char VAR_IPV4_ID[2];			// IPv4 fragment identifier
extern char VAR_FSP_KEY[2];			// FSP session key
extern char VAR_FSP_DATA_LENGTH[2];	// FSP payload size
extern char VAR_IGR_EXIT_FLAG[4];	// IGR exit flag
extern char VAR_IGR_DOL_SIZE[4];	// IGR DOL Size
extern char VAR_READS_IN_AS[4];		// How many times have we tried to read while streaming is on?
extern char VAR_AS_ENABLED[4];		// Is Audio streaming enabled by the user?
extern char VAR_RMODE[4];			// render mode
extern char VAR_VFILTER[7];			// vertical filter
extern char VAR_VFILTER_ON[1];		// vertical filter on
extern char VAR_FAKE_IRQ_SET[4];	// flag to say we are ready to fake irq.
extern char VAR_SAR_WIDTH[2];		// sample aspect ratio width
extern char VAR_SAR_HEIGHT[1];		// sample aspect ratio height
extern char VAR_CURRENT_FIELD[1];	// current video field
extern char VAR_CURRENT_DISC[4];	// current disc number
extern char VAR_SECOND_DISC[4];		// second disc present
extern char VAR_SD_LBA[4];			// is the SD Card sector being read
extern char VAR_LAST_OFFSET[4];		// the last offset a read was simulated from
extern char VAR_EXECD_OFFSET[4];	// offset of execD.bin on multi-dol discs
extern char VAR_SD_SHIFT[1];		// is the SD Card shift amount when issueing read cmds
extern char VAR_EXI_FREQ[1];		// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
extern char VAR_EXI_SLOT[1];		// is the EXI slot (0 = slot a, 1 = slot b)
extern char VAR_ATA_LBA48[1];		// Is the HDD in use a 48 bit LBA supported HDD?
extern char VAR_TMP1[4];			// space for a variable if required
extern char VAR_TMP2[4];			// space for a variable if required
extern char VAR_FLOAT9_16[4];		// constant 9/16
extern char VAR_FLOAT1_6[4];		// constant 1/6
extern char VAR_FLOAT3_4[4];		// constant 3/4
extern char VAR_FLOATM_1[4];		// constant -1
extern char VAR_DI_REGS[0x24];		// DI Regs are mapped to here...
extern char VAR_INTERRUPT_TIMES[4];	// how many times have we called the dvd queue
extern char VAR_DEVICE_SPEED[4];	// How long in usec does it take to read 1024 bytes
extern char VAR_STREAM_START[4];	// AS Start
extern char VAR_STREAM_CUR[4];		// AS Current Location
extern char VAR_STREAM_END[4];		// AS End
extern char VAR_STREAM_SIZE[4];		// AS Size
extern char VAR_STREAM_UPDATE[1];	// AS Update Request
extern char VAR_STREAM_ENDING[1];	// AS Ending
extern char VAR_STREAM_LOOPING[1];	// AS Looping
extern char VAR_AS_SAMPLECNT[1];	// AS Sample Counter
extern char VAR_STREAM_CURBUF[1];	// AS Current Main Buffer
extern char VAR_STREAM_DI[1];		// AS DI Status
extern char VAR_STREAM_BUFLOC[4];
extern char VAR_AS_HIST_0[4];
extern char VAR_AS_HIST_1[4];
extern char VAR_AS_HIST_2[4];
extern char VAR_AS_HIST_3[4];
extern char VAR_AS_TMP_LSAMPLE[2];
extern char VAR_AS_TMP_RSAMPLE[2];
extern char VAR_AS_OUTL[0x38];
extern char VAR_AS_OUTR[0x38];

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
#endif

