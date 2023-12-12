/* deviceHandler-gcloader.c
	- device implementation for GCLoader (FAT filesystem)
	by meneerbeer, emu_kidid
 */

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "deviceHandler-FAT.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "gcloader.h"
#include "patcher.h"
#include "dvd.h"

int gcloaderHwVersion;
char *gcloaderVersionStr;
static FATFS *gcloaderfs = NULL;

file_handle initial_GCLoader =
	{ "gcldr:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

s32 deviceHandler_GCLoader_readFile(file_handle* file, void* buffer, u32 length) {
	if(file->status == STATUS_MAPPED) {
		s32 bytes_read = DVD_ReadPrio((dvdcmdblk*)file->other, buffer, length, file->offset, 2);
		if(bytes_read > 0) file->offset += bytes_read;
		return bytes_read;
	}
	return deviceHandler_FAT_readFile(file, buffer, length);
}

static char *bootFile_names[] = {"boot.iso", "boot.iso.iso", "boot.gcm", "boot.gcm.gcm"};

static s32 setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	// GCLoader disc/file fragment setup
	file_frag *disc1FragList = NULL, *disc2FragList = NULL;
	u32 disc1Frags = 0, disc2Frags = 0;
	
	if(numToPatch < -1) {
		file_handle bootFile;
		memset(&bootFile, 0, sizeof(file_handle));
		concat_path(bootFile.name, devices[DEVICE_CUR]->initial->name, "boot.bin");
		
		if(!strncmp(&IPLInfo[0x55], "MPAL", 4))
			GCMDisk.RegionCode = 1;
		else if(!strncmp(&IPLInfo[0x55], "PAL ", 4))
			GCMDisk.RegionCode = 2;
		else
			GCMDisk.RegionCode = getFontEncode() ? 0:1;
		
		if(devices[DEVICE_CUR]->writeFile(&bootFile, &GCMDisk, sizeof(DiskHeader)) == sizeof(DiskHeader) &&
			!devices[DEVICE_CUR]->closeFile(&bootFile))
			getFragments(DEVICE_CUR, &bootFile, &disc1FragList, &disc1Frags, 0, 0, sizeof(DiskHeader));
		devices[DEVICE_CUR]->closeFile(&bootFile);
	}
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	if(!getFragments(DEVICE_CUR, file, &disc1FragList, &disc1Frags, 0, 0, UINT32_MAX))
		goto fail;
	
	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		if(!getFragments(DEVICE_CUR, file2, &disc2FragList, &disc2Frags, 1, 0, UINT32_MAX))
			goto fail;
		
		if(devices[DEVICE_PATCHES] == devices[DEVICE_CUR]) {
			file2->fileBase = (u32)installPatch2(disc2FragList, (disc2Frags + 1) * sizeof(file_frag)) | ((u64)disc2Frags << 32);
			file->fileBase  = (u32)installPatch2(disc1FragList, (disc1Frags + 1) * sizeof(file_frag)) | ((u64)disc1Frags << 32);
		}
	}
	
	// write disc 2 frags
	if(gcloaderWriteFrags(1, disc2FragList, disc2Frags))
		goto fail;
	
	// write disc 1 frags
	if(gcloaderWriteFrags(0, disc1FragList, disc1Frags))
		goto fail;
	
	// set disc 1 as active disc
	if(gcloaderWriteDiscNum(0))
		goto fail;
	
	free(disc2FragList);
	free(disc1FragList);
	file->status = STATUS_MAPPED;
	return 1;

fail:
	free(disc2FragList);
	free(disc1FragList);
	return 0;
}

s32 deviceHandler_GCLoader_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	if(!setupFile(file, file2, filesToPatch, numToPatch)) {
		goto fail;
	}
	if(numToPatch < 0) {
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
				goto fail;
			}
		}
		
		if(!getFragments(DEVICE_CUR, file, &fragList, &numFrags, FRAGS_DISC_1, 0, 0)) {
			free(fragList);
			goto fail;
		}
		
		if(file2) {
			if(!getFragments(DEVICE_CUR, file2, &fragList, &numFrags, FRAGS_DISC_2, 0, 0)) {
				free(fragList);
				goto fail;
			}
		}
		
		for(i = 0; i < sizeof(bootFile_names)/sizeof(char*); i++) {
			file_handle bootFile;
			memset(&bootFile, 0, sizeof(file_handle));
			concat_path(bootFile.name, devices[DEVICE_CUR]->initial->name, bootFile_names[i]);
			
			if(getFragments(DEVICE_CUR, &bootFile, &fragList, &numFrags, FRAGS_BOOT_GCM, 0, UINT32_MAX)) {
				devices[DEVICE_CUR]->closeFile(&bootFile);
				break;
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
		
		if(devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
			int slot = GET_SLOT(devices[DEVICE_PATCHES]->initial);
			// Card Type
			*(vu8*)VAR_SD_SHIFT = sdgecko_getAddressingType(slot) ? 0:9;
			// Copy the actual freq
			*(vu8*)VAR_EXI_FREQ = sdgecko_getSpeed(slot);
			// Device slot (0, 1 or 2)
			*(vu8*)VAR_EXI_SLOT = slot;
			*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[slot];
		}
	}
	
	if(file2 && file2->meta)
		memcpy(VAR_DISC_2_ID, &file2->meta->diskId, sizeof(VAR_DISC_2_ID));
	memcpy(VAR_DISC_1_ID, &GCMDisk, sizeof(VAR_DISC_1_ID));
	return 1;

fail:
	int i;
	for(i = 0; i < sizeof(bootFile_names)/sizeof(char*); i++) {
		file_handle bootFile;
		memset(&bootFile, 0, sizeof(file_handle));
		concat_path(bootFile.name, devices[DEVICE_CUR]->initial->name, bootFile_names[i]);
		
		if(setupFile(&bootFile, NULL, NULL, -1)) {
			devices[DEVICE_CUR]->closeFile(&bootFile);
			break;
		}
	}
	file->status = STATUS_NOT_MAPPED;
	return 0;
}

s32 deviceHandler_GCLoader_init(file_handle* file){
	if(gcloaderfs != NULL) {
		f_unmount("gcldr:/");
		free(gcloaderfs);
		gcloaderfs = NULL;
		disk_shutdown(DEV_GCLDR);
	}
	gcloaderfs = (FATFS*)malloc(sizeof(FATFS));
	file->status = f_mount(gcloaderfs, "gcldr:/", 1);
	return file->status == FR_OK ? 0 : EIO;
}

s32 deviceHandler_GCLoader_deinit(file_handle* file) {
	deviceHandler_FAT_closeFile(file);
	if(file) {
		f_unmount(file->name);
		free(gcloaderfs);
		gcloaderfs = NULL;
		disk_shutdown(DEV_GCLDR);
	}
	return 0;
}

bool deviceHandler_GCLoader_test() {
	free(gcloaderVersionStr);
	gcloaderVersionStr = NULL;
	gcloaderHwVersion = 0;
	
	if (swissSettings.hasDVDDrive && driveInfo.rel_date == 0x20196c64) {
		if (driveInfo.pad[1] == 'w')
			__device_gcloader.features |=  (FEAT_WRITE|FEAT_CONFIG_DEVICE|FEAT_PATCHES);
		else
			__device_gcloader.features &= ~(FEAT_WRITE|FEAT_CONFIG_DEVICE|FEAT_PATCHES);
		
		if (gcloaderReadId() == 0xAAAAAAAA) {
			gcloaderHwVersion = driveInfo.pad[2] + 1;
			gcloaderVersionStr = gcloaderGetVersion(driveInfo.pad[2]);
			
			if (gcloaderVersionStr) {
				switch (gcloaderHwVersion) {
					case 2:
						if (strverscmp(gcloaderVersionStr, "1.0.0") <= 0)
							__device_gcloader.features &= ~FEAT_PATCHES;
						break;
				}
			}
			__device_gcloader.hwName = "GC Loader";
		} else
			__device_gcloader.hwName = "GC Loader compatible";
		
		return true;
	}
	return false;
}

u32 deviceHandler_GCLoader_emulated() {
	if (devices[DEVICE_PATCHES]) {
		if (devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
			if (swissSettings.emulateReadSpeed)
				return EMU_READ | EMU_READ_SPEED | EMU_BUS_ARBITER;
			else if (swissSettings.emulateMemoryCard)
				return EMU_READ | EMU_MEMCARD | EMU_BUS_ARBITER;
			else
				return EMU_READ | EMU_BUS_ARBITER;
		} else {
			if (swissSettings.emulateReadSpeed)
				return EMU_READ | EMU_READ_SPEED;
			else if (swissSettings.emulateMemoryCard)
				return EMU_READ | EMU_MEMCARD;
			else
				return EMU_READ;
		}
	} else {
		if (swissSettings.emulateReadSpeed)
			return EMU_READ | EMU_READ_SPEED;
		else
			return EMU_READ;
	}
}

DEVICEHANDLER_INTERFACE __device_gcloader = {
	DEVICE_ID_G,
	"GC Loader",
	"GC Loader",
	"Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_GCLOADER, 115, 72, 120, 80},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_AUDIO_STREAMING,
	EMU_READ|EMU_READ_SPEED|EMU_MEMCARD,
	LOC_DVD_CONNECTOR,
	&initial_GCLoader,
	(_fn_test)&deviceHandler_GCLoader_test,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_GCLoader_init,
	(_fn_makeDir)&deviceHandler_FAT_makeDir,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_readFile)&deviceHandler_GCLoader_readFile,
	(_fn_writeFile)deviceHandler_FAT_writeFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deleteFile)deviceHandler_FAT_deleteFile,
	(_fn_renameFile)&deviceHandler_FAT_renameFile,
	(_fn_setupFile)&deviceHandler_GCLoader_setupFile,
	(_fn_deinit)&deviceHandler_GCLoader_deinit,
	(_fn_emulated)&deviceHandler_GCLoader_emulated,
	(_fn_status)&deviceHandler_FAT_status,
};
