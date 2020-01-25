/***************************************************************************
# SD Read code for GC/Wii via SD on EXI
# emu_kidid 2007-2012
#**************************************************************************/

#include "../../reservedarea.h"
#include "../base/common.h"
#include "../base/exi.h"

//CMD12 - Stop multiple block read command
#define CMD12				0x4C
//CMD17 - Read single block command
#define CMD17				(0x51)
//CMD18 - Read multiple block command
#define CMD18				(0x52)
//CMD24 - Write single block command 
#define CMD24				(0x58)

#define SECTOR_SIZE 		512

#define sectorBuf			((u8*)VAR_SECTOR_BUF)

#define exi_freq  			(*(u8*)VAR_EXI_FREQ)
// exi_channel is stored as number of u32's to index into the exi bus (0xCC006800)
#define exi_channel 		(*(u8*)VAR_EXI_SLOT)

void *memcpy(void *dest, const void *src, u32 size);

// EXI Functions
static void exi_select()
{
	EXI[exi_channel][0] = (EXI[exi_channel][0] & 0x405) | ((1<<0)<<7) | (exi_freq << 4);
}

static void exi_deselect()
{
	EXI[exi_channel][0] &= 0x405;
}

static void exi_imm_write(u32 data, int len)
{
	EXI[exi_channel][4] = data;
	// Tell EXI if this is a read or a write
	EXI[exi_channel][3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 1;
	// Wait for it to do its thing
	while (EXI[exi_channel][3] & 1);
}

static u32 exi_imm_read(int len)
{
	EXI[exi_channel][4] = -1;
	// Tell EXI if this is a read or a write
	EXI[exi_channel][3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;
	// Wait for it to do its thing
	while (EXI[exi_channel][3] & 1);
	// Read the 4 byte data off the EXI bus
	return EXI[exi_channel][4] >> ((4 - len) * 8);
}

// SD Functions
#define rcvr_spi() ((u8)(exi_imm_read(1) & 0xFF))

static void send_cmd(u32 cmd, u32 sector) {
	exi_deselect();
	exi_select();
	rcvr_spi();

	while(rcvr_spi() != 0xFF);
	
	exi_imm_write(cmd<<24, 1);
	exi_imm_write(sector, 4);
	exi_imm_write(1<<24, 1);
	if (cmd == CMD12) rcvr_spi();

	int timeout = 16;
	while((rcvr_spi() & 0x80) && --timeout);
}

static void exi_read_to_buffer(void *dest, u32 len) {
	u32 *destu = (u32*)dest;
	while(len>=4) {
		u32 read = exi_imm_read(4);
		if(dest) *destu++ = read;
		len-=4;
	}
	u8 *destb = (u8*)destu;
	while(len) {
		u8 read = rcvr_spi();
		if(dest) *destb++ = read;
		len--;
	}
}

static void rcvr_datablock(void *dest, u32 start_byte, u32 bytes_to_read) {
	while(rcvr_spi() != 0xFE);

	// Skip the start if it's a misaligned read
	exi_read_to_buffer(0, start_byte);
	
	// Read however much we need to in this block
	exi_read_to_buffer(dest, bytes_to_read);

	// Read out the rest from the SD as we've requested it anyway and can't have it hanging off the bus (2 for CRC discard)
	u32 remainder = 2 + (SECTOR_SIZE - (start_byte+bytes_to_read));
	exi_read_to_buffer(0, remainder);
}

#ifndef SINGLE_SECTOR
u32 do_read(void *dst, u32 len, u32 offset, u32 sectorLba) {
	u32 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numBytes = len;
	u8 lbaShift = *(u8*)VAR_SD_SHIFT;
	
	// SDHC uses sector addressing, SD uses byte
	lba <<= lbaShift;	
	// Send multiple block read command and the LBA we want to start reading at
	send_cmd(CMD18, lba);
	
	// Read block crossing a boundary
	if(startByte) {
		// amount to read in first block may vary if our read is small enough to fit in it
		u32 amountInBlock = (len + startByte > SECTOR_SIZE) ? SECTOR_SIZE-startByte : len;
		rcvr_datablock(dst,startByte, amountInBlock);
		numBytes-=amountInBlock;
		dst+=amountInBlock;
	}
	
	// Read any full, aligned blocks
	u32 numFullBlocks = numBytes>>9; 
	numBytes -= numFullBlocks << 9;
	while(numFullBlocks) {
		rcvr_datablock(dst, 0,SECTOR_SIZE);
		dst+=SECTOR_SIZE; 
		numFullBlocks--;
	}
	
	// Read any trailing half block
	if(numBytes) {
		rcvr_datablock(dst,0, numBytes);
		dst+=numBytes;
	}
	// End the read by sending CMD12 + Deselect SD + Burn a cycle after it
	send_cmd(CMD12, 0);
	exi_deselect();
	rcvr_spi();
	return len;
}
#else
u32 do_read(void *dst, u32 len, u32 offset, u32 sectorLba) {
	// Try locking EXI bus
	if(!EXILock(exi_channel, EXI_DEVICE_0, 0)) {
		return 0;
	}
	
	u32 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numBytes = MIN(len, SECTOR_SIZE-startByte);
	u8 lbaShift = *(u8*)VAR_SD_SHIFT;
	
	// SDHC uses sector addressing, SD uses byte
	lba <<= lbaShift;
	#if SINGLE_SECTOR < 2
	// Send single block read command and the LBA we want to read at
	send_cmd(CMD17, lba);
	// Read block
	rcvr_datablock(dst, startByte, numBytes);
	// Deselect SD + Burn a cycle after it
	exi_deselect();
	rcvr_spi();
	#else
	// If we saved this sector
	if(lba == *(u32*)VAR_SECTOR_CUR) {
		memcpy(dst, sectorBuf + startByte, numBytes);
		goto exit;
	}
	// If we weren't just reading this sector
	else if(lba != *(u32*)VAR_SD_LBA) {
		end_read();
		// Send multiple block read command and the LBA we want to start reading at
		send_cmd(CMD18, lba);
	}
	if(numBytes < SECTOR_SIZE) {
		// Read half block
		rcvr_datablock(sectorBuf, 0, SECTOR_SIZE);
		memcpy(dst, sectorBuf + startByte, numBytes);
		// Save current LBA
		*(u32*)VAR_SECTOR_CUR = lba;
	}
	else {
		// Read full block
		rcvr_datablock(dst, 0, SECTOR_SIZE);
	}
	// Save next LBA
	*(u32*)VAR_SD_LBA = lba + (1<<lbaShift);
	#endif
exit:
	// Unlock EXI bus
	EXIUnlock(exi_channel);
	return numBytes;
}
#endif

void end_read() {
	#if SINGLE_SECTOR == 2
	if(*(u32*)VAR_SD_LBA) {
		*(u32*)VAR_SD_LBA = 0;
		// End the read by sending CMD12 + Deselect SD + Burn a cycle after it
		send_cmd(CMD12, 0);
		exi_deselect();
		rcvr_spi();
	}
	#endif
}

/* End of SD functions */
