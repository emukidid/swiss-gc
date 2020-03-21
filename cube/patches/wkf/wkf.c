/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../alt/timer.h"
#include "../base/common.h"
#include "../base/exi.h"
#include "../base/os.h"

void schedule_read(void *address, uint32_t length, uint32_t offset, OSTick ticks);

void do_read_disc(void *address, uint32_t length, uint32_t offset, uint32_t sector)
{
	DI[2] = 0xDE000000;
	DI[3] = sector;
	DI[4] = 0x5A000000;
	DI[7] = 0b001;
	while (DI[7] & 0b001);

	DI[0] = 0b0011000;
	DI[1] = 0;
	DI[2] = 0xA8000000;
	DI[3] = offset >> 2;
	DI[4] = length;
	DI[5] = (uint32_t)address;
	DI[6] = length;
	DI[7] = 0b011;

	OSUnmaskInterrupts(OS_INTERRUPTMASK_PI_DI);
}

void dvd_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	OSMaskInterrupts(OS_INTERRUPTMASK_PI_DI);

	DI[0] = 0;
	DI[1] = 0;

	switch (DI[2] >> 24) {
		case 0xA8:
		{
			uint32_t position  = *(uint32_t *)VAR_LAST_OFFSET;
			uint32_t remainder = *(uint32_t *)VAR_TMP2;
			uint8_t *data      = *(uint8_t **)VAR_TMP1;
			uint32_t data_size = DI[4] - DI[6];

			position  += data_size;
			remainder -= data_size;

			schedule_read(data + data_size, remainder, position, 0);
			break;
		}
	}
}

bool exi_probe(int32_t chan)
{
	if (chan == EXI_CHANNEL_2)
		return false;
	if (chan == *VAR_EXI_SLOT)
		return false;
	return true;
}

bool exi_trylock(int32_t chan, uint32_t dev, EXIControl *exi)
{
	if (!(exi->state & EXI_STATE_LOCKED) || exi->dev != dev)
		return false;
	if (chan == *VAR_EXI_SLOT && dev == EXI_DEVICE_0)
		return false;
	return true;
}

void di_update_interrupts(void);
void di_complete_transfer(void);

void schedule_read(void *address, uint32_t length, uint32_t offset, OSTick ticks)
{
	*(uint32_t *)VAR_LAST_OFFSET = offset;
	*(uint32_t *)VAR_TMP2 = length;
	*(uint8_t **)VAR_TMP1 = address;

	if (length) {
		if (!is_frag_read(offset, length))
			read_disc_frag(address, length, offset);
		else
			timer1_start(ticks);
		return;
	}

	di_complete_transfer();
}

void perform_read(uint32_t address, uint32_t length, uint32_t offset)
{
	schedule_read(OSPhysicalToUncached(address), length, offset, 0);
}

void trickle_read(void)
{
	uint32_t position  = *(uint32_t *)VAR_LAST_OFFSET;
	uint32_t remainder = *(uint32_t *)VAR_TMP2;
	uint8_t *data      = *(uint8_t **)VAR_TMP1;
	uint32_t data_size;

	if (remainder) {
		if (is_frag_read(position, remainder)) {
			OSTick start = OSGetTick();
			data_size = read_frag(data, remainder, position);
			OSTick end = OSGetTick();

			position  += data_size;
			remainder -= data_size;

			schedule_read(data + data_size, remainder, position, OSDiffTick(end, start));
		}
	}
}

void device_reset(void)
{
	end_read();
}

void change_disc(void)
{
	*(uint32_t *)VAR_CURRENT_DISC = !*(uint32_t *)VAR_CURRENT_DISC;

	(*DI_EMU)[1] &= ~0b001;
	(*DI_EMU)[1] |=  0b100;
	di_update_interrupts();
}
