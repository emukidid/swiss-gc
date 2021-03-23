/* deviceHandler-FAT.c
	- device implementation for FAT (via SD Card Adapters/IDE-EXI)
	by emu_kidid
 */

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "ata.h"
#include "patcher.h"
#include "dvd.h"

const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;
const DISC_INTERFACE* cardc = &__io_gcsd2;
const DISC_INTERFACE* ideexia = &__io_ataa;
const DISC_INTERFACE* ideexib = &__io_atab;
extern FATFS *wkffs;
extern FATFS *gcloaderfs;
FATFS *fs[5] = {NULL, NULL, NULL, NULL, NULL};
extern void sdgecko_initIODefault();
#define SD_COUNT 3
#define IS_SDCARD(v) (v[0] == 's' && v[1] == 'd')
int GET_SLOT(char *path) {
	if(IS_SDCARD(path)) {
		return path[2] - 'a'; 
	}
	// IDE-EXI
	return path[3] == 'b';
}

file_handle initial_SD_A =
	{ "sda:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};
	
file_handle initial_SD_B =
	{ "sdb:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};
	
file_handle initial_SD_C =
	{ "sdc:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};	
	
file_handle initial_IDE_A =
	{ "idea:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};
	
file_handle initial_IDE_B =
	{ "ideb:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

static device_info initial_FAT_info[5];
	
device_info* deviceHandler_FAT_info(file_handle* file) {
	int isSDCard = IS_SDCARD(file->name);
	int slot = GET_SLOT(file->name);
	return &initial_FAT_info[(isSDCard ? slot : SD_COUNT + slot)];
}

void populateDeviceInfo(file_handle* file) {
	device_info* info = deviceHandler_FAT_info(file);
	
	if(deviceHandler_getStatEnabled()) {
		int isSDCard = IS_SDCARD(file->name);
		int slot = GET_SLOT(file->name);
		
		sprintf(txtbuffer, "Reading filesystem info for %s%c:/", isSDCard ? "sd":"ide", ('a'+slot));
		print_gecko(txtbuffer);
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));
		sprintf(txtbuffer, "%s%c:/",isSDCard ? "sd":"ide", ('a'+slot));
		DWORD free_clusters, free_sectors, total_sectors = 0;
		FATFS *fsptr = fs[(isSDCard ? slot : SD_COUNT + slot)];
		if(f_getfree(txtbuffer, &free_clusters, &fsptr) == FR_OK) {
			total_sectors = (fsptr->n_fatent - 2) * fsptr->csize;
			free_sectors = free_clusters * fsptr->csize;
			info->freeSpaceInKB = (u32)((free_sectors)>>1);
			info->totalSpaceInKB = (u32)((total_sectors)>>1);
		}
		else {
			info->freeSpaceInKB = info->totalSpaceInKB = 0;
		}
		DrawDispose(msgBox);
	}
}
	
s32 deviceHandler_FAT_readDir(file_handle* ffile, file_handle** dir, u32 type) {	

	DIRF* dp = malloc(sizeof(DIRF));
	memset(dp, 0, sizeof(DIRF));
	if(f_opendir(dp, ffile->name) != FR_OK) return -1;
	FILINFO entry;
	FILINFO fno;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	char file_name[PATHNAME_MAX];
	*dir = calloc(sizeof(file_handle), 1);
	(*dir)[0].fileAttrib = IS_SPECIAL;
	strcpy((*dir)[0].name, "..");
	memset(&entry, 0, sizeof(FILINFO));

	// Read each entry of the directory
	while( f_readdir(dp, &entry) == FR_OK && entry.fname[0] != '\0') {
		if(strlen(entry.fname) <= 2  && (entry.fname[0] == '.' || entry.fname[1] == '.')) {
			continue;
		}
		memset(&file_name[0],0,PATHNAME_MAX);
		snprintf(&file_name[0], PATHNAME_MAX, "%s/%s", ffile->name, entry.fname);
		if(f_stat(file_name, &fno) != FR_OK || (fno.fattrib & AM_HID) || entry.fname[0] == '.') {
			continue;
		}
		// Do we want this one?
		if((type == -1 || ((fno.fattrib & AM_DIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
			if(!(fno.fattrib & AM_DIR)) {
				if(!checkExtension(entry.fname)) continue;
			}
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			snprintf((*dir)[i].name, PATHNAME_MAX, "%s/%s", ffile->name, entry.fname);
			(*dir)[i].size     = fno.fsize;
			(*dir)[i].fileAttrib   = (fno.fattrib & AM_DIR) ? IS_DIR : IS_FILE;
			++i;
		}
	}
	
	f_closedir(dp);
	free(dp);
	return num_entries;
}

s32 deviceHandler_FAT_seekFile(file_handle* file, u32 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_FAT_readFile(file_handle* file, void* buffer, u32 length) {
  	if(!file->ffsFp) {
		file->ffsFp = malloc(sizeof(FIL));
		if(f_open(file->ffsFp, file->name, FA_READ ) != FR_OK) {
			free(file->ffsFp);
			file->ffsFp = 0;
			return -1;
		}
		if(file->size <= 0) {
			FILINFO fno;
			if(f_stat(file->name, &fno) != FR_OK) {
				free(file->ffsFp);
				file->ffsFp = 0;
				return -1;
			}
			file->size = fno.fsize;
		}
	}
	
	f_lseek(file->ffsFp, file->offset);
	if(length > 0) {
		UINT bytes_read = 0;
		if(f_read(file->ffsFp, buffer, length, &bytes_read) != FR_OK || bytes_read != length) {
			return -1;
		}
		file->offset += bytes_read;
		return bytes_read;
	}
	return 0;
}

s32 deviceHandler_FAT_writeFile(file_handle* file, void* buffer, u32 length) {
	if(!file->ffsFp) {
		file->ffsFp = malloc(sizeof(FIL));
		// Append
		if(f_open(file->ffsFp, file->name, FA_READ | FA_WRITE ) != FR_OK) {
			// Try to create
			if(f_open(file->ffsFp, file->name, FA_CREATE_NEW | FA_WRITE | FA_READ ) != FR_OK) {
				free(file->ffsFp);
				file->ffsFp = 0;
				return -1;
			}
		}
	}
	f_lseek(file->ffsFp, file->offset);
		
	UINT bytes_written = 0;
	if(f_write(file->ffsFp, buffer, length, &bytes_written) != FR_OK || bytes_written != length) {
		return -1;
	}
	file->offset += bytes_written;
	return bytes_written;
}

void print_frag_list(int hasDisc2) {
	print_gecko("== Fragments List ==\r\n");
	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	int maxFrags = hasDisc2 ? ((sizeof(VAR_FRAG_LIST)/12)/2) : (sizeof(VAR_FRAG_LIST)/12), i = 0;
	for(i = 0; i < maxFrags; i++) {
		if(!fragList[(i*3)+1]) break;
		
		print_gecko("Disc 1 Frag %i: ofs in file: [0x%08X] len [0x%08X] LBA on disk [0x%08X]\r\n", 
					i, fragList[(i*3)+0], fragList[(i*3)+1], fragList[(i*3)+2]);
	}
	if(hasDisc2) {
		for(i = 0; i < maxFrags; i++) {
			if(!fragList[(i*3)+1+(maxFrags*3)]) break;
		
			print_gecko("Disc 2 Frag %i: ofs in file: [0x%08X] len [0x%08X] LBA on disk [0x%08X]\r\n", 
					i, fragList[(i*3)+0+(maxFrags*3)], fragList[(i*3)+1+(maxFrags*3)], fragList[(i*3)+2+(maxFrags*3)]);
		}
	}
	print_gecko("== Fragments End ==\r\n");
}


/* 
	file: the file to get the fragments for
	fragTbl: a table of u32's {offsetInFile, size, sector},...
	maxFrags: maximum number of fragments allowed
	forceBaseOffset: only use this if the fragments need to fake a position in a larger file (e.g. patch files)
	forceSize: only use this to fake a total size (e.g. patch files again)
	
	return numfrags on success, 0 on failure
*/ 
s32 getFragments(file_handle* file, vu32* fragTbl, s32 maxFrags, u32 forceBaseOffset, u32 forceSize, u32 dev) {
	int i;
	if(!file->ffsFp) {
		devices[dev]->readFile(file, NULL, 0);	// open the file (should be open already)
	}
	
	if(!file->ffsFp) {
		return 0;
	}

	// fatfs - Cluster link table map buffer
	DWORD clmt[(maxFrags+1)*2];
	clmt[0] = (maxFrags+1)*2;
	file->ffsFp->cltbl = &clmt[0];
	if(f_lseek(file->ffsFp, CREATE_LINKMAP) != FR_OK) {
		file->ffsFp->cltbl = NULL;
		return 0;	// Too many fragments for our buffer
	}
	file->ffsFp->cltbl = NULL;
	
	print_gecko("getFragments [%s] - found %i fragments [%i arr]\r\n",file->name, (clmt[0] >> 1)-1, clmt[0]);
	
	// WKF also uses this, make sure we use the right fs obj
	FATFS* fatFS = NULL;
	if(file->name[0] == 'w') {
		fatFS = wkffs;
	}
	else if(file->name[0] == 'g') {
		fatFS = gcloaderfs;
	}
	else {
		int slot = GET_SLOT(file->name);
		fatFS = fs[IS_SDCARD(file->name) ? slot : SD_COUNT+slot];
	}
	if(forceSize == 0) {
		forceSize = file->size;
	}
	s32 numFrags = 0;
	for(i = 1; i < (clmt[0]); i+=2) {
		if(clmt[i] == 0) break;	// No more
		DWORD size = (clmt[i]) * fatFS->csize * 512;
		DWORD sector = clst2sect(fatFS, clmt[i+1]);
		// this frag offset in the file is the last frag offset+size
		size = forceSize < size ? forceSize : size;
		fragTbl[numFrags*3] = forceBaseOffset;
		fragTbl[(numFrags*3)+1] = size | (dev == DEVICE_PATCHES) << 31;
		fragTbl[(numFrags*3)+2] = sector;
		forceBaseOffset += size;
		forceSize -= size;
		numFrags++;
	}
	return numFrags;
}

s32 deviceHandler_FAT_setupFile(file_handle* file, file_handle* file2, int numToPatch) {
	
	// If there are 2 discs, we only allow 21 fragments per disc.
	int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	
	memset(VAR_FRAG_LIST, 0, sizeof(VAR_FRAG_LIST));
	
  	// Look for patch files, if we find some, open them and add them as fragments
	file_handle patchFile;
	s32 frags = 0, totFrags = 0;
	for(i = 0; i < numToPatch; i++) {
		u32 patchInfo[4];
		memset(patchInfo, 0, 16);
		char gameID[8];
		memset(&gameID, 0, 8);
		strncpy((char*)&gameID, (char*)&GCMDisk, 4);
		memset(&patchFile, 0, sizeof(file_handle));
		snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss_patches/%.4s/%i", devices[DEVICE_CUR]->initial->name, &gameID[0], i);

		FILINFO fno;
		if(f_stat(&patchFile.name[0], &fno) != FR_OK) {
			break;	// Patch file doesn't exist, don't bother with fragments
		}

		devices[DEVICE_CUR]->seekFile(&patchFile,fno.fsize-16,DEVICE_HANDLER_SEEK_SET);
		if((devices[DEVICE_CUR]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
			if(!(frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, patchInfo[0], patchInfo[1], DEVICE_CUR))) {
				return 0;
			}
			totFrags+=frags;
			devices[DEVICE_CUR]->closeFile(&patchFile);
		}
		else {
			break;
		}
	}
	
	// Check for igr.dol
	if(swissSettings.igrType == IGR_BOOTBIN) {
		memset(&patchFile, 0, sizeof(file_handle));
		snprintf(&patchFile.name[0], PATHNAME_MAX, "%sigr.dol", devices[DEVICE_CUR]->initial->name);
		
		if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_IGR_DOL, 0, DEVICE_CUR))) {
			*(vu32*)VAR_IGR_DOL_SIZE = patchFile.size;
			totFrags+=frags;
			devices[DEVICE_CUR]->closeFile(&patchFile);
		}
	}
	
	if(swissSettings.emulateMemoryCard) {
		if(devices[DEVICE_CUR] != &__device_sd_a && devices[DEVICE_CUR] != &__device_ide_a) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss_patches/MemoryCardA.%s.raw", devices[DEVICE_CUR]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
			
			if(devices[DEVICE_CUR]->readFile(&patchFile, NULL, 0) != 0) {
				devices[DEVICE_CUR]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_CUR]->writeFile(&patchFile, NULL, 0);
				devices[DEVICE_CUR]->closeFile(&patchFile);
			}
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_CARD_A, 31.5*1024*1024, DEVICE_CUR))) {
				*(vu8*)VAR_CARD_A_ID = (patchFile.size*8/1024/1024) & 0xFC;
				totFrags+=frags;
				devices[DEVICE_CUR]->closeFile(&patchFile);
			}
		}
		
		if(devices[DEVICE_CUR] != &__device_sd_b && devices[DEVICE_CUR] != &__device_ide_b) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss_patches/MemoryCardB.%s.raw", devices[DEVICE_CUR]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
			
			if(devices[DEVICE_CUR]->readFile(&patchFile, NULL, 0) != 0) {
				devices[DEVICE_CUR]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_CUR]->writeFile(&patchFile, NULL, 0);
				devices[DEVICE_CUR]->closeFile(&patchFile);
			}
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_CARD_B, 31.5*1024*1024, DEVICE_CUR))) {
				*(vu8*)VAR_CARD_B_ID = (patchFile.size*8/1024/1024) & 0xFC;
				totFrags+=frags;
				devices[DEVICE_CUR]->closeFile(&patchFile);
			}
		}
	}
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	if(!(frags = getFragments(file, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_DISC_1, DISC_SIZE, DEVICE_CUR))) {
		return 0;
	}
	totFrags += frags;

	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		// TODO fix 2 disc patched games
		if(!(frags = getFragments(file2, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_DISC_2, DISC_SIZE, DEVICE_CUR))) {
			return 0;
		}
		totFrags += frags;
	}
	
	memset(VAR_SECTOR_BUF, 0, sizeof(VAR_SECTOR_BUF));
	
	int isSDCard = IS_SDCARD(file->name);
	int slot = GET_SLOT(file->name);
	if(isSDCard) {
		// Card Type
		*(vu8*)VAR_SD_SHIFT = sdgecko_getAddressingType(slot) ? 9:0;
	}
	// Copy the actual freq
	*(vu8*)VAR_EXI_FREQ = isSDCard ? sdgecko_getSpeed(slot):(swissSettings.exiSpeed ? EXI_SPEED32MHZ:EXI_SPEED16MHZ);
	// Device slot (0, 1 or 2)
	*(vu8*)VAR_EXI_SLOT = slot;
	*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[slot];
	// IDE-EXI only settings
	if(!isSDCard) {
		// Is the HDD in use a 48 bit LBA supported HDD?
		*(vu8*)VAR_ATA_LBA48 = ataDriveInfo.lba48Support;
	}
	print_frag_list(0);
	return 1;
}

s32 fatFs_Mount(u8 devNum, char *path) {
	if(fs[devNum] != NULL) {
		print_gecko("Unmount %i devnum, %s path\r\n", devNum, path);
		f_unmount(path);
		free(fs[devNum]);
		fs[devNum] = NULL;
		disk_shutdown(devNum);
	}
	fs[devNum] = (FATFS*)malloc(sizeof(FATFS));
	return f_mount(fs[devNum], path, 1) == FR_OK;
}

void setSDGeckoSpeed(int slot, bool fast) {
	sdgecko_setSpeed(slot, fast ? EXI_SPEED32MHZ:EXI_SPEED16MHZ);
	print_gecko("SD speed set to %s\r\n", (fast ? "32MHz":"16MHz"));
}

s32 deviceHandler_FAT_init(file_handle* file) {
	int isSDCard = IS_SDCARD(file->name);
	int slot = GET_SLOT(file->name);
	int ret = 0;
	print_gecko("Init %s %i\r\n", (isSDCard ? "SD":"IDE"), slot);
	// Slot A - SD Card
	if(isSDCard && slot == 0) {
		setSDGeckoSpeed(0, swissSettings.exiSpeed);
		__device_sd_a.features |= FEAT_BOOT_GCM;
		ret = fatFs_Mount(0, "sda:/");
		if(!ret) {
			setSDGeckoSpeed(0, false);
			__device_sd_a.features &= ~FEAT_BOOT_GCM;
			ret = fatFs_Mount(0, "sda:/");
		}
	}
	// Slot B - SD Card
	if(isSDCard && slot == 1) {
		setSDGeckoSpeed(1, swissSettings.exiSpeed);
		__device_sd_b.features |= FEAT_BOOT_GCM;
		ret = fatFs_Mount(1, "sdb:/");
		if(!ret) {
			setSDGeckoSpeed(1, false);
			__device_sd_b.features &= ~FEAT_BOOT_GCM;
			ret = fatFs_Mount(1, "sdb:/");
		}
	}
	// SP2 - SD Card
	if(isSDCard && slot == 2) {
		setSDGeckoSpeed(2, swissSettings.exiSpeed);
		__device_sd_c.features |= FEAT_BOOT_GCM;
		ret = fatFs_Mount(2, "sdc:/");
		if(!ret) {
			setSDGeckoSpeed(2, false);
			__device_sd_c.features &= ~FEAT_BOOT_GCM;
			ret = fatFs_Mount(2, "sdc:/");
		}
	}
	// Slot A - IDE-EXI
	if(!isSDCard && !slot) {
		ret = fatFs_Mount(3, "idea:/");
	}
	// Slot B - IDE-EXI
	if(!isSDCard && slot) {
		ret = fatFs_Mount(4, "ideb:/");
	}
	if(ret)
		populateDeviceInfo(file);
	return ret;
}

char *getDeviceMountPath(char *str) {
	char *path = (char*)memalign(32, 64);
	memset(path, 0, 64);
	
	int i;
	for(i = 0; i < strlen(str); i++)
		if(str[i] == '/')
			break;
	memcpy(path, str, ++i);
	return path;
}

s32 deviceHandler_FAT_closeFile(file_handle* file) {
	int ret = 0;
	if(file && file->ffsFp) {
		ret = f_close(file->ffsFp);
		free(file->ffsFp);
		file->ffsFp = 0;
		int slot = GET_SLOT(file->name);
		disk_flush(IS_SDCARD(file->name) ? slot : SD_COUNT+slot);
	}
	return ret;
}

s32 deviceHandler_FAT_deinit(file_handle* file) {
	device_info* info = deviceHandler_FAT_info(file);
	info->freeSpaceInKB = 0;
	info->totalSpaceInKB = 0;
	if(file) {
		deviceHandler_FAT_closeFile(file);
		int isSDCard = IS_SDCARD(file->name);
		int slot = GET_SLOT(file->name);
		f_unmount(file->name);
		free(fs[isSDCard ? slot : SD_COUNT+slot]);
		fs[isSDCard ? slot : SD_COUNT+slot] = NULL;
		disk_shutdown(isSDCard ? slot : SD_COUNT+slot);
	}
	return 0;
}

s32 deviceHandler_FAT_deleteFile(file_handle* file) {
	deviceHandler_FAT_closeFile(file);
	return f_unlink(file->name);
}

bool deviceHandler_FAT_test_sd_a() {
	return carda->startup() && carda->shutdown();
}
bool deviceHandler_FAT_test_sd_b() {
	return cardb->startup() && cardb->shutdown();
}
bool deviceHandler_FAT_test_sd_c() {
	return cardc->startup() && cardc->shutdown();
}
bool deviceHandler_FAT_test_ide_a() {
	return ide_exi_inserted(0);
}
bool deviceHandler_FAT_test_ide_b() {
	return ide_exi_inserted(1);
}

u32 deviceHandler_FAT_emulated_sd() {
	if (GCMDisk.AudioStreaming)
		return EMU_READ|EMU_AUDIO_STREAMING;
	else if (swissSettings.emulateReadSpeed)
		return EMU_READ|EMU_READ_SPEED;
	else if (swissSettings.emulateMemoryCard)
		return EMU_READ|EMU_MEMCARD;
	else
		return EMU_READ;
}

u32 deviceHandler_FAT_emulated_ide() {
	if (GCMDisk.AudioStreaming)
		return EMU_READ|EMU_AUDIO_STREAMING;
	else if (swissSettings.emulateMemoryCard)
		return EMU_READ|EMU_MEMCARD;
	else
		return EMU_READ;
}

DEVICEHANDLER_INTERFACE __device_sd_a = {
	DEVICE_ID_1,
	"SD Card Adapter",
	"SD Card - Slot A",
	"SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_SDSMALL, 60, 84},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING,
	EMU_READ|EMU_READ_SPEED|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	LOC_MEMCARD_SLOT_A,
	&initial_SD_A,
	(_fn_test)&deviceHandler_FAT_test_sd_a,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_FAT_init,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_readFile)&deviceHandler_FAT_readFile,
	(_fn_writeFile)&deviceHandler_FAT_writeFile,
	(_fn_deleteFile)&deviceHandler_FAT_deleteFile,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_setupFile)&deviceHandler_FAT_setupFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deinit)&deviceHandler_FAT_deinit,
	(_fn_emulated)&deviceHandler_FAT_emulated_sd,
};

DEVICEHANDLER_INTERFACE __device_sd_b = {
	DEVICE_ID_2,
	"SD Card Adapter",
	"SD Card - Slot B",
	"SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_SDSMALL, 60, 84},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING,
	EMU_READ|EMU_READ_SPEED|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	LOC_MEMCARD_SLOT_B,
	&initial_SD_B,
	(_fn_test)&deviceHandler_FAT_test_sd_b,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_FAT_init,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_readFile)&deviceHandler_FAT_readFile,
	(_fn_writeFile)&deviceHandler_FAT_writeFile,
	(_fn_deleteFile)&deviceHandler_FAT_deleteFile,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_setupFile)&deviceHandler_FAT_setupFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deinit)&deviceHandler_FAT_deinit,
	(_fn_emulated)&deviceHandler_FAT_emulated_sd,
};

DEVICEHANDLER_INTERFACE __device_ide_a = {
	DEVICE_ID_3,
	"IDE-EXI",
	"IDE-EXI - Slot A",
	"IDE HDD - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_HDD, 104, 76},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING,
	EMU_READ|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	LOC_MEMCARD_SLOT_A,
	&initial_IDE_A,
	(_fn_test)&deviceHandler_FAT_test_ide_a,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_FAT_init,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_readFile)&deviceHandler_FAT_readFile,
	(_fn_writeFile)&deviceHandler_FAT_writeFile,
	(_fn_deleteFile)&deviceHandler_FAT_deleteFile,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_setupFile)&deviceHandler_FAT_setupFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deinit)&deviceHandler_FAT_deinit,
	(_fn_emulated)&deviceHandler_FAT_emulated_ide,
};

DEVICEHANDLER_INTERFACE __device_ide_b = {
	DEVICE_ID_4,
	"IDE-EXI",
	"IDE-EXI - Slot B",
	"IDE HDD - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_HDD, 104, 76},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING,
	EMU_READ|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	LOC_MEMCARD_SLOT_B,
	&initial_IDE_B,
	(_fn_test)&deviceHandler_FAT_test_ide_b,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_FAT_init,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_readFile)&deviceHandler_FAT_readFile,
	(_fn_writeFile)&deviceHandler_FAT_writeFile,
	(_fn_deleteFile)&deviceHandler_FAT_deleteFile,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_setupFile)&deviceHandler_FAT_setupFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deinit)&deviceHandler_FAT_deinit,
	(_fn_emulated)&deviceHandler_FAT_emulated_ide,
};

DEVICEHANDLER_INTERFACE __device_sd_c = {
	DEVICE_ID_F,
	"SD Card Adapter",
	"SD Card - SP2",
	"SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_SDSMALL, 60, 84},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_HYPERVISOR|FEAT_PATCHES|FEAT_AUDIO_STREAMING,
	EMU_READ|EMU_READ_SPEED|EMU_AUDIO_STREAMING|EMU_MEMCARD,
	LOC_SERIAL_PORT_2,
	&initial_SD_C,
	(_fn_test)&deviceHandler_FAT_test_sd_c,
	(_fn_info)&deviceHandler_FAT_info,
	(_fn_init)&deviceHandler_FAT_init,
	(_fn_readDir)&deviceHandler_FAT_readDir,
	(_fn_readFile)&deviceHandler_FAT_readFile,
	(_fn_writeFile)&deviceHandler_FAT_writeFile,
	(_fn_deleteFile)&deviceHandler_FAT_deleteFile,
	(_fn_seekFile)&deviceHandler_FAT_seekFile,
	(_fn_setupFile)&deviceHandler_FAT_setupFile,
	(_fn_closeFile)&deviceHandler_FAT_closeFile,
	(_fn_deinit)&deviceHandler_FAT_deinit,
	(_fn_emulated)&deviceHandler_FAT_emulated_sd,
};
