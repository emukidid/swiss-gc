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

char wode_regions[]  = {'J','E','P','U','K'};
char disktype[] = {'?', 'G','W','W','I' };
int wodeInited = 0;

file_handle initial_WODE =
	{ "\\",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  DRV_ERROR
	};
device_info initial_WODE_info = {
	TEX_WODEIMG,
	0,
	0
};
	
int startupWode() {
	if(OpenWode() == 0) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"No Wode found! Press A");
		DrawFrameFinish();
		wait_press_A();
		return -1;
	}
	return 0;
}

device_info* deviceHandler_WODE_info() {
	return &initial_WODE_info;
}
	
int deviceHandler_WODE_readDir(file_handle* ffile, file_handle** dir, unsigned int type){	

	if(!wodeInited) return 0;
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Reading WODE");
	DrawFrameFinish();
	
	//we don't care about partitions, just files!
	while(!GetTotalISOs()) {
		usleep(20000);
	}
   
	u32 numPartitions = 0, numIsoInPartition = 0, i,j, num_entries = 0;

	numPartitions = GetNumPartitions();
	for(i=0;i<numPartitions;i++) {
		numIsoInPartition = GetNumISOsInSelectedPartition(i);
		for(j=0;j<numIsoInPartition;j++) {
			ISOInfo_t tmp;
			GetISOInfo(i, j, &tmp);
			tmp.iso_partition = i;
			tmp.iso_number = j;
			if(tmp.iso_type==1) { //add gamecube only
				*dir = !num_entries ? malloc( sizeof(file_handle) ) : realloc( *dir, num_entries * sizeof(file_handle) );
				memset(&(*dir)[num_entries], 0, sizeof(file_handle));
				sprintf((*dir)[num_entries].name, "%s.gcm",&tmp.name[0]);
				(*dir)[num_entries].fileAttrib = IS_FILE;
				(*dir)[i].meta = 0;
				memcpy(&(*dir)[num_entries].other, &tmp, sizeof(ISOInfo_t));
				num_entries++;
				print_gecko("Adding WODE entry: %s part:%08X iso:%08X region:%08X\r\n",
					&tmp.name[0], tmp.iso_partition, tmp.iso_number, tmp.iso_region);
			}
		}
	}
	initial_WODE_info.totalSpaceInKB = num_entries;
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

int deviceHandler_WODE_setupFile(file_handle* file, file_handle* file2) {
	ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
	SetISO(isoInfo->iso_partition,isoInfo->iso_number);
	sleep(2);
	DVD_Reset(DVD_RESETHARD);
	while(dvd_get_error()) {dvd_read_id();}
	return 1;
}

int deviceHandler_WODE_init(file_handle* file){
	wodeInited = startupWode() == 0 ? 1:0;
	initial_WODE_info.totalSpaceInKB = 0;
	return wodeInited;
}

int deviceHandler_WODE_deinit(file_handle* file) {
	return 0;
}

char wodeRegionToChar(int region) {
	return wode_regions[region];
}
