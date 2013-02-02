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


int deviceHandler_CARD_readDir(file_handle* ffile, file_handle** dir, unsigned int type){	

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
		(*dir)[i].size     = memcard_dir->filelen + sizeof(GCI);
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

// This is only called where length = file.size
int deviceHandler_CARD_readFile(file_handle* file, void* buffer, unsigned int length){
	unsigned int slot = file->fileBase>>24, ret = 0, file_no = file->fileBase&0xFFFFFF;
	void *dst = buffer;
	
	// Get the sector size
	u32 SectorSize = 0;
	CARD_GetSectorSize (slot, &SectorSize);
	
	// Create the .gci header
	card_stat CardStat;
	GCI gci;
	CARD_GetStatus(slot,file_no,&CardStat);        
	memset(&gci, 0xFF, sizeof(GCI));
	/*** Populate the GCI ***/
	memcpy(&gci.gamecode, &CardStat.gamecode, 4);
	memcpy(&gci.company, &CardStat.company, 2);
	gci.banner_fmt = CardStat.banner_fmt;
	memcpy(&gci.filename, &CardStat.filename, 0x20);
	gci.time = CardStat.time;
	gci.icon_addr = CardStat.icon_addr;
	gci.icon_fmt = CardStat.icon_fmt;
	gci.icon_speed = CardStat.icon_speed;
	gci.unknown1 = gci.unknown2 = 0;
	gci.index = 32;
	gci.filesize8 = (CardStat.len / 8192);
	gci.comment_addr = CardStat.comment_addr;
	memcpy(dst, &gci, sizeof(GCI));
	dst+= sizeof(GCI);
	
	// Re-init the card with the original file gamecode & company
	char game_code[16];	
	memcpy(&game_code[0],&gci.gamecode, 4);
	game_code[4] = 0;
	CARD_SetGamecode((const char *)&game_code[0]);
	memcpy(&game_code[0],&gci.company, 2);
	game_code[2] = 0;
	CARD_SetCompany((const char *)&game_code[0]);
	
	// Use the original file name
	char file_name[CARD_FILENAMELEN];	
	memset(&file_name[0],0,CARD_FILENAMELEN);
	memcpy(&file_name[0], file->name,strlen(file->name)-4);
	
	print_gecko("Try to open: [%s]\r\n",&file_name[0]);

	// Read the actual file data now
	char *read_buffer = NULL;
	card_file *memcard_file = NULL;
	memcard_file = (card_file*)memalign(32,sizeof(card_file));
	if(!memcard_file) {
		return -2;
	}
	memset(memcard_file, 0, sizeof(card_file));
	
	// Open the Entry
	if((ret = CARD_Open(slot, (const char*)&file_name[0], memcard_file)) != CARD_ERROR_NOFILE){
		/* Allocate the read buffer */
		u32 readSize = ((memcard_file->len % SectorSize) != 0) ? 
						((memcard_file->len/SectorSize)+1) * SectorSize : memcard_file->len;
		read_buffer = memalign(32,SectorSize);
		if(!read_buffer) {
			free(memcard_file);
			return -2;
		}
		
		print_gecko("Reading: [%i] bytes\r\n",readSize);
		
		/* Read the file */
		int i = 0;
		for(i=0;i<(readSize/SectorSize);i++) {
			ret = CARD_Read(memcard_file,read_buffer, SectorSize, i*SectorSize);
			if(!ret) {
				memcpy(dst,read_buffer,(i==(readSize/SectorSize)-1) ? (memcard_file->len % SectorSize):SectorSize);
			}
			dst+=SectorSize;
			print_gecko("Read: [%i] bytes ret [%i]\r\n",SectorSize,ret);
			
		}
	}
	else {
		print_gecko("ret: [%i]\r\n",ret);
		
	}
	CARD_Close(memcard_file);
	free(read_buffer);
	free(memcard_file);      

	return !ret ? length : ret;
}

// Only accepts a .gci file
int deviceHandler_CARD_writeFile(file_handle* file, void* data, unsigned int length) {
	card_stat CardStatus;
	card_dir CardDir;
	card_file CardFile;
	GCI gci; 
	int err = 0;
	unsigned int SectorSize = 0, slot = file->fileBase>>24;
	char tmpBuf[5];
	
	/*** Clear out the status ***/
	memset(&CardStatus, 0, sizeof(card_stat));
	memcpy(&gci, data, sizeof(GCI));

	memcpy(&CardStatus.gamecode, &gci.gamecode, 4);
	memcpy(&CardStatus.company, &gci.company, 2);
	CardStatus.banner_fmt = gci.banner_fmt;
	memcpy(&CardStatus.filename, &gci.filename, CARD_FILENAMELEN);
	CardStatus.time = gci.time;
	CardStatus.icon_addr = gci.icon_addr;
	CardStatus.icon_fmt = gci.icon_fmt;
	CardStatus.icon_speed = gci.icon_speed;
	CardStatus.len = gci.filesize8 * 8192;
	CardStatus.comment_addr = gci.comment_addr;
	
	CARD_Init((char*)gci.gamecode,(char*)gci.company);
	memcpy(&tmpBuf[0],(char*)gci.gamecode,4);
	tmpBuf[4]='\0';
	CARD_SetGamecode(&tmpBuf[0]);
	memcpy(&tmpBuf[0],(char*)gci.company,2);
	tmpBuf[2]='\0';
	CARD_SetCompany(&tmpBuf[0]);
	err = CARD_Mount (slot, sys_area, NULL);
	if (err) return -1;

  	CARD_GetSectorSize (slot, &SectorSize);
  	
  	/*** If this file exists, abort ***/
	err = CARD_FindFirst (slot, &CardDir, false);
	while (err != CARD_ERROR_NOFILE){
	    if (strcmp ((char *) CardDir.filename, (char *)gci.filename) == 0){
			/*** Found the file - abort ***/
		    CARD_Unmount (slot);
		    return -2;
		}
    	err = CARD_FindNext (&CardDir);
	}
    /*** Now restore the file from backup ***/
  	err = CARD_Create (slot, (char *) gci.filename, gci.filesize8 * 8192, &CardFile);
  	if (err){
    	CARD_Unmount (slot);
      	return -3;
  	}
  	
  	/*** Now write the file data, in sector sized chunks ***/
  	int offset = 0;
  	while (offset < (gci.filesize8 * 8192)){
    	if ((offset + SectorSize) <= (gci.filesize8 * 8192)) {
        	CARD_Write (&CardFile, data + 0x40 + offset, SectorSize, offset);
    	}
     	else {	
     		CARD_Write (&CardFile, data + 0x40 + offset, ((offset + SectorSize) - (gci.filesize8 * 8192)), offset);
     	}
        offset += SectorSize;
  	}
  
	/*** Finally, update the status ***/
	CARD_SetStatus (slot, CardFile.filenum, &CardStatus);
	
	CARD_Close (&CardFile);
	CARD_Unmount (slot);

	return length;
}

int deviceHandler_CARD_setupFile(file_handle* file, file_handle* file2) {
	// Nothing here for this device..
	return 1;
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
	return 1;
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
	
	print_gecko("Deleting: %s\r\n", file->name);
	
	
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

