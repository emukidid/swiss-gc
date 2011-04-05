/* deviceHandler-FAT.c
	- device implementation for FAT (via SDGecko/IDE-EXI)
	by emu_kidid
 */

#include <fat.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <sys/dir.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "ata.h"
#include "frag.h"

const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;
const DISC_INTERFACE* ideexia = &__io_ataa;
const DISC_INTERFACE* ideexib = &__io_atab;


file_handle initial_SD0 =
	{ "sda:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0
	};
	
file_handle initial_SD1 =
	{ "sdb:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0
	};
	
file_handle initial_IDE0 =
	{ "idea:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0
	};
	
file_handle initial_IDE1 =
	{ "ideb:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0
	};

int deviceHandler_FAT_readDir(file_handle* ffile, file_handle** dir){	
  
	DIR_ITER* dp = diropen( ffile->name );
	if(!dp) return -1;
	struct stat fstat;
	
	// Set everything up to read
	char filename[MAXPATHLEN];
	int num_entries = 1, i = 0;
	num_entries = i = 1;
	*dir = malloc( num_entries * sizeof(file_handle) );
	strcpy((*dir)[0].name,"..");
	(*dir)[0].fileAttrib = IS_SPECIAL;
	
	// Read each entry of the directory
	while( dirnext(dp, filename, &fstat) == 0 ){
		// Make sure we have room for this one
		if(i == num_entries){
			++num_entries;
			*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
		}
		sprintf((*dir)[i].name, "%s/%s", ffile->name, filename);
		(*dir)[i].offset = 0;
		(*dir)[i].size     = fstat.st_size;
		(*dir)[i].fileAttrib   = (fstat.st_mode & S_IFDIR) ? IS_DIR : IS_FILE;
		if((*dir)[i].fileAttrib == IS_FILE) {
			get_frag_list((*dir)[i].name);
			u32 file_base = frag_list->num > 1 ? -1 : frag_list->frag[0].sector;
			(*dir)[i].fileBase = file_base;
		}
		else {
			(*dir)[i].fileBase = 0;
		}		
		++i;
	}
	
	dirclose(dp);

	int isSDCard = ffile->name[0] == 's';
	if(isSDCard) {
	    // Set the card type (either block addressed (0) or byte addressed (1))
	    SDHCCard = sdgecko_getAddressingType(GC_SD_CHANNEL);
	    // set the page size to 512 bytes
	    if(sdgecko_setPageSize(GC_SD_CHANNEL, 512)!=0) {
	      DrawFrameStart();
	      DrawMessageBox(D_WARN,"Failed to set the page size");
	      DrawFrameFinish();
	      sleep(2);
	    }
	}
	
  return num_entries;
}

int deviceHandler_FAT_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_FAT_readFile(file_handle* file, void* buffer, unsigned int length){
  	FILE* f = fopen( file->name, "rb" );
	if(!f) return -1;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

FILE *writeFile = NULL;

int deviceHandler_FAT_writeFile(file_handle* file, void* buffer, unsigned int length){
  	if(writeFile == NULL) {
		writeFile = fopen( file->name, "wb" );
		fseek(writeFile, file->offset, SEEK_SET);
	}
	if(!writeFile) return -1;
		
	int bytes_written = fwrite(buffer, 1, length, writeFile);
	if(bytes_written > 0) file->offset += bytes_written;
	
	return bytes_written;
}

int unlockedDVD = 0;
void unlockCB() {
  unlockedDVD = 1;
}

void deviceHandler_FAT_setupFile(file_handle* file, file_handle* file2) {
  	u32 file1_base = 0, file2_base = 0, file1_count = 0, file2_count = 0;
  	
  	// Check for fragments - currently we only support 1
	get_frag_list(file->name);
	file1_base = frag_list->frag[0].sector;
	file1_count = frag_list->num;
  	if(file2) {
  		get_frag_list(file2->name);
  		file2_base = frag_list->frag[0].sector;
  		file2_count = frag_list->num;
	}
	
	if(file1_count > 1) {
		DrawFrameStart();
		sprintf(txtbuffer, "CANNOT RUN FRAGMENTED FILE! %s", file->name);
		DrawMessageBox(D_INFO,txtbuffer);
		DrawFrameFinish();
	}
	if(file2 && (file2_count > 1)) {
		DrawFrameStart();
		sprintf(txtbuffer, "CANNOT RUN FRAGMENTED FILE DISC 2! %s", file2->name);
		DrawMessageBox(D_INFO,txtbuffer);
		DrawFrameFinish();
	}
	
  // Disk 1 sector
  *(volatile unsigned int*)0x80002F00 = (u32)(file1_base&0xFFFFFFFF);
  // Disk 2 sector
  *(volatile unsigned int*)0x80002F10 = file2 ? (u32)(file2_base&0xFFFFFFFF):(u32)(file1_base&0xFFFFFFFF);
  // Currently selected disk sector
  *(volatile unsigned int*)0x80002F20 = (u32)(file1_base&0xFFFFFFFF);
  // Copy the current speed
  *(volatile unsigned int*)0x80002F30 = (GC_SD_SPEED == EXI_SPEED16MHZ) ? 192:208;
  // Copy disc headers
  file->offset = 0;
  deviceHandler_FAT_readFile(file, (void*)0x80002F80, 32);
  file->offset = 0;
  deviceHandler_FAT_readFile(file, (void*)0x80002FA0, 32);
  if(file2) {
    file2->offset = 0;
    deviceHandler_FAT_readFile(file2, (void*)0x80002FA0, 32);
  }
}

int EXI_ResetSD(int drv) {
	/* Wait for any pending transfers to complete */
	while ( *(vu32 *)0xCC00680C & 1 );
	while ( *(vu32 *)0xCC006820 & 1 );
	while ( *(vu32 *)0xCC006834 & 1 );
	*(vu32 *)0xCC00680C = 0xC0A;
	*(vu32 *)0xCC006820 = 0xC0A;
	*(vu32 *)0xCC006834 = 0x80A;
	/*** Needed to re-kick after insertion etc ***/
	EXI_ProbeEx(drv);
	return 1;
}

int deviceHandler_FAT_init(file_handle* file){
	int isSDCard = file->name[0] == 's';
	int slot = isSDCard ? (file->name[2] == 'b') : (file->name[3] == 'b');
	
	DrawFrameStart();
	sprintf(txtbuffer, "Starting up %s for slot %s", isSDCard ? "SD":"IDE-EXI", !slot ? "A":"B");
	DrawMessageBox(D_INFO,txtbuffer);
	DrawFrameFinish();
	
	// Slot A - SD Card
	if(isSDCard && !slot && EXI_ResetSD(0) && carda->startup()) {
		return fatMountSimple ("sda", carda);
	}
	// Slot B - SD Card
	if(isSDCard && slot && EXI_ResetSD(1) && cardb->startup()) {
		return fatMountSimple ("sdb", cardb);
	}
	// Slot A - IDE-EXI
	if(!isSDCard && !slot && ideexia->startup()) {
		return fatMountSimple ("idea", ideexia);
	}
	// Slot B - IDE-EXI
	if(!isSDCard && slot && ideexib->startup()) {
		return fatMountSimple ("ideb", ideexib);
	}
	return 0;
}

int deviceHandler_FAT_deinit(file_handle* file) {
	if(writeFile) {
		fclose(writeFile);
		writeFile = NULL;
	}
	return 0;
}

