/* 
 * Copyright (c) 2021-2023, Extrems <extrems@extremscorner.org>
 * 
 * This file is part of Swiss.
 * 
 * Swiss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Swiss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * with Swiss.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "interrupt.h"

static struct {
	OSInterruptHandler handler[OS_INTERRUPT_MAX];
	OSInterruptMask mask;
} irq;

OSInterruptHandler set_interrupt_handler(OSInterrupt interrupt, OSInterruptHandler handler)
{
	OSInterruptHandler oldHandler = irq.handler[interrupt];
	irq.handler[interrupt] = handler;
	return oldHandler;
}

OSInterruptHandler get_interrupt_handler(OSInterrupt interrupt)
{
	return irq.handler[interrupt];
}

static OSInterruptMask set_interrupt_mask(OSInterruptMask mask, OSInterruptMask current)
{
	if (mask & OS_INTERRUPTMASK_EXI_0) {
		uint32_t exi0cpr = EXI[EXI_CHANNEL_0][0];

		if (mask & OS_INTERRUPTMASK_EXI_0_EXI)
			rlwimi(exi0cpr, current, OS_INTERRUPT_EXI_0_EXI - 31, 31, 31);
		if (mask & OS_INTERRUPTMASK_EXI_0_TC)
			rlwimi(exi0cpr, current, OS_INTERRUPT_EXI_0_TC  - 29, 29, 29);
		if (mask & OS_INTERRUPTMASK_EXI_0_EXT)
			rlwimi(exi0cpr, current, OS_INTERRUPT_EXI_0_EXT - 21, 21, 21);

		EXI[EXI_CHANNEL_0][0] = exi0cpr & 0b11011111110101;
	}
	if (mask & OS_INTERRUPTMASK_EXI_1) {
		uint32_t exi1cpr = EXI[EXI_CHANNEL_1][0];

		if (mask & OS_INTERRUPTMASK_EXI_1_EXI)
			rlwimi(exi1cpr, current, OS_INTERRUPT_EXI_1_EXI - 31, 31, 31);
		if (mask & OS_INTERRUPTMASK_EXI_1_TC)
			rlwimi(exi1cpr, current, OS_INTERRUPT_EXI_1_TC  - 29, 29, 29);
		if (mask & OS_INTERRUPTMASK_EXI_1_EXT)
			rlwimi(exi1cpr, current, OS_INTERRUPT_EXI_1_EXT - 21, 21, 21);

		EXI[EXI_CHANNEL_1][0] = exi1cpr & 0b11011111110101;
	}
	if (mask & OS_INTERRUPTMASK_EXI_2) {
		uint32_t exi2cpr = EXI[EXI_CHANNEL_2][0];

		if (mask & OS_INTERRUPTMASK_EXI_2_EXI)
			rlwimi(exi2cpr, current, OS_INTERRUPT_EXI_2_EXI - 31, 31, 31);
		if (mask & OS_INTERRUPTMASK_EXI_2_TC)
			rlwimi(exi2cpr, current, OS_INTERRUPT_EXI_2_TC  - 29, 29, 29);

		EXI[EXI_CHANNEL_2][0] = exi2cpr & 0b11011111110101;
	}
	if (mask & OS_INTERRUPTMASK_PI) {
		uint32_t piintmsk = PI[1];

		if (mask & OS_INTERRUPTMASK_PI_CP)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_CP - 20, 20, 20);
		if (mask & OS_INTERRUPTMASK_PI_PE_TOKEN)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_PE_TOKEN - 22, 22, 22);
		if (mask & OS_INTERRUPTMASK_PI_PE_FINISH)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_PE_FINISH - 21, 21, 21);
		if (mask & OS_INTERRUPTMASK_PI_SI)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_SI - 28, 28, 28);
		if (mask & OS_INTERRUPTMASK_PI_DI)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_DI - 29, 29, 29);
		if (mask & OS_INTERRUPTMASK_PI_RSW)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_RSW - 30, 30, 30);
		if (mask & OS_INTERRUPTMASK_PI_ERROR)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_ERROR - 31, 31, 31);
		if (mask & OS_INTERRUPTMASK_PI_VI)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_VI - 23, 23, 23);
		if (mask & OS_INTERRUPTMASK_PI_DEBUG)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_DEBUG - 19, 19, 19);
		if (mask & OS_INTERRUPTMASK_PI_HSP)
			rlwimi(piintmsk, current, OS_INTERRUPT_PI_HSP - 18, 18, 18);

		PI[1] = piintmsk;
	}
	return current;
}

OSInterruptMask mask_interrupts(OSInterruptMask mask)
{
	return set_interrupt_mask(mask, irq.mask &= ~mask);
}

OSInterruptMask unmask_interrupts(OSInterruptMask mask)
{
	return set_interrupt_mask(mask, irq.mask |= mask);
}

OSInterruptMask mask_user_interrupts(OSInterruptMask mask)
{
	OSLocalInterruptMask |= mask;
	return set_interrupt_mask(mask, ~(OSGlobalInterruptMask | OSLocalInterruptMask));
}

OSInterruptMask unmask_user_interrupts(OSInterruptMask mask)
{
	OSLocalInterruptMask &= ~mask;
	return set_interrupt_mask(mask, ~(OSGlobalInterruptMask | OSLocalInterruptMask));
}

uint32_t exi_get_interrupt_mask(unsigned chan)
{
	uint32_t mask;
	uint32_t current = (irq.mask & OS_INTERRUPTMASK_EXI) << (3 * chan);

	rlwinm(mask, current, OS_INTERRUPT_EXI_0_EXI - 30, 30, 30);
	rlwimi(mask, current, OS_INTERRUPT_EXI_0_TC  - 28, 28, 28);
	rlwimi(mask, current, OS_INTERRUPT_EXI_0_EXT - 20, 20, 20);

	return mask;
}

void dispatch_interrupt(OSException exception, OSContext *context)
{
	OSInterruptMask cause = 0;

	uint32_t piintsr = PI[0] & PI[1];

	if (piintsr & 0b00000000010000) {
		uint32_t exi0cpr = EXI[EXI_CHANNEL_0][0];
		rlwinm(cause, exi0cpr, 30 - OS_INTERRUPT_EXI_0_EXI, OS_INTERRUPT_EXI_0_EXI, OS_INTERRUPT_EXI_0_EXI);
		rlwimi(cause, exi0cpr, 28 - OS_INTERRUPT_EXI_0_TC,  OS_INTERRUPT_EXI_0_TC,  OS_INTERRUPT_EXI_0_TC);
		rlwimi(cause, exi0cpr, 20 - OS_INTERRUPT_EXI_0_EXT, OS_INTERRUPT_EXI_0_EXT, OS_INTERRUPT_EXI_0_EXT);

		uint32_t exi1cpr = EXI[EXI_CHANNEL_1][0];
		rlwimi(cause, exi1cpr, 30 - OS_INTERRUPT_EXI_1_EXI, OS_INTERRUPT_EXI_1_EXI, OS_INTERRUPT_EXI_1_EXI);
		rlwimi(cause, exi1cpr, 28 - OS_INTERRUPT_EXI_1_TC,  OS_INTERRUPT_EXI_1_TC,  OS_INTERRUPT_EXI_1_TC);
		rlwimi(cause, exi1cpr, 20 - OS_INTERRUPT_EXI_1_EXT, OS_INTERRUPT_EXI_1_EXT, OS_INTERRUPT_EXI_1_EXT);

		uint32_t exi2cpr = EXI[EXI_CHANNEL_2][0];
		rlwimi(cause, exi2cpr, 30 - OS_INTERRUPT_EXI_2_EXI, OS_INTERRUPT_EXI_2_EXI, OS_INTERRUPT_EXI_2_EXI);
		rlwimi(cause, exi2cpr, 28 - OS_INTERRUPT_EXI_2_TC,  OS_INTERRUPT_EXI_2_TC,  OS_INTERRUPT_EXI_2_TC);
	}

//	rlwimi(cause, piintsr, 20 - OS_INTERRUPT_PI_CP,        OS_INTERRUPT_PI_CP,        OS_INTERRUPT_PI_CP);
//	rlwimi(cause, piintsr, 22 - OS_INTERRUPT_PI_PE_TOKEN,  OS_INTERRUPT_PI_PE_TOKEN,  OS_INTERRUPT_PI_PE_TOKEN);
//	rlwimi(cause, piintsr, 21 - OS_INTERRUPT_PI_PE_FINISH, OS_INTERRUPT_PI_PE_FINISH, OS_INTERRUPT_PI_PE_FINISH);
	rlwimi(cause, piintsr, 31 - OS_INTERRUPT_PI_ERROR,     OS_INTERRUPT_PI_SI,        OS_INTERRUPT_PI_ERROR);
//	rlwimi(cause, piintsr, 23 - OS_INTERRUPT_PI_VI,        OS_INTERRUPT_PI_VI,        OS_INTERRUPT_PI_VI);
//	rlwimi(cause, piintsr, 19 - OS_INTERRUPT_PI_DEBUG,     OS_INTERRUPT_PI_DEBUG,     OS_INTERRUPT_PI_DEBUG);
//	rlwimi(cause, piintsr, 18 - OS_INTERRUPT_PI_HSP,       OS_INTERRUPT_PI_HSP,       OS_INTERRUPT_PI_HSP);

	if (cause & irq.mask) {
		OSInterrupt interrupt = __builtin_clz(cause & irq.mask);

		if (irq.handler[interrupt]) {
			irq.handler[interrupt](interrupt, context);
			OSLoadContext(context);
		}
	}

	OSDispatchInterrupt(exception, context);
}
