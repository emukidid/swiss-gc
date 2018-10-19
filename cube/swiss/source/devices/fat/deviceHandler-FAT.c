/* deviceHandler-FAT.c
	- device implementation for FAT (via SDGecko/IDE-EXI)
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

const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;
const DISC_INTERFACE* ideexia = &__io_ataa;
const DISC_INTERFACE* ideexib = &__io_atab;
extern FATFS *wkffs;
FATFS *fs[4] = {NULL, NULL, NULL, NULL};
extern void sdgecko_initIODefault();

#define IS_SDCARD(v) (v[0] == 's')
#define GET_SLOT(v) (IS_SDCARD(v) ? v[2] == 'b' : v[3] == 'b')

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

device_info initial_FAT_info = {
	0,
	0
};
	
device_info* deviceHandler_FAT_info() {
	return &initial_FAT_info;
}

void readDeviceInfo(file_handle* file) {
	if(deviceHandler_getStatEnabled()) {
		int isSDCard = IS_SDCARD(file->name);
		int slot = GET_SLOT(file->name);
		
		sprintf(txtbuffer, "Reading filesystem info for %s%s:/",isSDCard ? "sd":"ide", slot ? "b":"a");
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));
		sprintf(txtbuffer, "%s%s:/",isSDCard ? "sd":"ide", slot ? "b":"a");
		DWORD free_clusters, free_sectors, total_sectors = 0;
		FATFS *fsptr = fs[(isSDCard ? slot : 2 + slot)];
		if(f_getfree(txtbuffer, &free_clusters, &fsptr) == FR_OK) {
			total_sectors = (fsptr->n_fatent - 2) * fsptr->csize;
			free_sectors = free_clusters * fsptr->csize;
			initial_FAT_info.freeSpaceInKB = (u32)((free_sectors)>>1);
			initial_FAT_info.totalSpaceInKB = (u32)((total_sectors)>>1);
		}
		else {
			initial_FAT_info.freeSpaceInKB = initial_FAT_info.totalSpaceInKB = 0;
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
		sprintf(&file_name[0], "%s/%s", ffile->name, entry.fname);
		if(f_stat(file_name, &fno) != FR_OK || (fno.fattrib & AM_HID)) {
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
			sprintf((*dir)[i].name, "%s/%s", ffile->name, entry.fname);
			(*dir)[i].size     = fno.fsize;
			(*dir)[i].fileAttrib   = (fno.fattrib & AM_DIR) ? IS_DIR : IS_FILE;
			++i;
		}
	}
	
	f_closedir(dp);
	free(dp);
	if(IS_SDCARD(ffile->name)) {
		int slot = GET_SLOT(ffile->name);
	    // Set the card type (either block addressed (0) or byte addressed (1))
	    SDHCCard = sdgecko_getAddressingType(slot);
	    // set the page size to 512 bytes
	    if(sdgecko_setPageSize(slot, 512)!=0) {
	      return -1;
	    }
	}
	
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
		if(f_open(file->ffsFp, file->name, FA_READ | FA_WRITE ) != FR_OK) {
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
	int maxFrags = hasDisc2 ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0;
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
	if(file->ffsFp != NULL && file->ffsFp->cltbl != NULL) {
		devices[dev]->closeFile(file);	// If we've already read fragments, reopen it
	}
	if(!file->ffsFp) {
		devices[dev]->readFile(file, NULL, 0);	// open the file (should be open already)
	}
	
	if(!file->ffsFp) {
		return 0;
	}

	// fatfs - Cluster link table map buffer
	DWORD clmt[(maxFrags*2) + 1];
	clmt[0] = maxFrags;
	file->ffsFp->cltbl = &clmt[0];
	if(f_lseek(file->ffsFp, CREATE_LINKMAP) != FR_OK) return 0;	// Too many fragments for our buffer
	
	print_gecko("getFragments [%s] - found %i fragments [%i arr]\r\n",file->name, (clmt[0] >> 1)-1, clmt[0]);
	
	// WKF also uses this, make sure we use the right fs obj
	FATFS* fatFS = NULL;
	if(file->name[0] == 'w') {
		fatFS = wkffs;
	}
	else {
		int slot = GET_SLOT(file->name);
		fatFS = fs[IS_SDCARD(file->name) ? slot : 2+slot];
	}
	s32 numFrags = 0;
	for(i = 1; i < (clmt[0]); i+=2) {
		if(clmt[i] == 0) break;	// No more
		DWORD size = (clmt[i]) * fatFS->csize * 512;
		DWORD sector = clst2sect(fatFS, clmt[i+1]);
		// this frag offset in the file is the last frag offset+size
		fragTbl[numFrags*3] = forceBaseOffset + (i > 1 ? fragTbl[((numFrags-1)*3)+1]+(fragTbl[((numFrags-1)*3)]) : 0);
		fragTbl[(numFrags*3)+1] = (clmt[0] >> 1)-1 == 1 ? (forceSize!= 0?forceSize:file->size) : size;
		fragTbl[(numFrags*3)+2] = sector;
		numFrags++;
	}
	return numFrags;
}

s32 deviceHandler_FAT_setupFile(file_handle* file, file_handle* file2) {
	
	// If there are 2 discs, we only allow 21 fragments per disc.
	int maxFrags = file2 ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0;
	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	
	memset((void*)VAR_FRAG_LIST, 0, VAR_FRAG_SIZE);
	
  	// Look for patch files, if we find some, open them and add them as fragments
	file_handle patchFile;
	s32 frags = 0, totFrags = 0;
	for(i = 0; i < maxFrags; i++) {
		u32 patchInfo[4];
		patchInfo[0] = 0; patchInfo[1] = 0; 
		char gameID[8];
		memset(&gameID, 0, 8);
		strncpy((char*)&gameID, (char*)&GCMDisk, 4);
		memset(&patchFile, 0, sizeof(file_handle));
		sprintf(&patchFile.name[0], "%sswiss_patches/%s/%i", devices[DEVICE_CUR]->initial->name,&gameID[0], i);

		FILINFO fno;
		if(f_stat(&patchFile.name[0], &fno) != FR_OK) {
			break;	// Patch file doesn't exist, don't bother with fragments
		}

		devices[DEVICE_CUR]->seekFile(&patchFile,fno.fsize-16,DEVICE_HANDLER_SEEK_SET);
		if((devices[DEVICE_CUR]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
			if(!(frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags, patchInfo[0], patchInfo[1], DEVICE_CUR))) {
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
	memset(&patchFile, 0, sizeof(file_handle));
	sprintf(&patchFile.name[0], "%sigr.dol", devices[DEVICE_CUR]->initial->name);

	FILINFO fno;
	if(f_stat(&patchFile.name[0], &fno) == FR_OK) {
		print_gecko("IGR Boot DOL exists\r\n");
		if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags, 0x60000000, 0, DEVICE_CUR))) {
			totFrags+=frags;
			devices[DEVICE_CUR]->closeFile(&patchFile);
			*(vu32*)VAR_IGR_DOL_SIZE = fno.fsize;
		}
	}
	
	// No fragment room left for the actual game, fail.
	if(totFrags+1 == maxFrags) {
		return 0;
	}
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	if(!(frags = getFragments(file, &fragList[totFrags*3], maxFrags, 0, 0, DEVICE_CUR))) {
		return 0;
	}
	totFrags += frags;

	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		// No fragment room left for the second disc, fail.
		if(totFrags+1 == maxFrags) {
			return 0;
		}
		// TODO fix 2 disc patched games
		if((frags = getFragments(file2, &fragList[(maxFrags*3)], maxFrags, 0, 0, DEVICE_CUR))) {
			totFrags += frags;
		}
	}
	
	// Disk 1 base sector
	*(vu32*)VAR_DISC_1_LBA = fragList[2];
	// Disk 2 base sector
	*(vu32*)VAR_DISC_2_LBA = file2 ? fragList[2 + (maxFrags*3)]:fragList[2];
	// Currently selected disk base sector
	*(vu32*)VAR_CUR_DISC_LBA = fragList[2];
	// Copy the current speed
	*(vu32*)VAR_EXI_BUS_SPD = !swissSettings.exiSpeed ? 192:208;
	// Card Type
	*(vu32*)VAR_SD_TYPE = SDHCCard;
	// Copy the actual freq
	*(vu32*)VAR_EXI_FREQ = !swissSettings.exiSpeed ? EXI_SPEED16MHZ:EXI_SPEED32MHZ;
	// Device slot (0 or 1) // This represents 0xCC0068xx in number of u32's so, slot A = 0xCC006800, B = 0xCC006814
	*(vu32*)VAR_EXI_SLOT = ((devices[DEVICE_CUR]->location == LOC_MEMCARD_SLOT_A)? 0:1) * 5;
	// IDE-EXI only settings
	if((devices[DEVICE_CUR] == &__device_ide_a) || (devices[DEVICE_CUR] == &__device_ide_b)) {
		// Is the HDD in use a 48 bit LBA supported HDD?
		*(vu32*)VAR_TMP1 = ataDriveInfo.lba48Support;
	}
	print_frag_list(file2 != 0);
	return 1;
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
	EXI_Detach(drv);
	sdgecko_initIODefault();
	return 1;
}

s32 fatFs_Mount(u8 devNum, char *path) {
	if(fs[devNum] != NULL) {
		disk_flush(devNum);
		print_gecko("Unmount %i devnum, %s path\r\n", devNum, path);
		f_mount(0, path, 0);	// Unmount
		free(fs[devNum]);
		fs[devNum] = NULL;
	}
	fs[devNum] = (FATFS*)malloc(sizeof(FATFS));
	return f_mount(fs[devNum], path, 0) == FR_OK;
}

s32 deviceHandler_FAT_init(file_handle* file) {
	int isSDCard = IS_SDCARD(file->name);
	int slot = GET_SLOT(file->name);
	int ret = 0;
	print_gecko("Init %s %i\r\n", (isSDCard ? "SD":"IDE"), slot);
	// Slot A - SD Card
	if(isSDCard && !slot && EXI_ResetSD(0)) {
		carda->shutdown();
		carda->startup();
		ret = fatFs_Mount(0, "sda:\0");
	}
	// Slot B - SD Card
	if(isSDCard && slot && EXI_ResetSD(1)) {
		cardb->shutdown();
		cardb->startup();
		ret = fatFs_Mount(1, "sdb:\0");
	}
	// Slot A - IDE-EXI
	if(!isSDCard && !slot) {
		ideexia->startup();
		ret = fatFs_Mount(2, "idea:\0");
	}
	// Slot B - IDE-EXI
	if(!isSDCard && slot) {
		ideexib->startup();
		ret = fatFs_Mount(3, "ideb:\0");
	}
	if(ret)
		readDeviceInfo(file);
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
		disk_flush(IS_SDCARD(file->name) ? slot : 2+slot);
	}
	return ret;
}

s32 deviceHandler_FAT_deinit(file_handle* file) {
	initial_FAT_info.freeSpaceInKB = 0;
	initial_FAT_info.totalSpaceInKB = 0;
	if(file) {
		deviceHandler_FAT_closeFile(file);
		char *mountPath = getDeviceMountPath(file->name);
		int slot = GET_SLOT(file->name);
		disk_flush(IS_SDCARD(file->name) ? slot : 2+slot);
		f_mount(0, mountPath, 0);
		fs[IS_SDCARD(file->name) ? slot : 2+slot] = NULL;
		free(mountPath);
	}
	return 0;
}

s32 deviceHandler_FAT_deleteFile(file_handle* file) {
	deviceHandler_FAT_closeFile(file);
	int res = f_unlink(file->name) == FR_OK;
	int slot = GET_SLOT(file->name);
	disk_flush(IS_SDCARD(file->name) ? slot : 2+slot);
	return res;
}

bool deviceHandler_FAT_test_sd_a(int slot, bool isSdCard, char *mountPath) {
	carda->shutdown();
	carda->startup();
	return carda->isInserted();
}
bool deviceHandler_FAT_test_sd_b(int slot, bool isSdCard, char *mountPath) {
	cardb->shutdown();
	cardb->startup();
	return cardb->isInserted();
}
bool deviceHandler_FAT_test_ide_a(int slot, bool isSdCard, char *mountPath) {
	return ide_exi_inserted(0);
}
bool deviceHandler_FAT_test_ide_b(int slot, bool isSdCard, char *mountPath) {
	return ide_exi_inserted(1);
}

DEVICEHANDLER_INTERFACE __device_sd_a = {
	DEVICE_ID_1,
	"SD Gecko - Slot A",
	"SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_SDSMALL, 60, 80},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_CAN_READ_PATCHES|FEAT_REPLACES_DVD_FUNCS,
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
	(_fn_deinit)&deviceHandler_FAT_deinit
};

DEVICEHANDLER_INTERFACE __device_sd_b = {
	DEVICE_ID_2,
	"SD Gecko - Slot B",
	"SD(HC/XC) Card - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_SDSMALL, 60, 80},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_CAN_READ_PATCHES|FEAT_REPLACES_DVD_FUNCS,
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
	(_fn_deinit)&deviceHandler_FAT_deinit
};

DEVICEHANDLER_INTERFACE __device_ide_a = {
	DEVICE_ID_3,
	"IDE-EXI - Slot A",
	"IDE HDD - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_HDD, 80, 80},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_CAN_READ_PATCHES|FEAT_REPLACES_DVD_FUNCS,
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
	(_fn_deinit)&deviceHandler_FAT_deinit
};

DEVICEHANDLER_INTERFACE __device_ide_b = {
	DEVICE_ID_4,
	"IDE-EXI - Slot B",
	"IDE HDD - Supported File System(s): FAT16, FAT32, exFAT",
	{TEX_HDD, 80, 80},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL|FEAT_FAT_FUNCS|FEAT_CAN_READ_PATCHES|FEAT_REPLACES_DVD_FUNCS,
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
	(_fn_deinit)&deviceHandler_FAT_deinit
};
