/***************************************************************************
# SD Read code for GC/Wii via SDGecko
# emu_kidid 2007-2012
#**************************************************************************/

#include "../../reservedarea.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define EXI_READ			0
#define EXI_WRITE			1

//CMD12 - Stop multiple block read command
#define CMD12				0x4C
//CMD18 - Read multiple block command
#define CMD18				(0x52)
//CMD24 - Write single block command 
#define CMD24				(0x58)

#define SECTOR_SIZE 		512
#define exi_freq  			(*(u32*)VAR_EXI_FREQ)
// exi_channel is stored as number of u32's to index into the exi bus (0xCC006800)
#define exi_channel 		(*(u32*)VAR_EXI_SLOT)

// EXI Functions
static inline void exi_select()
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[exi_channel] = (exi[exi_channel] & 0x405) | ((1<<0)<<7) | (exi_freq << 4);
}

static inline void exi_deselect()
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

u32 exi_imm_read(int len)
{
	volatile unsigned long* exi = (volatile unsigned long*)0xCC006800;
	exi[exi_channel + 4] = -1;
	// Tell EXI if this is a read or a write
	exi[exi_channel + 3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;
	// Wait for it to do its thing
	while (exi[exi_channel + 3] & 1);

	// Read the 4 byte data off the EXI bus
	u32 d = exi[exi_channel + 4];
	if(len == 4) return d;
	return (d >> ((4 - len) * 8));
	
}

// SD Functions
#define rcvr_spi() ((u8)(exi_imm_read(1) & 0xFF))

void send_cmd(u32 cmd, u32 sector) {
	exi_deselect();
	exi_select();
	rcvr_spi();

	while(rcvr_spi() != 0xFF);
	
	exi_imm_write(cmd<<24, 1);
	exi_imm_write(sector, 4);
	exi_imm_write(1<<24, 1);
	if (cmd == CMD12) rcvr_spi();

	int timeout = 10;
	while((rcvr_spi() & 0x80) && timeout) {
		timeout--;
	}
}

void rcvr_datablock(void *dest, u32 start_byte, u32 bytes_to_read) {
	u8 res = rcvr_spi();
	int bytesRead = start_byte+bytes_to_read;
	
	while(rcvr_spi() == 0xFF);
	
	// Skip the start if it's a misaligned read
	while(start_byte>4) {
		exi_imm_read(4);
		start_byte-=4;
	}
	while(start_byte) {
		rcvr_spi();
		start_byte--;
	}
	u32 *destu = (u32*)dest;
	// Read however much we need to in this block
	while(bytes_to_read > 4) {
		*destu++ = exi_imm_read(4);
		bytes_to_read-=4;
	}
	u8 *destb = (u8*)destu;
	// Read the remainder
	while(bytes_to_read) {
		*destb++ = exi_imm_read(1);
		bytes_to_read--;
	}

	// Read out the rest from the SD as we've requested it anyway and can't have it hanging off the bus
	int remainder = SECTOR_SIZE-bytesRead;
	while(remainder>4) {
		exi_imm_read(4);
		remainder-=4;
	}
	while(remainder) {
		rcvr_spi();
		remainder--;
	}
	exi_imm_read(2);		// discard CRC
}

void do_read(void *dst, u32 len, u32 offset, u32 sectorLba) {
	u32 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numBytes = len;
	
	int lbaShift = 9 * (*(u32*)VAR_SD_TYPE);	// SD Card Type (SDHC=0, SD=1)
	
	// SDHC uses sector addressing, SD uses byte
	lba <<= lbaShift;	

	// If we weren't just reading this sector
	if(lba != *(u32*)VAR_TMP1) {
		if(*(u32*)VAR_TMP2) {
			// End the read by sending CMD12 + Deselect SD + Burn a cycle after it
			send_cmd(CMD12, 0);
			exi_deselect();
			rcvr_spi();
		}
		// Send multiple block read command and the LBA we want to start reading at
		send_cmd(CMD18, lba);
		*(u32*)VAR_TMP2 = 1;
	}
	
	// Read block crossing a boundary
	if(startByte) {
		// amount to read in first block may vary if our read is small enough to fit in it
		int amountInBlock = (len + startByte > SECTOR_SIZE) ? SECTOR_SIZE-startByte : len;
		rcvr_datablock(dst,startByte, amountInBlock);
		numBytes-=amountInBlock;
		dst+=amountInBlock;
	}
	
	// Read any full, aligned blocks
	int numFullBlocks = numBytes>>9; 
	while(numFullBlocks) {
		rcvr_datablock(dst, 0,SECTOR_SIZE);
		dst+=SECTOR_SIZE; 
		numBytes-=SECTOR_SIZE; 
		numFullBlocks--;
	}
	
	// Read any trailing half block
	if(numBytes) {
		rcvr_datablock(dst,0, numBytes);
	}
		
	*(u32*)VAR_TMP1 = lba + ((len + SECTOR_SIZE-startByte) >> lbaShift);
}

/* End of SD functions */
