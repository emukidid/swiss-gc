/***************************************************************************
* HDD Read code for GC/Wii via IDE-EXI
* emu_kidid 2010-2012
***************************************************************************/

// we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
// This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
// memory map for our variables that sit at the top 0x100 of memory
#define VAR_AREA 			(0x81800000)		// Base location of our variables
#define VAR_AREA_SIZE		(0x100)				// Size of our variables block
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
#define VAR_CB_ADDR			(VAR_AREA-0xC4)		// high level read callback addr
#define VAR_CB_ARG1			(VAR_AREA-0xC0)		// high level read callback r3
#define VAR_CB_ARG2			(VAR_AREA-0xBC)		// high level read callback r4
#define VAR_PROG_MODE		(VAR_AREA-0xB8)		// data/code to overwrite GXRMode obj with for 480p forcing

// NOTE: cs0 then cs1!
// ATA registers address        val  - cs0 cs1 a2 a1 a0
#define ATA_REG_DATA			0x10	//1 0000b
#define ATA_REG_COMMAND			0x17    //1 0111b 
#define ATA_REG_DEVICE			0x16	//1 0110b
#define ATA_REG_ERROR			0x11	//1 0001b
#define ATA_REG_LBAHI			0x15	//1 0101b
#define ATA_REG_LBAMID			0x14	//1 0100b
#define ATA_REG_LBALO			0x13	//1 0011b
#define ATA_REG_SECCOUNT		0x12	//1 0010b
#define ATA_REG_STATUS			0x17	//1 0111b

// ATA commands
#define ATA_CMD_READSECT		0x21
#define ATA_CMD_READSECTEXT		0x24

// ATA head register bits
#define ATA_HEAD_USE_LBA	0x40

// ATA Status register bits we care about
#define ATA_SR_BSY		0x80
#define ATA_SR_DRQ		0x08
#define ATA_SR_ERR		0x01

#define EXI_SPEED16MHZ				4			/*!< EXI device frequency 16MHz */
#define EXI_SPEED32MHZ				5			/*!< EXI device frequency 32MHz */
#define EXI_READ					0			/*!< EXI transfer type read */
#define EXI_WRITE					1			/*!< EXI transfer type write */

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define SECTOR_SIZE 		512
#define ATA_READ_CHUNK_SIZE	1

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define exi_freq  		*(u32*)VAR_TMP1
#define exi_device  	*(u32*)VAR_TMP2
#define exi_channel 	*(u32*)VAR_TMP3

void* mymemcpy(void* dest, const void* src, u32 count)
{
	char* tmp = (char*)dest,* s = (char*)src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

void exi_select()
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[exi_channel * 5] = (exi[exi_channel * 5] & 0x405) | ((1<<exi_device)<<7) | (exi_freq << 4);
}

void exi_deselect()
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[exi_channel * 5] &= 0x405;
}

void exi_imm(void* data, int len, int mode)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	if (mode == EXI_WRITE)
		exi[exi_channel * 5 + 4] = *(unsigned long*)data;
	else
		exi[exi_channel * 5 + 4] = -1;
	// Tell EXI if this is a read or a write
	exi[exi_channel * 5 + 3] = ((len - 1) << 4) | (mode << 2) | 1;
	// Wait for it to do its thing
	while (exi[exi_channel * 5 + 3] & 1);

	if (mode == EXI_READ)
	{
		// Read the 4 byte data off the EXI bus
		unsigned long d = exi[exi_channel * 5 + 4];
		if(len == 4) {
			*(u32*)data = d;
		}
		else {
			int i;	
			for (i = 0; i < len; ++i)
				((unsigned char*)data)[i] = (d >> ((3 - i) * 8)) & 0xFF;
		}
	}

}

/* simple wrapper for transfers > 4bytes */
void exi_imm_ex(void* data, int len, int mode)
{
	unsigned char* d = (unsigned char*)data;
	while (len)
	{
		int tc = len;
		if (tc > 4)
			tc = 4;
		exi_imm(d, tc, mode);
		len -= tc;
		d += tc;
	}
}

// Returns 8 bits from the ATA Status register
u8 ataReadStatusReg()
{
	// read ATA_REG_CMDSTATUS1 | 0x00 (dummy)
	u16 dat = 0x1700;
	exi_select();
	exi_imm_ex(&dat,2,EXI_WRITE);
	exi_imm_ex(&dat,1,EXI_READ);
	exi_deselect();
	return *(u8*)&dat;
}

// Writes 8 bits of data out to the specified ATA Register
void ataWriteByte(u8 addr, u8 data)
{
	u32 dat = 0x80000000 | (addr << 24) | (data<<16);
	exi_select();
	exi_imm_ex(&dat,3,EXI_WRITE);	
	exi_deselect();
}

// Reads up to 0xFFFF * 4 bytes of data (~255kb) from the hdd at the given offset
void ata_read_blocks(u16 numSectors, u8 *dst) 
{
	u16 dwords = (numSectors<<7);
	// (31:29) 011b | (28:24) 10000b | (23:16) <num_words_LSB> | (15:8) <num_words_MSB> | (7:0) 00h (4 bytes)
	u32 dat = 0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8);
	exi_select();
	exi_imm_ex(&dat,4,EXI_WRITE);
	exi_imm_ex(dst,numSectors*SECTOR_SIZE,EXI_READ);
	exi_deselect();
}

// Reads sectors from the specified lba, for the specified slot, 511 sectors at a time max for LBA48 drives
// Returns 0 on success, -1 on failure.
int _ataReadSectors(u32 lba, u16 numsectors, void *buffer)
{
	u32 temp = 0;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg() & ATA_SR_BSY);

	// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)
	ataWriteByte(ATA_REG_DEVICE, ATA_HEAD_USE_LBA);

	ataWriteByte(ATA_REG_SECCOUNT, (u8)((numsectors>>8) & 0xFF));	// Sector count (Hi)
	ataWriteByte(ATA_REG_LBALO, (u8)((lba>>24)& 0xFF));				// LBA 4
	ataWriteByte(ATA_REG_LBAMID, 0);								// LBA 5
	ataWriteByte(ATA_REG_LBAHI, 0);									// LBA 6
	ataWriteByte(ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));		// Sector count (Lo)
	ataWriteByte(ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA 1
	ataWriteByte(ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA 2
	ataWriteByte(ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA 3

	// Write the appropriate read command
  	ataWriteByte(ATA_REG_COMMAND, ATA_CMD_READSECTEXT );

	// Wait for BSY to clear
	while((temp = ataReadStatusReg()) & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		*(u32*)0xCC003024 = 0;
		return 1;
	}

	// Wait for drive to request data transfer
	while(!(ataReadStatusReg() & ATA_SR_DRQ));
	
	// Wait for BSY to clear
	while((temp = ataReadStatusReg()) & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		*(u32*)0xCC003024 = 0;
		return 1;
	}

	// read data from drive
	ata_read_blocks(numsectors, buffer);

	// Wait for BSY to clear
	while((temp = ataReadStatusReg()) & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		*(u32*)0xCC003024 = 0;
		return 1;
	}
	
	return ataReadStatusReg() & ATA_SR_ERR;
}

void do_read(void *dst,u32 size, u32 offset) {
	u8 sector[SECTOR_SIZE];
	u32 lba = (offset>>9) + (*(u32*)VAR_CUR_DISC_LBA);
	
	// Load these from what Swiss set them to be
	exi_freq = *(u32*)VAR_EXI_FREQ;
	exi_device = 0; // for now until I add BBA port support
	exi_channel = *(u32*)VAR_EXI_SLOT;
	
	// Read any half sector if we need to until we're aligned
	if(offset % SECTOR_SIZE) {
		u32 size_to_copy = MIN(size, SECTOR_SIZE-(offset%SECTOR_SIZE));
		if(_ataReadSectors(lba, 1, sector)) {
			*(u32*)0xCC003024 = 0;
		}
		mymemcpy(dst, &(sector[offset%SECTOR_SIZE]), size_to_copy);
		size -= size_to_copy;
		dst += size_to_copy;
		lba += 1;
	}
	// Read any whole blocks
	if(size>>9) {
		// Read in 255k blocks
		while((size>>9) >= ATA_READ_CHUNK_SIZE) {
			if(_ataReadSectors(lba, ATA_READ_CHUNK_SIZE, dst)) {
				*(u32*)0xCC003024 = 0;
			}
			size -= (ATA_READ_CHUNK_SIZE*SECTOR_SIZE);
			dst += (ATA_READ_CHUNK_SIZE*SECTOR_SIZE);
			lba += (ATA_READ_CHUNK_SIZE);
		}
		// Read remaining whole blocks
		if(size>>9) {
			if(_ataReadSectors(lba, (size>>9), dst)) {
				*(u32*)0xCC003024 = 0;
			}
			size -= ((size>>9)*SECTOR_SIZE);
			dst += ((size>>9)*SECTOR_SIZE);
			lba += (size>>9);
		}
	}
	// Read the last block if there's any half block
	if(size) {
		if(_ataReadSectors(lba, 1, sector)) {
			*(u32*)0xCC003024 = 0;
		}
		mymemcpy(dst, &(sector[0]), size);
	}	
}
