/***************************************************************************
# SD Read code for GC/Wii via SD on EXI
# emu_kidid 2007-2012
#**************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"

//CMD12 - Stop multiple block read command
#define CMD12				0x4C
//CMD17 - Read single block command
#define CMD17				(0x51)
//CMD18 - Read multiple block command
#define CMD18				(0x52)
//CMD24 - Write single block command 
#define CMD24				(0x58)

#ifndef QUEUE_SIZE
#define QUEUE_SIZE			2
#endif
#define SECTOR_SIZE 		512

#define sectorBuf			((u8*)VAR_SECTOR_BUF)

#define exi_freq			(*(u8*)VAR_EXI_FREQ)
#define exi_channel			(*(u8*)VAR_EXI_SLOT)
#define exi_regs			(*(vu32**)VAR_EXI_REGS)

static OSInterruptHandler TCIntrruptHandler = NULL;

static struct {
	uint32_t next_sector;
	uint32_t last_sector;
	#if ISR_READ
	int items;
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		uint32_t sector;
		frag_read_cb callback;
	} queue[QUEUE_SIZE];
	#endif
} mmc = {
	.next_sector = -1,
	.last_sector = -1
};

extern intptr_t isr_registers;
extern int isr_transferred;

void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context);

// EXI Functions
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
	exi_regs[4] = -1;
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;

	if (sync) {
		// Wait for it to do its thing
		while (exi_regs[3] & 1);
		// Read the 4 byte data off the EXI bus
		return exi_regs[4] >> ((4 - len) * 8);
	}
}

static void exi_dma_write(void* data, int len, int sync)
{
	exi_regs[1] = (unsigned long)data;
	exi_regs[2] = len;
	exi_regs[3] = (EXI_WRITE << 2) | 3;
	while (sync && (exi_regs[3] & 1));
}

// SD Functions
#define rcvr_spi() ((u8)(exi_imm_read(1, 1) & 0xFF))

static void send_cmd(u32 cmd, u32 sector) {
	exi_select();

	if(cmd != CMD12)
		while(rcvr_spi() != 0xFF);

	exi_imm_write(cmd<<24, 1, 1);
	exi_imm_write(sector, 4, 1);
	exi_imm_write(1<<24, 1, 1);

	while(rcvr_spi() & 0x80);

	exi_deselect();
}

static void exi_read_to_buffer(void *dest, u32 len) {
	u32 *destu = (u32*)dest;
	while(len>=4) {
		u32 read = exi_imm_read(4, 1);
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

static void rcvr_datablock(void *dest, u32 start_byte, u32 bytes_to_read, int sync) {
	exi_select();

	if(sync) {
		while(rcvr_spi() != 0xFE);

		// Skip the start if it's a misaligned read
		exi_read_to_buffer(0, start_byte);

		// Read however much we need to in this block
		exi_read_to_buffer(dest, bytes_to_read);

		// Read out the rest from the SD as we've requested it anyway and can't have it hanging off the bus (2 for CRC discard)
		u32 remainder = 2 + (SECTOR_SIZE - (start_byte+bytes_to_read));
		exi_read_to_buffer(0, remainder);

		exi_deselect();
	}
	else {
		#if ISR_READ
		exi_clear_interrupts(0, 1, 0);
		exi_imm_read(1, 0);

		isr_registers = OSUncachedToPhysical(exi_regs);
		isr_transferred = 0xFEFFFFFF;

		OSInterrupt interrupt = OS_INTERRUPT_EXI_0_TC + (3 * exi_channel);
		TCIntrruptHandler = OSSetInterruptHandler(interrupt, tc_interrupt_handler);
		OSUnmaskInterrupts(OS_INTERRUPTMASK(interrupt));
		#endif
	}
}

static int xmit_datablock(void *src, u32 token) {
	exi_select();

	while(rcvr_spi() != 0xFF);

	exi_imm_write(token<<24, 1, 1);
	exi_dma_write(src, SECTOR_SIZE, 1);
	exi_imm_write(0xFFFF<<16, 2, 1);

	u8 res = rcvr_spi();

	exi_deselect();

	return (res & 0x1F) == 0x05;
}

#ifndef SINGLE_SECTOR
int do_read(void *dst, u32 len, u32 offset, u32 sectorLba) {
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
		rcvr_datablock(dst,startByte, amountInBlock, 1);
		numBytes-=amountInBlock;
		dst+=amountInBlock;
	}
	
	// Read any full, aligned blocks
	u32 numFullBlocks = numBytes>>9; 
	numBytes -= numFullBlocks << 9;
	while(numFullBlocks) {
		rcvr_datablock(dst, 0,SECTOR_SIZE, 1);
		dst+=SECTOR_SIZE; 
		numFullBlocks--;
	}
	
	// Read any trailing half block
	if(numBytes) {
		rcvr_datablock(dst,0, numBytes, 1);
		dst+=numBytes;
	}
	// End the read by sending CMD12
	send_cmd(CMD12, 0);
	return len;
}
#else
#if ISR_READ
static void mmc_done_queued(void);
static void mmc_read_queued(void)
{
	if (!EXILock(exi_channel, EXI_DEVICE_0, (EXICallback)mmc_read_queued))
		return;

	void *buffer = mmc.queue[0].buffer;
	uint32_t length = mmc.queue[0].length;
	uint32_t offset = mmc.queue[0].offset;
	uint32_t sector = mmc.queue[0].sector;

	if (sector == mmc.last_sector) {
		OSSetAlarm(&read_alarm, COMMAND_LATENCY_TICKS, (OSAlarmHandler)mmc_done_queued);
		return;
	}

	if (sector != mmc.next_sector) {
		end_read(-1);
		send_cmd(CMD18, sector << *VAR_SD_SHIFT);
	}

	rcvr_datablock(sectorBuf, 0, SECTOR_SIZE, 0);

	mmc.last_sector = sector;
	mmc.next_sector = sector + 1;
}

static void mmc_done_queued(void)
{
	void *buffer = mmc.queue[0].buffer;
	uint32_t length = mmc.queue[0].length;
	uint32_t offset = mmc.queue[0].offset;
	uint32_t sector = mmc.queue[0].sector;
	frag_read_cb callback = mmc.queue[0].callback;

	if (buffer != VAR_SECTOR_BUF + offset)
		memcpy(buffer, sectorBuf + offset, length);
	callback(buffer, length);

	EXIUnlock(exi_channel);

	if (--mmc.items) {
		memcpy(mmc.queue, mmc.queue + 1, mmc.items * sizeof(*mmc.queue));
		mmc_read_queued();
	}
}

bool do_read_async(void *buffer, uint32_t length, uint32_t offset, uint32_t sector, frag_read_cb callback)
{
	sector = offset / SECTOR_SIZE + sector;
	offset = offset % SECTOR_SIZE;
	length = MIN(length, SECTOR_SIZE - offset);

	mmc.queue[mmc.items].buffer = buffer;
	mmc.queue[mmc.items].length = length;
	mmc.queue[mmc.items].offset = offset;
	mmc.queue[mmc.items].sector = sector;
	mmc.queue[mmc.items].callback = callback;
	if (mmc.items++) return true;

	mmc_read_queued();
	return true;
}

void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	if (isr_transferred < SECTOR_SIZE)
		return;

	OSMaskInterrupts(OS_INTERRUPTMASK(interrupt));
	OSSetInterruptHandler(interrupt, TCIntrruptHandler);
	exi_imm_read(2, 1);
	exi_deselect();

	mmc_done_queued();
}
#endif

int do_read(void *dst, u32 len, u32 offset, u32 sectorLba) {
	u32 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numBytes = MIN(len, SECTOR_SIZE-startByte);
	u8 lbaShift = *(u8*)VAR_SD_SHIFT;
	
	if(exi_selected()) {
		return 0;
	}
	#if SINGLE_SECTOR < 2
	// Send single block read command and the LBA we want to read at
	send_cmd(CMD17, lba << lbaShift);
	// Read block
	rcvr_datablock(dst, startByte, numBytes, 1);
	#else
	// If we saved this sector
	if(lba == mmc.last_sector) {
		memcpy(dst, sectorBuf + startByte, numBytes);
		return numBytes;
	}
	// If we weren't just reading this sector
	if(lba != mmc.next_sector) {
		end_read(-1);
		// Send multiple block read command and the LBA we want to start reading at
		send_cmd(CMD18, lba << lbaShift);
	}
	if(numBytes < SECTOR_SIZE) {
		// Read half block
		rcvr_datablock(sectorBuf, 0, SECTOR_SIZE, 1);
		memcpy(dst, sectorBuf + startByte, numBytes);
		// Save current LBA
		mmc.last_sector = lba;
	}
	else {
		// Read full block
		rcvr_datablock(dst, 0, SECTOR_SIZE, 1);
		// If we're reusing the sector buffer
		if(dst == VAR_SECTOR_BUF) {
			// Save current LBA
			mmc.last_sector = lba;
		}
	}
	// Save next LBA
	mmc.next_sector = lba + 1;
	#endif
	return numBytes;
}

int do_write(void *src, u32 len, u32 offset, u32 sectorLba) {
	u32 lba = (offset>>9) + sectorLba;
	u8 lbaShift = *(u8*)VAR_SD_SHIFT;
	
	if(exi_selected()) {
		return 0;
	}
	end_read(lba);
	// Send single block write command and the LBA we want to write at
	send_cmd(CMD24, lba << lbaShift);
	// Write block
	if(xmit_datablock(src, 0xFE)) {
		return SECTOR_SIZE;
	}
	return 0;
}
#endif

void end_read(u32 lba) {
	#if SINGLE_SECTOR == 2 || ISR_READ
	if(mmc.last_sector == lba) {
		mmc.last_sector = -1;
	}
	if(mmc.next_sector != -1) {
		mmc.next_sector = -1;
		// End the read by sending CMD12
		send_cmd(CMD12, 0);
	}
	#endif
}

/* End of SD functions */
