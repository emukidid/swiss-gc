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
#include "swiss.h"
#include "main.h"
#include "patcher.h"
#include "dvd.h"
#include "WodeInterface.h"

char wode_regions[]  = {'J','E','P','A','K'};
char *wode_regions_str[] = {"JPN","USA","EUR","ALL","KOR"};
char disktype[] = {'?', 'G','W','W','I' };
s32 wodeInited = 0;

file_handle initial_WODE =
	{ "wode:/",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  DRV_ERROR
	};
device_info initial_WODE_info = {
	0LL,
	0LL,
	true
};
	
s32 startupWode() {
	if(OpenWode() == 0) {
		CloseWode();
		if(OpenWode() == 0) {
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"No WODE found! Press A");
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
			return ENODEV;
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
	return EIO;
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
	concat_path((*dir)[0].name, ffile->name, "..");
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
				concatf_path((*dir)[num_entries].name, ffile->name, "%.64s.gcm", &tmp.name[0]);
				(*dir)[num_entries].fileAttrib = IS_FILE;
				(*dir)[num_entries].size = DISC_SIZE;
				memcpy(&(*dir)[num_entries].other, &tmp, sizeof(ISOInfo_t));
				print_gecko("Adding WODE entry: %s part:%08X iso:%08X region:%08X\r\n",
					&tmp.name[0], tmp.iso_partition, tmp.iso_number, tmp.iso_region);
				num_entries++;
			}
		}
	}
	initial_WODE_info.totalSpace = num_entries;
	DrawDispose(msgBox);
	return num_entries;
}

s64 deviceHandler_WODE_seekFile(file_handle* file, s64 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

s32 deviceHandler_WODE_readFile(file_handle* file, void* buffer, u32 length) {
	s32 bytesread = DVD_Read(buffer,file->offset,length);
	if(bytesread > 0) {
		file->offset += bytesread;
	}
	return bytesread;
}

s32 deviceHandler_WODE_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	if(numToPatch < 0) {
		if(numToPatch == -1) {
			ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
			SetISO(isoInfo->iso_partition,isoInfo->iso_number);
			sleep(2);
			DVD_Reset(DVD_RESETHARD);
			while(dvd_get_error()) {dvd_read_id();}
		}
		return 1;
	}
	
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		int i;
		file_frag *fragList = NULL;
		u32 numFrags = 0;
		
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		for(i = 0; i < numToPatch; i++) {
			if(!filesToPatch[i].patchFile) continue;
			if(!getFragments(DEVICE_PATCHES, filesToPatch[i].patchFile, &fragList, &numFrags, filesToPatch[i].file == file2, filesToPatch[i].offset, filesToPatch[i].size)) {
				free(fragList);
				return 0;
			}
		}
		
		if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, FRAGS_DISC_1, 0, 0)) {
			free(fragList);
			return 0;
		}
		
		if(file2) {
			if(!getFragments(DEVICE_CUR, file2, &fragList, &numFrags, FRAGS_DISC_2, 0, 0)) {
				free(fragList);
				return 0;
			}
		}
		
		if(swissSettings.igrType == IGR_BOOTBIN || endsWith(file->name,".tgc")) {
			memset(&patchFile, 0, sizeof(file_handle));
			concat_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/apploader.img");
			
			ApploaderHeader apploaderHeader;
			if(devices[DEVICE_PATCHES]->readFile(&patchFile, &apploaderHeader, sizeof(ApploaderHeader)) != sizeof(ApploaderHeader) || apploaderHeader.rebootSize != reboot_bin_size) {
				devices[DEVICE_PATCHES]->deleteFile(&patchFile);
				
				memset(&apploaderHeader, 0, sizeof(ApploaderHeader));
				apploaderHeader.rebootSize = reboot_bin_size;
				
				devices[DEVICE_PATCHES]->seekFile(&patchFile, 0, DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_PATCHES]->writeFile(&patchFile, &apploaderHeader, sizeof(ApploaderHeader));
				devices[DEVICE_PATCHES]->writeFile(&patchFile, reboot_bin, reboot_bin_size);
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			
			getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_APPLOADER, 0x2440, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
		}
		
		if(swissSettings.emulateMemoryCard) {
			if(devices[DEVICE_PATCHES] != &__device_sd_a) {
				memset(&patchFile, 0, sizeof(file_handle));
				concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_A, 0, 31.5*1024*1024))
					*(vu8*)VAR_CARD_A_ID = (patchFile.size * 8/1024/1024) & 0xFC;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			
			if(devices[DEVICE_PATCHES] != &__device_sd_b) {
				memset(&patchFile, 0, sizeof(file_handle));
				concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardB.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_CARD_B, 0, 31.5*1024*1024))
					*(vu8*)VAR_CARD_B_ID = (patchFile.size * 8/1024/1024) & 0xFC;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		if(fragList) {
			print_frag_list(fragList, numFrags);
			*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (numFrags + 1) * sizeof(file_frag));
			free(fragList);
			fragList = NULL;
		}
		
		// Card Type
		*(vu8*)VAR_SD_SHIFT = (u8)(sdgecko_getAddressingType(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)) ? 9:0);
		// Copy the actual freq
		*(vu8*)VAR_EXI_FREQ = (u8)(sdgecko_getSpeed(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)));
		// Device slot (0, 1 or 2)
		*(vu8*)VAR_EXI_SLOT = (u8)(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2));
		*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[*(vu8*)VAR_EXI_SLOT];
	}
	
	if(file2 && file2->meta)
		memcpy(VAR_DISC_2_ID, &file2->meta->diskId, sizeof(VAR_DISC_2_ID));
	memcpy(VAR_DISC_1_ID, &GCMDisk, sizeof(VAR_DISC_1_ID));
	return 1;
}

s32 deviceHandler_WODE_init(file_handle* file) {
	int res = startupWode();
	wodeInited = !res ? 1:0;
	return res;
}

s32 deviceHandler_WODE_deinit(file_handle* file) {
	initial_WODE_info.totalSpace = 0LL;
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
	return swissSettings.hasDVDDrive && driveInfo.rel_date == 0x20080714;
}

u32 deviceHandler_WODE_emulated() {
	if (devices[DEVICE_PATCHES]) {
		if (swissSettings.emulateMemoryCard)
			return EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER;
		else
			return EMU_READ | EMU_BUS_ARBITER;
	} else
		return EMU_READ;
}

char* deviceHandler_WODE_status(file_handle* file) {
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_wode = {
	DEVICE_ID_C,
	"WODE",
	"WODE Jukebox",
	"Supported File System(s): FAT32, NTFS, EXT2/3, HPFS",
	{TEX_WODEIMG, 116, 40, 120, 48},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_HYPERVISOR,
	EMU_READ|EMU_MEMCARD,
	LOC_DVD_CONNECTOR,
	&initial_WODE,
	(_fn_test)&deviceHandler_WODE_test,
	(_fn_info)&deviceHandler_WODE_info,
	(_fn_init)&deviceHandler_WODE_init,
	(_fn_makeDir)NULL,
	(_fn_readDir)&deviceHandler_WODE_readDir,
	(_fn_seekFile)&deviceHandler_WODE_seekFile,
	(_fn_readFile)&deviceHandler_WODE_readFile,
	(_fn_writeFile)NULL,
	(_fn_closeFile)&deviceHandler_WODE_closeFile,
	(_fn_deleteFile)NULL,
	(_fn_renameFile)NULL,
	(_fn_setupFile)&deviceHandler_WODE_setupFile,
	(_fn_deinit)&deviceHandler_WODE_deinit,
	(_fn_emulated)&deviceHandler_WODE_emulated,
	(_fn_status)&deviceHandler_WODE_status,
};
