/* deviceHandler-SYS.c
        - virtual device interface for dumping ROMs
        by novenary
 */

#include <stdlib.h>
#include <malloc.h>
#include "deviceHandler-SYS.h"
#include "swiss.h"
#include "main.h"
#include "dvd.h"
#include "files.h"
#include "gui/FrameBufferMagic.h"

s32 read_rom_ipl(file_handle* file, void* buffer, u32 length);
s32 read_rom_ipl_clear(file_handle* file, void* buffer, u32 length);
s32 read_rom_sram(file_handle* file, void* buffer, u32 length);
s32 write_rom_sram(file_handle* file, const void* buffer, u32 length);
s32 read_rom_dsp_rom(file_handle* file, void* buffer, u32 length);
s32 read_rom_dsp_coef(file_handle* file, void* buffer, u32 length);
s32 read_rom_dvd_ram_low(file_handle* file, void* buffer, u32 length);
s32 read_rom_dvd_rom(file_handle* file, void* buffer, u32 length);
s32 read_rom_dvd_ram_high(file_handle* file, void* buffer, u32 length);
s32 read_rom_dvd_disk_bca(file_handle* file, void* buffer, u32 length);
s32 read_rom_dvd_disk_pfi(file_handle* file, void* buffer, u32 length);
s32 read_rom_dvd_disk_dmi(file_handle* file, void* buffer, u32 length);
s32 read_rom_aram(file_handle* file, void* buffer, u32 length);
s32 write_rom_aram(file_handle* file, const void* buffer, u32 length);
s32 read_rom_aram_expansion(file_handle* file, void* buffer, u32 length);
s32 write_rom_aram_expansion(file_handle* file, const void* buffer, u32 length);
s32 read_rom_void(file_handle* file, void* buffer, u32 length);
s32 write_rom_void(file_handle* file, const void* buffer, u32 length);

enum rom_types
{
	ROM_VOID = 0,
	ROM_IPL,
	ROM_IPL_CLEAR,
	ROM_DSP_ROM,
	ROM_DSP_COEF,
	ROM_DVD_RAM_LOW,
	ROM_DVD_ROM,
	ROM_DVD_RAM_HIGH,
	ROM_DVD_DISK_BCA,
	ROM_DVD_DISK_PFI,
	ROM_DVD_DISK_DMI,
	ROM_SRAM,
	ROM_ARAM,
	ROM_ARAM_INTERNAL,
	ROM_ARAM_EXPANSION,
	NUM_ROMS
};

static char* rom_names[] =
{
	NULL,
	"/ipl.bin",
	"/ipl_clear.bin",
	"/dsp_rom.bin",
	"/dsp_coef.bin",
	"/dvd_ram_low.bin",
	"/dvd_rom.bin",
	"/dvd_ram_high.bin",
	"/dvd_disk_bca.bin",
	"/dvd_disk_pfi.bin",
	"/dvd_disk_dmi.bin",
	"/sram.bin",
	"/aram.bin",
	"/aram_internal.bin",
	"/aram_expansion.bin"
};

static int rom_sizes[] =
{
	0,
	2 * 1024 * 1024,
	2 * 1024 * 1024,
	8 * 1024,
	4 * 1024,
	32 * 1024,
	128 * 1024,
	192 * 1024,
	64,
	2048,
	2048,
	64,
	16 * 1024 * 1024,
	16 * 1024 * 1024,
	0
};

static s32 (*read_rom[])(file_handle* file, void* buffer, u32 length) =
{
	read_rom_void,
	read_rom_ipl,
	read_rom_ipl_clear,
	read_rom_dsp_rom,
	read_rom_dsp_coef,
	read_rom_dvd_ram_low,
	read_rom_dvd_rom,
	read_rom_dvd_ram_high,
	read_rom_dvd_disk_bca,
	read_rom_dvd_disk_pfi,
	read_rom_dvd_disk_dmi,
	read_rom_sram,
	read_rom_aram,
	read_rom_aram,
	read_rom_aram_expansion
};

static s32 (*write_rom[])(file_handle* file, const void* buffer, u32 length) =
{
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_void,
	write_rom_sram,
	write_rom_aram,
	write_rom_aram,
	write_rom_aram_expansion
};

file_handle initial_SYS =
{
	"sys:/",
	0ULL,
	0,
	0,
	IS_DIR,
	0,
	0,
	NULL
};

device_info initial_SYS_info =
{
	0LL,
	0LL,
	false
};

device_info* deviceHandler_SYS_info(file_handle* file) {
	return &initial_SYS_info;
}

static void descrambler(unsigned int offset, void* buffer, unsigned int length) {
	// bootrom descrambler reversed by segher
	// Copyright 2008 Segher Boessenkool <segher@kernel.crashing.org>

	unsigned char* data = buffer;
	unsigned int descrambled_length = 0x100;

	unsigned char acc = 0;
	unsigned char nacc = 0;

	unsigned short t = 0x2953;
	unsigned short u = 0xd9c2;
	unsigned short v = 0x3ff1;

	unsigned char x = 1;
	unsigned int it = offset < 0x100 ? 0x100 - offset : 0;

	while (it < length) {
		int t0 = t & 1;
		int t1 = (t >> 1) & 1;
		int u0 = u & 1;
		int u1 = (u >> 1) & 1;
		int v0 = v & 1;

		x ^= t1 ^ v0;
		x ^= (u0 | u1);
		x ^= (t0 ^ u1 ^ v0) & (t0 ^ u0);

		if (t0 == u0) {
			v >>= 1;
			if (v0)
				v ^= 0xb3d0;
		}

		if (t0 == 0) {
			u >>= 1;
			if (u0)
				u ^= 0xfb10;
		}

		t >>= 1;
		if (t0)
			t ^= 0xa740;

		nacc++;
		acc = 2*acc + x;
		if (nacc == 8) {
			if (descrambled_length >= 0x1aff00)
				it++;
			else if (descrambled_length >= offset)
				data[it++] ^= acc;
			descrambled_length++;
			nacc = 0;
		}
	}
}

bool load_rom_ipl(DEVICEHANDLER_INTERFACE* device, void** buffer, u32* length) {
	file_handle* file = calloc(1, sizeof(file_handle));
	concat_path(file->name, device->initial->name, "swiss/patches/ipl.bin");

	BS2Header bs2Header;
	device->seekFile(file, 0x800, DEVICE_HANDLER_SEEK_SET);
	if(device->readFile(file, &bs2Header, sizeof(BS2Header)) == sizeof(BS2Header)) {
		descrambler(0x800, &bs2Header, sizeof(BS2Header));

		if(in_range(bs2Header.bss, 0x81300000, 0x814AF6E0)) {
			u32 sizeToRead = (bs2Header.bss - 0x812FFFE0 + 31) & ~31;
			void* bs2Image = memalign(32, sizeToRead);

			if(bs2Image) {
				device->seekFile(file, 0x800, DEVICE_HANDLER_SEEK_SET);
				if(device->readFile(file, bs2Image, sizeToRead) == sizeToRead) {
					device->closeFile(file);
					free(file);

					descrambler(0x800, bs2Image, sizeToRead);
					*buffer = bs2Image;
					*length = bs2Header.bss - 0x812FFFE0;
					return true;
				}
				free(bs2Image);
			}
		}
	}
	device->closeFile(file);
	free(file);
	return false;
}

s32 read_rom_ipl(file_handle* file, void* buffer, u32 length) {
	__SYS_ReadROM(buffer, length, file->offset);
	return length;
}

s32 read_rom_ipl_clear(file_handle* file, void* buffer, u32 length) {
	s32 ret = read_rom_ipl(file, buffer, length);
	descrambler(file->offset, buffer, length);
	return ret;
}

s32 read_rom_sram(file_handle* file, void* buffer, u32 length) {
	syssram* sram = __SYS_LockSram();
	if(!sram) return 0;

	memcpy(buffer, (const void*) sram + file->offset, length);

	__SYS_UnlockSram(FALSE);
	return length;
}

s32 write_rom_sram(file_handle* file, const void* buffer, u32 length) {
	syssram* sram = __SYS_LockSram();
	if(!sram) return 0;

	memcpy((void*) sram + file->offset, buffer, length);

	__SYS_UnlockSram(TRUE);
	while(!__SYS_SyncSram());
	return length;
}

s32 read_rom_dsp_rom(file_handle* file, void* buffer, u32 length) {
	// TODO
	(void) file;
	(void) buffer;
	(void) length;
	return 0;
}

s32 read_rom_dsp_coef(file_handle* file, void* buffer, u32 length) {
	// TODO
	(void) file;
	(void) buffer;
	(void) length;
	return 0;
}

s32 read_rom_dvd_ram_low(file_handle* file, void* buffer, u32 length) {
	dvd_unlock();
	dvd_readmem_array(0x8000 + file->offset, buffer, length);
	return length;
}

s32 read_rom_dvd_rom(file_handle* file, void* buffer, u32 length) {
	dvd_unlock();
	dvd_readmem_array(0xA0000 + file->offset, buffer, length);
	return length;
}

s32 read_rom_dvd_ram_high(file_handle* file, void* buffer, u32 length) {
	dvd_unlock();
	dvd_readmem_array(0x400000 + file->offset, buffer, length);
	return length;
}

s32 read_rom_dvd_disk_bca(file_handle* file, void* buffer, u32 length) {
	if((dvd_get_error()>>24) != 5) {
		dvd_reset();
	}
	dvd_unlock();
	dvd_readmem_array(0x415460 + file->offset, buffer, length);
	return length;
}

s32 read_rom_dvd_disk_pfi(file_handle* file, void* buffer, u32 length) {
	if((dvd_get_error()>>24) != 5) {
		dvd_reset();
	}
	dvd_unlock();
	dvd_readmem_array(0x41710C + file->offset, buffer, length);
	return length;
}

s32 read_rom_dvd_disk_dmi(file_handle* file, void* buffer, u32 length) {
	if((dvd_get_error()>>24) != 5) {
		dvd_reset();
	}
	dvd_unlock();
	dvd_readmem_array(0x41791C + file->offset, buffer, length);
	return length;
}

s32 read_rom_aram(file_handle* file, void* buffer, u32 length) {
	DCInvalidateRange(buffer, length);
	ARQ_PostRequest((ARQRequest*) file->other, (u32) file, ARQ_ARAMTOMRAM, ARQ_PRIO_LO, file->offset, (u32) buffer, length);
	return length;
}

s32 write_rom_aram(file_handle* file, const void* buffer, u32 length) {
	DCFlushRange((void*) buffer, length);
	ARQ_PostRequest((ARQRequest*) file->other, (u32) file, ARQ_MRAMTOARAM, ARQ_PRIO_LO, file->offset, (u32) buffer, length);
	return length;
}

s32 read_rom_aram_expansion(file_handle* file, void* buffer, u32 length) {
	DCInvalidateRange(buffer, length);
	ARQ_PostRequest((ARQRequest*) file->other, (u32) file, ARQ_ARAMTOMRAM, ARQ_PRIO_LO, AR_GetInternalSize() + file->offset, (u32) buffer, length);
	return length;
}

s32 write_rom_aram_expansion(file_handle* file, const void* buffer, u32 length) {
	DCFlushRange((void*) buffer, length);
	ARQ_PostRequest((ARQRequest*) file->other, (u32) file, ARQ_MRAMTOARAM, ARQ_PRIO_LO, AR_GetInternalSize() + file->offset, (u32) buffer, length);
	return length;
}

s32 read_rom_void(file_handle* file, void* buffer, u32 length) {
	DCZeroRange(buffer, length);
	return 0;
}

s32 write_rom_void(file_handle* file, const void* buffer, u32 length) {
	return 0;
}

bool is_rom_name(char* filename) {
	int i;
	for(i = ROM_IPL; i < NUM_ROMS; i++) {
		if(endsWith(filename, rom_names[i])) {
			return true;
		}
	}
	return false;
}

s32 deviceHandler_SYS_init(file_handle* file) {
	s32 i;

	if(!AR_CheckInit()) {
		AR_Init(NULL, 0);
		AR_Reset();
	}
	if(!ARQ_CheckInit()) {
		ARQ_Init();
		ARQ_Reset();
	}

	rom_sizes[ROM_ARAM]           = AR_GetSize();
	rom_sizes[ROM_ARAM_INTERNAL]  = AR_GetInternalSize();
	rom_sizes[ROM_ARAM_EXPANSION] = rom_sizes[ROM_ARAM] - rom_sizes[ROM_ARAM_INTERNAL];

	for(i = ROM_IPL; i < NUM_ROMS; i++) {
		initial_SYS_info.totalSpace += rom_sizes[i];
	}

	return 0;
}

s32 deviceHandler_SYS_readDir(file_handle* ffile, file_handle** dir, u32 type) {
	int num_entries = 1, i;
	*dir = calloc(num_entries, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileAttrib = IS_SPECIAL;

	for(i = ROM_IPL; i < NUM_ROMS; i++) {
		*dir = reallocarray(*dir, num_entries + 1, sizeof(file_handle));
		memset(&(*dir)[num_entries], 0, sizeof(file_handle));
		concat_path((*dir)[num_entries].name, ffile->name, rom_names[i]);
		(*dir)[num_entries].fileAttrib = IS_FILE;
		(*dir)[num_entries].size       = rom_sizes[i];
		(*dir)[num_entries].fileBase   = i;
		num_entries++;
	}

	return num_entries;
}

s64 deviceHandler_SYS_seekFile(file_handle* file, s64 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

s32 deviceHandler_SYS_readFile(file_handle* file, void* buffer, u32 length) {
	s32 i;

	if(file->fileBase == ROM_VOID) {
		for(i = ROM_IPL; i < NUM_ROMS; i++) {
			if(endsWith(file->name, rom_names[i])) {
				file->fileBase = i;
				break;
			}
		}
	}

	s32 ret = read_rom[file->fileBase](file, buffer, length);
	file->offset += ret;
	file->size = rom_sizes[file->fileBase];
	return ret;
}

s32 deviceHandler_SYS_writeFile(file_handle* file, const void* buffer, u32 length) {
	s32 i;

	if(file->fileBase == ROM_VOID) {
		for(i = ROM_IPL; i < NUM_ROMS; i++) {
			if(endsWith(file->name, rom_names[i])) {
				file->fileBase = i;
				break;
			}
		}
	}

	s32 ret = write_rom[file->fileBase](file, buffer, length);
	file->offset += ret;
	file->size = rom_sizes[file->fileBase];
	return ret;
}

s32 deviceHandler_SYS_closeFile(file_handle* file) {
	return 0;
}

s32 deviceHandler_SYS_deinit() {
	initial_SYS_info.totalSpace = 0LL;
	return 0;
}

bool deviceHandler_SYS_test() {
	return true;
}

char* deviceHandler_SYS_status(file_handle* file) {
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_sys = {
	.deviceUniqueId = DEVICE_ID_9,
	.hwName = "Various",
	.deviceName = "System",
	.deviceDescription = "Backup IPL, DSP, DVD, SRAM, ARAM",
	.deviceTexture = {TEX_SYSTEM, 75, 48, 76, 48},
	.features = FEAT_READ|FEAT_WRITE,
	.location = LOC_SYSTEM,
	.initial = &initial_SYS,
	.test = deviceHandler_SYS_test,
	.info = deviceHandler_SYS_info,
	.init = deviceHandler_SYS_init,
	.readDir = deviceHandler_SYS_readDir,
	.seekFile = deviceHandler_SYS_seekFile,
	.readFile = deviceHandler_SYS_readFile,
	.writeFile = deviceHandler_SYS_writeFile,
	.closeFile = deviceHandler_SYS_closeFile,
	.deinit = deviceHandler_SYS_deinit,
	.status = deviceHandler_SYS_status,
};
