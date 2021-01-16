/* deviceHandler-DVD.c
	- device implementation for DVD (Discs)
	by emu_kidid
 */


#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include "swiss.h"
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "main.h"
#include "patcher.h"
#include "dvd.h"
#include "gcm.h"
#include "wkf.h"
#include "deviceHandler-FAT.h"

#define OFFSET_NOTSET 0
#define OFFSET_SET    1

file_handle initial_DVD =
	{ "dvd:/",     // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  DRV_ERROR
};

device_info initial_DVD_info = {
	1425760,
	1425760
};

static char error_str[256];
static int dvd_init = 0;
static int toc_read = 0;
char *dvdDiscTypeStr = NotInitStr;
int dvdDiscTypeInt = 0;
int drive_status = NORMAL_MODE;
int wkfDetected = 0;

char *dvd_error_str()
{
  unsigned int err = dvd_get_error();
  if(!err) return "OK";

  memset(&error_str[0],0,256);
  switch(err>>24) {
	case 0:
	  break;
	case 1:
	  strcpy(&error_str[0],"Lid open");
	  break;
	case 2:
	  strcpy(&error_str[0],"No disk/Disk changed");
	  break;
	case 3:
	  strcpy(&error_str[0],"No disk");
	  break;
	case 4:
	  strcpy(&error_str[0],"Motor off");
	  break;
	case 5:
	  strcpy(&error_str[0],"Disk not initialized");
	  break;
  }
  switch(err&0xFFFFFF) {
	case 0:
	  break;
	case 0x020400:
	  strcat(&error_str[0]," Motor Stopped");
	  break;
	case 0x020401:
	  strcat(&error_str[0]," Disk ID not read");
	  break;
	case 0x023A00:
	  strcat(&error_str[0]," Medium not present / Cover opened");
	  break;
	case 0x030200:
	  strcat(&error_str[0]," No Seek complete");
	  break;
	case 0x031100:
	  strcat(&error_str[0]," UnRecoverd read error");
	  break;
	case 0x040800:
	  strcat(&error_str[0]," Transfer protocol error");
	  break;
	case 0x052000:
	  strcat(&error_str[0]," Invalid command operation code");
	  break;
	case 0x052001:
	  strcat(&error_str[0]," Audio Buffer not set");
	  break;
	case 0x052100:
	  strcat(&error_str[0]," Logical block address out of range");
	  break;
	case 0x052400:
	  strcat(&error_str[0]," Invalid Field in command packet");
	  break;
	case 0x052401:
	  strcat(&error_str[0]," Invalid audio command");
	  break;
	case 0x052402:
	  strcat(&error_str[0]," Configuration out of permitted period");
	  break;
	case 0x053000:
	  strcat(&error_str[0]," DVD-R"); //?
	  break;
	case 0x053100:
	  strcat(&error_str[0]," Wrong Read Type"); //?
	  break;
	case 0x056300:
	  strcat(&error_str[0]," End of user area encountered on this track");
	  break;
	case 0x062800:
	  strcat(&error_str[0]," Medium may have changed");
	  break;
	case 0x0B5A01:
	  strcat(&error_str[0]," Operator medium removal request");
	  break;
  }
  if(!error_str[0])
	sprintf(&error_str[0],"Unknown %08X",err);
  return &error_str[0];
}

int initialize_disc(u32 streaming) {
	int patched = NORMAL_MODE;
	uiDrawObj_t* progBar = DrawPublish(DrawProgressBar(true, 0, "DVD Is Initializing"));
	if(is_gamecube())
	{
		// Reset WKF hard to allow for a real disc to be read if SD is removed
		if(wkfDetected || (__wkfSpiReadId() != 0 && __wkfSpiReadId() != 0xFFFFFFFF)) {
			print_gecko("Detected Wiikey Fusion with SPI Flash ID: %08X\r\n",__wkfSpiReadId());
			__wkfReset();
			print_gecko("WKF RESET\r\n");
			wkfDetected = 1;
		}

		DrawDispose(progBar);
		progBar = DrawPublish(DrawProgressBar(true, 0, "Resetting DVD drive - Detect Media"));
		dvd_reset();
		dvd_read_id();
		// Avoid lid open scenario
		if((dvd_get_error()>>24) && (dvd_get_error()>>24 != 1)) {
			DrawDispose(progBar);
			progBar = DrawPublish(DrawProgressBar(true, 0, "Possible DVD Backup - Enabling Patches"));
			dvd_enable_patches();
			if(!dvd_get_error()) {
				patched=DEBUG_MODE;
				drive_status = DEBUG_MODE;
			}
		}
		else if((dvd_get_error()>>24) == 1) {  // Lid is open, tell the user!
			DrawDispose(progBar);
			sprintf(txtbuffer, "Error %s. Press A.",dvd_error_str());
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, txtbuffer);
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
			return DRV_ERROR;
		}
		if((streaming == ENABLE_AUDIO) || (streaming == DISABLE_AUDIO)) {
			dvd_set_streaming(streaming);
		}
		else {
			dvd_set_streaming(*(char*)0x80000008);
		}
	}
	else {  //Wii, in GC Mode
		DVD_Reset(DVD_RESETHARD);
		dvd_read_id();
		if((streaming == ENABLE_AUDIO) || (streaming == DISABLE_AUDIO)) {
			dvd_set_streaming(streaming);
		}
		else {
			dvd_set_streaming(*(char*)0x80000008);
		}
	}
	dvd_read_id();
	if(dvd_get_error()) { //no disc, or no game id.
		DrawDispose(progBar);
		sprintf(txtbuffer, "Error: %s",dvd_error_str());
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, txtbuffer);
		DrawPublish(msgBox);
		wait_press_A();
		dvd_reset();	// for good measure
		DrawDispose(msgBox);
		return DRV_ERROR;
	}
	DrawDispose(progBar);
	return patched;
}

int gettype_disc() {
  char *checkBuf = (char*)memalign(32,32);
  char *iso9660Buf = (char*)memalign(32,32);
  int type = UNKNOWN_DISC;
  dvdDiscTypeStr = UnkStr;
  
  // Don't assume there will be a valid disc ID at 0x80000000. 
  // Lets read the first 32b off the disc.
  DVD_Read((void*)checkBuf,0,32);
  DVD_Read((void*)iso9660Buf,32769,32);
  
  // Attempt to determine the disc type from the 32 byte header at 0x80000000
  if (strncmp(checkBuf, "COBRAM", 6) == 0) {
	dvdDiscTypeStr = CobraStr;
	type = COBRA_MULTIGAME_DISC;  //"COBRAM" is Cobra MultiBoot
  }
  else if ((strncmp(checkBuf, "GGCOSD", 6) == 0) || (strncmp(checkBuf, "RGCOSD", 6) == 0)) {
	dvdDiscTypeStr = GCOSD5Str;
	type = GCOSD5_MULTIGAME_DISC;  //"RGCOSD" or "GGCOSD" is GCOS Single Layer MultiGame Disc
  }
  else if (strncmp(checkBuf, "GCOPDVD9", 8) == 0) {
	dvdDiscTypeStr = GCOSD9Str;
	type = GCOSD9_MULTIGAME_DISC;  //"GCOPDVD9" is GCOS Dual Layer MultiGame Disc
  }
  else if (strncmp(iso9660Buf, "CD001", 5) == 0) {
	dvdDiscTypeStr = ISO9660Str;
	type = ISO9660_DISC;  //"CD001" at 32769 is iso9660
  }
  else if ((*(vu32*)(&checkBuf[0x1C])) == DVD_MAGIC) {
	if(checkBuf[6]) {
	  dvdDiscTypeStr = MDStr;
	  type = MULTIDISC_DISC;  //Gamecube 2 Disc (or more?) Game
	}
	else {
	  dvdDiscTypeStr = GCDStr;
	  type = GAMECUBE_DISC;   //Gamecube Game
	}
  }

  free(iso9660Buf);
  free(checkBuf);  
  return type;
}

device_info* deviceHandler_DVD_info(file_handle* file) {
	return &initial_DVD_info;
}

s32 deviceHandler_DVD_readDir(file_handle* ffile, file_handle** dir, u32 type){

	unsigned int i = 0, isGC = is_gamecube();
	unsigned int  *tmpTable = NULL;
	char *tmpName  = NULL;
	u64 tmpOffset = 0LL;
	u64 usedSpace = 0LL;

	int num_entries = 0, ret = 0, num = 0;
	dvd_get_error(); // Clear any 0x052400's
	
	if(dvd_get_error() || !dvd_init) { //if some error
		ret = initialize_disc(ENABLE_BYDISK);
		if(ret == DRV_ERROR) {    //try init
			return -1; //fail
		}
		dvd_init = 1;
		toc_read = 0;
	}

	dvdDiscTypeInt = gettype_disc();
	if(dvdDiscTypeInt == UNKNOWN_DISC) {
		return -1;
	}

	//GCOS or Cobra MultiGame DVD Disc
	if((dvdDiscTypeInt == COBRA_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD5_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC)) {

		if(drive_status == NORMAL_MODE) {
			// This means we're using a drivechip, so our multigame command will be different.
			isXenoGC = 1;
		}
		//Read in the whole table of offsets
		tmpTable = (unsigned int*)memalign(32,MAX_MULTIGAME*4);
		tmpName = (char*)memalign(32,512);
		memset(tmpTable,0,MAX_MULTIGAME*4);
		memset(tmpName,0,512);
		DVD_Read(&tmpTable[0],MULTIGAME_TABLE_OFFSET,MAX_MULTIGAME*4);

		// count entries
		for(i = 0; i < MAX_MULTIGAME; i++) {
			tmpOffset = (dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC) ? (tmpTable[i]<<2):(tmpTable[i]);
			if((tmpOffset) && (tmpOffset%(isGC?0x8000:0x20000)==0) && (tmpOffset<(isGC?DISC_SIZE:WII_D9_SIZE))) {
				num_entries++;
			}
		}

		if(num_entries <= 0) { return num_entries; }
		// malloc the directory structure
		*dir = calloc( num_entries * sizeof(file_handle), 1 );

		// parse entries
		for(i = 0; i < MAX_MULTIGAME; i++) {
			tmpOffset = (dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC) ? (tmpTable[i]<<2):(tmpTable[i]);
			if(num >= 1 && tmpOffset) {
				(*dir)[num-1].size = tmpOffset - (*dir)[num-1].fileBase;
			}
			if((tmpOffset) && (tmpOffset%(isGC?0x8000:0x20000)==0) && (tmpOffset<(isGC?DISC_SIZE:WII_D9_SIZE))) {
				DVD_Read(&tmpName[0],tmpOffset+32, 512);
				sprintf( (*dir)[num].name,"%s.gcm", &tmpName[0] );
				(*dir)[num].fileBase = tmpOffset;
				(*dir)[num].offset = 0;
				(*dir)[num].size   = (isGC?DISC_SIZE:WII_D9_SIZE)-tmpOffset;
				(*dir)[num].fileAttrib	 = IS_FILE;
				(*dir)[num].meta = 0;
				(*dir)[num].status = OFFSET_NOTSET;
				num++;
			}
			free(tmpTable);
			free(tmpName);
		}
		usedSpace = (isGC?DISC_SIZE:WII_D9_SIZE);
	}
	else if((dvdDiscTypeInt == GAMECUBE_DISC) || (dvdDiscTypeInt == MULTIDISC_DISC)) {
		// TODO: BCA entry (dump from drive RAM on a GC, dump via BCA command on Wii)
		// Virtual entries for FST entries :D
		num_entries = read_fst(ffile, dir, !toc_read ? &usedSpace:NULL);
	}
	else if(dvdDiscTypeInt == ISO9660_DISC) {
		// Call the corresponding DVD function
		num_entries = dvd_read_directoryentries(ffile->fileBase,ffile->size);
		// If it was not successful, just return the error
		if(num_entries <= 0) return -1;
		// Convert the DVD "file" data to fileBrowser_files
		*dir = calloc( num_entries * sizeof(file_handle), 1);
		int i;
		for(i=0; i<num_entries; ++i){
			strcpy( (*dir)[i].name, DVDToc->file[i].name );
			(*dir)[i].fileBase = (uint64_t)(((uint64_t)DVDToc->file[i].sector)*2048);
			(*dir)[i].offset = 0;
			(*dir)[i].size   = DVDToc->file[i].size;
			(*dir)[i].fileAttrib	 = IS_FILE;
			if(DVDToc->file[i].flags == 2)//on DVD, 2 is a dir
			(*dir)[i].fileAttrib   = IS_DIR;
			if((*dir)[i].name[strlen((*dir)[i].name)-1] == '/' )
			(*dir)[i].name[strlen((*dir)[i].name)-1] = 0;	//get rid of trailing '/'
			(*dir)[i].meta = 0;
			usedSpace += (*dir)[i].size;
		}
		//kill the large TOC so we can have a lot more memory ingame (256k more)
		free(DVDToc);
		DVDToc = NULL;

		if(strlen((*dir)[0].name) == 0)
			strcpy( (*dir)[0].name, ".." );
	}
	usedSpace >>= 10;
	initial_DVD_info.freeSpaceInKB = initial_DVD_info.totalSpaceInKB - (u32)(usedSpace & 0xFFFFFFFF);
	return num_entries;
}

s32 deviceHandler_DVD_seekFile(file_handle* file, u32 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}

s32 deviceHandler_DVD_readFile(file_handle* file, void* buffer, u32 length){
	//print_gecko("read: status:%08X dst:%08X ofs:%08X base:%08X len:%08X\r\n"
	//						,file->status,(u32)buffer,file->offset,(u32)((file->fileBase) & 0xFFFFFFFF),length);
	if(file->size == 0) return 0;	// Don't read garbage
	u64 actualOffset = file->fileBase+file->offset;

	if(file->status == OFFSET_SET) {
		actualOffset = file->offset;
	}
	int bytesread = DVD_Read(buffer,actualOffset,length);
	if(bytesread > 0) {
		file->offset += bytesread;
	}
	return bytesread;
}

s32 deviceHandler_DVD_setupFile(file_handle* file, file_handle* file2, int numToPatch) {

	if(file2) {
		devices[DEVICE_CUR]->seekFile(file2, 0, DEVICE_HANDLER_SEEK_SET);
		devices[DEVICE_CUR]->readFile(file2, VAR_DISC_2_ID, sizeof(VAR_DISC_2_ID));
	}
	devices[DEVICE_CUR]->seekFile(file, 0, DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file, VAR_DISC_1_ID, sizeof(VAR_DISC_1_ID));

	// Multi-Game disc audio streaming setup
	if((dvdDiscTypeInt == COBRA_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD5_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC)) {
		dvddiskid *diskId = (dvddiskid*)VAR_DISC_1_ID;
		if(diskId->streaming && !isXenoGC) {
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "One moment, setting up audio streaming."));
			dvd_motor_off();
			print_gecko("Set extension %08X\r\n",dvd_get_error());
			dvd_setextension();
			print_gecko("Set extension - done\r\nUnlock %08X\r\n",dvd_get_error());
			dvd_unlock();
			print_gecko("Unlock - done\r\nDebug Motor On %08X\r\n",dvd_get_error());
			dvd_motor_on_extra();
			print_gecko("Debug Motor On - done\r\nSet Status %08X\r\n",dvd_get_error());
			dvd_setstatus();
			print_gecko("Set Status - done %08X\r\n",dvd_get_error());
			dvd_read_id();
			print_gecko("Read ID %08X\r\n",dvd_get_error());
			dvd_set_streaming(diskId->streaming);
			DrawDispose(msgBox);
		}
		dvd_set_offset(file->fileBase);
		file->status = OFFSET_SET;
		print_gecko("Streaming %s %08X\r\n",diskId->streaming?"Enabled":"Disabled",dvd_get_error());
	}

	// Set up Swiss game patches using a patch supporting device
	memset(VAR_FRAG_LIST, 0, sizeof(VAR_FRAG_LIST));

	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
		vu32 *fragList = (vu32*)VAR_FRAG_LIST;
		s32 frags = 0, totFrags = 0;
		
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		char gameID[8];
		memset(&gameID, 0, 8);
		strncpy((char*)&gameID, (char*)&GCMDisk, 4);
		
		for(i = 0; i < numToPatch; i++) {
			u32 patchInfo[4];
			memset(patchInfo, 0, 16);
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%.*sswiss_patches/%.4s/%i", PATHNAME_MAX-30, devices[DEVICE_PATCHES]->initial->name, gameID, i);
			print_gecko("Looking for file %s\r\n", &patchFile.name);
			FILINFO fno;
			if(f_stat(&patchFile.name[0], &fno) != FR_OK) {
				break;	// Patch file doesn't exist, don't bother with fragments
			}
			
			devices[DEVICE_PATCHES]->seekFile(&patchFile,fno.fsize-16,DEVICE_HANDLER_SEEK_SET);
			if((devices[DEVICE_PATCHES]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
				if(!(frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, patchInfo[0], patchInfo[1], DEVICE_PATCHES))) {
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
		if(swissSettings.igrType == IGR_BOOTBIN) {
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%.*sigr.dol", PATHNAME_MAX-8, devices[DEVICE_PATCHES]->initial->name);
			
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_IGR_DOL, 0, DEVICE_PATCHES))) {
				*(vu32*)VAR_IGR_DOL_SIZE = patchFile.size;
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		if(swissSettings.emulateMemoryCard) {
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%.*sswiss_patches/MemoryCardA.%s.raw", PATHNAME_MAX-34, devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
			
			devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
			
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_CARD_A, 16*1024*1024, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
			
			memset(&patchFile, 0, sizeof(file_handle));
			sprintf(&patchFile.name[0], "%.*sswiss_patches/MemoryCardB.%s.raw", PATHNAME_MAX-34, devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
			
			devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
			
			if((frags = getFragments(&patchFile, &fragList[totFrags*3], maxFrags-totFrags, FRAGS_CARD_B, 16*1024*1024, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		print_frag_list(0);
		// Card Type
		*(vu8*)VAR_SD_SHIFT = (u8)(sdgecko_getAddressingType(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)) ? 9:0);
		// Copy the actual freq
		*(vu8*)VAR_EXI_FREQ = (u8)(sdgecko_getSpeed(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2)));
		// Device slot (0, 1 or 2)
		*(vu8*)VAR_EXI_SLOT = (u8)(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2));
		*(vu32**)VAR_EXI_REGS = ((vu32(*)[5])0xCC006800)[*(vu8*)VAR_EXI_SLOT];
	}
	else {
		*(vu8*)VAR_SD_SHIFT = 32;
		*(vu8*)VAR_EXI_FREQ = EXI_SPEED1MHZ;
		*(vu8*)VAR_EXI_SLOT = EXI_CHANNEL_MAX;
		*(vu32**)VAR_EXI_REGS = NULL;
	}

	*(vu8*)VAR_DRIVE_PATCHED = drive_status == DEBUG_MODE;
	return 1;
}

s32 deviceHandler_DVD_init(file_handle* file){
  file->status = initialize_disc(ENABLE_BYDISK);
  if(file->status == DRV_ERROR){
	  uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"Failed to mount DVD. Press A");
	  DrawPublish(msgBox);
	  wait_press_A();
	  DrawDispose(msgBox);
	  return file->status;
  }
  dvd_init=1;
  return file->status;
}

s32 deviceHandler_DVD_deinit(file_handle* file) {
	dvd_motor_off();
	dvdDiscTypeStr = NotInitStr;
	return 0;
}

s32 deviceHandler_DVD_closeFile(file_handle* file){
	return 0;
}

bool deviceHandler_DVD_test() {
	return swissSettings.hasDVDDrive != 0;
}

u32 deviceHandler_DVD_emulated() {
	if (swissSettings.emulateMemoryCard)
		return EMU_MEMCARD;
	else
		return EMU_NONE;
}

DEVICEHANDLER_INTERFACE __device_dvd = {
	DEVICE_ID_0,
	"Disc Drive",
	"DVD",
	"Supported File System(s): GCM, ISO 9660, Multi-Game",
	{TEX_GCDVDSMALL, 84, 84},
	FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_HYPERVISOR|FEAT_AUDIO_STREAMING,
	EMU_MEMCARD,
	LOC_DVD_CONNECTOR,
	&initial_DVD,
	(_fn_test)&deviceHandler_DVD_test,
	(_fn_info)&deviceHandler_DVD_info,
	(_fn_init)&deviceHandler_DVD_init,
	(_fn_readDir)&deviceHandler_DVD_readDir,
	(_fn_readFile)&deviceHandler_DVD_readFile,
	(_fn_writeFile)NULL,
	(_fn_deleteFile)NULL,
	(_fn_seekFile)&deviceHandler_DVD_seekFile,
	(_fn_setupFile)&deviceHandler_DVD_setupFile,
	(_fn_closeFile)&deviceHandler_DVD_closeFile,
	(_fn_deinit)&deviceHandler_DVD_deinit,
	(_fn_emulated)&deviceHandler_DVD_emulated,
};
