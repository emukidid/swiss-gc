/***************************************************************************
* HDD Read code for GC/Wii via IDE-EXI
* emu_kidid 2010-2012
***************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"

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

#ifndef QUEUE_SIZE
#define QUEUE_SIZE			2
#endif
#define SECTOR_SIZE 		512

#define sectorBuf			((u8*)VAR_SECTOR_BUF + DMA_READ * 0x40000000)

#define _ata48bit *(u8*)VAR_ATA_LBA48

#define exi_freq			(*(u8*)VAR_EXI_FREQ)
#define exi_channel			(*(u8*)VAR_EXI_SLOT)
#define exi_regs			(*(vu32**)VAR_EXI_REGS)

static OSInterruptHandler TCIntrruptHandler = NULL;

static struct {
	uint32_t last_sector;
	#if DMA_READ
	int items;
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		uint32_t sector;
		frag_read_cb callback;
	} queue[QUEUE_SIZE];
	#endif
} ata = {
	.last_sector = -1
};

void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context);

static void exi_clear_interrupts(bool exi, bool tc, bool ext)
{
	exi_regs[0] = (exi_regs[0] & ~0x80A) | (ext << 11) | (tc << 3) | (exi << 1);
}

static int exi_selected()
{
	return !!(exi_regs[0] & 0x380);
}

static void exi_select()
{
	exi_regs[0] = (exi_regs[0] & 0x405) | ((1 << EXI_DEVICE_0) << 7) | (exi_freq << 4);
}

static void exi_deselect()
{
	exi_regs[0] &= 0x405;
}

static void exi_imm_write(u32 data, int len) 
{
	exi_regs[4] = data;
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 1;
	// Wait for it to do its thing
	while (exi_regs[3] & 1);
}

static u32 exi_imm_read(int len)
{
	exi_regs[4] = -1;
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;
	// Wait for it to do its thing
	while (exi_regs[3] & 1);
	// Read the 4 byte data off the EXI bus
	return exi_regs[4] >> ((4 - len) * 8);
}

static void exi_dma_read(void* data, int len)
{
	exi_regs[1] = (unsigned long)data;
	exi_regs[2] = len;
	exi_regs[3] = (EXI_READ << 2) | 3;
}

static void exi_sync()
{
	while (exi_regs[3] & 1);
}

// Returns 8 bits from the ATA Status register
static u8 ataReadStatusReg()
{
	// read ATA_REG_CMDSTATUS1 | 0x00 (dummy)
	u8 dat;
	exi_select();
	exi_imm_write(0x17000000, 2);
	dat=exi_imm_read(1);
	exi_deselect();
	return dat;
}

// Writes 8 bits of data out to the specified ATA Register
static void ataWriteByte(u8 addr, u8 data)
{
	exi_select();
	exi_imm_write(0x80000000 | (addr << 24) | (data<<16), 3);
	exi_deselect();
}

// Reads up to 0xFFFF * 4 bytes of data (~255kb) from the hdd at the given offset
static void ata_read_buffer(u8 *dst, int sync)
{
	u32 i = 0;
	u32 *ptr = (u32*)dst;
	u16 dwords = 128;
	// (31:29) 011b | (28:24) 10000b | (23:16) <num_words_LSB> | (15:8) <num_words_MSB> | (7:0) 00h (4 bytes)
	exi_select();
	exi_imm_write(0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8), 4);
	#if DMA_READ
	// v2, no deselect or extra read required.
	exi_clear_interrupts(0, 1, 0);
	exi_dma_read(ptr, SECTOR_SIZE);
	if(sync) {
		exi_sync();
		exi_deselect();
	}
	else {
		OSInterrupt interrupt = OS_INTERRUPT_EXI_0_TC + (3 * exi_channel);
		TCIntrruptHandler = OSSetInterruptHandler(interrupt, tc_interrupt_handler);
		OSUnmaskInterrupts(OS_INTERRUPTMASK(interrupt));
	}
	#else
	exi_deselect();
	for(i = 0; i < dwords; i++) {
		exi_select();
		*ptr++ = exi_imm_read(4);
		exi_deselect();
	}
	exi_select();
	exi_imm_read(4);
	exi_deselect();
	#endif
}

// Reads sectors from the specified lba, for the specified slot, 511 sectors at a time max for LBA48 drives
// Returns 0 on success, -1 on failure.
static int ataReadSector(u32 lba, void *buffer, int sync)
{
	u32 temp = 0;
  	
  	// Wait for drive to be ready (BSY to clear)
	while(ataReadStatusReg() & ATA_SR_BSY);

	// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)

	// Non LBA48
	if(!_ata48bit) {
		ataWriteByte(ATA_REG_DEVICE, 0xE0 | (u8)((lba >> 24) & 0x0F));
		ataWriteByte(ATA_REG_SECCOUNT, 1);								// Sector count (Lo)
		ataWriteByte(ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA 1
		ataWriteByte(ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA 2
		ataWriteByte(ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA 3
		ataWriteByte(ATA_REG_COMMAND, ATA_CMD_READSECT);
	}
	// LBA48
	else {
		ataWriteByte(ATA_REG_DEVICE, ATA_HEAD_USE_LBA);
		ataWriteByte(ATA_REG_SECCOUNT, 0);								// Sector count (Hi)
		ataWriteByte(ATA_REG_LBALO, (u8)((lba>>24)& 0xFF));				// LBA 4
		ataWriteByte(ATA_REG_LBAMID, 0);								// LBA 5
		ataWriteByte(ATA_REG_LBAHI, 0);									// LBA 6
		ataWriteByte(ATA_REG_SECCOUNT, 1);								// Sector count (Lo)
		ataWriteByte(ATA_REG_LBALO, (u8)(lba & 0xFF));					// LBA 1
		ataWriteByte(ATA_REG_LBAMID, (u8)((lba>>8) & 0xFF));			// LBA 2
		ataWriteByte(ATA_REG_LBAHI, (u8)((lba>>16) & 0xFF));			// LBA 3
		ataWriteByte(ATA_REG_COMMAND, ATA_CMD_READSECTEXT );
	}
	// Wait for BSY to clear
	while((temp = ataReadStatusReg()) & ATA_SR_BSY);
	
	// If the error bit was set, fail.
	if(temp & ATA_SR_ERR) {
		//*(u32*)buffer = 0x13370001;
		return 1;
	}

	// Wait for drive to request data transfer
	while(!(ataReadStatusReg() & ATA_SR_DRQ));
	
	// read data from drive
	ata_read_buffer(buffer, sync);

	if(sync) {
		// Wait for BSY to clear
		temp = ataReadStatusReg();
		
		// If the error bit was set, fail.
		if(temp & ATA_SR_ERR) {
			//*(u32*)buffer = 0x13370002;
			return 1;
		}
	}
	
	return 0;
}

#ifndef SINGLE_SECTOR
int do_read(void *dst, u32 len, u32 offset, u32 sectorLba) {
	u32 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numBytes = len;
	
	// Read any half sector if we need to until we're aligned
	if(startByte) {
		u32 size_to_copy = MIN(numBytes, SECTOR_SIZE-startByte);
		if(ataReadSector(lba, sectorBuf, 1)) {
			//*(u32*)dst = 0x13370003;
			return len-numBytes;
		}
		memcpy(dst, sectorBuf + startByte, size_to_copy);
		numBytes -= size_to_copy;
		dst += size_to_copy;
		lba ++;
	}
	// Read any whole sectors
	while(numBytes >= SECTOR_SIZE) {
		#if DMA_READ
		if(ataReadSector(lba, sectorBuf, 1)) {
			//*(u32*)dst = 0x13370004;
			return len-numBytes;
		}
		memcpy(dst, sectorBuf, SECTOR_SIZE);
		#else
		if(ataReadSector(lba, dst, 1)) {
			//*(u32*)dst = 0x13370004;
			return len-numBytes;
		}
		#endif
		numBytes -= SECTOR_SIZE;
		dst += SECTOR_SIZE;
		lba ++;
	}
	// Read the last sector if there's any half sector
	if(numBytes) {
		if(ataReadSector(lba, sectorBuf, 1)) {
			//*(u32*)dst = 0x13370006;
			return len-numBytes;
		}
		memcpy(dst, sectorBuf, numBytes);
	}	
	return len;
}
#else
#if DMA_READ
static void ata_done_queued(void);
static void ata_read_queued(void)
{
	if (!EXILock(exi_channel, EXI_DEVICE_0, (EXICallback)ata_read_queued))
		return;

	void *buffer = ata.queue[0].buffer;
	uint32_t length = ata.queue[0].length;
	uint32_t offset = ata.queue[0].offset;
	uint32_t sector = ata.queue[0].sector;

	if (sector == ata.last_sector) {
		OSSetAlarm(&read_alarm, COMMAND_LATENCY_TICKS, (OSAlarmHandler)ata_done_queued);
		return;
	}

	ataReadSector(sector, sectorBuf, 0);

	ata.last_sector = sector;
}

static void ata_done_queued(void)
{
	void *buffer = ata.queue[0].buffer;
	uint32_t length = ata.queue[0].length;
	uint32_t offset = ata.queue[0].offset;
	uint32_t sector = ata.queue[0].sector;
	frag_read_cb callback = ata.queue[0].callback;

	if (buffer != VAR_SECTOR_BUF + offset)
		memcpy(buffer, sectorBuf + offset, length);
	callback(buffer, length);

	EXIUnlock(exi_channel);

	if (--ata.items) {
		memcpy(ata.queue, ata.queue + 1, ata.items * sizeof(*ata.queue));
		ata_read_queued();
	}
}

bool do_read_async(void *buffer, uint32_t length, uint32_t offset, uint32_t sector, frag_read_cb callback)
{
	sector = offset / SECTOR_SIZE + sector;
	offset = offset % SECTOR_SIZE;
	length = MIN(length, SECTOR_SIZE - offset);

	ata.queue[ata.items].buffer = buffer;
	ata.queue[ata.items].length = length;
	ata.queue[ata.items].offset = offset;
	ata.queue[ata.items].sector = sector;
	ata.queue[ata.items].callback = callback;
	if (ata.items++) return true;

	ata_read_queued();
	return true;
}

void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSMaskInterrupts(OS_INTERRUPTMASK(interrupt));
	OSSetInterruptHandler(interrupt, TCIntrruptHandler);
	exi_deselect();

	ata_done_queued();
}
#endif

int do_read(void *dst, u32 len, u32 offset, u32 sectorLba) {
	u32 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numBytes = MIN(len, SECTOR_SIZE-startByte);
	
	if(exi_selected()) {
		return 0;
	}
	// If we saved this sector
	if(lba == ata.last_sector) {
		memcpy(dst, sectorBuf + startByte, numBytes);
		return numBytes;
	}
	if(numBytes < SECTOR_SIZE || DMA_READ) {
		// Read half sector
		if(ataReadSector(lba, sectorBuf, 1)) {
			return 0;
		}
		memcpy(dst, sectorBuf + startByte, numBytes);
		// Save current LBA
		ata.last_sector = lba;
	}
	else {
		// Read full sector
		if(ataReadSector(lba, dst, 1)) {
			return 0;
		}
		// If we're reusing the sector buffer
		if(dst == VAR_SECTOR_BUF) {
			// Save current LBA
			ata.last_sector = lba;
		}
	}
	return numBytes;
}
#endif

void end_read(u32 lba) {}
