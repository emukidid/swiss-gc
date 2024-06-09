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
#include "interrupt.h"

//CMD0 - Reset command
#define CMD0				0x40
//CMD12 - Stop multiple block read command
#define CMD12				0x4C
//CMD17 - Read single block command
#define CMD17				(0x51)
//CMD18 - Read multiple block command
#define CMD18				(0x52)
//CMD24 - Write single block command
#define CMD24				(0x58)
//CMD25 - Write multiple block command
#define CMD25				(0x59)

#ifndef QUEUE_SIZE
#define QUEUE_SIZE			2
#endif
#ifndef READ_MULTIPLE
#define READ_MULTIPLE		0
#endif
#define SECTOR_SIZE 		512
#ifndef WRITE
#define WRITE				write
#endif

#define exi_cpr				(*(u8*)VAR_EXI_CPR)
#define exi_channel			(*(u8*)VAR_EXI_SLOT & 0x3)
#define exi_device			((*(u8*)VAR_EXI_SLOT & 0xC) >> 2)
#define exi_regs			(*(vu32**)VAR_EXI_REGS)

#if ISR_READ
extern struct {
	int32_t transferred;
	#if !DMA_READ
	uint32_t data;
	#endif
	intptr_t buffer;
	intptr_t registers;
} _mmc;

void mmc_interrupt_vector(void);
#endif

static struct {
	char (*buffer)[SECTOR_SIZE];
	uint32_t last_sector;
	uint32_t next_sector;
	bool write;
	#if ISR_READ
	struct {
		void *buffer;
		uint16_t length;
		uint16_t offset;
		uint32_t sector;
		uint16_t count;
		bool write;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
	#endif
} mmc = {
	.buffer = &VAR_SECTOR_BUF,
	.last_sector = ~0,
	.next_sector = ~0
};

static void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context);

// EXI Functions
static void exi_clear_interrupts(bool exi, bool tc, bool ext)
{
	exi_regs[0] = (exi_regs[0] & (0x3FFF & ~0x80A)) | (ext << 11) | (tc << 3) | (exi << 1);
}

static bool exi_selected()
{
	return !!(exi_regs[0] & 0x380);
}

static void exi_select()
{
	exi_regs[0] = (exi_regs[0] & 0x405) | ((exi_cpr << 4) & 0x3F0);
}

#if DMA_READ
static void exi_selectsd()
{
	exi_regs[0] = (exi_regs[0] & 0x405) | ((exi_cpr << 4) & 0x070);
}
#else
#define exi_selectsd exi_select
#endif

static void exi_deselect()
{
	exi_regs[0] &= 0x405;
}

static void exi_imm_write(u32 data, int len, bool sync)
{
	exi_regs[4] = data;
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 1;
	// Wait for it to do its thing
	while (sync && (exi_regs[3] & 1));
}

static u32 exi_imm_read(int len, bool sync)
{
	exi_regs[4] = ~0;
	// Tell EXI if this is a read or a write
	exi_regs[3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;

	if (sync) {
		// Wait for it to do its thing
		while (exi_regs[3] & 1);
		// Read the 4 byte data off the EXI bus
		return exi_regs[4] >> ((4 - len) * 8);
	}
}

static void exi_dma_write(void* data, int len, bool sync)
{
	exi_regs[1] = (unsigned long)data;
	exi_regs[2] = len;
	exi_regs[3] = (EXI_WRITE << 2) | 3;
	while (sync && (exi_regs[3] & 1));
}

// SD Functions
#define rcvr_spi() ((u8)(exi_imm_read(1, true) & 0xFF))

static void send_cmd(u32 cmd, u32 sector) {
	exi_select();

	if(cmd != CMD12)
		while(rcvr_spi() != 0xFF);

	exi_imm_write(cmd<<24, 1, true);
	exi_imm_write(sector, 4, true);
	exi_imm_write(1<<24, 1, true);

	while(rcvr_spi() & 0x80);

	exi_deselect();
}

static void exi_read_to_buffer(void *dest, u32 len) {
	u32 *destu = (u32*)dest;
	while(len>=4) {
		u32 read = exi_imm_read(4, true);
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

static void rcvr_datablock(void *dest, u32 start_byte, u32 bytes_to_read, bool sync) {
	if(sync) {
		exi_selectsd();

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
		_mmc.registers = OSUncachedToPhysical(exi_regs);
		_mmc.buffer = (intptr_t)dest & ~OS_BASE_UNCACHED;
		_mmc.transferred = -1;

		exi_selectsd();
		exi_clear_interrupts(false, true, false);
		exi_imm_read(1, false);

		OSInterrupt interrupt = OS_INTERRUPT_EXI_0_TC + (3 * exi_channel);
		set_interrupt_handler(interrupt, tc_interrupt_handler);
		unmask_interrupts(OS_INTERRUPTMASK(interrupt) & (OS_INTERRUPTMASK_EXI_0_TC | OS_INTERRUPTMASK_EXI_1_TC | OS_INTERRUPTMASK_EXI_2_TC));
		#endif
	}
}

static bool xmit_datablock(void *src, u32 token) {
	exi_select();

	while(rcvr_spi() != 0xFF);

	exi_imm_write(token<<24, 1, true);

	if(token != 0xFD) {
		exi_dma_write(src, SECTOR_SIZE, true);
		exi_imm_write(0xFFFF<<16, 2, true);
	}

	u8 res = rcvr_spi();

	exi_deselect();

	return (res & 0x1F) == 0x05;
}

#if ISR_READ
static void mmc_done_queued(void);
static void mmc_read_queued(void)
{
	if (exi_channel >= EXI_CHANNEL_MAX)
		__builtin_trap();
	if (!EXILock(exi_channel, exi_device, (EXICallback)mmc_read_queued))
		return;

	void *buffer = mmc.queued->buffer;
	uint16_t length = mmc.queued->length;
	uint16_t offset = mmc.queued->offset;
	uint32_t sector = mmc.queued->sector;
	bool write = mmc.queued->write;

	if (WRITE) {
		if (mmc.last_sector == sector)
			mmc.last_sector = ~0;

		if (sector != mmc.next_sector || write != mmc.write) {
			end_read();
			send_cmd(CMD25, sector << *VAR_SD_SHIFT);
		}

		if (xmit_datablock(buffer, 0xFC))
			mmc.queued->length = SECTOR_SIZE;
		else
			mmc.queued->length = 0;

		mmc.next_sector = sector + 1;
		mmc.write = write;
		mmc_done_queued();
		return;
	}

	if (sector == mmc.last_sector) {
		mmc_done_queued();
		return;
	}

	if (length < SECTOR_SIZE || ((uintptr_t)buffer % 32 && DMA_READ)) {
		mmc.last_sector = sector;
		buffer = mmc.buffer;
		DCInvalidateRange(__builtin_assume_aligned(buffer, 32), SECTOR_SIZE);
	}

	if (sector != mmc.next_sector || WRITE != mmc.write) {
		end_read();
		send_cmd(CMD18, sector << *VAR_SD_SHIFT);
	}

	rcvr_datablock(buffer, 0, SECTOR_SIZE, false);
	mmc.next_sector = sector + 1;
	mmc.write = WRITE;
}

static void mmc_done_queued(void)
{
	void *buffer = mmc.queued->buffer;
	uint16_t length = mmc.queued->length;
	uint16_t offset = mmc.queued->offset;
	uint32_t sector = mmc.queued->sector;
	bool write = mmc.queued->write;

	if (!WRITE && sector == mmc.last_sector)
		buffer = memcpy(buffer, *mmc.buffer + offset, length);
	mmc.queued->callback(buffer, length);

	EXIUnlock(exi_channel);

	mmc.queued->callback = NULL;
	mmc.queued = NULL;

	#if QUEUE_SIZE > 2
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (mmc.queue[i].callback != NULL && mmc.queue[i].count == 1) {
			mmc.queued = &mmc.queue[i];
			mmc_read_queued();
			return;
		}
	}
	#endif
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (mmc.queue[i].callback != NULL) {
			mmc.queued = &mmc.queue[i];
			mmc_read_queued();
			return;
		}
	}
}

static void tc_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	if (_mmc.transferred < SECTOR_SIZE) {
		write_branch((void *)0x80000500, mmc_interrupt_vector);
		return;
	}

	mask_interrupts(OS_INTERRUPTMASK(interrupt) & (OS_INTERRUPTMASK_EXI_0_TC | OS_INTERRUPTMASK_EXI_1_TC | OS_INTERRUPTMASK_EXI_2_TC));
	exi_deselect();

	mmc_done_queued();
}

bool do_read_write_async(void *buffer, uint32_t length, uint32_t offset, uint64_t sector, bool write, frag_callback callback)
{
	uint16_t count;
	sector = offset / SECTOR_SIZE + sector;
	offset = offset % SECTOR_SIZE;
	count = (length + SECTOR_SIZE - 1 + offset) / SECTOR_SIZE;
	length = MIN(length, SECTOR_SIZE - offset);

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (mmc.queue[i].callback == NULL) {
			mmc.queue[i].buffer = buffer;
			mmc.queue[i].length = length;
			mmc.queue[i].offset = offset;
			mmc.queue[i].sector = sector;
			mmc.queue[i].count = count;
			mmc.queue[i].write = WRITE;
			mmc.queue[i].callback = callback;

			if (mmc.queued == NULL) {
				mmc.queued = &mmc.queue[i];
				mmc_read_queued();
			}
			return true;
		}
	}

	return false;
}
#endif

int do_read_write(void *buf, u32 len, u32 offset, u64 sectorLba, bool write) {
	u32 lba = (offset>>9) + sectorLba;
	u32 startByte = (offset%SECTOR_SIZE);
	u32 numBytes = MIN(len, SECTOR_SIZE-startByte);
	u8 lbaShift = *(u8*)VAR_SD_SHIFT;
	
	if(exi_selected()) {
		return 0;
	}
	if(WRITE) {
		#if ISR_READ || READ_MULTIPLE
		if(mmc.last_sector == lba) {
			mmc.last_sector = ~0;
		}
		#endif
		end_read();
		// Send single block write command and the LBA we want to write at
		send_cmd(CMD24, lba << lbaShift);
		// Write block
		if(xmit_datablock(buf, 0xFE)) {
			return SECTOR_SIZE;
		}
		return 0;
	}
	#if !READ_MULTIPLE
	end_read();
	// Send single block read command and the LBA we want to read at
	send_cmd(CMD17, lba << lbaShift);
	// Read block
	rcvr_datablock(buf, startByte, numBytes, true);
	#else
	// If we saved this sector
	if(lba == mmc.last_sector) {
		memcpy(buf, *mmc.buffer + startByte, numBytes);
		return numBytes;
	}
	// If we weren't just reading this sector
	if(lba != mmc.next_sector || WRITE != mmc.write) {
		end_read();
		// Send multiple block read command and the LBA we want to start reading at
		send_cmd(CMD18, lba << lbaShift);
	}
	if(numBytes < SECTOR_SIZE) {
		// Read half block
		rcvr_datablock(mmc.buffer, 0, SECTOR_SIZE, true);
		memcpy(buf, *mmc.buffer + startByte, numBytes);
		// Save current LBA
		mmc.last_sector = lba;
	}
	else {
		// Read full block
		rcvr_datablock(buf, 0, SECTOR_SIZE, true);
	}
	// Save next LBA
	mmc.next_sector = lba + 1;
	mmc.write = WRITE;
	#endif
	return numBytes;
}

void end_read() {
	#if ISR_READ || READ_MULTIPLE
	if(mmc.next_sector != ~0) {
		mmc.next_sector = ~0;

		if(mmc.write)
			xmit_datablock(0, 0xFD);
		else
			send_cmd(CMD12, 0);
	}
	#endif
}

void reset_device() {
	if(exi_regs != NULL) {
		end_read();
		send_cmd(CMD0, 0);
	}
}

/* End of SD functions */
