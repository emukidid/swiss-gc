/* deviceHandler-DVD.c
	- device implementation for DVD (Discs)
	by emu_kidid
 */

#include <stdlib.h>
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
#include "flippy.h"
#include "patcher.h"
#include "dvd.h"
#include "gcm.h"
#include "wkf.h"

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
	DISC_SIZE,
	true
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
	uiDrawObj_t* progBar = DrawPublish(DrawProgressBar(true, 0, "DVD Initializing"));
	if(is_gamecube())
	{
		// Reset WKF hard to allow for a real disc to be read if SD is removed
		if(wkfDetected || (__wkfSpiReadId() != 0 && __wkfSpiReadId() != 0xFFFFFFFF)) {
			print_debug("Detected Wiikey Fusion with SPI Flash ID: %08X\n",__wkfSpiReadId());
			__wkfReset();
			print_debug("WKF RESET\n");
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
		else if((dvd_get_error()>>24) == 1) {  // Lid is open
			DrawDispose(progBar);
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
		dvd_reset();
		dvd_read_id();
		if((streaming == ENABLE_AUDIO) || (streaming == DISABLE_AUDIO)) {
			dvd_set_streaming(streaming);
		}
		else {
			dvd_set_streaming(*(char*)0x80000008);
		}
	}
	dvd_read_id();
	DrawDispose(progBar);
	if(dvd_get_error()) {
		return DRV_ERROR;
	}
			
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
  DVD_Read((void*)iso9660Buf,32768,32);
  
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
  else if (strncmp(iso9660Buf+1, "CD001", 5) == 0) {
	if ((*(vu32*)(&checkBuf[0x1C])) == DVD_MAGIC) {
	  dvdDiscTypeStr = ISO9660Str;
	  type = ISO9660_GAMECUBE_DISC;
	}
	else {
	  dvdDiscTypeStr = ISO9660Str;
	  type = ISO9660_DISC;  //"CD001" at 32769 is iso9660
	}
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

		print_debug("Multi game disc type %i detected\n", dvdDiscTypeInt);
		if(drive_status == NORMAL_MODE) {
			// This means we're using a drivechip, so our multigame command will be different.
			isXenoGC = 1;
			print_debug("Drive isn't patched, drivechip is present.\n");
		}
		//Read in the whole table of offsets
		tmpTable = (unsigned int*)memalign(32,MAX_MULTIGAME*4);
		tmpName = (char*)memalign(32,64);
		memset(tmpTable,0,MAX_MULTIGAME*4);
		memset(tmpName,0,64);
		DVD_Read(&tmpTable[0],MULTIGAME_TABLE_OFFSET,MAX_MULTIGAME*4);

		// count entries
		for(i = 0; i < MAX_MULTIGAME; i++) {
			tmpOffset = (dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC) ? (tmpTable[i]<<2):(tmpTable[i]);
			if((tmpOffset) && (tmpOffset%(isGC?0x8000:0x20000)==0) && (tmpOffset<(isGC?DISC_SIZE:WII_D9_SIZE))) {
				num_entries++;
			}
		}
		print_debug("%i entries found\n", num_entries);

		if(num_entries <= 0) { return num_entries; }
		// malloc the directory structure
		*dir = calloc(num_entries, sizeof(file_handle));
		
		// parse entries
		for(i = 0; i < num_entries; i++) {
			tmpOffset = (dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC) ? (tmpTable[i]<<2):(tmpTable[i]);
			if(num > 0) {
				(*dir)[num-1].size = tmpOffset - (*dir)[num-1].fileBase;
			}
			print_debug("fileBase is %016llX isGC? %s\n", tmpOffset, isGC ? "yes" : "no");
			if((tmpOffset%(isGC?0x8000:0x20000)==0) && (tmpOffset<(isGC?DISC_SIZE:WII_D9_SIZE))) {
				DVD_Read(&tmpName[0],tmpOffset+32,64);
				concatf_path((*dir)[num].name, ffile->name, "%.64s.gcm", &tmpName[0]);
				(*dir)[num].fileBase = tmpOffset;
				(*dir)[num].offset = 0;
				(*dir)[num].size   = (isGC?DISC_SIZE:WII_D9_SIZE)-tmpOffset;
				(*dir)[num].fileType = IS_FILE;
				(*dir)[num].meta = 0;
				(*dir)[num].status = STATUS_NOT_MAPPED;
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
	else if((dvdDiscTypeInt == ISO9660_DISC) || (dvdDiscTypeInt == ISO9660_GAMECUBE_DISC)) {
		// Call the corresponding DVD function
		num_entries = dvd_read_directoryentries(ffile->fileBase,ffile->size);
		// If it was not successful, just return the error
		if(num_entries <= 0) return -1;
		// Convert the DVD "file" data to fileBrowser_files
		*dir = calloc(num_entries, sizeof(file_handle));
		int i;
		for(i=0; i<num_entries; ++i){
			concat_path((*dir)[i].name, ffile->name, DVDToc->file[i].name);
			(*dir)[i].fileBase = (uint64_t)(((uint64_t)DVDToc->file[i].sector)*2048);
			(*dir)[i].offset = 0;
			(*dir)[i].size   = DVDToc->file[i].size;
			(*dir)[i].fileType = IS_FILE;
			if(DVDToc->file[i].flags == 2)//on DVD, 2 is a dir
			(*dir)[i].fileType = IS_DIR;
			if((*dir)[i].name[strlen((*dir)[i].name)-1] == '/' )
			(*dir)[i].name[strlen((*dir)[i].name)-1] = 0;	//get rid of trailing '/'
			(*dir)[i].meta = 0;
			usedSpace += (*dir)[i].size;
		}
		//kill the large TOC so we can have a lot more memory ingame (256k more)
		free(DVDToc);
		DVDToc = NULL;

		if(strcmp((*dir)[0].name, ffile->name) == 0) {
			concat_path((*dir)[0].name, ffile->name, "..");
			(*dir)[0].fileType = IS_SPECIAL;
		}
		else if(dvdDiscTypeInt == ISO9660_GAMECUBE_DISC) {
			DiskHeader *diskHeader = get_gcm_header(ffile);
			if(!diskHeader) return -1;

			*dir = reallocarray(*dir, num_entries + 1, sizeof(file_handle));
			memset(&(*dir)[num_entries], 0, sizeof(file_handle));
			concatf_path((*dir)[num_entries].name, ffile->name, "%.64s [%.6s].gcm", stripInvalidChars(diskHeader->GameName), diskHeader);
			(*dir)[num_entries].fileBase = 0;
			(*dir)[num_entries].offset = 0;
			(*dir)[num_entries].size = DISC_SIZE;
			(*dir)[num_entries].fileType = IS_FILE;
			(*dir)[num_entries].meta = 0;
			num_entries++;

			free(diskHeader);
		}
	}
	initial_DVD_info.freeSpace = initial_DVD_info.totalSpace - usedSpace;
	return num_entries;
}

s64 deviceHandler_DVD_seekFile(file_handle* file, s64 where, u32 type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
	else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
	return file->offset;
}

s32 deviceHandler_DVD_readFile(file_handle* file, void* buffer, u32 length){
	//print_debug("read: status:%08X dst:%08X ofs:%08X base:%08X len:%08X\n",
	//	file->status,(u32)buffer,file->offset,(u32)((file->fileBase) & 0xFFFFFFFF),length);
	if(file->size == 0) return 0;	// Don't read garbage
	u64 actualOffset = file->fileBase+file->offset;

	if(file->status == STATUS_MAPPED) {
		actualOffset = file->offset;
	}
	int bytesread = DVD_Read(buffer,actualOffset,length);
	if(bytesread > 0) {
		file->offset += bytesread;
	}
	return bytesread;
}

s32 deviceHandler_DVD_setupFile(file_handle* file, file_handle* file2, ExecutableFile* filesToPatch, int numToPatch) {
	int i;
	file_frag *fragList = NULL;
	u32 numFrags = 0;
	
	// Multi-Game disc audio streaming setup
	if((dvdDiscTypeInt == COBRA_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD5_MULTIGAME_DISC)||(dvdDiscTypeInt == GCOSD9_MULTIGAME_DISC)) {
		if(swissSettings.audioStreaming && !isXenoGC) {
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "One moment, setting up audio streaming."));
			dvd_motor_off();
			print_debug("Set extension %08X\n",dvd_get_error());
			dvd_setextension();
			print_debug("Set extension - done\nUnlock %08X\n",dvd_get_error());
			dvd_unlock();
			print_debug("Unlock - done\nDebug Motor On %08X\n",dvd_get_error());
			dvd_motor_on_extra();
			print_debug("Debug Motor On - done\nSet Status %08X\n",dvd_get_error());
			dvd_setstatus();
			print_debug("Set Status - done %08X\n",dvd_get_error());
			dvd_read_id();
			print_debug("Read ID %08X\n",dvd_get_error());
			dvd_set_streaming(swissSettings.audioStreaming);
			DrawDispose(msgBox);
		}
		dvd_set_offset(file->fileBase);
		file->status = STATUS_MAPPED;
		print_debug("Streaming %s %08X\n",swissSettings.audioStreaming?"Enabled":"Disabled",dvd_get_error());
	}
	if(numToPatch < 0 || !devices[DEVICE_CUR]->emulated()) {
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

s32 deviceHandler_DVD_init(file_handle* file){
	if(devices[DEVICE_CUR] == &__device_flippy || devices[DEVICE_CUR] == &__device_flippyflash) {
		return EBUSY;
	}
	if(swissSettings.hasFlippyDrive) flippy_bypass(true);
	if(!swissSettings.hasDVDDrive) return ENODEV;
	
	file->status = initialize_disc(ENABLE_BYDISK);
	dvd_init = file->status != DRV_ERROR;
	return dvd_init ? 0 : EIO;
}

s32 deviceHandler_DVD_deinit(file_handle* file) {
	dvd_motor_off();
	dvdDiscTypeStr = NotInitStr;
	return 0;
}

s32 deviceHandler_DVD_closeFile(file_handle* file){
	if(file && file->status == STATUS_MAPPED) {
		dvd_set_offset(0);
		file->status = STATUS_NOT_MAPPED;
	}
	return 0;
}

bool deviceHandler_DVD_test() {
	if (*DVDDeviceCode & 0x8000) {
		switch (*DVDDeviceCode & ~0x8000) {
			case 0x0000:
				__device_dvd.hwName = "NROM Reader";
				break;
			case 0x0001:
				__device_dvd.hwName = "NR Reader";
				break;
			case 0x0002:
				__device_dvd.hwName = "RVL-ROM Reader";
				break;
			case 0x0003:
				__device_dvd.hwName = "RVL-R Reader";
				break;
			case 0x0200:
				__device_dvd.hwName = "NPDP Reader";
				break;
			case 0x0201:
				__device_dvd.hwName = "GDEV";
				break;
			case 0x0202:
				__device_dvd.hwName = "NDEV";
				break;
			case 0x0203:
				__device_dvd.hwName = "RVL-H Reader";
				break;
			default:
				__device_dvd.hwName = "Unknown";
				break;
		}
	}
	return *DVDDeviceCode != 0x0001;
}

u32 deviceHandler_DVD_emulated() {
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

char* deviceHandler_DVD_status(file_handle* file) {
	if(!swissSettings.hasDVDDrive) {
		return NULL;
	}
	if(file->status == DRV_ERROR) {
		return dvd_error_str();
	}
	return NULL;
}

DEVICEHANDLER_INTERFACE __device_dvd = {
	.deviceUniqueId = DEVICE_ID_0,
	.hwName = "NROM Reader",
	.deviceName = "DVD",
	.deviceDescription = "Supported File System(s): GCM, ISO 9660, Multi-Game",
	.deviceTexture = {TEX_GCDVDSMALL, 84, 84, 84, 84},
	.features = FEAT_READ|FEAT_BOOT_GCM|FEAT_BOOT_DEVICE|FEAT_HYPERVISOR|FEAT_AUDIO_STREAMING,
	.quirks = QUIRK_NO_DEINIT,
	.emulable = EMU_READ|EMU_MEMCARD,
	.location = LOC_DVD_CONNECTOR,
	.initial = &initial_DVD,
	.test = deviceHandler_DVD_test,
	.info = deviceHandler_DVD_info,
	.init = deviceHandler_DVD_init,
	.readDir = deviceHandler_DVD_readDir,
	.seekFile = deviceHandler_DVD_seekFile,
	.readFile = deviceHandler_DVD_readFile,
	.closeFile = deviceHandler_DVD_closeFile,
	.setupFile = deviceHandler_DVD_setupFile,
	.deinit = deviceHandler_DVD_deinit,
	.emulated = deviceHandler_DVD_emulated,
	.status = deviceHandler_DVD_status,
};
