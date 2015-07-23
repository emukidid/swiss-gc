/* deviceHandler-SYS.c
        - virtual device interface for dumping ROMs
        by Streetwalrus
 */

#include <malloc.h>
#include "deviceHandler-SYS.h"
#include "main.h"
#include "gui/FrameBufferMagic.h"

int read_rom_ipl(unsigned int offset, void* buffer, unsigned int length);
int read_rom_ipl_clear(unsigned int offset, void* buffer, unsigned int length);
int read_rom_sram(unsigned int offset, void* buffer, unsigned int length);
int read_rom_stub(unsigned int offset, void* buffer, unsigned int length);

#define NUM_ROMS 7

const char* rom_names[] =
{
	"ipl.bin",
	"ipl_clear.bin",
	"dsp_rom.bin",
	"dsp_coef.bin",
	"dvd_ram.bin",
	"dvd_rom.bin",
	"sram.bin"
};

const int rom_sizes[] =
{
	2 * 1024 * 1024,
	2 * 1024 * 1024,
	8 * 1024,
	4 * 1024,
	32 * 1024,
	128 * 1024,
	64
};

int (*read_rom[])(unsigned int offset, void* buffer, unsigned int length) =
{
	read_rom_ipl,
	read_rom_ipl_clear,
	read_rom_stub,
	read_rom_stub,
	read_rom_stub,
	read_rom_stub,
	read_rom_sram
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
	TEX_SYSTEM,
	0,
	0
};

device_info* deviceHandler_SYS_info() {
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

	unsigned it;
	u8* data = buffer;

	static u8 acc, nacc, x;
	static u16 t, u, v;
	static unsigned int descrambled_length;

	if(offset < 0x100) {
		acc = 0;
		nacc = 0;

		t = 0x2953;
		u = 0xd9c2;
		v = 0x3ff1;

		x = 1;

		data += 0x100;
		length -= 0x100;

		descrambled_length = 0;
	}

	for (it = 0; it < length;) {
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
			if(descrambled_length++ >= 0x1afe00)
				length = 0;
			else
				data[it++] ^= acc;
			nacc = 0;
		}
	}

	return ret;
}

int read_rom_sram(unsigned int offset, void* buffer, unsigned int length) {
	u8 sram[64];

	u32 command, ret;

	DCInvalidateRange(sram, 64);

	if(EXI_Lock(EXI_CHANNEL_0, EXI_DEVICE_1, NULL) == 0) return 0;
	if(EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ) == 0) {
		EXI_Unlock(EXI_CHANNEL_0);
		return 0;
	}

	ret = 0;
	command = 0x20000100;
	if(EXI_Imm(EXI_CHANNEL_0, &command, 4, EXI_WRITE, NULL) == 0) ret |= 0x01;
	if(EXI_Sync(EXI_CHANNEL_0) == 0) ret |= 0x02;
	if(EXI_Dma(EXI_CHANNEL_0, sram, 64, EXI_READ, NULL) == 0) ret |= 0x04;
	if(EXI_Sync(EXI_CHANNEL_0) == 0) ret |= 0x08;
	if(EXI_Deselect(EXI_CHANNEL_0) == 0) ret |= 0x10;
	if(EXI_Unlock(EXI_CHANNEL_0) == 0) ret |= 0x20;

	if(ret) return 0;

	memcpy(buffer, sram + offset, length);
	return length;
}

int read_rom_stub(unsigned int offset, void* buffer, unsigned int length) {
	(void) offset;
	(void) buffer;
	(void) length;
	return 0;
}

int deviceHandler_SYS_init(file_handle* file) {
	int i;

	for(i = 0; i < NUM_ROMS; i++) {
		initial_SYS_info.totalSpaceInKB += rom_sizes[i];
	}

	initial_SYS_info.totalSpaceInKB >>= 10;

	return 1;
}

int deviceHandler_SYS_readDir(file_handle* ffile, file_handle** dir, unsigned int type) {
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

int deviceHandler_SYS_readFile(file_handle* file, void* buffer, unsigned int length) {
	int ret = read_rom[file->fileBase](file->offset, buffer, length);
	file->offset += ret;
	return ret;
}

int deviceHandler_SYS_writeFile(file_handle* file, void* buffer, unsigned int length) {
	return 1;
}

int deviceHandler_SYS_deleteFile(file_handle* file) {
	return 1;
}

int deviceHandler_SYS_seekFile(file_handle* file, unsigned int where, unsigned int type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_SYS_setupFile(file_handle* file, file_handle* file2) {
	return 1;
}

int deviceHandler_SYS_closeFile(file_handle* file) {
	return 0;
}

int deviceHandler_SYS_deinit() {
	return 0;
}
