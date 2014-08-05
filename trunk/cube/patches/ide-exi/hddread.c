/***************************************************************************
* HDD Read code for GC/Wii via IDE-EXI
* emu_kidid 2010-2012
***************************************************************************/

#include "../../reservedarea.h"

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

#define EXI_READ					0			/*!< EXI transfer type read */
#define EXI_WRITE					1			/*!< EXI transfer type write */

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define SECTOR_SIZE 		512
#define ATA_READ_CHUNK_SIZE	1

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define _ideexi_version *(u32*)VAR_TMP4
#define _ata48bit *(u32*)VAR_TMP1

#define IDE_EXI_V1 0
#define IDE_EXI_V2 1

#define exi_freq  			(*(u32*)VAR_EXI_FREQ)
// exi_channel is stored as number of u32's to index into the exi bus (0xCC006800)
#define exi_channel 		(*(u32*)VAR_EXI_SLOT)

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
	exi[exi_channel] = (exi[exi_channel] & 0x405) | ((1<<0)<<7) | (exi_freq << 4);
}

void exi_deselect()
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[exi_channel] &= 0x405;
}

void exi_imm_write(u32 data, int len) 
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[exi_channel + 4] = data;
	// Tell EXI if this is a read or a write
	exi[exi_channel + 3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 1;
	// Wait for it to do its thing
	while (exi[exi_channel + 3] & 1);
}

u32 exi_imm_read()
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[exi_channel + 4] = -1;
	// Tell EXI if this is a read or a write
	exi[exi_channel + 3] = ((4 - 1) << 4) | (EXI_READ << 2) | 1;
	// Wait for it to do its thing
	while (exi[exi_channel + 3] & 1);
	return exi[exi_channel + 4];
}

// Returns 8 bits from the ATA Status register
u8 ataReadStatusReg()
{
	// read ATA_REG_CMDSTATUS1 | 0x00 (dummy)
	u8 dat;
	exi_select();
	exi_imm_write(0x17000000, 2);
	dat=exi_imm_read()>>24;
	exi_deselect();
	return dat;
}

// Writes 8 bits of data out to the specified ATA Register
void ataWriteByte(u8 addr, u8 data)
{
	exi_select();
	exi_imm_write(0x80000000 | (addr << 24) | (data<<16), 3);
	exi_deselect();
}

// Reads up to 0xFFFF * 4 bytes of data (~255kb) from the hdd at the given offset
void ata_read_blocks(u16 numSectors, u8 *dst) 
{
	u32 i = 0;
	u32 *ptr = (u32*)dst;
	u16 dwords = (numSectors<<7);
	// (31:29) 011b | (28:24) 10000b | (23:16) <num_words_LSB> | (15:8) <num_words_MSB> | (7:0) 00h (4 bytes)
	exi_select();
	exi_imm_write(0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8), 4);
	
	if(!_ideexi_version) {
		exi_deselect();
		for(i = 0; i < dwords; i++) {
			exi_select();
			*ptr++ = exi_imm_read();
			exi_deselect();
		}
		exi_select();
		exi_imm_read();
	}
	else {	// v2, no deselect or extra read required.
		for(i = 0; i < dwords; i++) {
			*ptr++ = exi_imm_read();
		}
	}
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

	// Non LBA48
	if(!_ata48bit) {
		ataWriteByte(ATA_REG_DEVICE, 0xE0 | (u8)((lba >> 24) & 0x0F));
		ataWriteByte(ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));		// Sector count (Lo)
		ataWriteByte(ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA 1
		ataWriteByte(ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA 2
		ataWriteByte(ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA 3
		ataWriteByte(ATA_REG_COMMAND, ATA_CMD_READSECT);
	}
	// LBA48
	else {
		ataWriteByte(ATA_REG_DEVICE, ATA_HEAD_USE_LBA);
		ataWriteByte(ATA_REG_SECCOUNT, (u8)((numsectors>>8) & 0xFF));	// Sector count (Hi)
		ataWriteByte(ATA_REG_LBALO, (u8)((lba>>24)& 0xFF));				// LBA 4
		ataWriteByte(ATA_REG_LBAMID, 0);								// LBA 5
		ataWriteByte(ATA_REG_LBAHI, 0);									// LBA 6
		ataWriteByte(ATA_REG_SECCOUNT, (u8)(numsectors & 0xFF));		// Sector count (Lo)
		ataWriteByte(ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA 1
		ataWriteByte(ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA 2
		ataWriteByte(ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA 3
		ataWriteByte(ATA_REG_COMMAND, ATA_CMD_READSECTEXT );
	}
	// Wait for BSY to clear
	while((temp = ataReadStatusReg()) & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		//*(u32*)0xCC003024 = 0;
		*(u32*)buffer = 0x13370001;
		return 1;
	}

	// Wait for drive to request data transfer
	while(!(ataReadStatusReg() & ATA_SR_DRQ));
	
	// read data from drive
	ata_read_blocks(numsectors, buffer);

	// Wait for BSY to clear
	temp = ataReadStatusReg();
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		//*(u32*)0xCC003024 = 0;
		*(u32*)buffer = 0x13370002;
		return 1;
	}
	
	return temp & ATA_SR_ERR;
}

void do_read(void *dst,u32 size, u32 offset, u32 sectorLba) {
	u8 sector[SECTOR_SIZE];
	u32 lba = (offset>>9) + sectorLba;
	
	// Read any half sector if we need to until we're aligned
	if(offset % SECTOR_SIZE) {
		u32 size_to_copy = MIN(size, SECTOR_SIZE-(offset%SECTOR_SIZE));
		if(_ataReadSectors(lba, 1, sector)) {
			//*(u32*)0xCC003024 = 0;
			*(u32*)dst = 0x13370003;
			return;
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
				//*(u32*)0xCC003024 = 0;
				*(u32*)dst = 0x13370004;
				return;
			}
			size -= (ATA_READ_CHUNK_SIZE*SECTOR_SIZE);
			dst += (ATA_READ_CHUNK_SIZE*SECTOR_SIZE);
			lba += (ATA_READ_CHUNK_SIZE);
		}
		// Read remaining whole blocks
		if(size>>9) {
			if(_ataReadSectors(lba, (size>>9), dst)) {
				//*(u32*)0xCC003024 = 0;
				*(u32*)dst = 0x13370005;
				return;
			}
			size -= ((size>>9)*SECTOR_SIZE);
			dst += ((size>>9)*SECTOR_SIZE);
			lba += (size>>9);
		}
	}
	// Read the last block if there's any half block
	if(size) {
		if(_ataReadSectors(lba, 1, sector)) {
			//*(u32*)0xCC003024 = 0;
			*(u32*)dst = 0x13370006;
			return;
		}
		mymemcpy(dst, &(sector[0]), size);
	}	
}


