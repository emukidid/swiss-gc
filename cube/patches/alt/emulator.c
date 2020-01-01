/* 
 * Copyright (c) 2019, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "timer.h"
#include "../base/common.h"
#include "../base/dvd.h"
#include "../base/os.h"

void perform_read(uint32_t offset, uint32_t length, uint32_t address);
void trickle_read(void);
void change_disc(void);

void di_update_interrupts(void)
{
	if (((*DI_EMU)[0] >> 1) & ((*DI_EMU)[0] & 0b0101010) ||
		((*DI_EMU)[1] >> 1) & ((*DI_EMU)[1] & 0b010))
		*(OSInterruptMask *)OSCachedToMirrored(VAR_FAKE_IRQ_SET) |=  OS_INTERRUPTMASK_EMU_DI;
	else
		*(OSInterruptMask *)OSCachedToUncached(VAR_FAKE_IRQ_SET) &= ~OS_INTERRUPTMASK_EMU_DI;

	asm volatile("sc" ::: "r9", "r10");
}

void di_complete_transfer(void)
{
	(*DI_EMU)[0] |=  0b0010000;
	(*DI_EMU)[6]  =  0;
	(*DI_EMU)[7] &= ~0b001;
	di_update_interrupts();
}

static void di_execute_command(void)
{
	uint32_t result = 0;

	switch ((*DI_EMU)[2] >> 24) {
		case 0xA8:
		{
			if (!((*DI_EMU)[1] & 0b001)) {
				uint32_t offset  = (*DI_EMU)[3] << 2 & ~0x80000000;
				uint32_t length  = (*DI_EMU)[6];
				uint32_t address = (*DI_EMU)[5];

				offset |= *(uint32_t *)VAR_CURRENT_DISC * 0x80000000;

				perform_read(offset, length, address);
			} else {
				(*DI_EMU)[0] |=  0b0000100;
				(*DI_EMU)[7] &= ~0b001;
				di_update_interrupts();
			}
			return;
		}
		case 0xE0:
		{
			if ((*DI_EMU)[1] & 0b001)
				result = 0x01023A00;
			break;
		}
		case 0xE3:
		{
			if (*(uint32_t *)VAR_SECOND_DISC) {
				(*DI_EMU)[1] |= 0b001;
				timer2_start(OSSecondsToTicks(1));
			}
			break;
		}
	}

	(*DI_EMU)[8] = result;
	di_complete_transfer();
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

static void pi_read(unsigned index, uint32_t *value)
{
	switch (index) {
		case 0:
			if (*(uint32_t *)VAR_IGR_EXIT_FLAG & 0xF0)
				(*DI_EMU)[1] |= 0b001;
			*value = PI[index] & ~(!!(*(uint32_t *)VAR_IGR_EXIT_FLAG & 0x0F) << 16);
			break;
		default:
			*value = PI[index];
	}
}

static void pi_write(unsigned index, uint32_t value)
{
	switch (index) {
		case 9:
			PI[index] = ((value << 1) & 0b100) | (value & ~0b100);
			break;
		default:
			PI[index] = value;
	}
}

static bool ppc_load32(uint32_t address, uint32_t *value)
{
	if ((address & ~0xFFC) == 0x0C003000) {
		pi_read((address >> 2) & 0x7F, value);
		return true;
	}
	if ((address & ~0x3FC) == 0x0C006000) {
		di_read((address >> 2) & 0xF, value);
		return true;
	}
	return false;
}

static bool ppc_store32(uint32_t address, uint32_t value)
{
	if ((address & ~0xFFC) == 0x0C003000) {
		pi_write((address >> 2) & 0x7F, value);
		return true;
	}
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

extern void load_context(void) __attribute((noreturn));
extern uint32_t load_context_end[];

void service_exception(OSException exception, OSContext *context, uint32_t dsisr, uint32_t dar)
{
	OSExceptionHandler handler;

	switch (exception) {
		case OS_EXCEPTION_DSI:
		{
			handler = OSGetExceptionHandler(OS_EXCEPTION_USER);

			if (ppc_step(context)) {
				context->srr0 += 4;
				break;
			}
			if (handler) {
				ptrdiff_t offset = (ptrdiff_t)handler - (ptrdiff_t)load_context_end;

				*load_context_end = 0x48000000 | (offset & 0x3FFFFFC);
				asm volatile("dcbst 0,%0; sync; icbi 0,%0" :: "r" (load_context_end));
				load_context();
				return;
			}
		}
		default:
		{
			handler = *OSExceptionHandlerTable;
			if (handler) (handler + 0x50)(exception, context, dsisr, dar);
			break;
		}
		case OS_EXCEPTION_PERFORMANCE_MONITOR:
		{
			if (timer1_interrupt()) {
				timer1_stop();
				trickle_read();
			}
			if (timer2_interrupt()) {
				timer2_stop();
				change_disc();
			}
			break;
		}
	}

	*load_context_end = 0x60000000;
	asm volatile("dcbst 0,%0; sync; icbi 0,%0" :: "r" (load_context_end));
	load_context();
}

void exception_handler(OSException exception, OSContext *context, ...);

static void mem_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSInterruptMask cause = *(OSInterruptMask *)OSCachedToUncached(VAR_FAKE_IRQ_SET);
	OSInterruptHandler handler;
	OSContext exceptionContext;

	if (cause) {
		interrupt = __builtin_clz(cause);

		OSClearContext(&exceptionContext);
		OSSetCurrentContext(&exceptionContext);

		handler = OSGetInterruptHandler(interrupt);
		if (handler) handler(interrupt, &exceptionContext);

		OSClearContext(&exceptionContext);
		OSSetCurrentContext(context);
		return;
	}

	MI[16] = 0;
}

void exi_interrupt_handler(OSInterrupt interrupt, OSContext *context);

OSInterruptHandler set_di_handler(OSInterrupt interrupt, OSInterruptHandler handler)
{
	OSSetExceptionHandler(OS_EXCEPTION_DSI, exception_handler);
	OSSetInterruptHandler(OS_INTERRUPT_MEM_ADDRESS, mem_interrupt_handler);
	#ifdef BBA
	OSSetInterruptHandler(OS_INTERRUPT_EXI_2_EXI, exi_interrupt_handler);
	#endif
	return OSSetInterruptHandler(interrupt, handler);
}

void idle_thread(void)
{
	disable_interrupts();
	trickle_read();
	enable_interrupts();
}
