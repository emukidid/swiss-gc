/* deviceHandler-WODE.c
	- device implementation for WODE (GC Only)
	by emu_kidid
 */

#include <stdlib.h>
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
#include "flippy.h"
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
		print_debug("WODE initialised: Loader:%04X WODE:%04X FPGA:%04X HW:%02X\n",
			wode_version_info->loader_version, wode_version_info->wode_version,
			wode_version_info->fpga_version, wode_version_info->hw_version);
		return 0;
	}
	return EIO;
}

device_info* deviceHandler_WODE_info(file_handle* file) {
	return NULL;
}
	
s32 deviceHandler_WODE_readDir(file_handle* ffile, file_handle** dir, u32 type){	
	if(type != -1) return -1;

	if(!wodeInited) return 0;
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading WODE"));
	
	//we don't care about partitions, just files!
	while(!GetTotalISOs()) {
		usleep(20000);
	}
   
	u32 numPartitions = 0, numIsoInPartition = 0, i,j, num_entries = 1;
	*dir = calloc(num_entries, sizeof(file_handle));
	concat_path((*dir)[0].name, ffile->name, "..");
	(*dir)[0].fileType = IS_SPECIAL;

	numPartitions = GetNumPartitions();
	for(i=0;i<numPartitions;i++) {
		numIsoInPartition = GetNumISOsInSelectedPartition(i);
		for(j=0;j<numIsoInPartition;j++) {
			ISOInfo_t tmp;
			GetISOInfo(i, j, &tmp);
			tmp.iso_partition = i;
			tmp.iso_number = j;
			if(tmp.iso_type==1) { //add gamecube only
				*dir = reallocarray(*dir, num_entries + 1, sizeof(file_handle));
				memset(&(*dir)[num_entries], 0, sizeof(file_handle));
				concatf_path((*dir)[num_entries].name, ffile->name, "%.64s.gcm", &tmp.name[0]);
				(*dir)[num_entries].size = DISC_SIZE;
				(*dir)[num_entries].fileType = IS_FILE;
				memcpy(&(*dir)[num_entries].other, &tmp, sizeof(ISOInfo_t));
				print_debug("Adding WODE entry: %s part:%08X iso:%08X region:%08X\n",
					&tmp.name[0], tmp.iso_partition, tmp.iso_number, tmp.iso_region);
				num_entries++;
			}
		}
	}
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
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	if(numToPatch < 0 || !devices[DEVICE_CUR]->emulated()) {
		if(file->status == STATUS_NOT_MAPPED) {
			ISOInfo_t* isoInfo = (ISOInfo_t*)&file->other;
			SetISO(isoInfo->iso_partition,isoInfo->iso_number);
			sleep(2);
			DVD_Reset(DVD_RESETHARD);
			while(dvd_get_error()) {dvd_read_id();}
			file->status = STATUS_MAPPED;
		}
		return 1;
	}
	
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		print_debug("Save Patch device found\n");
		
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
		
		if(swissSettings.igrType == IGR_APPLOADER || endsWith(file->name,".tgc")) {
			memset(&patchFile, 0, sizeof(file_handle));
			concat_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/apploader.img");
			
			getFragments(DEVICE_PATCHES, &patchFile, &fragList, &numFrags, FRAGS_APPLOADER, 0x2440, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
		}
		
		if(swissSettings.emulateMemoryCard) {
			if(devices[DEVICE_PATCHES] != &__device_sd_a) {
				memset(&patchFile, 0, sizeof(file_handle));
				concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				concatf_path(txtbuffer, devices[DEVICE_PATCHES]->initial->name, "swiss/saves/MemoryCardA.%s.raw", wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL, false);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->statFile(&patchFile)) {
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
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL, false);
				devices[DEVICE_PATCHES]->renameFile(&patchFile, txtbuffer);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->statFile(&patchFile)) {
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
		
		s32 exi_channel, exi_device;
		if(getExiDeviceByLocation(devices[DEVICE_PATCHES]->location, &exi_channel, &exi_device)) {
			exi_device = sdgecko_getDevice(exi_channel);
			// Card Type
			*(vu8*)VAR_SD_SHIFT = sdgecko_getAddressingType(exi_channel) ? 0:9;
			// Copy the actual freq
			*(vu8*)VAR_EXI_CPR = (exi_channel << 6) | ((1 << exi_device) << 3) | sdgecko_getSpeed(exi_channel);
			// Device slot (0, 1 or 2)
			*(vu8*)VAR_EXI_SLOT = (*(vu8*)VAR_EXI_SLOT & 0xF0) | (((exi_device << 2) | exi_channel) & 0x0F);
			*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[exi_channel];
		}
	}
	
	if(file2 && file2->meta)
		memcpy(VAR_DISC_2_ID, &file2->meta->diskId, sizeof(VAR_DISC_2_ID));
	memcpy(VAR_DISC_1_ID, &GCMDisk, sizeof(VAR_DISC_1_ID));
	return 1;
}

s32 deviceHandler_WODE_init(file_handle* file) {
	if(devices[DEVICE_CUR] == &__device_flippy || devices[DEVICE_CUR] == &__device_flippyflash) {
		return EBUSY;
	}
	if(swissSettings.hasFlippyDrive) flippy_bypass(true);
	if(!swissSettings.hasDVDDrive) return ENODEV;
	
	int res = startupWode();
	wodeInited = !res ? 1:0;
	return res;
}

s32 deviceHandler_WODE_deinit(file_handle* file) {
	return 0;
}

char wodeRegionToChar(int region) {
	return in_range(region, 0, 4) ? wode_regions[region] : '?';
}

char *wodeRegionToString(int region) {
	return in_range(region, 0, 4) ? wode_regions_str[region] : "UNK";
}

s32 deviceHandler_WODE_closeFile(file_handle* file) {
    return 0;
}

static const dvddiskid WODEExtCFG = {
	.gamename  = "GWDP",
	.company   = "CF",
	.magic     = DVD_MAGIC
};

bool deviceHandler_WODE_test() {
	return swissSettings.hasDVDDrive == 1 && !memcmp(DVDDiskID, &WODEExtCFG, sizeof(dvddiskid));
}

u32 deviceHandler_WODE_emulated() {
	if (devices[DEVICE_PATCHES]) {
		if (swissSettings.emulateMemoryCard)
			return EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER;
		else
			return EMU_READ | EMU_BUS_ARBITER;
	} else {
		if (swissSettings.disableHypervisor)
			return EMU_NONE;
		else
			return EMU_READ;
	}
}

char* deviceHandler_WODE_status(file_handle* file) {
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_wode = {
	.deviceUniqueId = DEVICE_ID_C,
	.hwName = "WODE",
	.deviceName = "WODE Jukebox",
	.deviceDescription = "Supported File System(s): FAT32, NTFS, EXT2/3, HPFS",
	.deviceTexture = {TEX_WODEIMG, 116, 40, 120, 40},
	.features = FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_HYPERVISOR,
	.emulable = EMU_READ|EMU_MEMCARD,
	.location = LOC_DVD_CONNECTOR,
	.initial = &initial_WODE,
	.test = deviceHandler_WODE_test,
	.info = deviceHandler_WODE_info,
	.init = deviceHandler_WODE_init,
	.readDir = deviceHandler_WODE_readDir,
	.seekFile = deviceHandler_WODE_seekFile,
	.readFile = deviceHandler_WODE_readFile,
	.closeFile = deviceHandler_WODE_closeFile,
	.setupFile = deviceHandler_WODE_setupFile,
	.deinit = deviceHandler_WODE_deinit,
	.emulated = deviceHandler_WODE_emulated,
	.status = deviceHandler_WODE_status,
};
