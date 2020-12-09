/* 
 * Copyright (c) 2020, Extrems <extrems@extremscorner.org>
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

#include "dolphin/dolformat.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "pff.h"

vu32(*EXIRegs)[5] = (vu32(*)[])0x0C006800;

void dly_us(UINT n)
{
	OSTick start = OSGetTick();
	while (OSDiffTick(OSGetTick(), start) < OSMicrosecondsToTicks(n));
}

static FATFS fs;

static void load_dol(const char *path)
{
	DOLImage image;
	UINT br;

	if (pf_open(path) != FR_OK) return;
	if (pf_read(&image, sizeof(image), &br) != FR_OK || br != sizeof(image)) return;

	for (int i = 0; i < DOL_MAX_TEXT; i++)
		image.text[i] = (void *)OSCachedToPhysical(image.text[i]);
	for (int i = 0; i < DOL_MAX_DATA; i++)
		image.data[i] = (void *)OSCachedToPhysical(image.data[i]);

	image.bss   = (void *)OSCachedToPhysical(image.bss);
	image.entry = (void *)OSCachedToPhysical(image.entry);

	for (int i = 0; i < DOL_MAX_TEXT; i++) {
		if (pf_lseek(image.textData[i]) != FR_OK) return;
		if (pf_read(image.text[i], image.textLen[i], &br) != FR_OK ||
			br != image.textLen[i]) return;

		DCFlushRangeNoSync(image.text[i], image.textLen[i]);
		asm volatile("sync");
		ICInvalidateRange(image.text[i], image.textLen[i]);
	}

	for (int i = 0; i < DOL_MAX_DATA; i++) {
		if (pf_lseek(image.dataData[i]) != FR_OK) return;
		if (pf_read(image.data[i], image.dataLen[i], &br) != FR_OK ||
			br != image.dataLen[i]) return;

		DCFlushRangeNoSync(image.data[i], image.dataLen[i]);
		asm volatile("sync");
	}

	image.entry();
}

void _main(void)
{
	for (int chan = 0; chan < EXI_CHANNEL_MAX; chan++, EXIRegs++) {
		if (pf_mount(&fs) == FR_OK) {
			load_dol("/AUTOEXEC.DOL");
			load_dol("/BOOT.DOL");
			load_dol("/BOOT2.DOL");
			load_dol("/IGR.DOL");
			load_dol("/IPL.DOL");
		}
	}
}
