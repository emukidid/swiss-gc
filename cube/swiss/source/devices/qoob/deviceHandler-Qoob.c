/* deviceHandler-Qoob.c
	- device implementation for Qoob Flash
	by emu_kidid
 */

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "exi.h"

typedef struct qoobEntryHeader qoobEntryHeader;

struct qoobEntryHeader {
	u32 entry_type;
	char entry_name[0xF8];
	u16 num_blocks;
	u16 unk;
};
#define QOOB_BLOCK_SIZE (65536)
#define QOOB_FILE_APPL (0x4170706C)	//Appl
#define QOOB_FILE_BIOS (0x42494F53)	//BIOS
#define QOOB_FILE_QPIC (0x51504943)	//QPIC
#define QOOB_FILE_QCFG (0x51434647)	//QCFG
#define QOOB_FILE_QCHT (0x51434854)	//QCHT
#define QOOB_FILE_QCHE (0x51434845)	//QCHE
#define QOOB_FILE_BIN  (0x42494E00)	//BIN.
#define QOOB_FILE_DOL  (0x444F4C00)	//DOL.
#define QOOB_FILE_ELF  (0x454C4600)	//ELF.
#define QOOB_FILE_SWIS (0x53574953)	//SWIS (custom for Swiss to be able to put whatever on Qoob Flash)

char iplBlock[256] __attribute__((aligned(32)));

file_handle initial_Qoob =
	{ "qoob:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0
	};
	
device_info initial_Qoob_info = {
	0x200000,
	0x200000,
	false
};
	
device_info* deviceHandler_Qoob_info(file_handle* file) {
	return &initial_Qoob_info;
}

char *getQoobExtension(qoobEntryHeader* entryHeader) {
	switch(entryHeader->entry_type) {
		case QOOB_FILE_ELF:
			return "elf";
		case QOOB_FILE_APPL:
			return "appl";
		case QOOB_FILE_BIOS:
			return "gcb";
		case QOOB_FILE_QPIC:
			return "m1v";
		case QOOB_FILE_QCFG:
			return "cfg";
		case QOOB_FILE_QCHT:
			return "qch";
		case QOOB_FILE_QCHE:
			return "qche";
		case QOOB_FILE_BIN:
			return "bin";
		case QOOB_FILE_DOL:	
			return "dol";
		default:
			return NULL;
	}
}
	
s32 deviceHandler_Qoob_readDir(file_handle* ffile, file_handle** dir, u32 type) {	
  
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading Qoob"));
	// Set everything up to read
	int num_entries = 1, i = 1, block = 0;
	*dir = malloc( num_entries * sizeof(file_handle) );
	memset(&(*dir)[0], 0, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileAttrib = IS_SPECIAL;
	
	u32 usedSpace = 0;
	
	while(block <0x200000) {
		char entryName[128];
		__SYS_ReadROM(iplBlock, sizeof(qoobEntryHeader), block);
		qoobEntryHeader entryHeader;
		memcpy(&entryHeader, &iplBlock, sizeof(qoobEntryHeader));
		
		switch(entryHeader.entry_type) {
			case QOOB_FILE_ELF:
			case QOOB_FILE_APPL:
			case QOOB_FILE_BIOS:
			case QOOB_FILE_QPIC:
			case QOOB_FILE_QCFG:
			case QOOB_FILE_QCHT:
			case QOOB_FILE_QCHE:
			case QOOB_FILE_BIN:
			case QOOB_FILE_DOL:	
			case QOOB_FILE_SWIS:
			{
				// Make sure we have room for this one
				if(i == num_entries){
					++num_entries;
					*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
				}
				memset(&(*dir)[i], 0, sizeof(file_handle));
				memset(entryName, 0, 128);
				
				// The windows flasher puts .dol files as "ELF" type, support this case.
				if(entryHeader.entry_type == QOOB_FILE_ELF) {
					__SYS_ReadROM(iplBlock, 256, block + sizeof(qoobEntryHeader));
					if (*(u32*)&iplBlock[0] == 0x00000100) {
						if(!endsWith(entryHeader.entry_name, ".dol")) {
							snprintf(&entryName[0], 64, "%s.dol", entryHeader.entry_name);
						}
						else {
							snprintf(&entryName[0], 64, "%s", entryHeader.entry_name);
						}
					}
				}
				else if(entryHeader.entry_type == QOOB_FILE_QCFG) {
					snprintf(&entryName[0], 64, "qoob.%s", getQoobExtension(&entryHeader));
				}
				else if(entryHeader.entry_type == QOOB_FILE_QCHE) {
					snprintf(&entryName[0], 64, "cheatengine.%s", getQoobExtension(&entryHeader));
				}
				if(!entryName[0]) {
					char *ext = getQoobExtension(&entryHeader);
					if(ext) {
						snprintf(&entryName[0], 64, "%s.%s", entryHeader.entry_name, ext);
					}
					else {
						snprintf(&entryName[0], 64, "%s", entryHeader.entry_name);
					}
				}
				concat_path((*dir)[i].name, ffile->name, entryName);
				(*dir)[i].size       = entryHeader.num_blocks * QOOB_BLOCK_SIZE;
				(*dir)[i].fileAttrib = IS_FILE;
				(*dir)[i].fileBase   = block;
				usedSpace += (*dir)[i].size;
				++i;
				
				print_gecko("Found [%08X] entry, %08X in size\r\n", entryHeader.entry_type, entryHeader.num_blocks);
				block += (entryHeader.num_blocks * QOOB_BLOCK_SIZE);
				break;
			}
			default:
				print_gecko("unknown/empty block found at %08X [%08X]\r\n", block, entryHeader.entry_type);
				block += QOOB_BLOCK_SIZE;
				break;
		}
	}
	initial_Qoob_info.freeSpace = initial_Qoob_info.totalSpace - usedSpace;
	DrawDispose(msgBox);
	return num_entries;
}

s64 deviceHandler_Qoob_seekFile(file_handle* file, s64 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

s32 deviceHandler_Qoob_readFile(file_handle* file, void* buffer, u32 length) {
	if(!file->fileBase) {
		return -1;
	}
	__SYS_ReadROM(buffer,length,file->fileBase+file->offset+sizeof(qoobEntryHeader));
	return length;
}

extern unsigned char rom_read(int addr);
extern void rom_write(int addr, unsigned char c);

int erase_sector(int addr)
{
	rom_write(0xAAA, 0xAA);
	rom_write(0x555, 0x55);
	rom_write(0xAAA, 0x80);
	rom_write(0xAAA, 0xAA);
	rom_write(0x555, 0x55);
	rom_write(addr, 0x30);
	printf("\r%08x erase..\n", addr);
	while (rom_read(addr) != rom_read(addr));
	return 0;
}

int is_eraseblock(int addr)
{
	if (addr >= 0x10000)
		return !(addr & 0xFFFF);
	return ((addr == 0) || (addr == 0x4000) || (addr == 0x6000) || (addr == 0x8000));
}

int erase_qoob_rom(u32 dest, int len)
{
	rom_write(0xAAA, 0xAA); rom_write(0x555, 0x55); rom_write(0xAAA, 0x20);
	int addr;
	print_gecko("erasing...\r\n");
	for (addr = dest; addr < (dest + len); ++addr) {
		if (is_eraseblock(addr)) {
			print_gecko("erase_sector(%08X)\r\n", addr);
			erase_sector(addr);
		}
	}
	return 0;
}

int write_qoob_rom(unsigned char *src, u32 dest, int len)
{
	print_gecko("Writing %08X to dest %08X with length %i\r\n", src, dest, len);
	// CFI query
	rom_write(0xAA, 0x98);

	print_gecko("CFI data:\r\n");
	int addr;
	for (addr = 0; addr < 0x100; addr += 0x10)
	{
		int i;
		for (i=0; i<0x10; i += 2)
			print_gecko("%02x ", rom_read(addr + i));

		for (i=0; i<0x10; i += 2)
		{
			int v = rom_read(addr + i);
			print_gecko("%c", v >= 0x20 ? v : '.');
		}
		print_gecko("\r\n");
	}
	
	print_gecko("man id:\r\n");
	int i;
	for (i=0; i<10; ++i)
	{
		rom_write(0, 0xF0);
		rom_write(0xAAA, 0xAA); rom_write(0x555, 0x55); rom_write(0xAAA, 0x90);
		print_gecko("%02x ", rom_read(i * 2));
	}
	print_gecko("\r\n");
	
	erase_qoob_rom(dest, len);
	
	print_gecko("flashing...\r\n");
	while (len)
	{
		rom_write(dest, 0xA0);
		rom_write(dest, *src);
		while (rom_read(dest) != *src);
		++src;
		++dest;
		--len;
	}
	print_gecko("done!\r\n");
	return 0;
}


// Assumes a single call to write a file.
s32 deviceHandler_Qoob_writeFile(file_handle* file, void* buffer, u32 length) {
	if(!length) return 0;

	if(!file->fileBase) {
		// check for enough contiguous blocks because files can only be contiguous
		u32 block = 0;
		u32 largestEmptyBlock = 0;
		u32 largestEmptyBlockSize = 0;
		u32 emptyBlock = 0;
		u32 emptyBlockSize = 0;
		
		while(block <0x200000) {
			memset(iplBlock, 0, 256);
			__SYS_ReadROM(iplBlock,sizeof(qoobEntryHeader),block);
			qoobEntryHeader entryHeader;
			memcpy(&entryHeader, &iplBlock, sizeof(qoobEntryHeader));
			print_gecko("Checking block at %08X\r\n", block);
			switch(entryHeader.entry_type) {
				case QOOB_FILE_APPL:
				case QOOB_FILE_BIOS:
				case QOOB_FILE_QPIC:
				case QOOB_FILE_QCFG:
				case QOOB_FILE_QCHT:
				case QOOB_FILE_QCHE:
				case QOOB_FILE_BIN:	
				case QOOB_FILE_DOL:	
				case QOOB_FILE_ELF:	
				case QOOB_FILE_SWIS:
					if(emptyBlock) {
						if(emptyBlockSize > largestEmptyBlockSize) {
							largestEmptyBlock = emptyBlock;
							largestEmptyBlockSize = emptyBlockSize;
							print_gecko("Largest empty block so far is %08X with size %08X\r\n", largestEmptyBlock, largestEmptyBlockSize);
						}
						emptyBlock = 0;
						emptyBlockSize = 0;
					}
					print_gecko("Found [%08X] entry, %08X in size\r\n", entryHeader.entry_type, entryHeader.num_blocks);
					block += (entryHeader.num_blocks * QOOB_BLOCK_SIZE);
					break;
				case 0xFFFFFFFF:
					print_gecko("empty block found at %08X [%08X]\r\n", block, entryHeader.entry_type);
					if(!emptyBlock) {
						emptyBlock = block;
					}
					emptyBlockSize += QOOB_BLOCK_SIZE;
					block += QOOB_BLOCK_SIZE;
					break;
				default:
					print_gecko("unknown block found at %08X [%08X]\r\n", block, entryHeader.entry_type);
					if(emptyBlock) {
						if(emptyBlockSize > largestEmptyBlockSize) {
							largestEmptyBlock = emptyBlock;
							largestEmptyBlockSize = emptyBlockSize;
							print_gecko("Largest empty block so far is %08X with size %08X\r\n", largestEmptyBlock, largestEmptyBlockSize);
						}
						emptyBlock = 0;
						emptyBlockSize = 0;
					}
					block += QOOB_BLOCK_SIZE;
					break;
			}
		}
		// If we were looking at an empty block but didn't terminate it:
		if(emptyBlock) {
			if(emptyBlockSize > largestEmptyBlockSize) {
				largestEmptyBlock = emptyBlock;
				largestEmptyBlockSize = emptyBlockSize;
				print_gecko("Largest empty block so far is %08X with size %08X\r\n", largestEmptyBlock, largestEmptyBlockSize);
			}
		}
		if(largestEmptyBlock) {
			print_gecko("Largest empty block found at %08X with size %08X\r\n", largestEmptyBlock, largestEmptyBlockSize);
		}
		else {
			print_gecko("No empty blocks found\r\n");
		}
		
		// No space.
		if(!largestEmptyBlock || length > largestEmptyBlockSize-0x100) {
			return -1;
		}
		file->fileBase = largestEmptyBlock;
		
		// Make a header, write it.
		qoobEntryHeader entryHeader;
		memset(&entryHeader, 0, sizeof(qoobEntryHeader));
		entryHeader.entry_type = endsWith(file->name,".dol") ? QOOB_FILE_ELF /*yes, these go in as "ELF" */ : QOOB_FILE_SWIS;
		entryHeader.num_blocks = (length >> 16) + (length % (QOOB_BLOCK_SIZE-1) > 0 ? 1 : 0);
		snprintf(&entryHeader.entry_name[0], 64, "%s", getRelativeName(file->name));
		write_qoob_rom((unsigned char*)&entryHeader, file->fileBase, sizeof(qoobEntryHeader));
	}	
	
	// Write the file out
	s32 written = write_qoob_rom((unsigned char*)buffer, file->fileBase + sizeof(qoobEntryHeader), length);
	file->size += length;
	file->offset += length;
	return !written ? length : -1;
}

s32 deviceHandler_Qoob_deleteFile(file_handle* file) {
	erase_qoob_rom(file->fileBase, file->size);
	return 0;
}

bool deviceHandler_Qoob_test() {
	// Read 1024 bytes from certain sections of the IPL Mask ROM and compare with Qoob enabled/disabled
	char *qoobData = (char*)memalign(32, 0x400);
	char *iplData = (char*)memalign(32, 0x400);
	
	memset(qoobData, 0, 0x400);
	memset(iplData, 0, 0x400);
	
	ipl_set_config(0);
	__SYS_ReadROM(qoobData,0x100,0);
	__SYS_ReadROM(qoobData+0x100,0x100,0x80000);
	__SYS_ReadROM(qoobData+0x200,0x100,0x1FCF00);
	__SYS_ReadROM(qoobData+0x300,0x100,0x1FFF00);
	
	
	ipl_set_config(6);
	__SYS_ReadROM(iplData,0x100,0);
	__SYS_ReadROM(iplData+0x100,0x100,0x80000);
	__SYS_ReadROM(iplData+0x200,0x100,0x1FCF00);
	__SYS_ReadROM(iplData+0x300,0x100,0x1FFF00);
	
	bool qoobFound = memcmp(qoobData, iplData, 0x400) != 0;
	
	free(qoobData);
	free(iplData);
	return qoobFound;
}

s32 deviceHandler_Qoob_init(file_handle* file) {
	if(!deviceHandler_Qoob_test()) {
		return ENODEV;
	}
	ipl_set_config(0);
	return 0;
}

s32 deviceHandler_Qoob_deinit(file_handle* file) {
	ipl_set_config(6);
	return 0;
}

s32 deviceHandler_Qoob_closeFile(file_handle* file) {
    return 0;
}

char* deviceHandler_Qoob_status(file_handle* file) {
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_qoob = {
	DEVICE_ID_7,
	"Qoob Modchip",
	"Qoob Pro",
	"Qoob Pro Flash File System",
	{TEX_QOOB, 82, 84, 88, 88},
	FEAT_READ|FEAT_WRITE,
	EMU_NONE,
	LOC_SYSTEM,
	&initial_Qoob,
	(_fn_test)&deviceHandler_Qoob_test,
	(_fn_info)&deviceHandler_Qoob_info,
	(_fn_init)&deviceHandler_Qoob_init,
	(_fn_makeDir)NULL,
	(_fn_readDir)&deviceHandler_Qoob_readDir,
	(_fn_seekFile)&deviceHandler_Qoob_seekFile,
	(_fn_readFile)&deviceHandler_Qoob_readFile,
	(_fn_writeFile)&deviceHandler_Qoob_writeFile,
	(_fn_closeFile)&deviceHandler_Qoob_closeFile,
	(_fn_deleteFile)&deviceHandler_Qoob_deleteFile,
	(_fn_renameFile)NULL,
	(_fn_setupFile)NULL,
	(_fn_deinit)&deviceHandler_Qoob_deinit,
	(_fn_emulated)NULL,
	(_fn_status)&deviceHandler_Qoob_status,
};
