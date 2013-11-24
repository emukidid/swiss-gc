/*
	This file is used from both C code and Assembly 
	- Make sure both sections in here are kept in sync !
	
	It contains hardcoded locations where Swiss and the patch 
	codes will store/retrieve certain values that are stashed away.
	
	Offsets are all relative to top of Main RAM reserved area on GameCube, 0x80001800-0x80003000.
*/

#ifdef _LANGUAGE_ASSEMBLY
.set VAR_AREA,			0x8000
.set VAR_PATCHES_BASE,	0x2E00	# Patches get copied to below this area.
.set VAR_FRAG_SIZE,		0xE0	# Size of frag array in bytes
.set VAR_FRAG_LIST,		0x2E00	# 0xE0 of fragments (18 frags max) (u32 sector, u32 size)
.set VAR_READ_DVDSTRUCT,0x2EE0	# 0x18 for our dvd structure
.set VAR_FAKE_IRQ_SET,	0x2EF8	# flag to say we are ready to fake irq.
.set VAR_DVDIRQ_HNDLR,	0x2EFC	# ptr to __DVDInterruptHandler
.set VAR_DISC_1_LBA, 	0x2F00	# is the base file sector for disk 1
.set VAR_DISC_2_LBA, 	0x2F04	# is the base file sector for disk 2
.set VAR_CUR_DISC_LBA, 	0x2F08	# is the currently selected disk sector
.set VAR_EXI_BUS_SPD, 	0x2F0C	# is the EXI bus speed (192 = 16mhz vs 208 = 32mhz)
.set VAR_SD_TYPE, 		0x2F10	# is the Card Type (SDHC=0, SD=1)
.set VAR_EXI_FREQ, 		0x2F14	# is the EXI frequency (4 = 16mhz, 5 = 32mhz)
.set VAR_EXI_SLOT, 		0x2F18	# is the EXI slot (0 = slot a, 1 = slot b)
.set VAR_TMP1,  		0x2F1C	# space for a variable if required
.set VAR_TMP2,  		0x2F20	# space for a variable if required
.set VAR_MC_FUNC,  		0x2F24	# which MC Function do we want to call?
.set VAR_TMP4,  		0x2F28	# space for a variable if required
.set VAR_DOL_SECTOR,	0x2F2C	# In-Game-Reset DOL address
.set VAR_DOL_SIZE,		0x2F30	# In-Game-Reset DOL size
.set VAR_FLOAT9_16,		0x2F34	# constant 9/16
.set VAR_FLOAT1_6,		0x2F38	# constant 1/6
.set VAR_FLOAT3_4,		0x2F3C	# constant 3/4
.set VAR_FLOATM_1,		0x2F40	# constant -1
.set VAR_MEMCARD_LBA,	0x2F44	# Memory card file base on SD
.set VAR_MEMCARD_WORK,	0x2F48	# ptr to memory card work area (40960 bytes big)
.set VAR_MEMCARD_RESULT,0x2F4C	# Last memory card status from a CARD func
.set VAR_MC_CB_ADDR,	0x2F50	# memcard callback addr
.set VAR_MC_CB_ARG1,	0x2F54	# memcard callback r3
.set VAR_MC_CB_ARG2,	0x2F58	# memcard callback r4
.set VAR_INTERRUPT_TIMES,	0x2F5C	# how many times have we called the dvd queue

.set _CARD_OPEN,		0x1
.set _CARD_FASTOPEN,	0x2
.set _CARD_CLOSE,		0x3
.set _CARD_CREATE,		0x4
.set _CARD_DELETE,		0x5
.set _CARD_READ,		0x6
.set _CARD_WRITE,		0x7
.set _CARD_GETSTATUS,	0x8
.set _CARD_SETSTATUS,	0x9
.set _CARD_SETUP,		0xA

#else

#define VAR_AREA			(0x80000000)
#define VAR_PATCHES_BASE	(VAR_AREA+0x2E00)	// Patches get copied to below this area.
#define VAR_FRAG_SIZE		(0xE0)				// Size of frag array in bytes
#define VAR_FRAG_LIST		(VAR_AREA+0x2E00)	// 0xE0 of fragments (18 frags max) (u32 sector, u32 size)
#define VAR_READ_DVDSTRUCT	(VAR_AREA+0x2EE0)	// 0x18 for our dvd structure
#define VAR_FAKE_IRQ_SET	(VAR_AREA+0x2EF8)	// flag to say we are ready to fake irq.
#define VAR_DVDIRQ_HNDLR	(VAR_AREA+0x2EFC)	// ptr to __DVDInterruptHandler
#define VAR_DISC_1_LBA 		(VAR_AREA+0x2F00)	// is the base file sector for disk 1
#define VAR_DISC_2_LBA 		(VAR_AREA+0x2F04)	// is the base file sector for disk 2
#define VAR_CUR_DISC_LBA 	(VAR_AREA+0x2F08)	// is the currently selected disk sector
#define VAR_EXI_BUS_SPD 	(VAR_AREA+0x2F0C)	// is the EXI bus speed (192 = 16mhz vs 208 = 32mhz)
#define VAR_SD_TYPE 		(VAR_AREA+0x2F10)	// is the Card Type (SDHC=0, SD=1)
#define VAR_EXI_FREQ 		(VAR_AREA+0x2F14)	// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
#define VAR_EXI_SLOT 		(VAR_AREA+0x2F18)	// is the EXI slot (0 = slot a, 1 = slot b)
#define VAR_TMP1  			(VAR_AREA+0x2F1C)	// space for a variable if required
#define VAR_TMP2  			(VAR_AREA+0x2F20)	// space for a variable if required
#define VAR_MC_FUNC  		(VAR_AREA+0x2F24)	// which MC Function do we want to call?
#define VAR_TMP4  			(VAR_AREA+0x2F28)	// space for a variable if required
#define VAR_DOL_SECTOR		(VAR_AREA+0x2F2C)	// In-Game-Reset DOL address
#define VAR_DOL_SIZE		(VAR_AREA+0x2F30)	// In-Game-Reset DOL size
#define VAR_FLOAT9_16		(VAR_AREA+0x2F34)	// constant 9/16
#define VAR_FLOAT1_6		(VAR_AREA+0x2F38)	// constant 1/6
#define VAR_FLOAT3_4		(VAR_AREA+0x2F3C)	// constant 3/4
#define VAR_FLOATM_1		(VAR_AREA+0x2F40)	// constant -1
#define VAR_MEMCARD_LBA		(VAR_AREA+0x2F44)	// Memory card file base on SD
#define VAR_MEMCARD_WORK	(VAR_AREA+0x2F48)	// ptr to memory card work area (40960 bytes big)
#define VAR_MEMCARD_RESULT	(VAR_AREA+0x2F4C)	// Last memory card status from a CARD func
#define VAR_MC_CB_ADDR		(VAR_AREA+0x2F50)	// memcard callback addr
#define VAR_MC_CB_ARG1		(VAR_AREA+0x2F54)	// memcard callback r3
#define VAR_MC_CB_ARG2		(VAR_AREA+0x2F58)	// memcard callback r4
#define VAR_INTERRUPT_TIMES	(VAR_AREA+0x2F5C)	// how many times have we called the dvd queue

#define _CARD_OPEN			(0x1)
#define _CARD_FASTOPEN		(0x2)
#define _CARD_CLOSE			(0x3)
#define _CARD_CREATE		(0x4)
#define _CARD_DELETE		(0x5)
#define _CARD_READ			(0x6)
#define _CARD_WRITE			(0x7)
#define _CARD_GETSTATUS		(0x8)
#define _CARD_SETSTATUS		(0x9)
#define _CARD_SETUP			(0xA)

#endif

