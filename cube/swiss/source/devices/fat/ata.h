/**
 * IDE EXI Driver for Gamecube & Wii
 *
 * Based loosely on code written by Dampro
 * Re-written by emu_kidid
 * 
 * Very helpful ATA document: http://www.t10.org/t13/project/d1410r3a-ATA-ATAPI-6.pdf
**/

#ifndef ATA_H
#define ATA_H

#include <gccore.h>
#include <ogc/disc_io.h>

#define DEVICE_TYPE_GC_SD	(('G'<<24)|('C'<<16)|('S'<<8)|'D')

extern const DISC_INTERFACE __io_ataa;
extern const DISC_INTERFACE __io_atab;

// ATA status register bits
#define ATA_SR_BSY		0x80
#define ATA_SR_DRDY		0x40
#define ATA_SR_DF		0x20
#define ATA_SR_DSC		0x10
#define ATA_SR_DRQ		0x08
#define ATA_SR_CORR		0x04
#define ATA_SR_IDX		0x02
#define ATA_SR_ERR		0x01

// ATA error register bits
#define ATA_ER_UNC		0x40
#define ATA_ER_MC		0x20
#define ATA_ER_IDNF		0x10
#define ATA_ER_MCR		0x08
#define ATA_ER_ABRT		0x04
#define ATA_ER_TK0NF	0x02
#define ATA_ER_AMNF		0x01

// ATA head register bits
#define ATA_HEAD_USE_LBA	0x40

// NOTE: cs0 then cs1!
// ATA registers address        val  - cs0 cs1 a2 a1 a0
#define ATA_REG_DATA			0x10	//1 0000b
#define ATA_REG_COMMAND			0x17    //1 0111b 
#define ATA_REG_ALTSTATUS		0x0E	//0 1110b
#define ATA_REG_DEVICE			0x16	//1 0110b
#define ATA_REG_DEVICECONTROL 	0x0E	//0 1110b
#define ATA_REG_ERROR			0x11	//1 0001b
#define ATA_REG_FEATURES		0x11	//1 0001b
#define ATA_REG_LBAHI			0x15	//1 0101b
#define ATA_REG_LBAMID			0x14	//1 0100b
#define ATA_REG_LBALO			0x13	//1 0011b
#define ATA_REG_SECCOUNT		0x12	//1 0010b
#define ATA_REG_STATUS			0x17	//1 0111b
                                       
// ATA commands
#define ATA_CMD_IDENTIFY	0xEC
#define ATA_CMD_READSECT	0x20
#define ATA_CMD_READSECTEXT	0x24
#define ATA_CMD_WRITESECT	0x30
#define ATA_CMD_WRITESECTEXT 0x34
#define ATA_CMD_UNLOCK		0xF2

// ATA Identity fields
// all offsets refer to word offset (2 byte increments)
#define ATA_IDENT_SERIAL		10		// Drive serial (20 characters)
#define ATA_IDENT_MODEL			27		// Drive model name (40 characters)
#define ATA_IDENT_LBASECTORS	60		// Number of sectors in LBA translation mode
#define ATA_IDENT_COMMANDSET	83		// Command sets supported
#define ATA_IDENT_LBA48SECTORS	100		// Number of sectors in LBA 48-bit mode
#define ATA_IDENT_LBA48MASK		0x4		// Mask for LBA support in the command set top byte

// typedefs
// drive info structure
typedef struct 
{
	u64  sizeInSectors;
	u32  sizeInGigaBytes;
	int  lba48Support;
	char model[48];
	char serial[24];
} typeDriveInfo ATTRIBUTE_ALIGN (32);

typedef struct
{
	u16 type; //1 = master pw, 0 = user
	char password[32];
	u8 reserved[479];
} unlockStruct ATTRIBUTE_ALIGN (32);

extern typeDriveInfo ataDriveInfo;

// Prototypes
int ataDriveInit(int chn);
int ataUnlock(int chn, int useMaster, char *password);
int ataReadSectors(int chn, u64 sector, unsigned int numSectors, unsigned char *dest);
int ataWriteSectors(int chn, u64 sector,unsigned int numSectors, unsigned char *src);
bool ataIsInserted(int chn);
int ataShutdown(int chn);

// Swap defines
#define __lhbrx(base,index)			\
({	register u16 res;				\
	__asm__ volatile ("lhbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })

#define __lwbrx(base,index)			\
({	register u32 res;				\
	__asm__ volatile ("lwbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })

#endif

