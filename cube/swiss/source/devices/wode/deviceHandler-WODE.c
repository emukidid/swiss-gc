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
#include "swiss.h"
#include "dvd.h"
#include "WodeInterface.h"

char wode_regions[]  = {'J','E','P','U','K'};
char *wode_regions_str[] = {"JPN","USA","EUR","UNK","KOR"};
char disktype[] = {'?', 'G','W','W','I' };
int wodeInited = 0;

file_handle initial_WODE =
	{ "wode:/",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  DRV_ERROR
	};
device_info initial_WODE_info = {
	0,
	0
};
	
int startupWode() {
	if(OpenWode() == 0) {
		CloseWode();
		if(OpenWode() == 0) {
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"No Wode found! Press A");
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
			return -1;
		}
	}
	// Wode initialised, return success
	device_versions *wode_version_info = calloc(1, sizeof(device_versions));
	if(GetVersions(wode_version_info)) {
		print_gecko("WODE initialised: Loader:%04X WODE:%04X FPGA:%04X HW:%02X\r\n",
			wode_version_info->loader_version, wode_version_info->wode_version,
			wode_version_info->fpga_version, wode_version_info->hw_version);
		return 0;
	}
	return -1;
}

device_info* deviceHandler_WODE_info(file_handle* file) {
	return &initial_WODE_info;
}
	
s32 deviceHandler_WODE_readDir(file_handle* ffile, file_handle** dir, u32 type){	

	if(!wodeInited) return 0;
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading WODE"));
	
	//we don't care about partitions, just files!
	while(!GetTotalISOs()) {
		usleep(20000);
	}
   
	u32 numPartitions = 0, numIsoInPartition = 0, i,j, num_entries = 1;
	*dir = malloc( num_entries * sizeof(file_handle) );
	memset(&(*dir)[0], 0, sizeof(file_handle));
	strcpy((*dir)[0].name,"..");
	(*dir)[0].fileAttrib = IS_SPECIAL;

	numPartitions = GetNumPartitions();
	for(i=0;i<numPartitions;i++) {
		numIsoInPartition = GetNumISOsInSelectedPartition(i);
		for(j=0;j<numIsoInPartition;j++) {
			ISOInfo_t tmp;
			GetISOInfo(i, j, &tmp);
			tmp.iso_partition = i;
			tmp.iso_number = j;
			if(tmp.iso_type==1) { //add gamecube only
				*dir = realloc( *dir, (num_entries+1) * sizeof(file_handle) ); 
				memset(&(*dir)[num_entries], 0, sizeof(file_handle));
				sprintf((*dir)[num_entries].name, "%s.gcm",&tmp.name[0]);
				(*dir)[num_entries].fileAttrib = IS_FILE;
				(*dir)[num_entries].size = DISC_SIZE;
				memcpy(&(*dir)[num_entries].other, &tmp, sizeof(ISOInfo_t));
				print_gecko("Adding WODE entry: %s part:%08X iso:%08X region:%08X\r\n",
					&tmp.name[0], tmp.iso_partition, tmp.iso_number, tmp.iso_region);
				num_entries++;
			}
		}
	}
	DrawDispose(msgBox);
	initial_WODE_info.totalSpaceInKB = num_entries;
	return num_entries;
}

s32 deviceHandler_WODE_seekFile(file_handle* file, unsigned int where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_WODE_readFile(file_handle* file, void* buffer, u32 length) {
	s32 bytesread = DVD_Read(buffer,file->offset,length);
	if(bytesread > 0) {
		file->offset += bytesread;
	}
	return bytesread;
}

s32 deviceHandler_WODE_setupFile(file_handle* file, file_handle* file2, int numToPatch) {
	ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
	SetISO(isoInfo->iso_partition,isoInfo->iso_number);
	sleep(2);
	DVD_Reset(DVD_RESETHARD);
	while(dvd_get_error()) {dvd_read_id();}
	return 1;
}

s32 deviceHandler_WODE_init(file_handle* file){
	wodeInited = startupWode() == 0 ? 1:0;
	initial_WODE_info.totalSpaceInKB = 0;
	return wodeInited;
}

s32 deviceHandler_WODE_deinit(file_handle* file) {
	return 0;
}

char wodeRegionToChar(int region) {
	return wode_regions[region];
}

char *wodeRegionToString(int region) {
	return wode_regions_str[region];
}

s32 deviceHandler_WODE_closeFile(file_handle* file) {
    return 0;
}

bool deviceHandler_WODE_test() {
	return swissSettings.hasDVDDrive && (*(u32*)&driveVersion[0] == 0x20080714);
}

DEVICEHANDLER_INTERFACE __device_wode = {
	DEVICE_ID_C,
	"WODE",
	"WODE Jukebox",
	"Supported File System(s): FAT32, NTFS, EXT2/3, HPFS",
	{TEX_WODEIMG, 116, 40},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE,
	EMU_NONE,
	LOC_DVD_CONNECTOR,
	&initial_WODE,
	(_fn_test)&deviceHandler_WODE_test,
	(_fn_info)&deviceHandler_WODE_info,
	(_fn_init)&deviceHandler_WODE_init,
	(_fn_readDir)&deviceHandler_WODE_readDir,
	(_fn_readFile)&deviceHandler_WODE_readFile,
	(_fn_writeFile)NULL,
	(_fn_deleteFile)NULL,
	(_fn_seekFile)&deviceHandler_WODE_seekFile,
	(_fn_setupFile)&deviceHandler_WODE_setupFile,
	(_fn_closeFile)&deviceHandler_WODE_closeFile,
	(_fn_deinit)&deviceHandler_WODE_deinit
};
