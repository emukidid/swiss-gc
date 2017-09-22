/*
	This file is used from both C code and Assembly 
	- Make sure both sections in here are kept in sync !
	
	It contains hardcoded locations where Swiss and the patch 
	codes will store/retrieve certain values that are stashed away.
	
	Offsets are all relative to top of Main RAM reserved area on GameCube, 0x80001800-0x80003000.
*/

#ifdef _LANGUAGE_ASSEMBLY
#include "asm.h"
.set VAR_AREA,				0x8000
.set VAR_FILENAME_LEN,		0x0E00	# filename length
.set VAR_FILENAME,			0x0E01	# filename
.set VAR_CLIENT_MAC,		0x0EEC	# client MAC address
.set VAR_CLIENT_IP,			0x0EF2	# client IPv4 address
.set VAR_SERVER_MAC,		0x0EF6	# server MAC address
.set VAR_SERVER_IP,			0x0EFC	# server IPv4 address
.set VAR_PATCHES_BASE,		0x2D00	# Patches get copied to below this area.
.set VAR_FRAG_SIZE,			0x1C8	# Size of frag array in bytes
.set VAR_FRAG_LIST,			0x2D00	# 0x1C8 of fragments (38 frags max) (u32 offset, u32 size, u32 rawsector)
.set VAR_TIMER_START,		0x2EC8	# Use this as a timer start (tbu,tb)
.set VAR_DISC_CHANGING,		0x2ED0	# Are we changing discs?
.set VAR_CURRENT_FIELD,		0x2ED4	# current video field
.set VAR_IGR_EXIT_TYPE,		0x2ED5	# IGR exit type
.set VAR_IPV4_ID,			0x2ED6	# IPv4 fragment identifier
.set VAR_FSP_KEY,			0x2ED8	# FSP session key
.set VAR_FSP_DATA_LENGTH,	0x2EDA	# FSP payload size
.set VAR_FSP_POSITION,		0x2EDC	# FSP file position
.set VAR_LAST_OFFSET,		0x2EE0	# The last offset we read from
.set VAR_READS_IN_AS,		0x2EE4	# How many times have we tried to read while streaming is on?
.set VAR_AS_ENABLED,		0x2EE8	# Is Audio streaming enabled by the user?
.set VAR_FAKE_IRQ_SET,		0x2EF8	# flag to say we are ready to fake irq.
.set VAR_EXECD_OFFSET,		0x2EFC	# offset of execD.bin on multi-dol discs
.set VAR_DISC_1_LBA, 		0x2F00	# is the base file sector for disk 1
.set VAR_DISC_2_LBA, 		0x2F04	# is the base file sector for disk 2
.set VAR_CUR_DISC_LBA, 		0x2F08	# is the currently selected disk sector
.set VAR_EXI_BUS_SPD, 		0x2F0C	# is the EXI bus speed (192 = 16mhz vs 208 = 32mhz)
.set VAR_SD_TYPE, 			0x2F10	# is the Card Type (SDHC=0, SD=1)
.set VAR_EXI_FREQ, 			0x2F14	# is the EXI frequency (4 = 16mhz, 5 = 32mhz)
.set VAR_EXI_SLOT, 			0x2F18	# is the EXI slot (0 = slot a, 1 = slot b)
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

#define VAR_AREA			(0x80000000)
#define VAR_FILENAME_LEN	(VAR_AREA+0x0E00)	// filename length
#define VAR_FILENAME		(VAR_AREA+0x0E01)	// filename
#define VAR_CLIENT_MAC		(VAR_AREA+0x0EEC)	// client MAC address
#define VAR_CLIENT_IP		(VAR_AREA+0x0EF2)	// client IPv4 address
#define VAR_SERVER_MAC		(VAR_AREA+0x0EF6)	// server MAC address
#define VAR_SERVER_IP		(VAR_AREA+0x0EFC)	// server IPv4 address
#define VAR_PATCHES_BASE	(VAR_AREA+0x2D00)	// Patches get copied to below this area.
#define VAR_FRAG_SIZE		(0x1C8)				// Size of frag array in bytes
#define VAR_FRAG_LIST		(VAR_AREA+0x2D00)	// 0x1C8 of fragments (40 frags max) (u32 offset, u32 size, u32 rawsector)
#define VAR_TIMER_START		(VAR_AREA+0x2EC8)	// Use this as a timer start (tbu,tb)
#define VAR_DISC_CHANGING	(VAR_AREA+0x2ED0)	// Are we changing discs?
#define VAR_CURRENT_FIELD	(VAR_AREA+0x2ED4)	// current video field
#define VAR_IGR_EXIT_TYPE	(VAR_AREA+0x2ED5)	// IGR exit type
#define VAR_IPV4_ID			(VAR_AREA+0x2ED6)	// IPv4 fragment identifier
#define VAR_FSP_KEY			(VAR_AREA+0x2ED8)	// FSP session key
#define VAR_FSP_DATA_LENGTH	(VAR_AREA+0x2EDA)	// FSP payload size
#define VAR_FSP_POSITION	(VAR_AREA+0x2EDC)	// FSP file position
#define VAR_LAST_OFFSET		(VAR_AREA+0x2EE0)	// The last offset we read from
#define VAR_READS_IN_AS		(VAR_AREA+0x2EE4)	// How many times have we tried to read while streaming is on?
#define VAR_AS_ENABLED		(VAR_AREA+0x2EE8)	// Is Audio streaming enabled by the user?
#define VAR_FAKE_IRQ_SET	(VAR_AREA+0x2EF8)	// flag to say we are ready to fake irq.
#define VAR_EXECD_OFFSET	(VAR_AREA+0x2EFC)	// offset of execD.bin on multi-dol discs
#define VAR_DISC_1_LBA 		(VAR_AREA+0x2F00)	// is the base file sector for disk 1
#define VAR_DISC_2_LBA 		(VAR_AREA+0x2F04)	// is the base file sector for disk 2
#define VAR_CUR_DISC_LBA 	(VAR_AREA+0x2F08)	// is the currently selected disk sector
#define VAR_EXI_BUS_SPD 	(VAR_AREA+0x2F0C)	// is the EXI bus speed (192 = 16mhz vs 208 = 32mhz)
#define VAR_SD_TYPE 		(VAR_AREA+0x2F10)	// is the Card Type (SDHC=0, SD=1)
#define VAR_EXI_FREQ 		(VAR_AREA+0x2F14)	// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
#define VAR_EXI_SLOT 		(VAR_AREA+0x2F18)	// is the EXI slot (0 = slot a, 1 = slot b)
#define VAR_TMP1  			(VAR_AREA+0x2F1C)	// space for a variable if required
#define VAR_TMP2  			(VAR_AREA+0x2F20)	// space for a variable if required
#define VAR_FLOAT9_16		(VAR_AREA+0x2F24)	// constant 9/16
#define VAR_FLOAT1_6		(VAR_AREA+0x2F28)	// constant 1/6
#define VAR_FLOAT3_4		(VAR_AREA+0x2F2C)	// constant 3/4
#define VAR_FLOATM_1		(VAR_AREA+0x2F30)	// constant -1
#define VAR_DI_REGS			(VAR_AREA+0x2F34)	// DI Regs are mapped to here...
#define VAR_INTERRUPT_TIMES	(VAR_AREA+0x2F58)	// how many times have we called the dvd queue
#define VAR_DEVICE_SPEED	(VAR_AREA+0x2F5C)	// How long in usec does it take to read 1024 bytes
#define VAR_STREAM_START	(VAR_AREA+0x2F60)	// AS Start
#define VAR_STREAM_CUR		(VAR_AREA+0x2F64)	// AS Current Location
#define VAR_STREAM_END		(VAR_AREA+0x2F68)	// AS End
#define VAR_STREAM_SIZE		(VAR_AREA+0x2F6C)	// AS Size
#define VAR_STREAM_UPDATE	(VAR_AREA+0x2F70)	// AS Update Request
#define VAR_STREAM_ENDING	(VAR_AREA+0x2F71)	// AS Ending
#define VAR_STREAM_LOOPING	(VAR_AREA+0x2F72)	// AS Looping
#define VAR_AS_SAMPLECNT	(VAR_AREA+0x2F73)	// AS Sample Counter
#define VAR_STREAM_CURBUF	(VAR_AREA+0x2F74)	// AS Current Main Buffer
#define VAR_STREAM_DI		(VAR_AREA+0x2F75)	// AS DI Status
#define VAR_STREAM_BUFLOC	(VAR_AREA+0x2F78)
#define VAR_AS_HIST_0		(VAR_AREA+0x2F7C)
#define VAR_AS_HIST_1		(VAR_AREA+0x2F80)
#define VAR_AS_HIST_2		(VAR_AREA+0x2F84)
#define VAR_AS_HIST_3		(VAR_AREA+0x2F88)
#define VAR_AS_TMP_LSAMPLE	(VAR_AREA+0x2F8C)
#define VAR_AS_TMP_RSAMPLE	(VAR_AREA+0x2F8E)
#define VAR_AS_OUTL			(VAR_AREA+0x2F90)
#define VAR_AS_OUTR			(VAR_AREA+0x2FC8)

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

