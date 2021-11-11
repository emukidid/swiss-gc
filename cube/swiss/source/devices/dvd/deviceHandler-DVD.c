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
	DISC_SIZE,
	DISC_SIZE
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
		npdp_start();
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

		print_gecko("Multi game disc type %i detected\r\n", dvdDiscTypeInt);
		if(drive_status == NORMAL_MODE) {
			// This means we're using a drivechip, so our multigame command will be different.
			isXenoGC = 1;
			print_gecko("Drive isn't patched, drivechip is present.\r\n");
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
		print_gecko("%i entries found\r\n", num_entries);

		if(num_entries <= 0) { return num_entries; }
		// malloc the directory structure
		*dir = calloc( num_entries * sizeof(file_handle), 1 );
		
		// parse entries
		for(i = 0; i < num_entries; i++) {
			tmpOffset = (dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC) ? (tmpTable[i]<<2):(tmpTable[i]);
			if(num > 0) {
				(*dir)[num-1].size = tmpOffset - (*dir)[num-1].fileBase;
			}
			print_gecko("fileBase is %016llX isGC? %s\r\n", tmpOffset, isGC ? "yes" : "no");
			if((tmpOffset%(isGC?0x8000:0x20000)==0) && (tmpOffset<(isGC?DISC_SIZE:WII_D9_SIZE))) {
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
		}
		free(tmpTable);
		free(tmpName);
		usedSpace = (isGC?DISC_SIZE:WII_D9_SIZE);
	}
	else if((dvdDiscTypeInt == GAMECUBE_DISC) || (dvdDiscTypeInt == MULTIDISC_DISC)) {
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
	initial_DVD_info.freeSpace = initial_DVD_info.totalSpace - usedSpace;
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
	// Multi-Game disc audio streaming setup
	if((dvdDiscTypeInt == COBRA_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD5_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC)) {
		if(swissSettings.audioStreaming && !isXenoGC) {
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
			dvd_set_streaming(swissSettings.audioStreaming);
			DrawDispose(msgBox);
		}
		dvd_set_offset(file->fileBase);
		file->status = OFFSET_SET;
		print_gecko("Streaming %s %08X\r\n",swissSettings.audioStreaming?"Enabled":"Disabled",dvd_get_error());
	}
	if(numToPatch < 0) {
		return 1;
	}
	// Check if there are any fragments in our patch location for this game
	if(devices[DEVICE_PATCHES] != NULL) {
		int i;
		u32 (*fragList)[4] = NULL;
		s32 frags = 0, totFrags = 0;
		
		print_gecko("Save Patch device found\r\n");
		
		// Look for patch files, if we find some, open them and add them as fragments
		file_handle patchFile;
		for(i = 0; i < numToPatch; i++) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/patches/%.4s/%i", devices[DEVICE_PATCHES]->initial->name, (char*)&GCMDisk, i);
			
			if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) == 0) {
				u32 patchInfo[4];
				memset(patchInfo, 0, 16);
				devices[DEVICE_PATCHES]->seekFile(&patchFile, patchFile.size-16, DEVICE_HANDLER_SEEK_SET);
				if((devices[DEVICE_PATCHES]->readFile(&patchFile, &patchInfo, 16) == 16) && (patchInfo[2] == SWISS_MAGIC)) {
					if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
						return 0;
					}
					if(!(frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_DISC_1, patchInfo[0], patchInfo[1], DEVICE_PATCHES))) {
						free(fragList);
						return 0;
					}
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				else {
					devices[DEVICE_PATCHES]->deleteFile(&patchFile);
					free(fragList);
					return 0;
				}
			}
			else {
				free(fragList);
				return 0;
			}
		}
		
		if(!(fragList = realloc(fragList, (totFrags + 1 + !!file2 + 1) * sizeof(*fragList)))) {
			return 0;
		}
		fragList[totFrags][0] = 0;
		fragList[totFrags][1] = file->size;
		fragList[totFrags][2] = (u16)(file->fileBase >> 32) | ((u8)FRAGS_DISC_1 << 24);
		fragList[totFrags][3] = (u32)(file->fileBase);
		totFrags++;
		
		if(file2) {
			fragList[totFrags][0] = 0;
			fragList[totFrags][1] = file2->size;
			fragList[totFrags][2] = (u16)(file2->fileBase >> 32) | ((u8)FRAGS_DISC_2 << 24);
			fragList[totFrags][3] = (u32)(file2->fileBase);
			totFrags++;
		}
		
		if(swissSettings.igrType == IGR_BOOTBIN) {
			memset(&patchFile, 0, sizeof(file_handle));
			snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/patches/apploader.img", devices[DEVICE_PATCHES]->initial->name);
			
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
			
			if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
				return 0;
			}
			if((frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_APPLOADER, 0x2440, 0, DEVICE_PATCHES))) {
				totFrags+=frags;
				devices[DEVICE_PATCHES]->closeFile(&patchFile);
			}
		}
		
		if(swissSettings.emulateMemoryCard) {
			if(devices[DEVICE_PATCHES] != &__device_sd_a) {
				memset(&patchFile, 0, sizeof(file_handle));
				snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/saves/MemoryCardA.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				snprintf(txtbuffer, PATHNAME_MAX, "%sswiss/patches/MemoryCardA.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				f_rename(txtbuffer, &patchFile.name[0]);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
					return 0;
				}
				if((frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_CARD_A, 0, 31.5*1024*1024, DEVICE_PATCHES))) {
					*(vu8*)VAR_CARD_A_ID = (patchFile.size*8/1024/1024) & 0xFC;
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
			}
			
			if(devices[DEVICE_PATCHES] != &__device_sd_b) {
				memset(&patchFile, 0, sizeof(file_handle));
				snprintf(&patchFile.name[0], PATHNAME_MAX, "%sswiss/saves/MemoryCardB.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				snprintf(txtbuffer, PATHNAME_MAX, "%sswiss/patches/MemoryCardB.%s.raw", devices[DEVICE_PATCHES]->initial->name, wodeRegionToString(GCMDisk.RegionCode));
				ensure_path(DEVICE_PATCHES, "swiss/saves", NULL);
				f_rename(txtbuffer, &patchFile.name[0]);	// TODO remove this in our next major release
				
				if(devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0) != 0) {
					devices[DEVICE_PATCHES]->seekFile(&patchFile, 16*1024*1024, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_PATCHES]->writeFile(&patchFile, NULL, 0);
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
				
				if(!(fragList = realloc(fragList, (totFrags + MAX_FRAGS + 1) * sizeof(*fragList)))) {
					return 0;
				}
				if((frags = getFragments(&patchFile, &fragList[totFrags], MAX_FRAGS, FRAGS_CARD_B, 0, 31.5*1024*1024, DEVICE_PATCHES))) {
					*(vu8*)VAR_CARD_B_ID = (patchFile.size*8/1024/1024) & 0xFC;
					totFrags+=frags;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
				}
			}
		}
		
		if(fragList) {
			memset(&fragList[totFrags], 0, sizeof(*fragList));
			print_frag_list(fragList);
			
			*(vu32**)VAR_FRAG_LIST = installPatch2(fragList, (totFrags + 1) * sizeof(*fragList));
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
	*(vu8*)VAR_DRIVE_PATCHED = drive_status == DEBUG_MODE;
	memcpy(VAR_DISC_1_ID, (char*)&GCMDisk, sizeof(VAR_DISC_1_ID));
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
