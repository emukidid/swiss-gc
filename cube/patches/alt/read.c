#include <stdint.h>
#include <string.h>
#include "../../reservedarea.h"
#include "../base/common.h"
#include "../base/dvd.h"
#include "../base/os.h"

void exi_handler() {}

int exi_lock(int32_t channel, uint32_t device)
{
	if (channel == *(uint8_t *)VAR_EXI_SLOT)
		end_read();
	return 1;
}

void trigger_dvd_interrupt(void)
{
	uint32_t dst = (*DI)[5] | 0x80000000;
	uint32_t len = (*DI)[6];

	(*DI)[2] = 0xE0000000;
	(*DI)[3] = 0;
	(*DI)[4] = 0;
	(*DI)[5] = 0;
	(*DI)[6] = 0;
	(*DI)[8] = 0;
	(*DI)[7] = 1;

	asm volatile("mtdabr %0" :: "r" (0));
	dcache_flush_icache_inv((void *)dst, len);
}

void dsi_exception_handler(OSException exception, OSContext *context, ...);

void perform_read(DVDCommandBlock *block)
{
	uint32_t dabr = (uint32_t)&block->state & ~7;

	uint32_t off = (*DI)[3] << 2;
	uint32_t len = (*DI)[4];
	uint32_t dst = (*DI)[5] | 0x80000000;

	*(uint32_t *)VAR_LAST_OFFSET = off;
	*(uint32_t *)VAR_TMP2 = len;
	*(uint32_t *)VAR_TMP1 = dst;

	OSExceptionTable[OS_EXCEPTION_DSI] = dsi_exception_handler;
	asm volatile("mtdabr %0" :: "r" (dabr | 0b101));
}

void tickle_read(void)
{
	uint32_t position  = *(uint32_t *)VAR_LAST_OFFSET;
	uint32_t remainder = *(uint32_t *)VAR_TMP2;
	uint8_t *data      = *(uint8_t **)VAR_TMP1;
	uint32_t data_size;

	if (remainder) {
		data_size = read_frag(data, remainder, position);

		position  += data_size;
		remainder -= data_size;

		*(uint32_t *)VAR_LAST_OFFSET = position;
		*(uint32_t *)VAR_TMP2 = remainder;
		*(uint8_t **)VAR_TMP1 = data + data_size;

		if (!remainder) trigger_dvd_interrupt();
	}
}

void tickle_read_idle(void)
{
	disable_interrupts();
	tickle_read();
	enable_interrupts();
}

OSContext *tickle_read_trap(OSException exception, OSContext *context, uint32_t dsisr, uint32_t dar)
{
	uint32_t dabr;

	if ((dsisr & 0x400000) == 0x400000) {
		asm volatile("mfdabr %0" : "=r" (dabr));
		asm volatile("mtdabr %0" :: "r" (dabr & ~0b011));

		tickle_read();
		context->srr1 |= 0x400;
	} else {
		OSExceptionHandler handler = *OSExceptionTable;
		if (handler) (handler + 0x50)(exception, context, dsisr, dar);
	}

	return context;
}
