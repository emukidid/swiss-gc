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
#include "common.h"
#include "dolphin/dolformat.h"
#include "dolphin/os.h"
#include "dolphin/pad.h"

void check_pad(int32_t chan, PADStatus *status)
{
	status->button &= ~PAD_USE_ORIGIN;

	if ((status->button & PAD_COMBO_EXIT) == PAD_COMBO_EXIT) {
		enable_interrupts();
		if (OSResetSystem) OSResetSystem(OS_RESET_HOTRESET, 0, 0);
		disable_interrupts();
	}
}

static void load_dol(uint32_t offset, uint32_t size)
{
	DOLImage image;
	device_frag_read(&image, sizeof(image), offset);

	for (int i = 0; i < DOL_MAX_TEXT; i++) {
		device_frag_read(image.text[i], image.textLen[i], offset + image.textData[i]);
		dcache_flush_icache_inv(image.text[i], image.textLen[i]);
	}

	for (int i = 0; i < DOL_MAX_DATA; i++) {
		device_frag_read(image.data[i], image.dataLen[i], offset + image.dataData[i]);
		dcache_flush_icache_inv(image.data[i], image.dataLen[i]);
	}

	end_read();
	image.entry();
}

static void clear_jump_table(void)
{
	DCBlockZero(VAR_JUMP_TABLE);
}

void fini(void)
{
	disable_interrupts();
	clear_jump_table();
	device_reset();

	uint8_t igr_exit_type = *(uint8_t *)VAR_IGR_EXIT_TYPE;
	uint32_t igr_dol_size = *(uint32_t *)VAR_IGR_DOL_SIZE;

	switch (igr_exit_type) {
		case IGR_BOOTBIN:
			if (igr_dol_size)
				switch_fiber(0xE0000000, igr_dol_size, 0, 0, (intptr_t)load_dol, 0x81800000);
			break;
	}
}
