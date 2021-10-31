/* deviceHandler-CARD.c
	- device implementation for CARD (NGC Memory Cards)
	by emu_kidid
 */


#include <stdio.h>
#include <string.h>
#include <gccore.h>
#include <malloc.h>
#include <ogc/card.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "images/gamecube_rgb.h"


file_handle initial_CARDA =
	{ "carda:/",       // directory
	  0,
	  0,          // offset
	  0,          // size
	  IS_DIR,
	  0
};

file_handle initial_CARDB =
	{ "cardb:/",       // directory
	  0	,
	  0,          // offset
	  0,          // size
	  IS_DIR,
	  0
};

static device_info initial_CARD_info[2];

static unsigned char *sys_area[2] = {NULL, NULL};
static int card_init[2] = {0,0};
static u32 card_sectorsize[2] = {8192, 8192};
static GCI *gciInfo;
static bool isCopyGCIMode = 0;

void card_removed_cb(s32 chn, s32 result) {  
  card_init[chn] = 0; 
  CARD_Unmount(chn); 
}

char *cardError(int error_code) {
  switch(error_code) {
    case CARD_ERROR_BUSY:
      return "Memory Card Busy";
    case CARD_ERROR_WRONGDEVICE:
      return "Inserted device is not a memory card";
    case CARD_ERROR_NOCARD:
      return "No Card Inserted";
    case CARD_ERROR_BROKEN:
      return "Card Corrupted(?)";
	case CARD_ERROR_NOFILE:
		return "File does not exist";
    default:
      return "Unknown error";
  }
}

#define CARD_SetGameAndCompany() \
	CARD_SetGamecode("SWIS"); \
	CARD_SetCompany("S0");


int initialize_card(int slot) {
	EXI_Detach(slot);
	int slot_error = CARD_ERROR_READY, i = 0;
  
	if(!card_init[slot]) {
		/* Pass company identifier and number */
		CARD_Init ("SWIS", "S0");
		if(!sys_area[slot]) sys_area[slot] = memalign(32,CARD_WORKAREA);
		  
		/* Lets try 50 times to mount it. Sometimes it takes a while */
		for(i = 0; i<50; i++) {
			slot_error = CARD_Mount (slot, sys_area[slot], card_removed_cb);
			if(slot_error == CARD_ERROR_READY) {
				CARD_GetSectorSize (slot, &card_sectorsize[slot]);
				break;
			}
		}
	}
	card_init[slot] = slot_error == CARD_ERROR_READY ? 1 : 0;
	return slot_error;
}

device_info* deviceHandler_CARD_info(file_handle* file) {
	int slot = (!strncmp((const char*)initial_CARDB.name, file->name, 7));
	return &initial_CARD_info[slot];
}

s32 deviceHandler_CARD_readDir(file_handle* ffile, file_handle** dir, u32 type){	

	int num_entries = 1, ret = 0, i = 1, slot = (!strncmp((const char*)initial_CARDB.name, ffile->name, 7));
	card_dir *memcard_dir = NULL;
  
	if(!card_init[slot]) { //if some error
		ret = initialize_card(slot);
		if(ret != CARD_ERROR_READY) {
			return -1; //fail
		}
	}
	CARD_SetGameAndCompany();
  
	memcard_dir = (card_dir*)memalign(32,sizeof(card_dir));
	memset(memcard_dir, 0, sizeof(card_dir));
 	
	/* Convert the Memory Card "file" data to fileBrowser_files */
	*dir = calloc(sizeof(file_handle), 1);
	(*dir)[0].fileAttrib = IS_SPECIAL;
	strcpy((*dir)[0].name, "..");

	int usedSpace = 0;
	ret = CARD_FindFirst (slot, memcard_dir, true);
	while (CARD_ERROR_NOFILE != ret) {
		// Make sure we have room for this one
		if(i == num_entries){
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
		}
		memset(&(*dir)[i], 0, sizeof(file_handle));
		strcpy((*dir)[i].name, ffile->name);
		strncat((*dir)[i].name, memcard_dir->filename, CARD_FILENAMELEN);
		(*dir)[i].fileAttrib = IS_FILE;
		(*dir)[i].size     = memcard_dir->filelen;
		memcpy( (*dir)[i].other, memcard_dir, sizeof(card_dir));
		usedSpace += memcard_dir->filelen;
		ret = CARD_FindNext (memcard_dir);
		++i;
	}
	free(memcard_dir);
	
	initial_CARD_info[slot].freeSpace = initial_CARD_info[slot].totalSpace - usedSpace;

	return num_entries;
}

// Finds a file based on file->name and populates file->size if found.
bool findFile(file_handle* file) {
	unsigned int slot = (!strncmp((const char*)initial_CARDB.name, file->name, 7)), ret = 0;
	char *filename = getRelativeName(file->name);
	bool found = false;
	CARD_SetGameAndCompany();
  
	card_dir* memcard_dir = (card_dir*)memalign(32,sizeof(card_dir));
	memset(memcard_dir, 0, sizeof(card_dir));
 	
	ret = CARD_FindFirst (slot, memcard_dir, true);
	while (CARD_ERROR_NOFILE != ret && !found) {
		if(!strncmp(filename, memcard_dir->filename, CARD_FILENAMELEN)) {
			file->size = memcard_dir->filelen;
			memcpy( file->other, memcard_dir, sizeof(card_dir));
			found = true;
 		}
		ret = CARD_FindNext (memcard_dir);
	}
	free(memcard_dir);
	return found;
}

s32 deviceHandler_CARD_seekFile(file_handle* file, u32 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int CARD_ReadUnaligned(card_file *cardfile, void *buffer, unsigned int length, unsigned int offset, int slot) {
	void *dst = buffer;
	u8 *read_buffer = (u8*)memalign(32,card_sectorsize[slot]);
	int ret = CARD_ERROR_READY;
	print_gecko("Unaligned read dst %08X offset %08X length %i\r\n", dst, offset, length);
	// Unaligned because we're in the middle of a sector, read partial
	ret = CARD_Read(cardfile, read_buffer, card_sectorsize[slot], offset-(offset&0x1ff));
	print_gecko("CARD_Read offset %08X length %i\r\n", offset-(offset&0x1ff), card_sectorsize[slot]);
	if(ret != CARD_ERROR_READY) {
		free(read_buffer);
		return ret;
	}

	int amountRead = card_sectorsize[slot]-(offset&0x1ff);
	int amountToCopy = length > amountRead ? amountRead : length;
	memcpy(dst, read_buffer+(offset&0x1ff), amountToCopy);
	dst += amountToCopy;
	length -= amountToCopy;
	offset += amountToCopy;
	if(length != 0) {
		print_gecko("Unaligned read leftovers offset %08X length %i\r\n", offset, length);
		// At least this will be aligned
		ret = CARD_Read(cardfile, read_buffer, card_sectorsize[slot], offset);
		print_gecko("CARD_Read offset %08X length %i\r\n", offset, card_sectorsize[slot]);
		if(ret != CARD_ERROR_READY) {
			free(read_buffer);
			return ret;
		}
		memcpy(dst, read_buffer, length);
	}

	free(read_buffer);
	return ret;
}


s32 deviceHandler_CARD_readFile(file_handle* file, void* buffer, u32 length){
	card_file cardfile;
	void *dst = buffer;
	card_dir* cd = (card_dir*)&file->other;
	char *filename = getRelativeName(file->name);
	unsigned int slot = (!strncmp((const char*)initial_CARDB.name, file->name, 7)), ret = 0;

	if(cd->company[0] == '\0' && cd->gamecode[0] == '\0') {
		// Find the file we don't know about and populate this file_handle if we find it.
		if(!findFile(file)) {
			return CARD_ERROR_NOFILE;
		}
		CARD_SetCompany((const char*)cd->company);
		CARD_SetGamecode((const char*)cd->gamecode);
	} 
	else {
		CARD_SetCompany((const char*)cd->company);
		CARD_SetGamecode((const char*)cd->gamecode);
	}
	int swissFile = !strncmp((const char*)cd->gamecode, "SWIS", 4)
				 && !strncmp((const char*)cd->company, "S0", 2);
	
	// Open the file based on the slot & file name
	ret = CARD_Open(slot,filename, &cardfile);

	print_gecko("Tried to open: [%s] in slot %s got res: %i\r\n",filename, slot ? "B":"A", ret);

	if(ret != CARD_ERROR_READY)	return ret;

	u8 *read_buffer = (u8*)memalign(32,card_sectorsize[slot]);
	if(!read_buffer) {
		return -1;
	}

	/* Read from the file */
	u32 amountRead = 0;
	// If this file was put here by swiss, then skip the first 8192 bytes
	if(swissFile && !isCopyGCIMode) {
		print_gecko("Swiss copied file detected, skipping icon\r\n");
		file->offset += 8192;
	}
	if(isCopyGCIMode) {
		// Write out a .GCI
		card_stat cardstat;
		GCI gci;
		CARD_GetStatus(slot, cardfile.filenum, &cardstat);
		memset(&gci, 0, sizeof(GCI));
		memcpy(&gci.gamecode,cardstat.gamecode,4);
		memcpy(&gci.company,cardstat.company,2);
		memcpy(&gci.filename,cardstat.filename,CARD_FILENAMELEN);
		gci.reserved01 = 0xFF;
		gci.banner_fmt = cardstat.banner_fmt;
		gci.time = cardstat.time;
		gci.icon_addr = cardstat.icon_addr;
		gci.icon_fmt = cardstat.icon_fmt;
		gci.icon_speed = cardstat.icon_speed;
		gci.unknown1 = cd->permissions;
		gci.filesize8 = cardstat.len / 8192;
		gci.reserved02 = 0xFFFF;
		gci.comment_addr = cardstat.comment_addr;
		memcpy(dst, &gci, sizeof(GCI));
		dst+=sizeof(GCI);
		length-=sizeof(GCI);
		amountRead += sizeof(GCI);
	}
	while(length > 0 && file->offset < file->size) {
		int readsize = length > card_sectorsize[slot] ? card_sectorsize[slot] : length;
		print_gecko("Need to read: [%i] more bytes\r\n", length);
		
		if(file->offset&0x1ff) {
			ret = CARD_ReadUnaligned(&cardfile, read_buffer, card_sectorsize[slot], file->offset, slot);
		}
		else {
			ret = CARD_Read(&cardfile, read_buffer, card_sectorsize[slot], file->offset);
			// Sometimes reads fail the first time stupidly, retry at least once.
			print_gecko("Read: [%i] bytes ret [%i] from offset [%i]\r\n",card_sectorsize[slot],ret, file->offset);
			if(ret == CARD_ERROR_BUSY) {
				print_gecko("Read retry\r\n");
				usleep(2000);
				ret = CARD_Read(&cardfile, read_buffer, card_sectorsize[slot], file->offset);
			}
		}
		
		if(ret == CARD_ERROR_READY)
			memcpy(dst,read_buffer,readsize);
		else 
			return ret;
		dst+=readsize;
		file->offset += readsize;
		length -= readsize;
		amountRead += readsize;
	}
	// For swiss adjusted files, don't trail off the end.
	if(swissFile && length != 0) {
		amountRead+= length;
	}
	DCFlushRange(dst, amountRead);
	CARD_Close(&cardfile);
	free(read_buffer);   

	return amountRead;
}

// This function should always be called for the FULL length cause CARD is lame like that.
s32 deviceHandler_CARD_writeFile(file_handle* file, void* data, u32 length) {
	
	if(gciInfo == NULL) {	// Swiss ID for this
		CARD_SetGameAndCompany();
	} else {	// Game specific
		char company[4], gamecode[8];
		memset(company, 0, 4);
		memset(gamecode, 0, 8);
		memcpy(gamecode, gciInfo->gamecode, 4);
		memcpy(company, gciInfo->company, 2);
		CARD_SetCompany(company);
		CARD_SetGamecode(gamecode);
	}
	
	card_file cardfile;
	unsigned int slot = (!strncmp((const char*)initial_CARDB.name, file->name, 7)), ret = 0;
	unsigned int adj_length = (length % 8192 == 0 ? length : (length + (8192-length%8192)))+8192;
	char *filename = NULL;
	char fname[CARD_FILENAMELEN+1];
	if(gciInfo != NULL) {
		memset(fname, 0, CARD_FILENAMELEN+1);
		memcpy(fname, gciInfo->filename, CARD_FILENAMELEN);
		filename = &fname[0];
		adj_length = gciInfo->filesize8*8192;
	} else {
		filename = getRelativeName(file->name);
	}
	
	// Open the file based on the slot & file name
	ret = CARD_Open(slot,filename, &cardfile);

	print_gecko("Tried to open: [%s] in slot %s got res: %i\r\n",filename, slot ? "B":"A", ret);
	
	if(ret == CARD_ERROR_NOFILE) {
		// If the file doesn't exist, create it.
		ret = CARD_Create(slot, filename,adj_length,&cardfile);
		print_gecko("Tried to create: [%s] in slot %s got res: %i\r\n",filename, slot ? "B":"A", ret);
		if(ret != CARD_ERROR_READY) {
			return ret;
		}
		ret = CARD_Open(slot,filename, &cardfile);
		print_gecko("Tried to open after create: [%s] in slot %s got res: %i\r\n",filename, slot ? "B":"A", ret);
	}
	
	if(ret != CARD_ERROR_READY) {
		// Something went wrong
		return -1;
	}

	// Write the icon, comment, etc.
	time_t gc_time = time (NULL);
	char *tmpBuffer = memalign(32,adj_length);
	memset(tmpBuffer,0,adj_length);
	if(gciInfo == NULL) {
		strcpy(tmpBuffer, filename);
		strcpy(tmpBuffer+0x20, ctime (&gc_time));
		memcpy(tmpBuffer+0x40, gamecube_rgb, sizeof(gamecube_rgb));	// Copy icon
		memcpy(tmpBuffer+0x2000, data, length);	// Copy data
	}
	else {
		data+=sizeof(GCI);
		memcpy(tmpBuffer, data, length);	// Copy data
	}
	
	// The file exists by now, write at offset.
	int amount_written = 0;
	while(amount_written != adj_length) {
		ret = CARD_Write(&cardfile, tmpBuffer+amount_written, card_sectorsize[slot], file->offset);
		file->offset += card_sectorsize[slot];
		amount_written += card_sectorsize[slot];
		print_gecko("Tried to write: [%s] in slot %s got res: %i\r\n",filename, slot ? "B":"A", ret);
		if(ret != CARD_ERROR_READY) break;
	}
	
	card_stat cardstat;
	if(gciInfo == NULL) {	// Swiss
		memset(&cardstat, 0, sizeof(card_stat));
		memcpy(&cardstat.filename, filename, CARD_FILENAMELEN-1);
		cardstat.time = gc_time;
		cardstat.comment_addr = 0;
		cardstat.icon_addr = 0x40;
		cardstat.icon_speed = CARD_SPEED_FAST;
		cardstat.banner_fmt = CARD_BANNER_NONE;
		cardstat.icon_fmt = CARD_ICON_RGB;
	}
	else {	// Info is coming from a GCI
		cardstat.banner_fmt = gciInfo->banner_fmt;
		cardstat.time = gciInfo->time;
		cardstat.icon_addr = gciInfo->icon_addr;
		cardstat.icon_fmt = gciInfo->icon_fmt;
		cardstat.icon_speed = gciInfo->icon_speed;
		cardstat.comment_addr = gciInfo->comment_addr;
	}
	CARD_SetStatus (slot, cardfile.filenum, &cardstat);
	
	free(tmpBuffer);
	CARD_Close(&cardfile);
	return ret == CARD_ERROR_READY ? length : ret;
}

void setCopyGCIMode(bool _isCopyGCIMode) {
	isCopyGCIMode = _isCopyGCIMode;
}

void setGCIInfo(void *buffer) {
	if(buffer == NULL) {
		if(gciInfo != NULL) {
			free(gciInfo);
			gciInfo = NULL;
		}
	}
	else {
		gciInfo = (GCI*)memalign(32, sizeof(GCI));
		memcpy(gciInfo, buffer, sizeof(GCI));
	}
}

char getGCIRegion(const char *gameID)
{
	if (!strncmp(gameID, "DOLX00", 6) || !strncmp(gameID, "SWISS0", 6))
		return 'A';

	switch (gameID[3]) {
		case 'J':
		case 'K':
		case 'W':
			return 'J';
		case 'E':
			return 'E';
		case 'D':
		case 'F':
		case 'H':
		case 'I':
		case 'P':
		case 'S':
		case 'U':
		case 'X':
		case 'Y':
			return 'P';
		case 'A':
			return 'A';
		default:
			return '?';
	}
}

s32 deviceHandler_CARD_setupFile(file_handle* file, file_handle* file2, int numToPatch) {
	return 1;
}

s32 deviceHandler_CARD_init(file_handle* file){
	int slot = (!strncmp((const char*)initial_CARDB.name, file->name, 7));
	file->status = initialize_card(slot);
	s32 memSize = 0, sectSize = 0;
	int ret = CARD_ProbeEx(slot,&memSize,&sectSize);
	if(ret==CARD_ERROR_READY) {
		initial_CARD_info[slot].totalSpace = (u64)(memSize<<17);
	} else {
		print_gecko("CARD_ProbeEx failed %i\r\n", ret);
	}
	initial_CARD_info[slot].freeSpace = 0LL;
	
	return file->status == CARD_ERROR_READY ? 1 : 0;
}

s32 deviceHandler_CARD_deinit(file_handle* file) {
	if(file) {
		int slot = (!strncmp((const char*)initial_CARDB.name, file->name, 7));
		card_init[slot] = 0;
		CARD_Unmount(slot);
	}
	return 0;
}

s32 deviceHandler_CARD_deleteFile(file_handle* file) {
	int slot = (!strncmp((const char*)initial_CARDB.name, file->name, 7));
	char *filename = getRelativeName(file->name);
	card_dir* cd = (card_dir*)&file->other;
	
	if(cd->company[0] == '\0' && cd->gamecode[0] == '\0') {
		// Find the file we don't know about and populate this file_handle if we find it.
		if(!findFile(file)) {
			return CARD_ERROR_NOFILE;
		}
		CARD_SetCompany((const char*)cd->company);
		CARD_SetGamecode((const char*)cd->gamecode);
	}
	else {
		CARD_SetCompany((const char*)cd->company);
		CARD_SetGamecode((const char*)cd->gamecode);
	}
	print_gecko("Deleting: %s from slot %i\r\n", filename, slot);
	
	int ret = CARD_Delete(slot, filename);
	if(ret != CARD_ERROR_READY) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,cardError(ret));
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
	}
	return ret;
}

s32 deviceHandler_CARD_closeFile(file_handle* file) {
    return 0;
}

bool deviceHandler_CARD_test_a() {
	s32 memSize = 0, sectSize = 0;
	return ((initialize_card(0)==CARD_ERROR_READY) && (CARD_ProbeEx(0, &memSize,&sectSize)==CARD_ERROR_READY));
}

bool deviceHandler_CARD_test_b() {
	s32 memSize = 0, sectSize = 0;
	return ((initialize_card(1)==CARD_ERROR_READY) && (CARD_ProbeEx(1, &memSize,&sectSize)==CARD_ERROR_READY));
}

DEVICEHANDLER_INTERFACE __device_card_a = {
	DEVICE_ID_5,
	"Memory Card",
	"Memory Card - Slot A",
	"Backup & Restore save games",
	{TEX_MEMCARD, 76, 84},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_DEVICE,
	EMU_NONE,
	LOC_MEMCARD_SLOT_A,
	&initial_CARDA,
	(_fn_test)&deviceHandler_CARD_test_a,
	(_fn_info)&deviceHandler_CARD_info,
	(_fn_init)&deviceHandler_CARD_init,
	(_fn_readDir)&deviceHandler_CARD_readDir,
	(_fn_readFile)&deviceHandler_CARD_readFile,
	(_fn_writeFile)&deviceHandler_CARD_writeFile,
	(_fn_deleteFile)&deviceHandler_CARD_deleteFile,
	(_fn_seekFile)&deviceHandler_CARD_seekFile,
	(_fn_setupFile)&deviceHandler_CARD_setupFile,
	(_fn_closeFile)&deviceHandler_CARD_closeFile,
	(_fn_deinit)&deviceHandler_CARD_deinit,
	(_fn_emulated)NULL,
};

DEVICEHANDLER_INTERFACE __device_card_b = {
	DEVICE_ID_6,
	"Memory Card",
	"Memory Card - Slot B",
	"Backup & Restore save games",
	{TEX_MEMCARD, 76, 84},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_DEVICE,
	EMU_NONE,
	LOC_MEMCARD_SLOT_B,
	&initial_CARDB,
	(_fn_test)&deviceHandler_CARD_test_b,
	(_fn_info)&deviceHandler_CARD_info,
	(_fn_init)&deviceHandler_CARD_init,
	(_fn_readDir)&deviceHandler_CARD_readDir,
	(_fn_readFile)&deviceHandler_CARD_readFile,
	(_fn_writeFile)&deviceHandler_CARD_writeFile,
	(_fn_deleteFile)&deviceHandler_CARD_deleteFile,
	(_fn_seekFile)&deviceHandler_CARD_seekFile,
	(_fn_setupFile)&deviceHandler_CARD_setupFile,
	(_fn_closeFile)&deviceHandler_CARD_closeFile,
	(_fn_deinit)&deviceHandler_CARD_deinit,
	(_fn_emulated)NULL,
};
