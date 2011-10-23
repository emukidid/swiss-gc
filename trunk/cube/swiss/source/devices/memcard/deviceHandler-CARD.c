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
#include "main.h"


file_handle initial_CARDA =
	{ "\\",       // directory
	  0, 		  // slot
	  0,          // offset
	  0,          // size
	  IS_DIR,
	  0
};

file_handle initial_CARDB =
	{ "\\",       // directory
	  0x01000000, // slot
	  0,          // offset
	  0,          // size
	  IS_DIR,
	  0
};

static unsigned char *sys_area = NULL;
static unsigned int sector_size = 0;
static int card_init[2] = {0,0};


void card_removed_cb(s32 chn, s32 result) {  
  card_init[chn] = 0; 
  CARD_Unmount(chn); 
}

char *cardError(int error_code) {
  switch(error_code) {
    case CARD_ERROR_BUSY:
      return "Memory Card Busy";
    case CARD_ERROR_WRONGDEVICE:
      return "Wrong Device Inserted";
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

int initialize_card(int slot) {
  int slot_error = CARD_ERROR_READY, i = 0;
  
  if(!card_init[slot]) {
    /* Pass company identifier and number */
    CARD_Init ("SWIE", "SS");
  	if(!sys_area) sys_area = memalign(32,CARD_WORKAREA);
      
    /* Lets try 50 times to mount it. Sometimes it takes a while */
   	for(i = 0; i<50; i++) {
   		slot_error = CARD_Mount (slot, sys_area, card_removed_cb);
   		if(slot_error == CARD_ERROR_READY) {
        CARD_GetSectorSize (slot, &sector_size);
     		break;
   		}
   	}
	}
	card_init[slot] = slot_error == CARD_ERROR_READY ? 1 : 0;
	return slot_error;
}


int deviceHandler_CARD_readDir(file_handle* ffile, file_handle** dir){	

	int num_entries = 1, ret = 0, i = 1, slot = ffile->fileBase>>24;
	card_dir *memcard_dir = NULL;
  
	if(!card_init[slot]) { //if some error
		ret = initialize_card(slot);
		if(ret != CARD_ERROR_READY) {
			return -1; //fail
		}
	}
  
	memcard_dir = (card_dir*)memalign(32,sizeof(card_dir));
	memset(memcard_dir, 0, sizeof(card_dir));
 	
	/* Convert the Memory Card "file" data to fileBrowser_files */
	*dir = malloc( num_entries * sizeof(file_handle) );
	// Virtual Entry for entire card.
	sprintf((*dir)[0].name,"RAW Image");
	(*dir)[0].fileAttrib = IS_SPECIAL;
	(*dir)[0].fileBase     = ffile->fileBase;
	
	ret = CARD_FindFirst (slot, memcard_dir, true);
	while (CARD_ERROR_NOFILE != ret) {
		// Make sure we have room for this one
		if(i == num_entries){
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
		}
		strcpy( (*dir)[i].name, (char*)memcard_dir->filename);
		(*dir)[i].name[CARD_FILENAMELEN] = '\0';
		strcat( (*dir)[i].name, ".gci");
		(*dir)[i].name[CARD_FILENAMELEN+4] = '\0';
		(*dir)[i].fileAttrib = IS_FILE;
		(*dir)[i].offset = 0;
		(*dir)[i].size     = memcard_dir->filelen;
		(*dir)[i].fileBase     = ffile->fileBase | (memcard_dir->fileno & 0xFFFFFF);
		ret = CARD_FindNext (memcard_dir);
		++i;
	}
	free(memcard_dir);

	return num_entries;
}

int deviceHandler_CARD_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_CARD_readFile(file_handle* file, void* buffer, unsigned int length){
	char *read_buffer = NULL;
	card_file *memcard_file = NULL;
	unsigned int slot = file->fileBase>>24, ret = 0;
	
	memcard_file = (card_file*)memalign(32,sizeof(card_file));
	if(!memcard_file) {
		return -2;
	}
	memset(memcard_file, 0, sizeof(card_file));
	file->name[strlen(file->name)-5]=0;
	
	if(CARD_Open(slot, (const char*)txtbuffer, memcard_file) != CARD_ERROR_NOFILE){
		/* Allocate the read buffer */
		read_buffer = memalign(32,memcard_file->len);
		if(!read_buffer) {
			free(memcard_file);
			return -2;
		}
		
		/* Read the file */
		ret = CARD_Read(memcard_file,read_buffer, memcard_file->len, 0);
		if(!ret) {
			memcpy(buffer,read_buffer+file->offset,length);
			file->offset += length;
		}
	}
	CARD_Close(memcard_file);
	free(read_buffer);
	free(memcard_file);      

	return !ret ? length : ret;
}

int deviceHandler_CARD_writeFile(file_handle* file, void* buffer, unsigned int length){
	return 0;
}

void deviceHandler_CARD_setupFile(file_handle* file, file_handle* file2) {
  // Nothing here for this device..
}

int deviceHandler_CARD_init(file_handle* file){
	file->status = initialize_card(file->fileBase>>24);
	if(file->status < CARD_ERROR_READY){
		DrawFrameStart();
		DrawMessageBox(D_FAIL,cardError(file->status));
		DrawFrameFinish();
		wait_press_A();
		return file->status;
	}
	return file->status;
}

int deviceHandler_CARD_deinit(file_handle* file) {
	card_init[file->fileBase] = 0;
	return CARD_Unmount(file->fileBase>>24);
}

int deviceHandler_CARD_deleteFile(file_handle* file) {
	card_dir *_file = NULL;	
	_file = (card_dir*)memalign(32,sizeof(card_dir));
	memset(_file, 0, sizeof(card_dir));
	_file->fileno = file->fileBase&0xFFFFFF;
	
	sprintf(txtbuffer, "Deleting: %s\r\n", file->name);
	print_gecko(txtbuffer);
	
	file->status = CARD_DeleteEntry(file->fileBase>>24,_file);
	if(file->status != CARD_ERROR_READY) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,cardError(file->status));
		DrawFrameFinish();
		wait_press_A();
	}
	free(_file);
	return file->status;
}

