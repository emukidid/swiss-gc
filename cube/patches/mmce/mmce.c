/***************************************************************************
# MMCE Read code for GC/Wii via SD on EXI
# bbsan2k 2025
#**************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "emulator.h"
#include "frag.h"
#include "interrupt.h"

#define EXI_INT    0


#define DEBUG(string) WriteUARTN(string, sizeof(string))
char text[32] = {0};

#ifndef QUEUE_SIZE
#define QUEUE_SIZE			2
#endif
#define SECTOR_SIZE 		512
#ifndef WRITE
#define WRITE				write
#endif

#define exi_cpr				(*(u8*)VAR_EXI_CPR)
#define exi_channel			(*(u8*)VAR_EXI_SLOT & 0x3)
#define exi_device			((*(u8*)VAR_EXI_SLOT & 0xC) >> 2)
#define exi_regs			(*(vu32**)VAR_EXI_REGS)


static struct {
	char (*buffer)[SECTOR_SIZE];
	uint32_t last_sector;
	uint32_t next_sector;
    uint16_t count;
	bool write;
	struct {
		void *buffer;
		uint16_t length;
		uint16_t offset;
        uint16_t count;
		uint32_t sector;
		bool write;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
} mmc = {
	.buffer = &VAR_SECTOR_BUF,
    .count = 0,
	.last_sector = ~0,
	.next_sector = ~0
};

#if EXI_INT
static volatile int sem = 0;
#endif

static void mmce_interrupt_handler(OSInterrupt interrupt, OSContext *context);
static void mmce_done_queued(void);


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
    return 0;
}

static void exi_dma_write(void* data, int len, bool sync)
{
	exi_regs[1] = (unsigned long)data;
	exi_regs[2] = len;
	exi_regs[3] = (EXI_WRITE << 2) | 3;
	while (sync && (exi_regs[3] & 1));
}

static void exi_dma_read(void* data, int len, bool sync)
{
	exi_regs[1] = (unsigned long)data;
	exi_regs[2] = len;
	exi_regs[3] = 3;

    if (!sync) {
        OSInterrupt interrupt = OS_INTERRUPT_EXI_0_TC + (3 * exi_channel);
        set_interrupt_handler(interrupt, mmce_interrupt_handler);
        unmask_interrupts(OS_INTERRUPTMASK(interrupt) & 
                        (OS_INTERRUPTMASK_EXI_0_TC | OS_INTERRUPTMASK_EXI_1_TC | OS_INTERRUPTMASK_EXI_2_TC));
    } else {
    	while ((exi_regs[3] & 1));
    }
}
    
static void mmce_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
    if (OS_INTERRUPTMASK(interrupt) & (OS_INTERRUPTMASK_EXI_0_TC | OS_INTERRUPTMASK_EXI_1_TC | OS_INTERRUPTMASK_EXI_2_TC)) {
        if ((exi_regs[3] & 1))
            return;
        
        mask_interrupts(OS_INTERRUPTMASK(interrupt) & (OS_INTERRUPTMASK_EXI_0_TC | OS_INTERRUPTMASK_EXI_1_TC | OS_INTERRUPTMASK_EXI_2_TC));
        exi_deselect();

#if EXI_INT
        OSInterrupt interrupt = OS_INTERRUPT_EXI_0_EXI + (3 * exi_channel);
        set_interrupt_handler(interrupt, mmce_interrupt_handler);
        unmask_interrupts(OS_INTERRUPTMASK(interrupt) & 
                (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_2_EXI));
#endif

        mmce_done_queued();
    } 
#if EXI_INT
    else if (OS_INTERRUPTMASK(interrupt) & (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_2_EXI))  {
        mask_interrupts(OS_INTERRUPTMASK(interrupt) & (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_2_EXI));
    
        sem++;
    }
#endif
}


static void mmce_start_read(u32 sector, u16 count)
{
#if EXI_INT
    OSInterrupt interrupt = OS_INTERRUPT_EXI_0_EXI + (3 * exi_channel);
    set_interrupt_handler(interrupt, mmce_interrupt_handler);
    unmask_interrupts(OS_INTERRUPTMASK(interrupt) & 
            (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_2_EXI));
#endif
    exi_regs[0] |= 3;
    exi_select();
    exi_imm_write(0x8B << 24 | 0x20 << 16 | ((sector >> 24) & 0xFF) << 8 | ((sector >> 16) & 0xFF), 4, true);
    exi_imm_write(((sector >> 8) & 0xFF) << 24 | (sector & 0xFF) << 16 | ((count >> 8) & 0xFF) << 8 | (count & 0xFF), 4, true);
    exi_deselect();
}


static void mmce_start_write(u32 sector, u16 count)
{
#if EXI_INT
    OSInterrupt interrupt = OS_INTERRUPT_EXI_0_EXI + (3 * exi_channel);
    set_interrupt_handler(interrupt, mmce_interrupt_handler);
    unmask_interrupts(OS_INTERRUPTMASK(interrupt) & 
            (OS_INTERRUPTMASK_EXI_0_EXI | OS_INTERRUPTMASK_EXI_1_EXI | OS_INTERRUPTMASK_EXI_2_EXI));
#endif
    exi_regs[0] |= 3;
    exi_select();
    exi_imm_write(0x8B << 24 | 0x22 << 16 | ((sector >> 24) & 0xFF) << 8 | ((sector >> 16) & 0xFF), 4, true);
    exi_imm_write(((sector >> 8) & 0xFF) << 24 | (sector & 0xFF) << 16 | ((count >> 8) & 0xFF) << 8 | (count & 0xFF), 4, true);
    exi_deselect();
}

bool mmce_check_interrupt() {
    return !!(exi_regs[0] & 0x2);
}


static void mmce_read(u8* buffer)
{
#if EXI_INT
    while (sem < 0);
#else
    while(!mmce_check_interrupt());
    exi_regs[0] |= 3;
#endif

    exi_select();
    exi_imm_write(0x8B << 24 | 0x21 << 16 | 0x00, 3, true);

    exi_dma_read(buffer, SECTOR_SIZE, false);
}


static void mmce_write(u8* buffer)
{
#if EXI_INT
    while (sem < 0);
#else
    while(!mmce_check_interrupt());
    exi_regs[0] |= 3;
#endif

    exi_select();
    exi_imm_write(0x8B << 24 | 0x23 << 16 | 0x00, 3, true);
    exi_dma_write(buffer, SECTOR_SIZE, true);
}

static void mmce_read_queued(void)
{
	if (exi_channel >= EXI_CHANNEL_MAX)
		__builtin_trap();

	if (!EXILock(exi_channel, exi_device, (EXICallback)mmce_read_queued))
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
			mmce_start_write(sector, mmc.count);
		}

        mmc.queued->length = SECTOR_SIZE;
		mmc.next_sector = sector + 1;
		mmc.write = write;
		mmce_done_queued();
		return;
	}

	if (sector == mmc.last_sector) {
		mmce_done_queued();
		return;
	}

	if (length < SECTOR_SIZE || ((uintptr_t)buffer % 32 && DMA_READ)) {
		mmc.last_sector = sector;
		buffer = mmc.buffer;
		DCInvalidateRange(__builtin_assume_aligned(buffer, 32), SECTOR_SIZE);
	}

    if ((mmc.next_sector != sector) || (mmc.count == 0)) {
        mmce_start_read(sector, mmc.queued->count);
        mmc.count = mmc.queued->count;
    }

    mmc.count--;
    mmc.next_sector = sector + 1;
    mmc.write = WRITE;

    mmce_read(buffer);
}

static void mmce_done_queued(void)
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
			mmce_read_queued();
			return;
		}
	}
	#endif
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (mmc.queue[i].callback != NULL) {
            mmc.queued = &mmc.queue[i];
			mmce_read_queued();
			return;
		}
	}
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
				mmce_read_queued();
			}
			return true;
		}
	}

	return false;
}

/* End of MMCE functions */
