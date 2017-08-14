/* deviceHandler-FAT.c
	- device implementation for FAT (via SDGecko/IDE-EXI)
	by emu_kidid
 */

#include <fat.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <sys/dir.h>
#include <sys/statvfs.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "ata.h"
#include "frag.h"
#include "patcher.h"

const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;
const DISC_INTERFACE* ideexia = &__io_ataa;
const DISC_INTERFACE* ideexib = &__io_atab;
extern void sdgecko_initIODefault();

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
		struct statvfs buf;
		int isSDCard = file->name[0] == 's';
		int slot = isSDCard ? (file->name[2] == 'b') : (file->name[3] == 'b');
		
		memset(&buf, 0, sizeof(statvfs));
		DrawFrameStart();
		sprintf(txtbuffer, "Reading filesystem info for %s%s:/",isSDCard ? "sd":"ide", slot ? "b":"a");
		DrawMessageBox(D_INFO,txtbuffer);
		DrawFrameFinish();
		
		sprintf(txtbuffer, "%s%s:/",isSDCard ? "sd":"ide", slot ? "b":"a");
		int res = statvfs(txtbuffer, &buf);
		initial_FAT_info.freeSpaceInKB = !res ? (u32)((uint64_t)((uint64_t)buf.f_bsize*(uint64_t)buf.f_bfree)/1024LL):0;
		initial_FAT_info.totalSpaceInKB = !res ? (u32)((uint64_t)((uint64_t)buf.f_bsize*(uint64_t)buf.f_blocks)/1024LL):0;
	}
}
	
s32 deviceHandler_FAT_readDir(file_handle* ffile, file_handle** dir, u32 type){	

	DIR* dp = opendir( ffile->name );
	if(!dp) return -1;
	struct dirent *entry;
	struct stat fstat;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	char file_name[1024];
	*dir = malloc( num_entries * sizeof(file_handle) );
	memset(*dir,0,sizeof(file_handle) * num_entries);
	(*dir)[0].fileAttrib = IS_SPECIAL;
	strcpy((*dir)[0].name, "..");

	// Read each entry of the directory
	while( (entry = readdir(dp)) != NULL ){
		if(strlen(entry->d_name) <= 2  && (entry->d_name[0] == '.' || entry->d_name[1] == '.')) {
			continue;
		}
		memset(&file_name[0],0,1024);
		sprintf(&file_name[0], "%s/%s", ffile->name, entry->d_name);
		stat(&file_name[0],&fstat);
		// Do we want this one?
		if((type == -1 || ((fstat.st_mode & S_IFDIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
			if(!(fstat.st_mode & S_IFDIR)) {
				if(!checkExtension(entry->d_name)) continue;
			}
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			sprintf((*dir)[i].name, "%s/%s", ffile->name, entry->d_name);
			(*dir)[i].size     = fstat.st_size;
			(*dir)[i].fileAttrib   = (fstat.st_mode & S_IFDIR) ? IS_DIR : IS_FILE;
			++i;
		}
	}
	
	closedir(dp);

	int isSDCard = ffile->name[0] == 's';
	if(isSDCard) {
		int slot = (ffile->name[2] == 'b');
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

s32 deviceHandler_FAT_readFile(file_handle* file, void* buffer, u32 length){
  	if(!file->fp) {
		file->fp = fopen( file->name, "r+" );
		if(file->size <= 0) {
			struct stat fstat;
			stat(file->name,&fstat);
			file->size = fstat.st_size;
		}
	}
	if(!file->fp) return -1;
	
	fseek(file->fp, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, file->fp);
	if(bytes_read > 0) file->offset += bytes_read;
	
	return bytes_read;
}

s32 deviceHandler_FAT_writeFile(file_handle* file, void* buffer, u32 length){
	if(!file->fp) {
		// Append
		file->fp = fopen( file->name, "r+" );
		if(file->fp <= 0) {
			// Create
			file->fp = fopen( file->name, "wb" );
		}
	}
	if(!file->fp) return -1;
	fseek(file->fp, file->offset, SEEK_SET);
		
	int bytes_written = fwrite(buffer, 1, length, file->fp);
	if(bytes_written > 0) file->offset += bytes_written;
	
	return bytes_written;
}

void print_frag_list(int hasDisc2) {
	print_gecko("== Fragments List ==\r\n");
	u32 *fragList = (u32*)VAR_FRAG_LIST;
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

s32 deviceHandler_FAT_setupFile(file_handle* file, file_handle* file2) {
	// Check if file2 exists
	if(file2) {
		get_frag_list(file2->name);
		if(frag_list->num <= 0)
			file2 = NULL;
	}
	
	// If there are 2 discs, we only allow 21 fragments per disc.
	int maxFrags = file2 ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0;
	u32 *fragList = (u32*)VAR_FRAG_LIST;
	
	memset((void*)VAR_FRAG_LIST, 0, VAR_FRAG_SIZE);
	
  	// Look for patch files, if we find some, open them and add them as fragments
	file_handle patchFile;
	int patches = 0;
	for(i = 0; i < maxFrags; i++) {
		u32 patchInfo[4];
		patchInfo[0] = 0; patchInfo[1] = 0; 
		char gameID[8];
		memset(&gameID, 0, 8);
		strncpy((char*)&gameID, (char*)&GCMDisk, 4);
		memset(&patchFile, 0, sizeof(file_handle));
		sprintf(&patchFile.name[0], "%sswiss_patches/%s/%i", devices[DEVICE_CUR]->initial->name,&gameID[0], i);

		struct stat fstat;
		if(stat(&patchFile.name[0],&fstat)) {
			break;
		}
		devices[DEVICE_CUR]->seekFile(&patchFile,fstat.st_size-16,DEVICE_HANDLER_SEEK_SET);
		if((devices[DEVICE_CUR]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
			get_frag_list(&patchFile.name[0]);
			print_gecko("Found patch file %i ofs 0x%08X len 0x%08X base 0x%08X (%i pieces)\r\n", 
							i, patchInfo[0], patchInfo[1], frag_list->frag[0].sector,frag_list->num );
			fclose(patchFile.fp);
			int j = 0;
			u32 totalSize = patchInfo[1];
			for(j = 0; j < frag_list->num; j++) {
				u32 fragmentSize = frag_list->frag[j].count*512;
				if(j+1 == frag_list->num)
					fragmentSize = totalSize;
				fragList[patches*3] = patchInfo[0] + frag_list->frag[j].offset*512;
				fragList[(patches*3)+1] = fragmentSize;
				fragList[(patches*3)+2] = frag_list->frag[j].sector;
				patches++;
				totalSize -= fragmentSize;
			}
		}
		else {
			break;
		}
	}
	
	// No fragment room left for the actual game, fail.
	if(patches+1 == maxFrags) {
		return 0;
	}
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	get_frag_list(file->name);
	print_gecko("Found disc 1 [%s] %i fragments\r\n",file->name, frag_list->num);
	if(frag_list->num < maxFrags) {
		for(i = 0; i < frag_list->num; i++) {
			fragList[patches*3] = frag_list->frag[i].offset*512;
			fragList[(patches*3)+1] = frag_list->frag[i].count*512;
			fragList[(patches*3)+2] = frag_list->frag[i].sector;
			patches++;
		}
	}
	else {
		// file is too fragmented - go defrag it!
		return 0;
	}
		
	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		// No fragment room left for the second disc, fail.
		if(patches+1 == maxFrags) {
			return 0;
		}
		get_frag_list(file2->name);
		print_gecko("Found disc 2 [%s] %i fragments\r\n",file2->name, frag_list->num);
		patches = 0;	// This breaks 2 disc games with patches (do any exist?)
		if(frag_list->num < maxFrags) {
			for(i = 0; i < frag_list->num; i++) {
				fragList[(patches*3) + (maxFrags*3)] = frag_list->frag[i].offset*512;
				fragList[((patches*3) + 1) + (maxFrags*3)]  = frag_list->frag[i].count*512;
				fragList[((patches*3) + 2) + (maxFrags*3)] = frag_list->frag[i].sector;
				patches++;
			}
		}
		else {
			// file is too fragmented - go defrag it!
			return 0;
		}
	}
	
	// Disk 1 base sector
	*(volatile unsigned int*)VAR_DISC_1_LBA = fragList[2];
	// Disk 2 base sector
	*(volatile unsigned int*)VAR_DISC_2_LBA = file2 ? fragList[2 + (maxFrags*3)]:fragList[2];
	// Currently selected disk base sector
	*(volatile unsigned int*)VAR_CUR_DISC_LBA = fragList[2];
	// Copy the current speed
	*(volatile unsigned int*)VAR_EXI_BUS_SPD = !swissSettings.exiSpeed ? 192:208;
	// Card Type
	*(volatile unsigned int*)VAR_SD_TYPE = SDHCCard;
	// Copy the actual freq
	*(volatile unsigned int*)VAR_EXI_FREQ = !swissSettings.exiSpeed ? EXI_SPEED16MHZ:EXI_SPEED32MHZ;
	// Device slot (0 or 1) // This represents 0xCC0068xx in number of u32's so, slot A = 0xCC006800, B = 0xCC006814
	*(volatile unsigned int*)VAR_EXI_SLOT = ((devices[DEVICE_CUR]->location == LOC_MEMCARD_SLOT_A)? 0:1) * 5;
	// IDE-EXI only settings
	if((devices[DEVICE_CUR] == &__device_ide_a) || (devices[DEVICE_CUR] == &__device_ide_b)) {
		// Is the HDD in use a 48 bit LBA supported HDD?
		*(volatile unsigned int*)VAR_TMP1 = ataDriveInfo.lba48Support;
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

s32 deviceHandler_FAT_init(file_handle* file) {
	int isSDCard = file->name[0] == 's';
	int slot = isSDCard ? (file->name[2] == 'b') : (file->name[3] == 'b');
	int ret = 0;
	
	// Slot A - SD Card
	if(isSDCard && !slot && EXI_ResetSD(0)) {
		carda->shutdown();
		carda->startup();
		ret = fatMount ("sda", carda, 0, 16, 128) ? 1 : 0;
	}
	// Slot B - SD Card
	if(isSDCard && slot && EXI_ResetSD(1)) {
		cardb->shutdown();
		cardb->startup();
		ret = fatMount ("sdb", cardb, 0, 16, 128) ? 1 : 0;
	}
	// Slot A - IDE-EXI
	if(!isSDCard && !slot) {
		ideexia->startup();
		ret = fatMount ("idea", ideexia, 0, 16, 128) ? 1 : 0;
	}
	// Slot B - IDE-EXI
	if(!isSDCard && slot) {
		ideexib->startup();
		ret = fatMount ("ideb", ideexib, 0, 16, 128) ? 1 : 0;
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

s32 deviceHandler_FAT_deinit(file_handle* file) {
	initial_FAT_info.freeSpaceInKB = 0;
	initial_FAT_info.totalSpaceInKB = 0;
	if(file && file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
	if(file) {
		char *mountPath = getDeviceMountPath(file->name);
		print_gecko("Unmounting [%s]\r\n", mountPath);
		fatUnmount(mountPath);
		free(mountPath);
	}
	return 0;
}

s32 deviceHandler_FAT_deleteFile(file_handle* file) {
	if(file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
	return (remove(file->name) == -1) ? -1:0;
}

s32 deviceHandler_FAT_closeFile(file_handle* file) {
	int ret = 0;
	if(file->fp) {
		ret = fclose(file->fp);
		file->fp = 0;
	}
	return ret;
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
	"SD(HC/XC) Card - Supported File System(s): FAT16, FAT32",
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
	"SD(HC/XC) Card - Supported File System(s): FAT16, FAT32",
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
	"IDE HDD - Supported File System(s): FAT16, FAT32",
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
	"IDE HDD - Supported File System(s): FAT16, FAT32",
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
