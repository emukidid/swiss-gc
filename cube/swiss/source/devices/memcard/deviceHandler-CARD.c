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
	  CARD_SLOTA, // slot
	  0,          // offset
	  0,          // size
	  IS_DIR,
	  FR_OK
};

file_handle initial_CARDB =
	{ "\\",       // directory
	  CARD_SLOTA, // slot
	  0,          // offset
	  0,          // size
	  IS_DIR,
	  FR_OK
};

static unsigned char *sys_area = NULL;
static unsigned int sector_size = 0;
static int card_init[2] = {1,1};


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

  int num_entries = 0, ret = 0, i = 0, slot = ffile->fileBase;
  card_dir *memcard_dir = NULL;
  
  if(!card_init[ffile->fileBase]) { //if some error
    ret = initialize_card(ffile->fileBase);
    if(ret != CARD_ERROR_READY) {
      return -1; //fail
    }
  }
  
  memcard_dir = (card_dir*)memalign(32,sizeof(card_dir));
  if(!memcard_dir) return -2;
  memset(memcard_dir, 0, sizeof(card_dir));
 	
  /* Count the Files */
  ret = CARD_FindFirst (slot, memcard_dir, true);
  while (CARD_ERROR_NOFILE != ret) {
    ret = CARD_FindNext (memcard_dir);		 
    num_entries++;
	}
		
	if(num_entries <= 0) { return num_entries; }
	
	/* Convert the Memory Card "file" data to fileBrowser_files */
  *dir = malloc( num_entries * sizeof(file_handle) );
  if(!dir) return -2;
  
  ret = CARD_FindFirst (slot, memcard_dir, true);
  while (CARD_ERROR_NOFILE != ret) {
    strncpy( (*dir)[i].name, (char*)memcard_dir->gamecode, 4);
    strncat( (*dir)[i].name, (char*)memcard_dir->company, 2);
  	(*dir)[i].name[6] = '\0';
  	//memcpy(&memcard_file_direntry[memcard_file_count],memcard_dir,sizeof(CardDir));
		ret = CARD_FindNext (memcard_dir);		
		i++;  
	}
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
	unsigned int slot = file->fileBase, ret = 0;
	
	memcard_file = (card_file*)memalign(32,sizeof(card_file));
	if(!memcard_file) {
  	return -2;
	}
	memset(memcard_file, 0, sizeof(card_file));
    
	if(CARD_Open(slot, (const char*)file->name, memcard_file) != CARD_ERROR_NOFILE){
	  /* Allocate the read buffer */
  	read_buffer = memalign(32,memcard_file->len);
  	if(!read_buffer) {
    	free(memcard_file);
    	return -2;
  	}
  	
	  /* Read the file */
	  ret = CARD_Read(memcard_file,read_buffer, memcard_file->len, 0);
		if(ret == 0) {
			memcpy(buffer,read_buffer+file->offset,length);
			file->offset += length;
		}
  }
  CARD_Close(memcard_file);
  free(read_buffer);
	free(memcard_file);      

  return !ret ? length : ret;
}

void deviceHandler_CARD_setupFile(file_handle* file, file_handle* file2) {
  // Nothing here for this device..
}

int deviceHandler_CARD_init(file_handle* file){
  file->status = initialize_card(file->fileBase);
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
	return CARD_Unmount(file->fileBase);
}

