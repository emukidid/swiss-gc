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
#define ATA_CMD_READSECT		0x20
#define ATA_CMD_READSECTEXT		0x24
#define ATA_CMD_WRITESECT		0x30
#define ATA_CMD_WRITESECTEXT	0x34

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

#define _ata48bit *(u8*)VAR_ATA_LBA48

#define exi_freq			(*(u8*)VAR_EXI_FREQ)
#define exi_channel			(*(u8*)VAR_EXI_SLOT & (EXI_CHANNEL_0 | EXI_CHANNEL_1))
#define exi_device			(*(u8*)VAR_EXI_SLOT & (EXI_DEVICE_0  | EXI_DEVICE_2))
#define exi_regs			(*(vu32**)VAR_EXI_REGS)

#if ISR_READ
extern struct {
	int transferred;
	intptr_t registers;
} _ata;
#endif

static struct {
	char (*buffer)[SECTOR_SIZE];
	uint64_t last_sector;
	uint64_t next_sector;
	uint16_t count;
	#if DMA_READ || ISR_READ
	struct {
		void *buffer;
		uint16_t length;
		uint16_t offset;
		uint64_t sector;
		uint16_t count;
		bool write;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
	#endif
} ata = {
	#if DMA_READ
	.buffer = OSCachedToUncached(&VAR_SECTOR_BUF),
	#else
	.buffer = &VAR_SECTOR_BUF,
	#endif
	.last_sector = ~0,
	.next_sector = ~0
};

static OSInterruptHandler TCIntrruptHandler = NULL;
static void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context);

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
	exi_regs[0] = (exi_regs[0] & 0x405) | ((1 << exi_device) << 7) | (exi_freq << 4);
}

static void exi_deselect()
{
	exi_regs[0] &= 0x405;
}

static void exi_imm_write(u32 data, int len, int sync)
{
	exi_regs[4] = data;
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 1;
	// Wait for it to do its thing
	while (sync && (exi_regs[3] & 1));
}

static u32 exi_imm_read(int len, int sync)
{
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;

	if (sync) {
		// Wait for it to do its thing
		while (exi_regs[3] & 1);
		// Read the 4 byte data off the EXI bus
		return exi_regs[4] >> ((4 - len) * 8);
	}
}

static u32 exi_imm_read_write(u32 data, int len, int sync)
{
	exi_regs[4] = data;
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 1;

	if (sync) {
		// Wait for it to do its thing
		while (exi_regs[3] & 1);
		// Read the 4 byte data off the EXI bus
		return exi_regs[4] >> ((4 - len) * 8);
	}
}

static void exi_dma_read(void* data, int len, int sync)
{
	exi_regs[1] = (unsigned long)data;
	exi_regs[2] = len;
	exi_regs[3] = (EXI_READ << 2) | 3;
	while (sync && (exi_regs[3] & 1));
}

// Returns 8 bits from the ATA Status register
static u8 ataReadStatusReg()
{
	// read ATA_REG_CMDSTATUS1 | 0x00 (dummy)
	u8 dat;
	exi_select();
	dat=exi_imm_read_write(0x17000000, 3, 1);
	exi_deselect();
	return dat;
}

// Writes 8 bits of data out to the specified ATA Register
static void ataWriteByte(u8 addr, u8 data)
{
	exi_select();
	exi_imm_write(0x80000000 | (addr << 24) | (data<<16), 3, 1);
	exi_deselect();
}

// Writes 16 bits to the ATA Data register
static void ataWriteu16(u16 data)
{
	// write 16 bit to ATA_REG_DATA | data LSB | data MSB | 0x00 (dummy)
	exi_select();
	exi_imm_write(0xD0000000 | (((data>>8) & 0xff)<<16) | ((data & 0xff)<<8), 4, 1);
	exi_deselect();
}

// Reads sectors from the specified lba
static void ataReadSectors(u64 sector, u16 count)
{
	u8 status;

	// Wait for drive to be ready (BSY to clear)
	do {
		status = ataReadStatusReg();
	} while(status & ATA_SR_BSY);

	// Select the device differently based on 28 or 48bit mode
	if(!_ata48bit) {
		ataWriteByte(ATA_REG_DEVICE, 0xE0 | (u8)((sector >> 24) & 0x0F));
	}
	else {
		// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)
		ataWriteByte(ATA_REG_DEVICE, ATA_HEAD_USE_LBA);

		ataWriteByte(ATA_REG_LBAHI, (u8)((sector >> 40) & 0xFF));	// LBA 6
		ataWriteByte(ATA_REG_LBAMID, (u8)((sector >> 32) & 0xFF));	// LBA 5
		ataWriteByte(ATA_REG_LBALO, (u8)((sector >> 24) & 0xFF));	// LBA 4
		ataWriteByte(ATA_REG_SECCOUNT, (u8)((count >> 8) & 0xFF));	// Sector count (Hi)
	}

	ataWriteByte(ATA_REG_LBAHI, (u8)((sector >> 16) & 0xFF));		// LBA 3
	ataWriteByte(ATA_REG_LBAMID, (u8)((sector >> 8) & 0xFF));		// LBA 2
	ataWriteByte(ATA_REG_LBALO, (u8)(sector & 0xFF));				// LBA 1
	ataWriteByte(ATA_REG_SECCOUNT, (u8)(count & 0xFF));				// Sector count (Lo)

	// Write the appropriate read command
	ataWriteByte(ATA_REG_COMMAND, !_ata48bit ? ATA_CMD_READSECT : ATA_CMD_READSECTEXT);
}

static int ataReadBuffer(void *buffer)
{
	u8 status;

	// Wait for drive to request data transfer
	do {
		status = ataReadStatusReg();
		// If the error bit was set, fail.
		if(status & ATA_SR_ERR) return 0;
	} while((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ));

	// read data from drive
	int i;
	u32 *ptr = (u32*)buffer;
	u16 dwords = 128;
	// (31:29) 011b | (28:24) 10000b | (23:16) <num_words_LSB> | (15:8) <num_words_MSB> | (7:0) 00h (4 bytes)
	exi_select();
	exi_imm_write(0x70000000 | ((dwords&0xff) << 16) | (((dwords>>8)&0xff) << 8), 4, 1);
	#if DMA_READ
	// v2, no deselect or extra read required.
	exi_dma_read(ptr, SECTOR_SIZE, 1);
	exi_deselect();
	#else
	exi_deselect();
	for(i = 0; i < dwords; i++) {
		exi_select();
		*ptr++ = exi_imm_read(4, 1);
		exi_deselect();
	}
	exi_select();
	exi_imm_read(4, 1);
	exi_deselect();
	#endif
	return 1;
}

static void ataReadBufferAsync(void *buffer)
{
	#if ISR_READ
	_ata.registers = OSUncachedToPhysical(exi_regs);
	_ata.transferred = -1;

	exi_select();
	exi_clear_interrupts(0, 1, 0);
	exi_imm_read_write(0x17000000, 3, 0);

	OSInterrupt interrupt = OS_INTERRUPT_EXI_0_TC + (3 * exi_channel);
	TCIntrruptHandler = OSSetInterruptHandler(interrupt, tc_interrupt_handler);
	OSUnmaskInterrupts(OS_INTERRUPTMASK(interrupt));
	#endif
}

// Writes sectors to the specified lba
static void ataWriteSectors(u64 sector, u16 count)
{
	u8 status;

	// Wait for drive to be ready (BSY to clear)
	do {
		status = ataReadStatusReg();
	} while(status & ATA_SR_BSY);

	// Select the device differently based on 28 or 48bit mode
	if(!_ata48bit) {
		ataWriteByte(ATA_REG_DEVICE, 0xE0 | (u8)((sector >> 24) & 0x0F));
	}
	else {
		// Select the device (ATA_HEAD_USE_LBA is 0x40 for master, 0x50 for slave)
		ataWriteByte(ATA_REG_DEVICE, ATA_HEAD_USE_LBA);

		ataWriteByte(ATA_REG_LBAHI, (u8)((sector >> 40) & 0xFF));	// LBA 6
		ataWriteByte(ATA_REG_LBAMID, (u8)((sector >> 32) & 0xFF));	// LBA 5
		ataWriteByte(ATA_REG_LBALO, (u8)((sector >> 24) & 0xFF));	// LBA 4
		ataWriteByte(ATA_REG_SECCOUNT, (u8)((count >> 8) & 0xFF));	// Sector count (Hi)
	}

	ataWriteByte(ATA_REG_LBAHI, (u8)((sector >> 16) & 0xFF));		// LBA 3
	ataWriteByte(ATA_REG_LBAMID, (u8)((sector >> 8) & 0xFF));		// LBA 2
	ataWriteByte(ATA_REG_LBALO, (u8)(sector & 0xFF));				// LBA 1
	ataWriteByte(ATA_REG_SECCOUNT, (u8)(count & 0xFF));				// Sector count (Lo)

	// Write the appropriate write command
	ataWriteByte(ATA_REG_COMMAND, !_ata48bit ? ATA_CMD_WRITESECT : ATA_CMD_WRITESECTEXT);
}

static int ataWriteBuffer(void *buffer)
{
	u8 status;

	// Wait for drive to request data transfer
	do {
		status = ataReadStatusReg();
		// If the error bit was set, fail.
		if(status & ATA_SR_ERR) return 0;
	} while((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ));

	// Write data to the drive
	int i;
	u16 *ptr = (u16*)buffer;
	for(i = 0; i < 256; i++) {
		ataWriteu16(ptr[i]);
	}

	// Wait for the write to finish
	do {
		status = ataReadStatusReg();
		// If the error bit was set, fail.
		if(status & ATA_SR_ERR) return 0;
	} while(status & ATA_SR_BSY);

	return 1;
}

#if DMA_READ || ISR_READ
static void ata_done_queued(void);
static void ata_read_queued(void)
{
	if (!EXILock(exi_channel, exi_device, (EXICallback)ata_read_queued))
		return;

	void *buffer = ata.queued->buffer;
	uint16_t length = ata.queued->length;
	uint16_t offset = ata.queued->offset;
	uint64_t sector = ata.queued->sector;
	uint16_t count = ata.queued->count;
	bool write = ata.queued->write;

	if (write) {
		if (ata.last_sector == sector)
			ata.last_sector = ~0;

		ata.count = 0;
		ataWriteSectors(sector, 1);

		if (ataWriteBuffer(buffer))
			ata.queued->length = SECTOR_SIZE;
		else
			ata.queued->length = 0;

		ata_done_queued();
		return;
	}

	if (sector == ata.last_sector) {
		ata_done_queued();
		return;
	}

	if (sector != ata.next_sector || ata.count == 0) {
		ata.count = count;
		ataReadSectors(sector, count);
	}

	ataReadBufferAsync(ata.buffer);

	ata.last_sector = sector;
	ata.next_sector = sector + 1;
	ata.count--;
}

static void ata_done_queued(void)
{
	void *buffer = ata.queued->buffer;
	uint16_t length = ata.queued->length;
	uint16_t offset = ata.queued->offset;
	uint64_t sector = ata.queued->sector;
	uint16_t count = ata.queued->count;
	bool write = ata.queued->write;

	if (!write)
		buffer = memcpy(buffer, *ata.buffer + offset, length);
	ata.queued->callback(buffer, length);

	EXIUnlock(exi_channel);

	ata.queued->callback = NULL;
	ata.queued = NULL;

	if (ata.count > 0) {
		for (int i = 0; i < QUEUE_SIZE; i++) {
			if (ata.queue[i].callback != NULL && ata.queue[i].sector == ata.next_sector) {
				ata.queued = &ata.queue[i];
				ata_read_queued();
				return;
			}
		}
	}
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (ata.queue[i].callback != NULL && ata.queue[i].count == 1) {
			ata.queued = &ata.queue[i];
			ata_read_queued();
			return;
		}
	}
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (ata.queue[i].callback != NULL) {
			ata.queued = &ata.queue[i];
			ata_read_queued();
			return;
		}
	}
}

static void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	#if ISR_READ
	if (_ata.transferred < SECTOR_SIZE)
		return;
	#endif

	OSMaskInterrupts(OS_INTERRUPTMASK(interrupt));
	OSSetInterruptHandler(interrupt, TCIntrruptHandler);
	exi_deselect();

	ata_done_queued();
}

bool do_read_write_async(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write, frag_callback callback)
{
	length = MIN(length, 32768 - OSRoundUp32B(offset) % 32768);

	uint16_t count;
	sector = offset / SECTOR_SIZE + sector;
	offset = offset % SECTOR_SIZE;
	count = MIN((length + SECTOR_SIZE - 1 + offset) / SECTOR_SIZE, 0x100);
	length = MIN(length, SECTOR_SIZE - offset);

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (ata.queue[i].callback == NULL) {
			ata.queue[i].buffer = buffer;
			ata.queue[i].length = length;
			ata.queue[i].offset = offset;
			ata.queue[i].sector = sector;
			ata.queue[i].count = count;
			ata.queue[i].write = write;
			ata.queue[i].callback = callback;

			if (ata.queued == NULL) {
				ata.queued = &ata.queue[i];
				ata_read_queued();
			}
			return true;
		}
	}

	return false;
}
#else
bool do_read_write_async(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write, frag_callback callback)
{
	return false;
}
#endif

int do_read_write(void *buf, u32 len, u32 offset, u64 sectorLba, bool write) {
	u64 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numSectors = MIN((len+SECTOR_SIZE-1+startByte)>>9, 0x100);
	u32 numBytes = MIN(len, SECTOR_SIZE-startByte);
	
	if(exi_selected()) {
		return 0;
	}
	if(write) {
		if(ata.last_sector == lba) {
			ata.last_sector = ~0;
		}
		ata.count = 0;
		ataWriteSectors(lba, 1);
		// Write full sector
		if(ataWriteBuffer(buf)) {
			return SECTOR_SIZE;
		}
		return 0;
	}
	// If we saved this sector
	if(lba == ata.last_sector) {
		memcpy(buf, *ata.buffer + startByte, numBytes);
		return numBytes;
	}
	// If we weren't just reading this sector
	if(lba != ata.next_sector || ata.count == 0) {
		ata.count = numSectors;
		ataReadSectors(lba, numSectors);
	}
	if(numBytes < SECTOR_SIZE || DMA_READ) {
		// Read half sector
		if(!ataReadBuffer(ata.buffer)) {
			ata.count = 0;
			return 0;
		}
		memcpy(buf, *ata.buffer + startByte, numBytes);
		// Save current LBA
		ata.last_sector = lba;
	}
	else {
		// Read full sector
		if(!ataReadBuffer(buf)) {
			ata.count = 0;
			return 0;
		}
	}
	// Save next LBA
	ata.next_sector = lba + 1;
	ata.count--;
	return numBytes;
}

void end_read() {}
