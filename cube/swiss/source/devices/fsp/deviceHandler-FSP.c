/* deviceHandler-FSP.c
	- device implementation for File Sharing Protocol
	by Extrems
 */

#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <network.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "fsplib.h"
#include "exi.h"
#include "bba.h"
#include "patcher.h"

extern int net_initialized;
static FSP_SESSION *fsp_session;

file_handle initial_FSP =
	{ "/",      // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};

device_info initial_FSP_info = {
	0,
	0
};

device_info* deviceHandler_FSP_info(file_handle* file) {
	return &initial_FSP_info;
}

s32 deviceHandler_FSP_readDir(file_handle* ffile, file_handle** dir, u32 type) {

	FSP_DIR* dp = fsp_opendir(fsp_session, ffile->name);
	if(!dp) return -1;
	FSP_RDENTRY entry;
	FSP_RDENTRY *result;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	char file_name[PATHNAME_MAX];
	*dir = calloc(sizeof(file_handle), 1);
	(*dir)[0].fileAttrib = IS_SPECIAL;
	strcpy((*dir)[0].name, "..");
	
	// Read each entry of the directory
	while( !fsp_readdir_native(dp, &entry, &result) && result == &entry ){
		if(strlen(entry.name) <= 2  && (entry.name[0] == '.' || entry.name[1] == '.')) {
			continue;
		}
		memset(&file_name[0],0,PATHNAME_MAX);
		sprintf(&file_name[0], "%s/%s", ffile->name, entry.name);
		// Do we want this one?
		if((type == -1 || ((entry.type & FSP_RDTYPE_DIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
			if(!(entry.type & FSP_RDTYPE_DIR)) {
				if(!checkExtension(entry.name)) continue;
			}
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
			}
			memset(&(*dir)[i], 0, sizeof(file_handle));
			sprintf((*dir)[i].name, "%s/%s", ffile->name, entry.name);
			(*dir)[i].size     = entry.size;
			(*dir)[i].fileAttrib   = (entry.type & FSP_RDTYPE_DIR) ? IS_DIR : IS_FILE;
			++i;
		}
	}
	
	fsp_closedir(dp);
	return num_entries;
}

s32 deviceHandler_FSP_seekFile(file_handle* file, u32 where, u32 type) {
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_FSP_readFile(file_handle* file, void* buffer, u32 length) {
	if(!file->fp) {
		file->fp = fsp_fopen(fsp_session, file->name, "rb");
	}
	if(!file->fp) return -1;
	if(file->size <= 0) {
		struct stat fstat;
		if(fsp_stat(fsp_session, file->name, &fstat)) {
			fsp_fclose(file->fp);
			file->fp = NULL;
			return -1;
		}
		file->size = fstat.st_size;
	}
	
	fsp_fseek(file->fp, file->offset, SEEK_SET);
	s32 bytes_read = fsp_fread(buffer, 1, length, file->fp);
	if(bytes_read > 0) file->offset += bytes_read;
	return bytes_read;
}

s32 deviceHandler_FSP_writeFile(file_handle* file, void* buffer, u32 length) {
	if(!file->fp) {
		file->fp = fsp_fopen(fsp_session, file->name, "wb");
	}
	if(!file->fp) return -1;
	
	fsp_fseek(file->fp, file->offset, SEEK_SET);
	s32 bytes_read = fsp_fwrite(buffer, 1, length, file->fp);
	if(bytes_read > 0) file->offset += bytes_read;
	return bytes_read;
}

s32 deviceHandler_FSP_setupFile(file_handle* file, file_handle* file2) {
	
	// If there are 2 discs, we only allow 21 fragments per disc.
	int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	s32 frags = 0, totFrags = 0;
	
	memset(VAR_FRAG_LIST, 0, sizeof(VAR_FRAG_LIST));

	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		char gameID[8];
		memset(&gameID, 0, 8);
		strncpy((char*)&gameID, (char*)&GCMDisk, 4);
		
		for(i = 0; i < maxFrags; i++) {
			u32 patchInfo[4];
			patchInfo[0] = 0; patchInfo[1] = 0; 
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%sswiss_patches/%s/%i",devices[DEVICE_PATCHES]->initial->name,gameID, i);
			print_gecko("Looking for file %s\r\n", &patchFile.name);
			FILINFO fno;
			if(f_stat(&patchFile.name[0], &fno) != FR_OK) {
				break;	// Patch file doesn't exist, don't bother with fragments
			}
			
			devices[DEVICE_PATCHES]->seekFile(&patchFile,fno.fsize-16,DEVICE_HANDLER_SEEK_SET);
			if((devices[DEVICE_PATCHES]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
				if(!(frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags, patchInfo[0], patchInfo[1], DEVICE_PATCHES))) {
					return 0;
				}
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			else {
				break;
			}
		}
		// Check for igr.dol
		memset(&patchFile, 0, sizeof(file_handle));
		sprintf(&patchFile.name[0], "%sigr.dol", devices[DEVICE_PATCHES]->initial->name);

		FILINFO fno;
		if(f_stat(&patchFile.name[0], &fno) == FR_OK) {
			print_gecko("IGR Boot DOL exists\r\n");
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags, 0xE0000000, 0, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
				*(vu32*)VAR_IGR_DOL_SIZE = fno.fsize;
			}
		}
		
		print_frag_list(0);
		// Card Type
		*(vu8*)VAR_SD_SHIFT = (u8)(sdgecko_getAddressingType(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)) ? 9:0);
		// Copy the actual freq
		*(vu8*)VAR_EXI_FREQ = (u8)(swissSettings.exiSpeed ? EXI_SPEED32MHZ:EXI_SPEED16MHZ);
		// Device slot (0, 1 or 2)
		*(vu8*)VAR_EXI_SLOT = (u8)(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2));
	}
	
	net_get_mac_address(VAR_CLIENT_MAC);
	*(vu32*)VAR_CLIENT_IP = net_gethostip();
	((vu8*)VAR_SERVER_MAC)[0] = 0xFF;
	((vu8*)VAR_SERVER_MAC)[1] = 0xFF;
	((vu8*)VAR_SERVER_MAC)[2] = 0xFF;
	((vu8*)VAR_SERVER_MAC)[3] = 0xFF;
	((vu8*)VAR_SERVER_MAC)[4] = 0xFF;
	((vu8*)VAR_SERVER_MAC)[5] = 0xFF;
	*(vu32*)VAR_SERVER_IP = inet_addr(swissSettings.fspHostIp);
	*(vu16*)VAR_SERVER_PORT = swissSettings.fspPort ? swissSettings.fspPort : 21;
	*(vu8*)VAR_DISC_1_FNLEN = snprintf(VAR_DISC_1_FN, sizeof(VAR_DISC_1_FN), "%s\n%s", file->name, swissSettings.fspPassword) + 1;
	*(vu8*)VAR_DISC_2_FNLEN = snprintf(VAR_DISC_2_FN, sizeof(VAR_DISC_2_FN), "%s\n%s", file2 ? file2->name : file->name, swissSettings.fspPassword) + 1;
	*(vu16*)VAR_IPV4_ID = 0;
	*(vu16*)VAR_FSP_KEY = 0;
	*(vu16*)VAR_FSP_DATA_LENGTH = 0;
	return 1;
}

s32 deviceHandler_FSP_init(file_handle* file) {
	init_network();
	fsp_session = fsp_open_session(swissSettings.fspHostIp, swissSettings.fspPort, swissSettings.fspPassword);
	return 1;
}

s32 deviceHandler_FSP_closeFile(file_handle* file) {
	if(file && file->fp) {
		fsp_fclose(file->fp);
		file->fp = NULL;
	}
	return 0;
}

s32 deviceHandler_FSP_deinit(file_handle* file) {
	deviceHandler_FSP_closeFile(file);
	fsp_close_session(fsp_session);
	fsp_session = NULL;
	return 0;
}

s32 deviceHandler_FSP_deleteFile(file_handle* file) {
	deviceHandler_FSP_closeFile(file);
	return fsp_unlink(fsp_session, file->name);
}

bool deviceHandler_FSP_test() {
	return exi_bba_exists();
}

DEVICEHANDLER_INTERFACE __device_fsp = {
	DEVICE_ID_E,
	"BBA",
	"File Service Protocol",
	"Must be pre-configured via swiss.ini",
	{TEX_SAMBA, 160, 85},
	FEAT_READ|FEAT_WRITE|FEAT_BOOT_GCM|FEAT_REPLACES_DVD_FUNCS|FEAT_ALT_READ_PATCHES|FEAT_CAN_READ_PATCHES,
	LOC_SERIAL_PORT_1,
	&initial_FSP,
	(_fn_test)&deviceHandler_FSP_test,
	(_fn_info)&deviceHandler_FSP_info,
	(_fn_init)&deviceHandler_FSP_init,
	(_fn_readDir)&deviceHandler_FSP_readDir,
	(_fn_readFile)&deviceHandler_FSP_readFile,
	(_fn_writeFile)&deviceHandler_FSP_writeFile,
	(_fn_deleteFile)&deviceHandler_FSP_deleteFile,
	(_fn_seekFile)&deviceHandler_FSP_seekFile,
	(_fn_setupFile)&deviceHandler_FSP_setupFile,
	(_fn_closeFile)&deviceHandler_FSP_closeFile,
	(_fn_deinit)&deviceHandler_FSP_deinit
};
