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
#include <math.h>
#include "audio.h"
#include "common.h"
#include "dolphin/dvd.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "DVDMath.h"
#include "emulator.h"
#include "fifo.h"

static struct {
	OSInterruptHandler handler[2];
	OSInterruptMask status;
	OSInterruptMask mask;
} irq = {0};

static struct {
	union {
		uint32_t regs[9];
		struct {
			uint32_t sr;
			uint32_t cvr;
			uint32_t cmdbuf0;
			uint32_t cmdbuf1;
			uint32_t cmdbuf2;
			uint32_t mar;
			uint32_t length;
			uint32_t cr;
			uint32_t immbuf;
		} reg;
	};

	uint32_t error;
	#ifdef DVD
	int reset;
	#endif
} di = {0};

static struct {
	adpcm_t adpcm;

	bool playing;
	bool stopping;

	struct {
		uint32_t position;
		uint32_t start;
		uint32_t length;
	} current;

	struct {
		uint32_t start;
		uint32_t length;
	} next;

	uint8_t (*buffer)[512];
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

__attribute((always_inline))
double fmod(double x, double y)
{
	return x - floor(x / y) * y;
}

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

void memzero(void *buf, size_t size)
{
	uint8_t *b = buf;
	uint8_t *e = b + size;

	while (b != e)
		*b++ = '\0';
}

#ifdef DTK
static void dtk_decode_buffer(void *address, uint32_t length)
{
	sample_t stream[448] __attribute((aligned(32)));

	adpcm_decode(&dtk.adpcm, stream, *dtk.buffer, 448);
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

		adpcm_reset(&dtk.adpcm);
	}
}

bool dtk_fill_buffer(void)
{
	if (!dtk.playing)
		return false;
	if (fifo_space() < 448 * sizeof(sample_t))
		return false;

	#ifdef ASYNC_READ
	void read_callback(void *address, uint32_t length)
	{
		dtk_decode_buffer(address, length);
		dtk_fill_buffer();
	}

	#ifndef ISR
	DCInvalidateRange(__builtin_assume_aligned(dtk.buffer, 32), sizeof(*dtk.buffer));
	#endif
	read_disc_frag(__builtin_assume_aligned(dtk.buffer, 32), sizeof(*dtk.buffer), dtk.current.position, read_callback);
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

static void di_update_interrupts(void)
{
	if ((di.reg.sr  >> 1) & (di.reg.sr  & 0b0101010) ||
		(di.reg.cvr >> 1) & (di.reg.cvr & 0b010))
		irq.status |=  OS_INTERRUPTMASK_PI_DI;
	else
		irq.status &= ~OS_INTERRUPTMASK_PI_DI;

	if (irq.status & irq.mask)
		*(volatile int *)OS_BASE_MIRRORED;
}

static void di_error(uint32_t error)
{
	di.error = error;

	di.reg.sr |=  0b0000100;
	di.reg.cr &= ~0b001;
	di_update_interrupts();
}

void di_complete_transfer(void)
{
	if (di.reg.cr & 0b010) {
		di.reg.mar += di.reg.length;
		di.reg.length = 0;
	}

	di.reg.sr |=  0b0010000;
	di.reg.cr &= ~0b001;
	di_update_interrupts();
}

static void di_close_cover(void)
{
	di.reg.cvr &= ~0b001;
	di.reg.cvr |=  0b100;
	di_update_interrupts();
}

#ifndef DI_PASSTHROUGH
static void di_execute_command(OSAlarm *alarm)
{
	uint32_t result = 0;

	switch (di.reg.cmdbuf0 >> 24) {
		case 0xA8:
		{
			uint32_t address = di.reg.mar;
			uint32_t length  = di.reg.length;
			uint32_t offset  = di.reg.cmdbuf1 << 2 & ~0x80000000;

			if (!(di.reg.cvr & 0b001))
				perform_read(address, length, offset);
			else
				di_error(0x01023A00);
			return;
		}
		case 0xAB:
		{
			uint32_t offset = di.reg.cmdbuf1 << 2 & ~0x80000000;

			if (!(di.reg.cvr & 0b001))
				perform_read(0, 0, offset);
			else
				di_error(0x01023A00);
			return;
		}
		case 0xE0:
		{
			result = di.error;
			di.error = 0;
			break;
		}
		case 0xE1:
		{
			switch ((di.reg.cmdbuf0 >> 16) & 0x03) {
				case 0x00:
				{
					uint32_t offset = DVDRoundDown32KB(di.reg.cmdbuf1 << 2) & ~0x80000000;
					uint32_t length = DVDRoundDown32KB(di.reg.cmdbuf2);

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
					adpcm_reset(&dtk.adpcm);
					#endif
					break;
				}
			}
			break;
		}
		case 0xE2:
		{
			switch ((di.reg.cmdbuf0 >> 16) & 0x03) {
				case 0x00:
				{
					result = dtk.playing;
					break;
				}
				case 0x01:
				{
					result = DVDRoundDown32KB(dtk.current.position & ~0x80000000) >> 2;
					break;
				}
				case 0x02:
				{
					result = DVDRoundDown32KB(dtk.current.start & ~0x80000000) >> 2;
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
			if (!(di.reg.cvr & 0b001) && change_disc()) {
				di.reg.cvr |= 0b001;
				OSSetAlarm(&command_alarm, OSSecondsToTicks(1.5), (OSAlarmHandler)di_close_cover);
			}
			break;
		}
	}

	di.reg.immbuf = result;
	di_complete_transfer();
}
#else
static void di_execute_command(OSAlarm *alarm)
{
	#ifdef DVD
	if (di.reset)
		return;
	#endif

	switch (di.reg.cmdbuf0 >> 24) {
		case 0xA8:
		{
			uint32_t address = di.reg.mar;
			uint32_t length  = di.reg.length;
			uint32_t offset  = di.reg.cmdbuf1 << 2;

			#ifdef GCODE
			if (*VAR_EMU_READ_SPEED && !alarm) {
				di_defer_transfer(offset, length);
				return;
			}
			#endif

			switch (di.reg.cmdbuf0 & 0xC0) {
				default:
				{
					DVDDiskID *id1 = (DVDDiskID *)VAR_AREA;
					DVDDiskID *id2 = (DVDDiskID *)VAR_DISC_1_ID;

					if (!memeq(id1, id2, sizeof(DVDDiskID)))
						break;

					perform_read(address, length, offset);
					return;
				}
				case 0x40:
				{
					DVDDiskID *id = (DVDDiskID *)VAR_AREA;
					DCBlockInvalidate(id);
					break;
				}
			}
			break;
		}
		#ifdef GCODE
		case 0xAB:
		{
			uint32_t offset = di.reg.cmdbuf1 << 2;

			if (*VAR_EMU_READ_SPEED && !alarm) {
				di_defer_transfer(offset, 0);
				return;
			}
			break;
		}
		#endif
	}

	DI[2] = di.reg.cmdbuf0;
	DI[3] = di.reg.cmdbuf1;
	DI[4] = di.reg.cmdbuf2;
	DI[5] = di.reg.mar;
	DI[6] = di.reg.length;
	DI[7] = di.reg.cr;
	DI[8] = di.reg.immbuf;
}
#endif

#define DVD_SECTOR_SIZE    2048
#define DVD_ECC_BLOCK_SIZE (16 * DVD_SECTOR_SIZE)

#define BUFFER_TRANSFER_RATE (13.5 * 1000 * 1000)

static struct {
	union {
		struct {
			uint32_t start_offset;
			uint32_t end_offset;
		};

		struct {
			uint32_t start_block        : 17;
			uint32_t start_block_offset : 15;
			uint32_t end_block          : 17;
			uint32_t end_block_offset   : 15;
		};
	};

	OSTime start_time;
	OSTime end_time;
} read_buffer = {0};

static uint32_t dvd_buffer_size(void)
{
	DVDDiskID *id = (DVDDiskID *)VAR_AREA;
	return (*VAR_EMU_READ_SPEED == 1 ? id->streaming ? 5 : 15 : 32) * DVD_ECC_BLOCK_SIZE;
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
		OSTime current_time = OSGetTime();

		uint32_t head_position;

		uint32_t buffer_start, buffer_end;
		uint32_t buffer_size = dvd_buffer_size();

		uint32_t dvd_offset = DVDRoundDown32KB(offset);

		if (read_buffer.start_time == read_buffer.end_time) {
			buffer_start = buffer_end = head_position = 0;
		} else {
			if (current_time >= read_buffer.end_time) {
				head_position = read_buffer.end_offset;
			} else {
				head_position = read_buffer.start_offset +
					OSDiffTick(current_time, read_buffer.start_time) *
					(read_buffer.end_block - read_buffer.start_block) /
					OSDiffTick(read_buffer.end_time, read_buffer.start_time) *
					DVD_ECC_BLOCK_SIZE;
			}

			buffer_start = read_buffer.end_offset >= buffer_size ? read_buffer.end_offset - buffer_size : 0;

			if (dvd_offset < buffer_start)
				buffer_start = buffer_end = 0;
			else
				buffer_end = head_position;
		}

		OSTick ticks_until_execution = 0;
		OSTick ticks_until_completion = READ_COMMAND_LATENCY;

		do {
			uint32_t chunk_length = MIN(length, DVD_ECC_BLOCK_SIZE - offset % DVD_ECC_BLOCK_SIZE);

			if (dvd_offset >= buffer_start && dvd_offset < buffer_end) {
				ticks_until_completion += READ_COMMAND_LATENCY;
				ticks_until_completion += OSSecondsToTicks(chunk_length / BUFFER_TRANSFER_RATE);
			} else {
				if (dvd_offset != head_position) {
					ticks_until_completion += OSSecondsToTicks(CalculateSeekTime(head_position, dvd_offset));
					ticks_until_completion += OSSecondsToTicks(CalculateRotationalLatency(dvd_offset,
						OSTicksToSeconds((double)(current_time + ticks_until_completion))));
				} else {
					ticks_until_completion += OSSecondsToTicks(CalculateRawDiscReadTime(dvd_offset, DVD_ECC_BLOCK_SIZE));
				}

				ticks_until_execution = ticks_until_completion;
				head_position = dvd_offset + DVD_ECC_BLOCK_SIZE;
			}

			offset += chunk_length;
			length -= chunk_length;
			dvd_offset += DVD_ECC_BLOCK_SIZE;
		} while (length);

		if (dvd_offset != buffer_start + DVD_ECC_BLOCK_SIZE || buffer_start == buffer_end) {
			read_buffer.start_offset = dvd_offset >= buffer_end ? dvd_offset : buffer_end;
			read_buffer.end_offset = dvd_offset + buffer_size - DVD_ECC_BLOCK_SIZE;

			read_buffer.start_time = current_time + ticks_until_completion;
			read_buffer.end_time = read_buffer.start_time +
				(OSTick)OSSecondsToTicks(CalculateRawDiscReadTime(read_buffer.start_offset,
				read_buffer.end_offset - read_buffer.start_offset));
		}

		OSSetAlarm(&command_alarm, ticks_until_execution, (OSAlarmHandler)di_execute_command);
	}

	di_defer_transfer(offset, length);

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);
}

static void di_read(unsigned index, uint32_t *value)
{
	#ifndef DI_PASSTHROUGH
	*value = di.regs[index];
	#else
	switch (index) {
		case 0 ... 1:
			*value = di.regs[index] | DI[index];
			break;
		case 2 ... 4:
		case 7 ... 8:
			*value = di.regs[index];
			break;
		case 5 ... 6:
			if ((DI[7] & 0b011) == 0b011)
				*value = di.regs[index] = DI[index];
			else
				*value = di.regs[index];
			break;
		default:
			*value = DI[index];
	}
	#endif
}

static void di_write(unsigned index, uint32_t value)
{
	switch (index) {
		case 0:
			#ifdef DI_PASSTHROUGH
			DI[0] = value & ~(di.reg.sr & 0b1010100);
			#endif
			di.reg.sr = ((value & 0b1010100) ^ di.reg.sr) & di.reg.sr;
			di.reg.sr = (value & 0b0101011) | (di.reg.sr & ~0b0101010);
			di_update_interrupts();
			break;
		case 1:
			#ifdef DI_PASSTHROUGH
			DI[1] = value & ~(di.reg.cvr & 0b100);
			#endif
			di.reg.cvr = ((value & 0b100) ^ di.reg.cvr) & di.reg.cvr;
			di.reg.cvr = (value & 0b010) | (di.reg.cvr & ~0b010);
			di_update_interrupts();
			break;
		case 2 ... 4:
		case 8:
			di.regs[index] = value;
			break;
		case 5 ... 6:
			di.regs[index] = value & 0x3FFFFE0;
			break;
		case 7:
			di.regs[index] = value & 0b111;

			if (value & 0b001)
				di_execute_command(NULL);
			break;
	}
}

#ifdef DTK
static struct {
	union {
		uint16_t regs[4];
		struct {
			uint32_t aima;
			uint32_t aibl;
		} reg;
	};

	struct {
		int aima;
	} req;

	uint8_t (*buffer[2])[640];
} dsp = {0};

static void dsp_read(unsigned index, uint32_t *value)
{
	switch (index) {
		case 24:
		case 25:
			*value = ((uint16_t *)DSP)[index];
			dsp.req.aima--;
			break;
		default:
			*value = ((uint16_t *)DSP)[index];
	}
}

static void dsp_write(unsigned index, uint16_t value)
{
	switch (index) {
		case 24:
			dsp.regs[index - 24] = value & 0x3FF;
			dsp.req.aima++;
			break;
		case 25:
			dsp.regs[index - 24] = value & 0xFFE0;
			dsp.req.aima++;
			break;
		case 27:
			dsp.regs[index - 24] = value;

			if ((value & 0x8000) && dsp.req.aima >= 0) {
				void *buffer = OSPhysicalToUncached(dsp.reg.aima);
				int length = (dsp.reg.aibl & 0x7FFF) << 5;
				int count = length / sizeof(sample_t);

				if (length <= sizeof(**dsp.buffer)) {
					void *buffer2 = dsp.buffer[0];
					dsp.buffer[0] = dsp.buffer[1];
					dsp.buffer[1] = buffer2;
					buffer = memcpy(buffer2, buffer, length);
				}

				uint32_t aicr = AI[0];
				uint32_t aivr = AI[1];

				if (aicr & 0b0000001) {
					if (aicr & 0b1000000) {
						sample_t stream[count * 3 / 2] __attribute((aligned(32)));

						if (fifo_size() >= sizeof(stream)) {
							fifo_read(stream, sizeof(stream));
							mix_samples(buffer, stream, true, count, aivr, aivr >> 8);
						}
					} else {
						sample_t stream[count] __attribute((aligned(32)));

						if (fifo_size() >= sizeof(stream)) {
							fifo_read(stream, sizeof(stream));
							mix_samples(buffer, stream, false, count, aivr, aivr >> 8);
						}
					}
				}

				DSP[12] = (intptr_t)buffer;
				DSP[13] = ((length >> 5) & 0x7FFF) | 0x8000;

				if (fifo_size() < (count * 3 / 2) * sizeof(sample_t))
					dtk_fill_buffer();
			} else {
				DSP[12] = dsp.reg.aima;
				DSP[13] = dsp.reg.aibl;
			}

			dsp.req.aima = 0;
			break;
	}
}
#endif

#ifdef DVD
static void di_reset(void)
{
	DI[0] = DI[0];
	DI[1] = DI[1];

	switch (di.reset++) {
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
			di.reset = 0;

			if (di.reg.cr & 0b001)
				di_execute_command(NULL);
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

				if (!di.reset && !(value & 0b100))
					di_reset();
				break;
			}
			#endif
		default:
			PI[index] = value;
	}
}

static void efb_read(uint32_t address, uint32_t *value)
{
	uint16_t zmode = PE[0];
	PE[0] = 0;
	*(volatile uint32_t *)OSPhysicalToUncached(address) = 0xFFFFFF;
	*value = *(uint32_t *)OSPhysicalToUncached(address);
	PE[0] = zmode;
}

static bool ppc_load32(uint32_t address, uint32_t *value)
{
	if ((address & ~0x3FFFFC) == 0x08400000) {
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

void external_interrupt_vector(void);

static void write_branch(void *a, void *b)
{
	*(uint32_t *)a = (uint32_t)(b - (OS_BASE_CACHED - 0x48000002));
	asm volatile("dcbst 0,%0; sync; icbi 0,%0" :: "r" (a));
}

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
				write_branch(load_context_end, handler);
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

	handler = irq.handler[interrupt - OS_INTERRUPT_PI_DI];
	if (handler) handler(interrupt, &exceptionContext);

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);
}

static void mem_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	if (irq.status & irq.mask) {
		dispatch_interrupt(__builtin_clz(irq.status & irq.mask), context);
		return;
	}

	MI[16] = 0;
}

#ifdef DI_PASSTHROUGH
static void di_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	#ifdef DVD
	if (di.reset) {
		di_reset();
		return;
	}
	#endif

	if (DI[0] & 0b1010100) {
		di.reg.cr = DI[7];

		if (di.reg.cr & 0b010) {
			di.reg.mar    = DI[5];
			di.reg.length = DI[6];
		} else {
			di.reg.immbuf = DI[8];
		}
	}

	dispatch_interrupt(interrupt, context);
}
#endif

void *init(void *arenaLo)
{
	#ifdef BBA
	OSCreateAlarm(&bba_alarm);
	#endif
	OSCreateAlarm(&read_alarm);
	OSCreateAlarm(&command_alarm);

	memzero(irq.handler, sizeof(irq.handler));
	irq.mask = 0;

	OSSetExceptionHandler(OS_EXCEPTION_DSI, exception_handler);
	#ifdef ISR
	write_branch((void *)0x80000500, external_interrupt_vector);
	#endif

	#ifdef DTK
	dsp.buffer[0] = OSCachedToUncached(arenaLo); arenaLo += sizeof(*dsp.buffer[0]);
	dsp.buffer[1] = OSCachedToUncached(arenaLo); arenaLo += sizeof(*dsp.buffer[1]);
	#endif
	return arenaLo;
}

bool exi_probe(int32_t chan)
{
	if (chan == EXI_CHANNEL_2)
		return false;
	if (chan == *VAR_EXI_SLOT)
		return false;

	return true;
}

bool exi_try_lock(int32_t chan, uint32_t dev, EXIControl *exi)
{
	if (!(exi->state & EXI_STATE_LOCKED) || exi->dev != dev)
		return false;
	#ifdef BBA
	if (chan == EXI_CHANNEL_0 && dev == EXI_DEVICE_2)
		return false;
	#endif
	if (chan == *VAR_EXI_SLOT && dev == EXI_DEVICE_0)
		return false;
	if (chan == *VAR_EXI_SLOT)
		end_read();

	return true;
}

OSInterruptHandler set_irq_handler(OSInterrupt interrupt, OSInterruptHandler handler)
{
	if (interrupt == OS_INTERRUPT_PI_DI) {
		#if defined WKF || defined DI_PASSTHROUGH
		OSSetInterruptHandler(OS_INTERRUPT_PI_DI, di_interrupt_handler);
		#endif
		OSSetInterruptHandler(OS_INTERRUPT_MEM_ADDRESS, mem_interrupt_handler);
		#ifdef BBA
		OSSetInterruptHandler(OS_INTERRUPT_EXI_2_EXI, exi_interrupt_handler);
		#endif
	} else {
		OSSetInterruptHandler(interrupt, dispatch_interrupt);
	}

	OSInterruptHandler oldHandler = irq.handler[interrupt - OS_INTERRUPT_PI_DI];
	irq.handler[interrupt - OS_INTERRUPT_PI_DI] = handler;
	return oldHandler;
}

OSInterruptMask mask_irq(OSInterruptMask mask)
{
	irq.mask &= ~mask;

	#ifndef DI_PASSTHROUGH
	if (mask == OS_INTERRUPTMASK_PI_DI)
		mask &= ~OS_INTERRUPTMASK_PI_DI;
	#endif

	return OSMaskInterrupts(mask);
}

OSInterruptMask unmask_irq(OSInterruptMask mask)
{
	irq.mask |= mask;

	if (mask == OS_INTERRUPTMASK_PI_DI) {
		#ifndef DI_PASSTHROUGH
		mask &= ~OS_INTERRUPTMASK_PI_DI;
		#endif
		mask |=  OS_INTERRUPTMASK_MEM_ADDRESS;
		#ifdef BBA
		mask |=  OS_INTERRUPTMASK_EXI_2_EXI;
		#endif
	}

	return OSUnmaskInterrupts(mask);
}

void idle_thread(void)
{
	disable_interrupts();
	trickle_read();
	#ifdef BBA
	OSSetInterruptHandler(OS_INTERRUPT_EXI_2_EXI, exi_interrupt_handler);
	OSUnmaskInterrupts(OS_INTERRUPTMASK_EXI_2_EXI);
	#endif
	enable_interrupts();
}
