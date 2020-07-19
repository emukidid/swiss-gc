/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
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
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "emulator.h"
#include "fifo.h"
#include "mix.h"
#include "../base/common.h"
#include "../base/dvd.h"
#include "../base/DVDMath.h"
#include "../base/os.h"

static struct {
	bool playing;
	bool stopping;

	uint8_t (*buffer)[512];

	struct {
		uint32_t position;
		uint32_t start;
		uint32_t length;
	} current;

	struct {
		uint32_t start;
		uint32_t length;
	} next;
} dtk = {
	.buffer = (void *)VAR_SECTOR_BUF
};

OSAlarm command_alarm = {0};

#ifdef BBA
void exi_interrupt_handler(OSInterrupt interrupt, OSContext *context);
#endif
#ifdef WKF
void di_interrupt_handler(OSInterrupt interrupt, OSContext *context);
#endif

bool memeq(const void *a, const void *b, size_t size)
{
	const uint8_t *x = a;
	const uint8_t *y = b;
	size_t i;

	for (i = 0; i < size; ++i)
		if (x[i] != y[i])
			return false;

	return true;
}

#ifdef DTK
static void dtk_decode_buffer(void *address, uint32_t length)
{
	int16_t stream[448][2] __attribute((aligned(32)));

	for (int i = 0; i < sizeof(*dtk.buffer) / 32; i++)
		ADPDecodeBlock(*dtk.buffer + i * 32, stream + i * 28);

	fifo_write(stream, sizeof(stream));
	dtk.current.position += sizeof(*dtk.buffer);

	if (dtk.current.position == dtk.current.start + dtk.current.length) {
		dtk.current.position = 
		dtk.current.start  = dtk.next.start;
		dtk.current.length = dtk.next.length;

		if (dtk.stopping) {
			dtk.stopping = false;
			dtk.playing  = false;
		}

		ADPResetFilter();
	}
}

bool dtk_fill_buffer(void)
{
	if (!dtk.playing)
		return false;
	if (fifo_space() < 448 * sizeof(sample_t))
		return false;

	#ifdef WKF
	void read_callback(void *address, uint32_t length)
	{
		dtk_decode_buffer(address, length);
		dtk_fill_buffer();
	}

	dcache_flush_icache_inv(dtk.buffer, sizeof(*dtk.buffer));
	read_disc_frag(dtk.buffer, sizeof(*dtk.buffer), dtk.current.position, read_callback);
	#else
	OSCancelAlarm(&read_alarm);
	OSTick start = OSGetTick();

	int size = read_frag(dtk.buffer, sizeof(*dtk.buffer), dtk.current.position);
	if (size == sizeof(*dtk.buffer))
		dtk_decode_buffer(dtk.buffer, size);

	OSTick end = OSGetTick();
	OSSetAlarm(&read_alarm, OSDiffTick(end, start), (OSAlarmHandler)trickle_read);
	#endif

	return true;
}
#endif

#ifndef DVD
void di_update_interrupts(void)
{
	if (((*DI_EMU)[0] >> 1) & ((*DI_EMU)[0] & 0b0101010) ||
		((*DI_EMU)[1] >> 1) & ((*DI_EMU)[1] & 0b010))
		*(OSInterruptMask *)VAR_FAKE_IRQ_SET |=  OS_INTERRUPTMASK_EMU_DI;
	else
		*(OSInterruptMask *)VAR_FAKE_IRQ_SET &= ~OS_INTERRUPTMASK_EMU_DI;

	if (*(OSInterruptMask *)VAR_FAKE_IRQ_SET)
		*(volatile int *)OS_BASE_MIRRORED;
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

				perform_read(address, length, offset);
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
		case 0xE1:
		{
			switch (((*DI_EMU)[2] >> 16) & 0xFF) {
				case 0x00:
				{
					uint32_t offset = (*DI_EMU)[3] << 2 & ~0x80007FFF;
					uint32_t length = (*DI_EMU)[4] & ~0x7FFF;

					if (!offset && !length) {
						dtk.stopping = true;
					} else if (!dtk.stopping) {
						dtk.next.start  = offset;
						dtk.next.length = length;

						if (!dtk.playing) {
							dtk.current.position = 
							dtk.current.start  = offset;
							dtk.current.length = length;

							#ifdef DTK
							fifo_reset();
							#endif

							dtk.playing = true;
						}
					}
					break;
				}
				case 0x01:
				{
					dtk.stopping = false;
					dtk.playing  = false;

					#ifdef DTK
					ADPResetFilter();
					#endif
					break;
				}
			}
			break;
		}
		case 0xE2:
		{
			switch (((*DI_EMU)[2] >> 16) & 0xFF) {
				case 0x00:
				{
					result = dtk.playing;
					break;
				}
				case 0x01:
				{
					result = dtk.current.position & ~0x80007FFF >> 2;
					break;
				}
				case 0x02:
				{
					result = dtk.current.start & ~0x80007FFF >> 2;
					break;
				}
				case 0x03:
				{
					result = dtk.current.length;
					break;
				}
			}
			break;
		}
		case 0xE3:
		{
			if (!((*DI_EMU)[1] & 0b001) && *(uint32_t *)VAR_SECOND_DISC) {
				(*DI_EMU)[1] |= 0b001;
				OSSetAlarm(&command_alarm, OSSecondsToTicks(1), (OSAlarmHandler)change_disc);
			}
			break;
		}
	}

	(*DI_EMU)[8] = result;
	di_complete_transfer();
}
#else
static void di_execute_command(void)
{
	if (*VAR_DRIVE_RESETTING)
		return;

	switch ((*DI_EMU)[2] >> 24) {
		case 0xA8:
		{
			switch ((*DI_EMU)[2] & 0xFF) {
				case 0x00:
				{
					DVDDiskID *id1 = (DVDDiskID *)VAR_AREA;
					DVDDiskID *id2 = (DVDDiskID *)VAR_DISC_1_ID;

					if (!memeq(id1, id2, sizeof(DVDDiskID)))
						break;

					uint32_t offset  = (*DI_EMU)[3] << 2;
					uint32_t length  = (*DI_EMU)[6];
					uint32_t address = (*DI_EMU)[5];

					perform_read(address, length, offset);
					return;
				}
			}
			break;
		}
	}

	DI[2] = (*DI_EMU)[2];
	DI[3] = (*DI_EMU)[3];
	DI[4] = (*DI_EMU)[4];
	DI[5] = (*DI_EMU)[5];
	DI[6] = (*DI_EMU)[6];
	DI[7] = (*DI_EMU)[7];
}

void di_defer_transfer(uint32_t offset, uint32_t length)
{
	OSContext *context;
	OSContext exceptionContext;

	context = OSGetCurrentContext();
	OSClearContext(&exceptionContext);
	OSSetCurrentContext(&exceptionContext);

	__attribute((noinline))
	void di_defer_transfer(uint32_t offset, uint32_t length)
	{
		void alarm_handler(OSAlarm *alarm, OSContext *context)
		{
			#ifndef DVD
			(*DI_EMU)[0] |= ((*DI_EMU)[0] << 2) & 0b0001000;
			di_update_interrupts();
			#else
			uint32_t status = DI[0];
			uint32_t mask = status & 0b0101010;
			mask |= (mask << 2) & 0b0001000;
			DI[0] = mask;
			#endif
		}

		uint32_t ticks = OSMicrosecondsToTicks(300);
		ticks += OSSecondsToTicks(CalculateRawDiscReadTime(offset, length));
		OSSetAlarm(&command_alarm, ticks, alarm_handler);

		#ifndef DVD
		(*DI_EMU)[0] &= ~0b0001000;
		di_update_interrupts();
		#else
		uint32_t status = DI[0];
		uint32_t mask = status & 0b0101010;
		mask &= ~0b0001000;
		DI[0] = mask;
		#endif
	}

	di_defer_transfer(offset, length);

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);
}
#endif

static void di_read(unsigned index, uint32_t *value)
{
	#ifndef DVD
	*value = (*DI_EMU)[index];
	#else
	*value = DI[index];
	#endif
}

static void di_write(unsigned index, uint32_t value)
{
	switch (index) {
		#ifndef DVD
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
		#else
		default:
			DI[index] = value;
			break;
		#endif
		case 2 ... 4:
		case 8:
			(*DI_EMU)[index] = value;
			break;
		case 5 ... 6:
			(*DI_EMU)[index] = value & 0x3FFFFE0;
			break;
		case 7:
			(*DI_EMU)[index] = value & 0b111;

			if (value & 0b001)
				di_execute_command();
			break;
	}
}

#ifdef DTK
static sample_t __AXOutBuffer[2][160] __attribute((aligned(32))) = {0};
static sample_t (*AXOutBuffer)[160] = OSCachedToUncached(__AXOutBuffer);
static int AXOutFrame = 0;

static uint32_t DSP_EMU[15] = {0};
static int dsp_aimar_count = 0;

static void dsp_read(unsigned index, uint32_t *value)
{
	switch (index) {
		case 24:
		case 25:
			*value = ((uint16_t *)DSP)[index];
			dsp_aimar_count--;
			break;
		default:
			*value = ((uint16_t *)DSP)[index];
	}
}

static void dsp_write(unsigned index, uint16_t value)
{
	switch (index) {
		case 24:
			((uint16_t *)DSP_EMU)[index] = value & 0x3FF;
			dsp_aimar_count++;
			break;
		case 25:
			((uint16_t *)DSP_EMU)[index] = value & 0xFFE0;
			dsp_aimar_count++;
			break;
		case 27:
			((uint16_t *)DSP_EMU)[index] = value;

			if ((value & 0x8000) && dsp_aimar_count >= 0) {
				void *buffer = OSPhysicalToUncached(DSP_EMU[12]);
				int length = (DSP_EMU[13] & 0x7FFF) << 5;
				int count = length / sizeof(sample_t);

				if (length <= sizeof(*AXOutBuffer))
					buffer = memcpy(AXOutBuffer[AXOutFrame ^= 1], buffer, length);

				if ((AI[0] & 0b0000001) && fifo_size() >= length * 3 / 2) {
					uint8_t volume_l = AI[1];
					uint8_t volume_r = AI[1] >> 8;

					if (AI[0] & 0b1000000) {
						sample_t stream[count * 3 / 2] __attribute((aligned(32)));
						fifo_read(stream, sizeof(stream));
						mix_samples(buffer, stream, true, count, volume_l, volume_r);
					} else {
						sample_t stream[count] __attribute((aligned(32)));
						fifo_read(stream, sizeof(stream));
						mix_samples(buffer, stream, false, count, volume_l, volume_r);
					}
				}

				DSP[12] = (intptr_t)buffer;
				DSP[13] = ((length >> 5) & 0x7FFF) | 0x8000;

				if (fifo_size() < length * 3 / 2)
					dtk_fill_buffer();
			} else {
				DSP[12] = DSP_EMU[12];
				DSP[13] = DSP_EMU[13];
			}

			dsp_aimar_count = 0;
			break;
		default:
			DSP[index] = value;
	}
}
#endif

#ifdef DVD
static void dvd_reset(void)
{
	DI[0] = DI[0];
	DI[1] = DI[1];

	switch ((*VAR_DRIVE_RESETTING)++) {
		case 0:
			DI[2] = 0xFF014D41;
			DI[3] = 0x54534849;
			DI[4] = 0x54410200;
			DI[7] = 0b001;
			break;
		case 1:
			DI[2] = 0xFF004456;
			DI[3] = 0x442D4741;
			DI[4] = 0x4D450300;
			DI[7] = 0b001;
			break;
		case 2:
			DI[2] = 0x55010000;
			DI[3] = 0;
			DI[4] = 0;
			DI[7] = 0b001;
			break;
		case 3:
			DI[2] = 0xFE114100;
			DI[3] = 0;
			DI[4] = 0;
			DI[7] = 0b001;
			break;
		case 4:
			DI[2] = 0xEE060300;
			DI[3] = 0;
			DI[4] = 0;
			DI[7] = 0b001;
			break;
		case 5:
			*VAR_DRIVE_RESETTING = 0;

			if ((*DI_EMU)[7] & 0b001)
				di_execute_command();
			break;
	}
}
#endif

static void pi_read(unsigned index, uint32_t *value)
{
	*value = PI[index];
}

static void pi_write(unsigned index, uint32_t value)
{
	switch (index) {
		case 9:
			#ifndef DVD
			PI[index] = ((value << 2) & 0b100) | (value & ~0b100);
			break;
			#else
			if (*VAR_DRIVE_PATCHED) {
				PI[index] = ((value << 2) & 0b100) | (value & ~0b100);

				if (!*VAR_DRIVE_RESETTING && !(value & 0b100))
					dvd_reset();
				break;
			}
			#endif
		default:
			PI[index] = value;
	}
}

void wait_pecopydone(void);

static void efb_read(uint32_t address, uint32_t *value)
{
	wait_pecopydone();
	*value = *(uint32_t *)OSPhysicalToUncached(address);
}

static bool ppc_load32(uint32_t address, uint32_t *value)
{
	if ((address & ~0xFFFFFC) == 0x08000000) {
		efb_read(address, value);
		return true;
	}
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

#ifdef DTK
static bool ppc_load16(uint32_t address, uint32_t *value)
{
	if ((address & ~0xFFE) == 0x0C005000) {
		dsp_read((address >> 1) & 0xFF, value);
		return true;
	}
	return false;
}

static bool ppc_store16(uint32_t address, uint16_t value)
{
	if ((address & ~0xFFE) == 0x0C005000) {
		dsp_write((address >> 1) & 0xFF, value);
		return true;
	}
	return false;
}
#endif

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
		#ifdef DTK
		case 40:
		{
			int rd = (opcode >> 21) & 0x1F;
			int ra = (opcode >> 16) & 0x1F;
			short d = opcode & 0xFFFF;
			return ppc_load16(context->gpr[ra] + d, &context->gpr[rd]);
		}
		case 44:
		{
			int rs = (opcode >> 21) & 0x1F;
			int ra = (opcode >> 16) & 0x1F;
			short d = opcode & 0xFFFF;
			return ppc_store16(context->gpr[ra] + d, context->gpr[rs]);
		}
		#endif
	}

	return false;
}

void exception_handler(OSException exception, OSContext *context, ...);

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
			OSUnhandledException(exception, context, dsisr, dar);
			break;
		}
	}

	*load_context_end = 0x60000000;
	asm volatile("dcbst 0,%0; sync; icbi 0,%0" :: "r" (load_context_end));
	load_context();
}

static void dispatch_interrupt(OSInterrupt interrupt, OSContext *context)
{
	OSInterruptHandler handler;
	OSContext exceptionContext;

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(&exceptionContext);

	handler = OSGetInterruptHandler(interrupt);
	if (handler) handler(interrupt, &exceptionContext);

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);
}

static void mem_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSInterruptMask cause = *(OSInterruptMask *)VAR_FAKE_IRQ_SET;

	if (cause) {
		dispatch_interrupt(__builtin_clz(cause), context);
		return;
	}

	MI[16] = 0;
}

#ifdef DVD
static void di_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	if (*VAR_DRIVE_RESETTING) {
		dvd_reset();
		return;
	}

	dispatch_interrupt(OS_INTERRUPT_EMU_DI, context);
}
#endif

void init(void)
{
	#ifdef BBA
	OSCreateAlarm(&bba_alarm);
	#endif
	OSCreateAlarm(&read_alarm);
	OSCreateAlarm(&command_alarm);

	OSSetExceptionHandler(OS_EXCEPTION_DSI, exception_handler);
}

OSInterruptHandler set_di_handler(OSInterrupt interrupt, OSInterruptHandler handler)
{
	OSSetInterruptHandler(OS_INTERRUPT_MEM_ADDRESS, mem_interrupt_handler);
	#ifdef BBA
	OSSetInterruptHandler(OS_INTERRUPT_EXI_2_EXI, exi_interrupt_handler);
	#endif
	#if defined DVD || defined WKF
	OSSetInterruptHandler(OS_INTERRUPT_PI_DI, di_interrupt_handler);
	#endif
	return OSSetInterruptHandler(interrupt, handler);
}

void idle_thread(void)
{
	disable_interrupts();
	trickle_read();
	#ifdef BBA
	exi_interrupt_handler(OS_INTERRUPT_EXI_2_EXI, OSGetCurrentContext());
	#endif
	enable_interrupts();
}
