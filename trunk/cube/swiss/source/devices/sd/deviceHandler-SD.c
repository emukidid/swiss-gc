/* deviceHandler-SD.c
	- device implementation for SD (via SDGecko)
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

static FATFS _ffs;

file_handle initial_SD0 =
	{ "0:",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  FR_OK
	};
	
file_handle initial_SD1 =
	{ "1:",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  FR_OK
	};

DWORD mclust2sect (	/* !=0: sector number, 0: failed - invalid cluster# */
  DWORD clst		/* Cluster# to be converted */
)
{
	clst -= 2;
	if (clst >= (_ffs.n_fatent - 2)) return 0;		/* Invalid cluster# */
	return (clst * _ffs.csize + _ffs.database);
}
	
FATFS *_getffs() {
  return &_ffs;
}

int deviceHandler_SD_readDir(file_handle* ffile, file_handle** dir){	
  
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
		  (*dir)[i].fileBase = mclust2sect(tempFile.org_clust);
		  f_close(&tempFile);
		  ++i;
    }
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
  else if(res != FR_OK)
  {
    sprintf(txtbuffer,"Error %d (re)insert SDGecko. Press A",res);
    DrawFrameStart();
    DrawMessageBox(D_FAIL,txtbuffer);
    DrawFrameFinish();
    wait_press_A();
    num_entries = -1;
  }
  return num_entries;
}

int deviceHandler_SD_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_SD_readFile(file_handle* file, void* buffer, unsigned int length){
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

int unlockedDVD = 0;
void unlockCB() {
  unlockedDVD = 1;
}

void deviceHandler_SD_setupFile(file_handle* file, file_handle* file2) {
  // Disk 1 sector
  *(volatile unsigned int*)0x80002F00 = (u32)(file->fileBase&0xFFFFFFFF);
  // Disk 2 sector
  *(volatile unsigned int*)0x80002F10 = file2 ? (u32)(file2->fileBase&0xFFFFFFFF):(u32)(file->fileBase&0xFFFFFFFF);
  // Currently selected disk sector
  *(volatile unsigned int*)0x80002F20 = (u32)(file->fileBase&0xFFFFFFFF);
  // Copy the current speed
  *(volatile unsigned int*)0x80002F30 = (GC_SD_SPEED == EXI_SPEED16MHZ) ? 192:208;
  // Copy disc headers
  file->offset = 0;
  deviceHandler_SD_readFile(file, (void*)0x80002F80, 32);
  file->offset = 0;
  deviceHandler_SD_readFile(file, (void*)0x80002FA0, 32);
  if(file2) {
    file2->offset = 0;
    deviceHandler_SD_readFile(file2, (void*)0x80002FA0, 32);
  }
}

int deviceHandler_SD_init(file_handle* file){
  DrawFrameStart();
  DrawMessageBox(D_INFO,"Mounting SD card ...");
  DrawFrameFinish();
  memset(&_ffs,0,sizeof(FATFS));
  f_mount(GC_SD_CHANNEL, &_ffs);
  return 0;
}

int deviceHandler_SD_deinit(file_handle* file) {
	return 0;
}

