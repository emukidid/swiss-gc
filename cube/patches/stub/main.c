/* 
 * Copyright (c) 2020-2025, Extrems <extrems@extremscorner.org>
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
#include "dolphin/dolformat.h"
#include "dolphin/dvd.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "eltorito.h"
#include "pff.h"

volatile uint32_t(*EXIRegs)[5] = EXI;
uint16_t EXICsb = 010141;

void run(void (*entry)(void));

void dly_us(UINT n)
{
	OSTick start = OSGetTick();
	while (OSDiffTick(OSGetTick(), start) < OSMicrosecondsToTicks(n));
}

static bool memeq(const void *a, const void *b, size_t size)
{
	const uint8_t *x = a;
	const uint8_t *y = b;
	size_t i;

	for (i = 0; i < size; ++i)
		if (x[i] != y[i])
			return false;

	return true;
}

void dvd_reset(void);
uint32_t dvd_inquiry(DVDDriveInfo *info);
uint32_t dvd_read_id(DVDDiskID *id);
uint32_t dvd_read(void *address, uint32_t length, uint32_t offset);
void dvd_stop_motor(void);
void flippy_reset(void);

static void dvd_load(uint32_t offset)
{
	DOLImage image __attribute((aligned(32)));

	if (dvd_read(&image, sizeof(image), offset) != sizeof(image)) return;

	for (int i = 0; i < DOL_MAX_TEXT; i++)
		image.text[i] = (void *)OSCachedToPhysical(image.text[i]);
	for (int i = 0; i < DOL_MAX_DATA; i++)
		image.data[i] = (void *)OSCachedToPhysical(image.data[i]);

	image.bss   = (void *)OSCachedToPhysical(image.bss);
	image.entry = (void *)OSCachedToPhysical(image.entry);

	for (int i = 0; i < DOL_MAX_TEXT; i++) {
		if (dvd_read(image.text[i], image.textLen[i],
			offset + image.textData[i]) < image.textLen[i]) return;
		ICInvalidateRange(image.text[i], image.textLen[i]);
	}

	for (int i = 0; i < DOL_MAX_DATA; i++) {
		if (dvd_read(image.data[i], image.dataLen[i],
			offset + image.dataData[i]) < image.dataLen[i]) return;
	}

	run(image.entry);
}

void dvd_main(void)
{
	DVDDriveInfo info __attribute((aligned(32)));
	if (!dvd_inquiry(&info)) return;

	switch (info.releaseDate) {
		case 0x20196C64:
		{
			dvd_stop_motor();
			dvd_reset();
			dly_us(1150 * 1000);
			dvd_reset();
			if (!dvd_read_id(NULL)) return;
			break;
		}
		case 0x20220426:
			flippy_reset();
		case 0x20220420:
		{
			DVDBB2 BB2 __attribute((aligned(32)));
			if (dvd_read(&BB2, sizeof(BB2), 0x420) != sizeof(BB2)) return;
			if (BB2.bootFilePosition) dvd_load(BB2.bootFilePosition);
			break;
		}
	}

	uint32_t offset = 17 * DI_SECTOR_SIZE;

	struct di_boot_record boot_record __attribute((aligned(32)));
	if (dvd_read(&boot_record, sizeof(boot_record), offset) != sizeof(boot_record)) return;
	if (!memeq(boot_record.boot_system_id, "EL TORITO SPECIFICATION", 23)) return;
	offset = boot_record.boot_catalog_offset * DI_SECTOR_SIZE;

	struct di_validation_entry validation_entry __attribute((aligned(32)));
	if (dvd_read(&validation_entry, sizeof(validation_entry), offset) != sizeof(validation_entry)) return;
	if (validation_entry.header_id != 1 || validation_entry.key_55 != 0x55 || validation_entry.key_AA != 0xAA) return;
	offset += sizeof(validation_entry);

	struct di_default_entry default_entry __attribute((aligned(32)));
	if (dvd_read(&default_entry, sizeof(default_entry), offset) != sizeof(default_entry)) return;
	if (default_entry.boot_indicator != 0x88) return;
	offset = default_entry.load_rba * DI_SECTOR_SIZE;

	dvd_load(offset);
}

static void pf_load(const char *path)
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

	run(image.entry);
}

void pf_main(void)
{
	FATFS fs;
	EXIRegs = EXI;
	EXICsb = 010141;

	for (int i = 0; i < 6; i++) {
		if (pf_mount(&fs) == FR_OK) {
			pf_load("/AUTOEXEC.DOL");
			pf_load("/BOOT.DOL");
			pf_load("/BOOT2.DOL");
			pf_load("/IPL.DOL");
		}

		EXICsb >>= 3;
		if (EXICsb & (1 << EXI_DEVICE_0))
			EXIRegs++;
	}
}
