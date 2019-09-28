/* 
 * Copyright (c) 2019, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../base/common.h"
#include "../base/dvd.h"
#include "../base/os.h"

void perform_read(uint32_t offset, uint32_t length, uint32_t address);
void tickle_read(void);

void di_update_interrupts(void)
{
	if (((*DI_EMU)[0] >> 1) & ((*DI_EMU)[0] & 0b0101010) ||
		((*DI_EMU)[1] >> 1) & ((*DI_EMU)[1] & 0b010))
		*(OSInterruptMask *)OSCachedToMirrored(VAR_FAKE_IRQ_SET) |=  OS_INTERRUPTMASK_EMU_DI;
	else
		*(OSInterruptMask *)OSCachedToUncached(VAR_FAKE_IRQ_SET) &= ~OS_INTERRUPTMASK_EMU_DI;
}

void di_complete_transfer(void)
{
	uint32_t address = (*DI_EMU)[5] + OS_BASE_CACHED;
	uint32_t length  = (*DI_EMU)[6];

	(*DI_EMU)[0] |= 0b0010000;
	(*DI_EMU)[6]  = 0;
	(*DI_EMU)[7] &= ~0b001;

	if ((*DI_EMU)[7] & 0b010)
		dcache_flush_icache_inv((void *)address, length);

	di_update_interrupts();
}

static void di_execute_command(void)
{
	switch ((*DI_EMU)[2] >> 24) {
		case 0xA8:
		{
			uint32_t offset  = (*DI_EMU)[3] << 2;
			uint32_t length  = (*DI_EMU)[4];
			uint32_t address = (*DI_EMU)[5] + OS_BASE_CACHED;

			perform_read(offset, length, address);
			break;
		}
		default:
		{
			(*DI_EMU)[8] = 0;
			di_complete_transfer();
		}
	}
}

static void di_read(unsigned index, uint32_t *value)
{
	*value = (*DI_EMU)[index];
}

static void di_write(unsigned index, uint32_t value)
{
	switch (index) {
		case 0:
			(*DI_EMU)[0] = ((value & 0b1010100) ^ (*DI_EMU)[0]) & (*DI_EMU)[0];
			(*DI_EMU)[0] = (value & 0b0101011) | ((*DI_EMU)[0] & ~0b0101010);
			di_update_interrupts();
			break;
		case 1:
			(*DI_EMU)[1] = ((value & 0b100) ^ (*DI_EMU)[1]) & (*DI_EMU)[1];
			(*DI_EMU)[1] = (value & 0b010) | ((*DI_EMU)[1] & ~0b010);
			di_update_interrupts();
			break;
		case 2 ... 4:
		case 8:
			(*DI_EMU)[index] = value;
			break;
		case 5:
			(*DI_EMU)[index] = value & 0x3FFFFE0;
			break;
		case 6:
			(*DI_EMU)[index] = value & ~0x1F;
			break;
		case 7:
			(*DI_EMU)[index] = value & 0b111;

			if (value & 0b001)
				di_execute_command();
			break;
	}
}

static bool ppc_load32(uint32_t address, uint32_t *value)
{
	if ((address & ~0x3FC) == 0x0C006000) {
		di_read((address >> 2) & 0xF, value);
		return true;
	}
	return false;
}

static bool ppc_store32(uint32_t address, uint32_t value)
{
	if ((address & ~0x3FC) == 0x0C006000) {
		di_write((address >> 2) & 0xF, value);
		return true;
	}
	return false;
}

static bool ppc_step(OSContext *context)
{
	uint32_t opcode = *(uint32_t *)context->srr0;

	switch (opcode >> 26) {
		case 32:
		{
			int rd = (opcode >> 21) & 0x1F;
			int ra = (opcode >> 16) & 0x1F;
			short d = opcode & 0xFFFF;
			return ppc_load32(context->gpr[ra] + d, &context->gpr[rd]);
		}
		case 33:
		{
			int rd = (opcode >> 21) & 0x1F;
			int ra = (opcode >> 16) & 0x1F;
			short d = opcode & 0xFFFF;
			return ppc_load32(context->gpr[ra] += d, &context->gpr[rd]);
		}
		case 36:
		{
			int rs = (opcode >> 21) & 0x1F;
			int ra = (opcode >> 16) & 0x1F;
			short d = opcode & 0xFFFF;
			return ppc_store32(context->gpr[ra] + d, context->gpr[rs]);
		}
	}

	return false;
}

OSContext *exception_handler(OSException exception, OSContext *context, uint32_t dsisr, uint32_t dar)
{
	uint32_t dabr;

	switch (exception) {
		case OS_EXCEPTION_DSI:
		{
			if ((dsisr & 0x400000) == 0x400000) {
				asm volatile("mfdabr %0" : "=r" (dabr));
				asm volatile("mtdabr %0" :: "r" (dabr & ~0b011));

				tickle_read();
				context->srr1 |= 0x400;
				break;
			}
			if (ppc_step(context)) {
				context->srr0 += 4;
				break;
			}
		}
		default:
		{
			OSExceptionHandler handler = *OSExceptionHandlerTable;
			if (handler) (handler + 0x50)(exception, context, dsisr, dar);
		}
	}

	return context;
}

void dsi_exception_handler(OSException exception, OSContext *context, ...);

static void mem_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSInterruptMask cause = *(OSInterruptMask *)OSCachedToUncached(VAR_FAKE_IRQ_SET);
	OSContext exceptionContext;

	if (cause) {
		interrupt = __builtin_clz(cause);

		OSClearContext(&exceptionContext);
		OSSetCurrentContext(&exceptionContext);

		OSInterruptHandler handler = OSGetInterruptHandler(interrupt);
		if (handler) handler(interrupt, &exceptionContext);

		OSClearContext(&exceptionContext);
		OSSetCurrentContext(context);
		return;
	}

	(*MI)[16] = 0;
}

OSInterruptHandler set_di_handler(OSInterrupt interrupt, OSInterruptHandler handler)
{
	OSSetExceptionHandler(OS_EXCEPTION_DSI, dsi_exception_handler);
	OSSetInterruptHandler(OS_INTERRUPT_MEM_ADDRESS, mem_interrupt_handler);

	return OSSetInterruptHandler(interrupt, handler);
}

DVDCommandBlock *set_breakpoint(DVDCommandBlock *block)
{
	uint32_t dabr = (uint32_t)&block->state & ~0b111;
	asm volatile("mtdabr %0" :: "r" (dabr | 0b101));
	return block;
}

bool unset_breakpoint(bool queued)
{
	asm volatile("mtdabr %0" :: "r" (0));
	return queued;
}
