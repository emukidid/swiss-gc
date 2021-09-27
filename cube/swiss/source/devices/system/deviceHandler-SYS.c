/* deviceHandler-SYS.c
        - virtual device interface for dumping ROMs
        by novenary
 */

#include <malloc.h>
#include "deviceHandler-SYS.h"
#include "main.h"
#include "dvd.h"
#include "gui/FrameBufferMagic.h"

int read_rom_ipl(unsigned int offset, void* buffer, unsigned int length);
int read_rom_ipl_clear(unsigned int offset, void* buffer, unsigned int length);
int read_rom_sram(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dsp_rom(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dsp_coef(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dvd_ram_low(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dvd_rom(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dvd_ram_high(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dvd_disk_bca(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dvd_disk_pfi(unsigned int offset, void* buffer, unsigned int length);
int read_rom_dvd_disk_dmi(unsigned int offset, void* buffer, unsigned int length);
int read_rom_aram(unsigned int offset, void* buffer, unsigned int length);

#define NUM_ROMS 12

const char* rom_names[] =
{
	"ipl.bin",
	"ipl_clear.bin",
	"dsp_rom.bin",
	"dsp_coef.bin",
	"dvd_ram_low.bin",
	"dvd_rom.bin",
	"dvd_ram_high.bin",
	"dvd_disk_bca.bin",
	"dvd_disk_pfi.bin",
	"dvd_disk_dmi.bin",
	"sram.bin",
	"aram.bin"
};

const int rom_sizes[] =
{
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
	16 * 1024 * 1024
};

int (*read_rom[])(unsigned int offset, void* buffer, unsigned int length) =
{
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
	read_rom_aram
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
	0LL
};

device_info* deviceHandler_SYS_info(file_handle* file) {
	return &initial_SYS_info;
}

int read_rom_ipl(unsigned int offset, void* buffer, unsigned int length) {
	__SYS_ReadROM(buffer, length, offset);
	return length;
}

int read_rom_ipl_clear(unsigned int offset, void* buffer, unsigned int length) {
	int ret = read_rom_ipl(offset, buffer, length);

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

	return ret;
}

int read_rom_sram(unsigned int offset, void* buffer, unsigned int length) {
	u32 command, ret;

	DCInvalidateRange(buffer, length);

	if(EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL) == 0) return 0;
	if(EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ) == 0) {
		EXI_Unlock(EXI_CHANNEL_0);
		return 0;
	}

	ret = 0;
	command = 0x20000100 + (offset << 6);
	if(EXI_Imm(EXI_CHANNEL_0, &command, 4, EXI_WRITE, NULL) == 0) ret |= 0x01;
	if(EXI_Sync(EXI_CHANNEL_0) == 0) ret |= 0x02;
	if(EXI_Dma(EXI_CHANNEL_0, buffer, length, EXI_READ, NULL) == 0) ret |= 0x04;
	if(EXI_Sync(EXI_CHANNEL_0) == 0) ret |= 0x08;
	if(EXI_Deselect(EXI_CHANNEL_0) == 0) ret |= 0x10;
	if(EXI_Unlock(EXI_CHANNEL_0) == 0) ret |= 0x20;

	if(ret) return 0;
	return length;
}

int read_rom_dsp_rom(unsigned int offset, void* buffer, unsigned int length) {
	// TODO
	(void) offset;
	(void) buffer;
	(void) length;
	return 0;
}

int read_rom_dsp_coef(unsigned int offset, void* buffer, unsigned int length) {
	// TODO
	(void) offset;
	(void) buffer;
	(void) length;
	return 0;
}

int read_rom_dvd_ram_low(unsigned int offset, void* buffer, unsigned int length) {
	dvd_unlock();
	dvd_readmem_array(0x8000 + offset, buffer, length);
	return length;
}

int read_rom_dvd_rom(unsigned int offset, void* buffer, unsigned int length) {
	dvd_unlock();
	dvd_readmem_array(0xA0000 + offset, buffer, length);
	return length;
}

int read_rom_dvd_ram_high(unsigned int offset, void* buffer, unsigned int length) {
	dvd_unlock();
	dvd_readmem_array(0x400000 + offset, buffer, length);
	return length;
}

int read_rom_dvd_disk_bca(unsigned int offset, void* buffer, unsigned int length) {
	if((dvd_get_error()>>24) != 5) {
		dvd_reset();
	}
	dvd_unlock();
	dvd_readmem_array(0x415460 + offset, buffer, length);
	return length;
}

int read_rom_dvd_disk_pfi(unsigned int offset, void* buffer, unsigned int length) {
	if((dvd_get_error()>>24) != 5) {
		dvd_reset();
	}
	dvd_unlock();
	dvd_readmem_array(0x41710C + offset, buffer, length);
	return length;
}

int read_rom_dvd_disk_dmi(unsigned int offset, void* buffer, unsigned int length) {
	if((dvd_get_error()>>24) != 5) {
		dvd_reset();
	}
	dvd_unlock();
	dvd_readmem_array(0x41791C + offset, buffer, length);
	return length;
}

int read_rom_aram(unsigned int offset, void* buffer, unsigned int length) {
	u32 ol = length;

	u8 *sector_buffer = (u8*)memalign(32,2048);
	while (length)
	{
		uint32_t sector = offset / 2048;
		DCInvalidateRange(buffer, length);
		AR_StartDMA(1, (u32) sector_buffer, sector * 2048, 2048);
		while (AR_GetDMAStatus());
		uint32_t off = offset & 2047;

		int rl = 2048 - off;
		if (rl > length)
			rl = length;
		memcpy(buffer, sector_buffer + off, rl);	

		offset += rl;
		length -= rl;
		buffer += rl;
	}
	free(sector_buffer);

	return ol;
}

s32 deviceHandler_SYS_init(file_handle* file) {
	s32 i;

	for(i = 0; i < NUM_ROMS; i++) {
		initial_SYS_info.totalSpace += rom_sizes[i];
	}

	return 1;
}

s32 deviceHandler_SYS_readDir(file_handle* ffile, file_handle** dir, u32 type) {
	int num_entries = 1, i;
	*dir = malloc(num_entries * sizeof(file_handle));
	memset(&(*dir)[0], 0, sizeof(file_handle));
	strcpy((*dir)[0].name,"..");
	(*dir)[0].fileAttrib = IS_SPECIAL;

	for(i = 0; i < NUM_ROMS; i++) {
		num_entries++;
		*dir = realloc(*dir, num_entries * sizeof(file_handle));
		memset(&(*dir)[i + 1], 0, sizeof(file_handle));
		strcpy((*dir)[i + 1].name, rom_names[i]);
		(*dir)[i + 1].fileAttrib = IS_FILE;
		(*dir)[i + 1].size       = rom_sizes[i];
		(*dir)[i + 1].fileBase   = i;
	}

	return num_entries;
}

s32 deviceHandler_SYS_readFile(file_handle* file, void* buffer, u32 length) {
	int ret = read_rom[file->fileBase](file->offset, buffer, length);
	file->offset += ret;
	return ret;
}

s32 deviceHandler_SYS_writeFile(file_handle* file, void* buffer, u32 length) {
	return 1;
}

s32 deviceHandler_SYS_deleteFile(file_handle* file) {
	return 1;
}

s32 deviceHandler_SYS_seekFile(file_handle* file, u32 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_SYS_setupFile(file_handle* file, file_handle* file2, int numToPatch) {
	return 1;
}

s32 deviceHandler_SYS_closeFile(file_handle* file) {
	return 0;
}

s32 deviceHandler_SYS_deinit() {
	return 0;
}

bool deviceHandler_SYS_test() {
	return true;
}

DEVICEHANDLER_INTERFACE __device_sys = {
	DEVICE_ID_9,
	"Various",
	"System",
	"Backup IPL, DSP, DVD, SRAM",
	{TEX_SYSTEM, 80, 84}, 
	FEAT_READ,
	EMU_NONE,
	LOC_SYSTEM,
	&initial_SYS,
	(_fn_test)&deviceHandler_SYS_test,
	(_fn_info)&deviceHandler_SYS_info,
	(_fn_init)&deviceHandler_SYS_init,
	(_fn_readDir)&deviceHandler_SYS_readDir,
	(_fn_readFile)&deviceHandler_SYS_readFile,
	(_fn_writeFile)NULL,
	(_fn_deleteFile)NULL,
	(_fn_seekFile)&deviceHandler_SYS_seekFile,
	(_fn_setupFile)&deviceHandler_SYS_setupFile,
	(_fn_closeFile)&deviceHandler_SYS_closeFile,
	(_fn_deinit)&deviceHandler_SYS_deinit,
	(_fn_emulated)NULL,
};
