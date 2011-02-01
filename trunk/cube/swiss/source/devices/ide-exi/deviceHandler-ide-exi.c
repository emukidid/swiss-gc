/* deviceHandler-DVD-Emu.c
	- device implementation for DVD-Emulator
	by emu_kidid
 */


#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "dvd.h"

extern FATFS *_getffs();


file_handle initial_IDEEXI0 =
	{ "0:",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  FR_OK
	};
	
file_handle initial_IDEEXI1 =
	{ "1:",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  FR_OK
	};

u32 clust2sector (	/* !=0: sector number, 0: failed - invalid cluster# */
  u32 clst		/* Cluster# to be converted */
)
{
	clst -= 2;
	if (clst >= ((*_getffs()).n_fatent - 2)) return 0;		/* Invalid cluster# */
	return clst * (*_getffs()).csize + (*_getffs()).database;
}
	
int deviceHandler_IDEEXI_readDir(file_handle* ffile, file_handle** dir){	
  
  int i, num_entries;
  FRESULT res = 0;
  FILINFO fno;
  FIL tempFile;
  DIR fatdir;
  char *fn;
  static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];

  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);  
  
  for(i = 0; i<10; i++) {
    res = f_opendir(&fatdir, ffile->name);
    if(res) break;
  }
  
  num_entries = i = 1;
  *dir = malloc( num_entries * sizeof(file_handle) );
  strcpy((*dir)[0].name,"..");
  (*dir)[0].fileAttrib = IS_SPECIAL;
  
  if (res == FR_OK) { 
    while ((f_readdir(&fatdir, &fno) == FR_OK) && (fno.fname[0] != 0)) {
      if(i == num_entries){
			  ++num_entries;
			  *dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
		  } 
      fn = *fno.lfname ? fno.lfname : fno.fname;
      sprintf((*dir)[i].name, "%s/%s", ffile->name, fn);
      f_open(&tempFile,(*dir)[i].name,FA_READ);
		  (*dir)[i].offset = 0;
		  (*dir)[i].size     = fno.fsize;
		  (*dir)[i].fileAttrib   = (fno.fattrib & AM_DIR) ? IS_DIR : IS_FILE;
		  (*dir)[i].fileBase = clust2sector(tempFile.org_clust);
		  f_close(&tempFile);
		  ++i;
    }
  }
  else if(res != FR_OK)
  {
    sprintf(txtbuffer,"Error %d check IDE-EXI. Press A",res);
    DrawFrameStart();
    DrawMessageBox(D_FAIL,txtbuffer);
    DrawFrameFinish();
    wait_press_A();
    num_entries = -1;
  }
  return num_entries;
}

int deviceHandler_IDEEXI_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_IDEEXI_readFile(file_handle* file, void* buffer, unsigned int length) {
  	FIL tempFile;
  	UINT bytesRead;
  
  	if(f_open(&tempFile,file->name,FA_READ) != FR_OK) {
    	return -1;
  	}
  	f_lseek(&tempFile, file->offset);
	if(f_read(&tempFile,buffer,length,&bytesRead) != FR_OK) {
  		return -1;
	}
	if(bytesRead > 0) {
		file->offset += bytesRead;
	}
	f_close(&tempFile);
	return bytesRead;
}

void deviceHandler_IDEEXI_setupFile(file_handle* file, file_handle* file2) {
    // Disk 1 sector
  *(volatile unsigned int*)0x80002F00 = (u32)(file->fileBase>>32&0xFFFFFFFF);
  *(volatile unsigned int*)0x80002F04 = (u32)(file->fileBase&0xFFFFFFFF);
  // Disk 2 sector
  *(volatile unsigned int*)0x80002F10 = file2 ? (u32)(file2->fileBase>>32&0xFFFFFFFF):(u32)(file->fileBase>>32&0xFFFFFFFF);
  *(volatile unsigned int*)0x80002F14 = file2 ? (u32)(file2->fileBase&0xFFFFFFFF):(u32)(file->fileBase&0xFFFFFFFF);
  // Currently selected disk sector
  *(volatile unsigned int*)0x80002F20 = (u32)(file->fileBase>>32&0xFFFFFFFF);
  *(volatile unsigned int*)0x80002F24 = (u32)(file->fileBase&0xFFFFFFFF);
  // Copy the current speed
  *(volatile unsigned int*)0x80002F30 = (GC_SD_SPEED == EXI_SPEED16MHZ) ? 192:208;
  // Copy disc headers
  file->offset = 0;
  deviceHandler_IDEEXI_readFile(file, (void*)0x80002F80, 32);
  file->offset = 0;
  deviceHandler_IDEEXI_readFile(file, (void*)0x80002FA0, 32);
  if(file2) {
    file2->offset = 0;
    deviceHandler_IDEEXI_readFile(file2, (void*)0x80002FA0, 32);
  }
}

int deviceHandler_IDEEXI_init(file_handle* file){
  DrawFrameStart();
  sprintf(txtbuffer, "Initializing IDE-EXI Slot %i", GC_SD_CHANNEL);
  DrawMessageBox(D_INFO,txtbuffer);
  DrawFrameFinish();
  memset(_getffs(),0,sizeof(FATFS));
  if(f_mount(GC_SD_CHANNEL, _getffs())!=FR_OK) {
    DrawFrameStart();
    DrawMessageBox(D_FAIL,"Error Mounting ...");
    DrawFrameFinish();
  }  
  return 0;
}

int deviceHandler_IDEEXI_deinit(file_handle* file) {
	return 0;
}

