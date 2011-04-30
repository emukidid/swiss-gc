/* deviceHandler-WODE.c
	- device implementation for WODE (GC Only)
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
#include "main.h"
#include "dvd.h"
#include "WodeInterface.h"

char regions[]  = {'J','U','E','?','K'};
char disktype[] = {'?', 'G','W','W','I' };

file_handle initial_WODE =
	{ "\\",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  DRV_ERROR
	};
	
int deviceHandler_WODE_readDir(file_handle* ffile, file_handle** dir){	
  //we don't care about partitions, just files!
  while(!GetTotalISOs()) {
    usleep(20000);
  }
  
  int numPartitions = 0, numIsoInPartition = 0, i,j, num_entries = 0;

  numPartitions = GetNumPartitions();
  for(i=0;i<numPartitions;i++) {
    numIsoInPartition = GetNumISOsInSelectedPartition(i);
    for(j=0;j<numIsoInPartition;j++) {
      ISOInfo_t tmp;
      GetISOInfo(i, j, &tmp);
      if(tmp.iso_type==1) { //add gamecube only
        u64 wode_iso_info = ((tmp.iso_type<<28) | (tmp.iso_region<<24) | (i<<16) | (j&0xFFFF));
        *dir = !num_entries ? malloc( sizeof(file_handle) ) : realloc( *dir, num_entries * sizeof(file_handle) );
        sprintf((*dir)[num_entries].name, "%s.gcm",&tmp.name[0]);
		    (*dir)[num_entries].fileBase = wode_iso_info;  //we use offset to store a few things
  	    (*dir)[num_entries].fileAttrib = IS_FILE;
  	    num_entries++;
      }
    }
  }
  return num_entries;
}

int deviceHandler_WODE_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

int deviceHandler_WODE_readFile(file_handle* file, void* buffer, unsigned int length){
	int bytesread = DVD_Read(buffer,file->offset,length);
	if(bytesread > 0) {
		file->offset += bytesread;
	}
	return bytesread;
}

void deviceHandler_WODE_setupFile(file_handle* file, file_handle* file2) {
  SetISO((file->fileBase>>16)&0xFF,file->fileBase&0xFFFF);
  sleep(2);
  DVD_Reset(DVD_RESETHARD);
  while(dvd_get_error()) {dvd_read_id();}
}

int deviceHandler_WODE_init(file_handle* file){
  if(OpenWode() == 0) {
    DrawFrameStart();
    DrawMessageBox(D_FAIL,"No Wode found! Press A");
    DrawFrameFinish();
    wait_press_A();
    return -1;
  }
  return 0;
}

int deviceHandler_WODE_deinit(file_handle* file) {
	return 0;
}

