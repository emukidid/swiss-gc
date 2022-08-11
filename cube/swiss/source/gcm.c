#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "dvd.h"
#include "elf.h"
#include "gcm.h"
#include "main.h"
#include "nkit.h"
#include "util.h"
#include "swiss.h"
#include "cheats.h"
#include "patcher.h"
#include "sidestep.h"
#include "devices/deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "psoarchive/PRS.h"
#include <zlib.h>

#define FST_ENTRY_SIZE 12

// Parse disc header & read FST into a buffer
DiskHeader *get_gcm_header(file_handle *file) {
	DiskHeader *diskHeader;
	
	// Grab disc header
	diskHeader=(DiskHeader*)memalign(32,sizeof(DiskHeader));
	if(!diskHeader) return NULL;
	
	if(devices[DEVICE_CUR] == &__device_dvd && (dvdDiscTypeInt == GAMECUBE_DISC || dvdDiscTypeInt == MULTIDISC_DISC)) {
		if(DVD_Read(diskHeader, 0, sizeof(DiskHeader)) != sizeof(DiskHeader)) {
			free(diskHeader);
			return NULL;
		}
	}
	else {
		devices[DEVICE_CUR]->seekFile(file,0,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(file,diskHeader,sizeof(DiskHeader)) != sizeof(DiskHeader)) {
			free(diskHeader);
			return NULL;
		}
	}
	
	if(!valid_gcm_magic(diskHeader)) {
		free(diskHeader);
		return NULL;
	}
	return diskHeader;
}

char *get_fst(file_handle *file, u32 file_offset, u32 file_size) {
	char *FST;
	
	// Alloc and read FST
	FST=(char*)memalign(32,file_size);
	if(!FST) return NULL;
	
	if(devices[DEVICE_CUR] == &__device_dvd && (dvdDiscTypeInt == GAMECUBE_DISC || dvdDiscTypeInt == MULTIDISC_DISC)) {
		if(DVD_Read(FST, file_offset, file_size) != file_size) {
			free(FST);
			return NULL;
		}
	}
 	else {
		devices[DEVICE_CUR]->seekFile(file,file_offset,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(file,FST,file_size) != file_size) {
			free(FST);
			return NULL;
		}
	}
	return FST;
}

// Populate the file_offset and file_size for searchFileName from the GCM FST
void get_fst_details(char *FST, char *searchFileName, u32 *file_offset, u32 *file_size) {
	u32 filename_offset, entries, string_table_offset, offset, i;
	char filename[256];
	// number of entries and string table location
	entries = *(unsigned int*)&FST[8];
	string_table_offset=12*entries; 
    
	// go through every entry
	for (i=1;i<entries;i++) 
	{ 
		offset=i*0x0c; 
		if(FST[offset]==0) //skip directories
		{ 
			filename_offset=(unsigned int)FST[offset+1]*256*256+(unsigned int)FST[offset+2]*256+(unsigned int)FST[offset+3]; 
			memset(filename,0,256);
			strcpy(filename,&FST[string_table_offset+filename_offset]); 
			if(!strcasecmp(filename,searchFileName)) 
			{
				memcpy(file_offset,&FST[offset+4],4);
				memcpy(file_size,&FST[offset+8],4);
				return;
			}		
		} 
	}
	*file_offset = -1;
}

//Lets parse the entire game FST in search for the banner
void get_gcm_banner(file_handle *file, u32 *file_offset, u32 *file_size) {
	DiskHeader *diskHeader = get_gcm_header(file);
	if(!diskHeader) return;
	
	char *FST = get_fst(file, diskHeader->FSTOffset, diskHeader->FSTSize);
	if(!FST) return;
	
	get_fst_details(FST, "opening.bnr", file_offset, file_size);
	free(FST);
	free(diskHeader);
}

// Add a file to our current filesToPatch based on fileName
void parse_gcm_add(file_handle *file, ExecutableFile *filesToPatch, u32 *numToPatch, char *fileName) {
	DiskHeader *diskHeader = get_gcm_header(file);
	if(!diskHeader) return;
	
	char *FST = get_fst(file, diskHeader->FSTOffset, diskHeader->FSTSize);
	if(!FST) return;
	
	u32 file_offset, file_size;
	get_fst_details(FST, fileName, &file_offset, &file_size);
	free(FST);
	free(diskHeader);
	if(file_offset != -1) {
		filesToPatch[*numToPatch].offset = file_offset;
		filesToPatch[*numToPatch].size = file_size;
		filesToPatch[*numToPatch].type = endsWith(fileName,".prs") ? PATCH_OTHER_PRS:PATCH_OTHER;
		memcpy(&filesToPatch[*numToPatch].name,fileName,64); 
		*numToPatch += 1;
	}
}

bool valid_dol_file(file_handle *file, u32 file_offset, u32 file_size) {
	bool valid = false;
	
	if(file_size % 32 || file_size < DOLHDRLENGTH) {
		return false;
	}
	DOLHEADER dolhdr;
	devices[DEVICE_CUR]->seekFile(file, file_offset, DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file, &dolhdr, DOLHDRLENGTH);
	
	int i;
	for(i = 0; i < MAXTEXTSECTION; i++) {
		if(dolhdr.textOffset[i]) {
			if(dolhdr.textOffset[i] % 32 || dolhdr.textOffset[i] < DOLHDRLENGTH || dolhdr.textOffset[i] + dolhdr.textLength[i] > file_size) {
				return false;
			}
			if(dolhdr.textAddress[i] % 32 || dolhdr.textAddress[i] < 0x80003100 || dolhdr.textAddress[i] + dolhdr.textLength[i] > 0x81200000) {
				return false;
			}
			if(dolhdr.entryPoint >= dolhdr.textAddress[i] && dolhdr.entryPoint < dolhdr.textAddress[i] + dolhdr.textLength[i]) {
				valid = true;
			}
		}
	}
	for(i = 0; i < MAXDATASECTION; i++) {
		if(dolhdr.dataOffset[i]) {
			if(dolhdr.dataOffset[i] % 32 || dolhdr.dataOffset[i] < DOLHDRLENGTH || dolhdr.dataOffset[i] + dolhdr.dataLength[i] > file_size) {
				return false;
			}
			if(dolhdr.dataAddress[i] % 32 || dolhdr.dataAddress[i] < 0x80003100 || dolhdr.dataAddress[i] + dolhdr.dataLength[i] > 0x81200000) {
				return false;
			}
		}
	}
	if(dolhdr.bssLength) {
		if(dolhdr.bssAddress % 32 || dolhdr.bssAddress < 0x80003100 || dolhdr.bssAddress + dolhdr.bssLength > 0x81200000) {
			return false;
		}
	}
	return valid;
}

u32 calc_elf_segments_size(file_handle *file, u32 file_offset, u32 *file_size) {
	u32 size = 0;
	
	Elf32_Ehdr *ehdr = calloc(1, sizeof(Elf32_Ehdr));
	devices[DEVICE_CUR]->seekFile(file, file_offset, DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file, ehdr, sizeof(Elf32_Ehdr));
	if(!valid_elf_image(ehdr)) {
		free(ehdr);
		return size;
	}
	
	Elf32_Phdr *phdr = calloc(ehdr->e_phnum, sizeof(Elf32_Phdr));
	devices[DEVICE_CUR]->seekFile(file, file_offset + ehdr->e_phoff, DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));
	
	*file_size = ehdr->e_phoff + ehdr->e_phnum * sizeof(Elf32_Phdr);
	
	int i;
	for(i = 0; i < ehdr->e_phnum; i++) {
		if(phdr[i].p_type == PT_LOAD) {
			phdr[i].p_filesz = (phdr[i].p_filesz + 31) & ~31;
			if(phdr[i].p_offset + phdr[i].p_filesz > *file_size) {
				*file_size = phdr[i].p_offset + phdr[i].p_filesz;
			}
			size += phdr[i].p_filesz;
		}
	}
	free(ehdr);
	free(phdr);
	return size;
}

// Returns the number of filesToPatch and fills out the filesToPatch array passed in (pre-allocated)
int parse_gcm(file_handle *file, ExecutableFile *filesToPatch) {
	char	filename[256];
	int		dolOffset = 0, dolSize = 0, numFiles = 0;

	DiskHeader *diskHeader = get_gcm_header(file);
	if(!diskHeader) return 0;

	// Patch the apploader too!
	// Calc Apploader size
	ApploaderHeader apploaderHeader;
	devices[DEVICE_CUR]->seekFile(file,0x2440,DEVICE_HANDLER_SEEK_SET);
	if(devices[DEVICE_CUR]->readFile(file,&apploaderHeader,sizeof(ApploaderHeader)) != sizeof(ApploaderHeader)) {
		DrawPublish(DrawMessageBox(D_FAIL, "Failed to read Apploader Header"));
		while(1);
	}
	filesToPatch[numFiles].offset = 0x2440;
	filesToPatch[numFiles].size = sizeof(ApploaderHeader) + ((apploaderHeader.size + 31) & ~31) + ((apploaderHeader.rebootSize + 31) & ~31);
	filesToPatch[numFiles].type = PATCH_APPLOADER;
	sprintf(filesToPatch[numFiles].name, "apploader.img");
	numFiles++;

	if(diskHeader->DOLOffset != 0) {
		// Multi-DOL games may re-load the main DOL, so make sure we patch it too.
		// Calc size
		DOLHEADER dolhdr;
		devices[DEVICE_CUR]->seekFile(file,diskHeader->DOLOffset,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(file,&dolhdr,DOLHDRLENGTH) != DOLHDRLENGTH) {
			DrawPublish(DrawMessageBox(D_FAIL, "Failed to read Main DOL Header"));
			while(1);
		}
		filesToPatch[numFiles].offset = dolOffset = diskHeader->DOLOffset;
		filesToPatch[numFiles].size = dolSize = DOLSize(&dolhdr);
		filesToPatch[numFiles].type = PATCH_DOL;
		sprintf(filesToPatch[numFiles].name, "default.dol");
		numFiles++;
	}

	char *FST = get_fst(file, diskHeader->FSTOffset, diskHeader->FSTSize);
	if(!FST) return 0;

	u32 entries=*(unsigned int*)&FST[8];
	u32 string_table_offset=FST_ENTRY_SIZE*entries;
		
	int i;
	// go through every entry
	for (i=1;i<entries;i++) 
	{ 
		u32 offset=i*0x0c; 
		if(FST[offset]==0) //skip directories
		{ 
			u32 file_offset,size = 0;
			u32 filename_offset=((*(unsigned int*)&FST[offset]) & 0x00FFFFFF); 
			memset(&filename[0],0,256);
			memcpy(&filename[0],&FST[string_table_offset+filename_offset],255); 
			memcpy(&file_offset,&FST[offset+4],4);
			memcpy(&size,&FST[offset+8],4);
			if(endsWith(filename,".dol")) {
				if(strcasecmp(filename,"ffe.dol")) {
					// Some games contain a single "default.dol", these do not need 
					// pre-patching because they are what is actually pointed to by the apploader (and loaded by us)
					if(dolOffset == file_offset || dolSize == size || !valid_dol_file(file, file_offset, size)) {
						continue;
					}
				}
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_DOL;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(!strcasecmp(filename,"switcher.prs")) {
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_DOL_PRS;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(endsWith(filename,".elf")) {
				if(strcasecmp(filename,"STUBRDVD.ELF")) {
					if(dolSize == calc_elf_segments_size(file, file_offset, &size) + DOLHDRLENGTH) {
						continue;
					}
				}
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_ELF;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(!strcasecmp(filename,"execD.img")) {
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_BS2;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(endsWith(filename,".tgc")) {
				// Go through all the TGC's internal files
				numFiles += parse_tgc(file, &filesToPatch[numFiles], file_offset, filename);
			}
		} 
	}
	free(FST);
	free(diskHeader);
	
	// This need to be last.
	filesToPatch[numFiles].offset = 0;
	filesToPatch[numFiles].size = 0x2440;
	filesToPatch[numFiles].type = PATCH_OTHER;
	sprintf(filesToPatch[numFiles].name, "boot.bin");
	numFiles++;
	return numFiles;
}

// Adjust TGC FST entries in case we load a DOL from one directly
void adjust_tgc_fst(char* FST, u32 tgc_base, u32 fileAreaStart, u32 fakeAmount) {
	u32 entries=*(unsigned int*)&FST[8];
		
	int i;
	// go through every entry
	for (i=1;i<entries;i++) 
	{ 
		u32 offset=i*0x0c; 
		if(FST[offset]==0) //skip directories
		{ 
			u32 file_offset = 0;
			memcpy(&file_offset, &FST[offset+4], 4);
			*(unsigned int*)&FST[offset+4] = (file_offset-fakeAmount) + (tgc_base+fileAreaStart);
		} 
	}
}

int parse_tgc(file_handle *file, ExecutableFile *filesToPatch, u32 tgc_base, char* tgcname) {
	char	filename[256];
	int		numFiles = 0;
	
	TGCHeader tgcHeader;
	devices[DEVICE_CUR]->seekFile(file,tgc_base,DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file,&tgcHeader,sizeof(TGCHeader));
	
	if(tgcHeader.magic != TGC_MAGIC) return 0;
	if(tgcHeader.fstMaxLength > GCMDisk.MaxFSTSize) {
		GCMDisk.MaxFSTSize = tgcHeader.fstMaxLength;
	}
	
	if(tgcHeader.apploaderOffset != 0) {
		ApploaderHeader apploaderHeader;
		devices[DEVICE_CUR]->seekFile(file,tgc_base+tgcHeader.apploaderOffset,DEVICE_HANDLER_SEEK_SET);
		devices[DEVICE_CUR]->readFile(file,&apploaderHeader,sizeof(ApploaderHeader));
		
		filesToPatch[numFiles].offset = tgc_base+tgcHeader.apploaderOffset;
		filesToPatch[numFiles].size = sizeof(ApploaderHeader) + ((apploaderHeader.size + 31) & ~31) + ((apploaderHeader.rebootSize + 31) & ~31);
		filesToPatch[numFiles].type = PATCH_APPLOADER;
		sprintf(filesToPatch[numFiles].name, "%s/%s", tgcname, "apploader.img");
		numFiles++;
	}
	
	filesToPatch[numFiles].offset = tgc_base+tgcHeader.dolStart;
	filesToPatch[numFiles].size = tgcHeader.dolLength;
	filesToPatch[numFiles].type = PATCH_DOL;
	filesToPatch[numFiles].tgcFstOffset = tgc_base+tgcHeader.fstStart;
	filesToPatch[numFiles].tgcFstSize = tgcHeader.fstLength;
	filesToPatch[numFiles].tgcBase = tgc_base;
	filesToPatch[numFiles].tgcFileStartArea = tgcHeader.userStart;
	filesToPatch[numFiles].tgcFakeOffset = tgcHeader.gcmUserStart;
	sprintf(filesToPatch[numFiles].name, "%s/%s", tgcname, "default.dol");
	numFiles++;

	// Alloc and read FST
	char *FST = get_fst(file, tgc_base + tgcHeader.fstStart, tgcHeader.fstLength);

	// Adjust TGC FST offsets
	adjust_tgc_fst(FST, tgc_base, tgcHeader.userStart, tgcHeader.gcmUserStart);
	
	u32 entries=*(unsigned int*)&FST[8];
	u32 string_table_offset=FST_ENTRY_SIZE*entries;
		
	int i;
	// go through every entry
	for (i=1;i<entries;i++) 
	{ 
		u32 offset=i*0x0c; 
		if(FST[offset]==0) //skip directories
		{ 
			u32 file_offset,size = 0;
			u32 filename_offset=((*(unsigned int*)&FST[offset]) & 0x00FFFFFF); 
			memset(&filename[0],0,256);
			memcpy(&filename[0],&FST[string_table_offset+filename_offset],255); 
			memcpy(&file_offset,&FST[offset+4],4);
			memcpy(&size,&FST[offset+8],4);
			if(endsWith(filename,".dol")) {
				if(tgcHeader.dolLength == size || !valid_dol_file(file, file_offset, size)) {
					continue;
				}
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_DOL;
				filesToPatch[numFiles].tgcFstOffset = tgc_base+tgcHeader.fstStart;
				filesToPatch[numFiles].tgcFstSize = tgcHeader.fstLength;
				filesToPatch[numFiles].tgcBase = tgc_base;
				filesToPatch[numFiles].tgcFileStartArea = tgcHeader.userStart;
				filesToPatch[numFiles].tgcFakeOffset = tgcHeader.gcmUserStart;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(endsWith(filename,".elf")) {
				if(tgcHeader.dolLength == calc_elf_segments_size(file, file_offset, &size) + DOLHDRLENGTH) {
					continue;
				}
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_ELF;
				filesToPatch[numFiles].tgcFstOffset = tgc_base+tgcHeader.fstStart;
				filesToPatch[numFiles].tgcFstSize = tgcHeader.fstLength;
				filesToPatch[numFiles].tgcBase = tgc_base;
				filesToPatch[numFiles].tgcFileStartArea = tgcHeader.userStart;
				filesToPatch[numFiles].tgcFakeOffset = tgcHeader.gcmUserStart;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(!strcasecmp(filename,"execD.img")) {
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_BS2;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
		} 
	}
	free(FST);
	return numFiles;
}

int patch_gcm(file_handle *file, ExecutableFile *filesToPatch, int numToPatch) {
	int i, num_patched = 0;
	// If the current device isn't SD via EXI, init one slot to write patches.
	// TODO expand this to support IDE-EXI and other writable devices (requires dvd patch re-write/modularity)
	bool patchDeviceReady = false;
	if(devices[DEVICE_CUR]->features & FEAT_PATCHES) {
		if(devices[DEVICE_CUR] == &__device_gcloader && (devices[DEVICE_CONFIG] == &__device_sd_a || devices[DEVICE_CONFIG] == &__device_sd_b || devices[DEVICE_CONFIG] == &__device_sd_c)) {
			devices[DEVICE_PATCHES] = devices[DEVICE_CONFIG];
		}
		else {
			devices[DEVICE_PATCHES] = devices[DEVICE_CUR];
			patchDeviceReady = true;
		}
	}
	else {
		if(devices[DEVICE_CONFIG] == &__device_sd_a || devices[DEVICE_CONFIG] == &__device_sd_b || devices[DEVICE_CONFIG] == &__device_sd_c) {
			devices[DEVICE_PATCHES] = devices[DEVICE_CONFIG];
		}
		else if(deviceHandler_test(&__device_sd_c)) {
			devices[DEVICE_PATCHES] = &__device_sd_c;
		}
		else if(deviceHandler_test(&__device_sd_b)) {
			devices[DEVICE_PATCHES] = &__device_sd_b;
		}
		else if(deviceHandler_test(&__device_sd_a)) {
			devices[DEVICE_PATCHES] = &__device_sd_a;
		}
		else {
			devices[DEVICE_PATCHES] = NULL;
		}
	}

	if(devices[DEVICE_PATCHES] == NULL) {
		if(numToPatch > 0) {
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_FAIL, "No writable device available.\nAn SD Card Adapter is necessary in order\nfor patches to survive application restart."));
			sleep(5);
			DrawDispose(msgBox);
		}
		return 0;
	}

	char patchDirName[256];
	char gameID[16];
	memset(&gameID, 0, 16);
	memset(&patchDirName, 0, 256);
	strncpy((char*)&gameID, (char*)&GCMDisk, 4);
	snprintf(&patchDirName[0], 256, "swiss/patches/%.4s", &gameID[0]);
	memcpy((char*)&gameID, (char*)&GCMDisk, 12);
	print_gecko("Patch dir will be: %s%s if required\r\n", devices[DEVICE_PATCHES]->initial->name, patchDirName);
	// Go through all the possible files we think need patching..
	for(i = 0; i < numToPatch; i++) {
		u32 patched = 0;

		sprintf(txtbuffer, "Patching File %i/%i\n%s [%iKB]",i+1,numToPatch,filesToPatch[i].name,filesToPatch[i].size/1024);
		
		if(filesToPatch[i].size > 8*1024*1024) {
			print_gecko("Skipping %s %iKB too large\r\n", filesToPatch[i].name, filesToPatch[i].size/1024);
			continue;
		}
		print_gecko("Checking %s %iKb\r\n", filesToPatch[i].name, filesToPatch[i].size/1024);
		
		if(!strcasecmp(filesToPatch[i].name, "iwanagaD.dol") || !strcasecmp(filesToPatch[i].name, "switcherD.dol")) {
			continue;	// skip unused PSO files
		}
		uiDrawObj_t* progBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));
		int sizeToRead = (filesToPatch[i].size + 31) & ~31;
		void *buffer = memalign(32, sizeToRead);
		
		devices[DEVICE_CUR]->seekFile(file,filesToPatch[i].offset, DEVICE_HANDLER_SEEK_SET);
		int ret = devices[DEVICE_CUR]->readFile(file,buffer,sizeToRead);
		print_gecko("Read from %08X Size %08X - Result: %08X\r\n", filesToPatch[i].offset, sizeToRead, ret);
		if(ret != sizeToRead) {
			DrawDispose(progBox);			
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Failed to read!"));
			sleep(5);
			DrawDispose(msgBox);
			return 0;
		}
		
		u8 *oldBuffer = NULL, *newBuffer = NULL;
		if(filesToPatch[i].type == PATCH_DOL_PRS || filesToPatch[i].type == PATCH_OTHER_PRS) {
			sizeToRead = pso_prs_decompress_buf(buffer, &newBuffer, filesToPatch[i].size);
			if(sizeToRead < 0) {
				DrawDispose(progBox);
				uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Failed to decompress!"));
				sleep(5);
				DrawDispose(msgBox);
				return 0;
			}
			oldBuffer = buffer;
			buffer = newBuffer;
		}
		
		// Patch raw files for certain games
		if(filesToPatch[i].type == PATCH_OTHER || filesToPatch[i].type == PATCH_OTHER_PRS) {
			patched += Patch_GameSpecificFile(buffer, sizeToRead, gameID, filesToPatch[i].name);
		}
		else { 
			// Patch executable files
			if(devices[DEVICE_CUR]->features & FEAT_HYPERVISOR) {
				patched += Patch_Hypervisor(buffer, sizeToRead, filesToPatch[i].type);
				patched += Patch_GameSpecificHypervisor(buffer, sizeToRead, gameID, filesToPatch[i].type);
			}
		
			// Patch specific game hacks
			patched += Patch_GameSpecific(buffer, sizeToRead, gameID, filesToPatch[i].type);
			
			// Patch CARD, PAD
			patched += Patch_Miscellaneous(buffer, sizeToRead, filesToPatch[i].type);
			
			if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
				Patch_CheatsHook(buffer, sizeToRead, filesToPatch[i].type);
			}
			
			patched += Patch_FontEncode(buffer, sizeToRead);
			
			if(swissSettings.disableVideoPatches < 2) {
				if(swissSettings.disableVideoPatches < 1) {
					Patch_GameSpecificVideo(buffer, sizeToRead, gameID, filesToPatch[i].type);
				}
				Patch_VideoMode(buffer, sizeToRead, filesToPatch[i].type);
			}
			
			if(swissSettings.forceWidescreen)
				Patch_Widescreen(buffer, sizeToRead, filesToPatch[i].type);
			if(swissSettings.forceAnisotropy)
				Patch_TexFilt(buffer, sizeToRead, filesToPatch[i].type);
		}
		
		if(filesToPatch[i].type == PATCH_DOL_PRS || filesToPatch[i].type == PATCH_OTHER_PRS) {
			sizeToRead = pso_prs_compress2(buffer, oldBuffer, sizeToRead, filesToPatch[i].size);
			if(sizeToRead < 0) {
				DrawDispose(progBox);
				uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Failed to recompress!"));
				sleep(5);
				DrawDispose(msgBox);
				return 0;
			}
			filesToPatch[i].size = sizeToRead;
			sizeToRead = (filesToPatch[i].size + 31) & ~31;
			buffer = oldBuffer;
			oldBuffer = NULL;
			free(newBuffer);
			newBuffer = NULL;
		}
		
		if(patched) {
			if(!patchDeviceReady) {
				deviceHandler_setStatEnabled(0);
				if(!devices[DEVICE_PATCHES]->init(devices[DEVICE_PATCHES]->initial)) {
					deviceHandler_setStatEnabled(1);
					DrawDispose(progBox);
					return false;
				}
				deviceHandler_setStatEnabled(1);
				patchDeviceReady = true;
			}
			
			// Make /swiss/, it'll likely exist already anyway.
			ensure_path(DEVICE_PATCHES, "swiss", NULL);
			
			// If the old directory exists, lets move it to the new location (swiss_patches is now just patches under /swiss/)
			ensure_path(DEVICE_PATCHES, "swiss/patches", "swiss_patches");	// TODO kill this off in our next major release.
			ensure_path(DEVICE_PATCHES, patchDirName, NULL);
			
			// File handle for a patch we might need to write
			file_handle patchFile;
			memset(&patchFile, 0, sizeof(file_handle));
			concatf_path(patchFile.name, devices[DEVICE_PATCHES]->initial->name, "swiss/patches/%.4s/%i", gameID, num_patched);

			// Make patch trailer
			u32 patchInfo[4];
			patchInfo[0] = filesToPatch[i].offset;
			patchInfo[1] = filesToPatch[i].size;
			patchInfo[2] = SWISS_MAGIC;
			patchInfo[3] = crc32(0, buffer, sizeToRead);

			// See if this file already exists, if it does, match crc
			if(!devices[DEVICE_PATCHES]->readFile(&patchFile, NULL, 0)) {
				//print_gecko("Old Patch exists\r\n");
				u32 oldPatchInfo[4];
				memset(oldPatchInfo, 0, 16);
				devices[DEVICE_PATCHES]->seekFile(&patchFile, -16, DEVICE_HANDLER_SEEK_END);
				devices[DEVICE_PATCHES]->readFile(&patchFile, oldPatchInfo, 16);
				if(!memcmp(oldPatchInfo, patchInfo, 16)) {
					num_patched++;
					devices[DEVICE_PATCHES]->closeFile(&patchFile);
					free(buffer);
					print_gecko("CRC matched, no need to patch again\r\n");
					DrawDispose(progBox);
					continue;
				}
				else {
					devices[DEVICE_PATCHES]->deleteFile(&patchFile);
					print_gecko("CRC mismatch, writing patch again\r\n");
				}
			}
			// Otherwise, write a file out for this game with the patched buffer inside.
			print_gecko("Writing patch file: %s %i bytes (disc offset %08X)\r\n", patchFile.name, filesToPatch[i].size, filesToPatch[i].offset);
			devices[DEVICE_PATCHES]->seekFile(&patchFile, 0, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, buffer, sizeToRead);
			devices[DEVICE_PATCHES]->writeFile(&patchFile, patchInfo, 16);
			devices[DEVICE_PATCHES]->closeFile(&patchFile);
			num_patched++;
		}
		DrawDispose(progBox);
		free(buffer);
	}

	return num_patched;
}

u64 calc_fst_entries_size(char *FST) {
	
	u64 totalSize = 0LL;
	u32 entries=*(unsigned int*)&FST[8];
	int i;
	for (i=1;i<entries;i++)	// go through every entry
	{ 
		if(!FST[i*0x0c]) {	// Skip directories
			totalSize += *(u32*)&FST[(i*0x0c)+8];
		} 
	}
	return totalSize;
}

// Returns the number of filesToPatch and fills out the filesToPatch array passed in (pre-allocated)
int read_fst(file_handle *file, file_handle** dir, u64 *usedSpace) {

	print_gecko("Read dir for directory: %s\r\n",file->name);
	char	filename[PATHNAME_MAX];
	int		numFiles = 1, idx = 0;
	int		isRoot = !strcmp((const char*)&file->name[0], "dvd:/");
	
	DiskHeader *diskHeader = get_gcm_header(file);
	if(!diskHeader) return -1;
	
	char *FST = get_fst(file, diskHeader->FSTOffset, diskHeader->FSTSize);
	if(!FST) return -1;
	
	// Get the space taken up by this disc
	if(usedSpace) *usedSpace = calc_fst_entries_size(FST);
		
	// Add the disc itself as a "file"
	*dir = calloc( numFiles * sizeof(file_handle), 1 );
	concatf_path((*dir)[idx].name, __device_dvd.initial->name, "%.64s.gcm", diskHeader->GameName);
	(*dir)[idx].fileBase = 0;
	(*dir)[idx].offset = 0;
	(*dir)[idx].size = DISC_SIZE;
	(*dir)[idx].fileAttrib = IS_FILE;
	(*dir)[idx].meta = 0;
	idx++;
	
	u32 entries=*(unsigned int*)&FST[8];
	u32 string_table_offset=FST_ENTRY_SIZE*entries;
		
	int i;
	// Go through the FST and find our DIR (or ROOT)
	int parent_dir_offset = (u32)file->fileBase;
	int dir_end_offset = isRoot ? *(u32*)&FST[8] : 0;
	isRoot = parent_dir_offset==0;
	
	// Haxx because we navigate GCM FST by offset and not name
	if(endsWith(file->name, "..") && strlen(file->name) > 2) {
		int len = strlen(file->name)-3;
		while(len && file->name[len-1]!='/')
      		len--;
		file->name[len-1] = 0;
	}
	
	u32 filename_offset=((*(u32*)&FST[parent_dir_offset*0x0C]) & 0x00FFFFFF); 
	concat_path(filename, file->name, &FST[string_table_offset+filename_offset]);
	dir_end_offset = *(u32*)&FST[(parent_dir_offset*0x0C) + 8];
	
	if(!isRoot) {
		// Add a special ".." dir which will take us back up a dir
		if(idx == numFiles){
			++numFiles;
			*dir = realloc( *dir, numFiles * sizeof(file_handle) ); 
		}
		memset(&(*dir)[idx], 0, sizeof(file_handle));
		concat_path((*dir)[idx].name, file->name, "..");
		(*dir)[idx].fileBase = *(u32*)&FST[(parent_dir_offset*0x0C)+4];
		(*dir)[idx].offset = 0;
		(*dir)[idx].size = 0;
		(*dir)[idx].fileAttrib = IS_DIR;
		(*dir)[idx].meta = 0;
		idx++;
	}
	//print_gecko("Found DIR [%03i]:%s\r\n",parent_dir_offset,isRoot ? "ROOT":filename);
	
	// Traverse the FST now adding all the files which come under our DIR
	for (i=parent_dir_offset;i<dir_end_offset;i++) {
		if(i==0) continue;
		
		u32 offset=i*0x0c;
		u32 file_offset,size = 0;
		u32 filename_offset=((*(unsigned int*)&FST[offset]) & 0x00FFFFFF); 
		concat_path(filename, file->name, &FST[string_table_offset+filename_offset]);
		memcpy(&file_offset,&FST[offset+4],4);
		memcpy(&size,&FST[offset+8],4);
		
		// Is this a sub dir of our dir?
		if(FST[offset]) {
			if(file_offset == parent_dir_offset) {
				//print_gecko("Adding: [%03i]%s:%s offset %08X length %08X\r\n",i,!FST[offset] ? "File" : "Dir",filename,file_offset,size);
				if(idx == numFiles){
					++numFiles;
					*dir = realloc( *dir, numFiles * sizeof(file_handle) ); 
				}
				memset(&(*dir)[idx], 0, sizeof(file_handle));
				strcpy((*dir)[idx].name, filename);
				(*dir)[idx].fileBase = i;
				(*dir)[idx].offset = 0;
				(*dir)[idx].size = size;
				(*dir)[idx].fileAttrib = IS_DIR;
				(*dir)[idx].meta = 0;
				idx++;
				// Skip the entries that sit in this dir
				i = size-1;
			}
		}
		else {
			// File, add it.
			//print_gecko("Adding: [%03i]%s:%s offset %08X length %08X\r\n",i,!FST[offset] ? "File" : "Dir",filename,file_offset,size);
			if(idx == numFiles){
				++numFiles;
				*dir = realloc( *dir, numFiles * sizeof(file_handle) ); 
			}
			memset(&(*dir)[idx], 0, sizeof(file_handle));
			strcpy((*dir)[idx].name, filename);
			(*dir)[idx].fileBase = file_offset;
			(*dir)[idx].offset = 0;
			(*dir)[idx].size = size;
			(*dir)[idx].fileAttrib = IS_FILE;
			(*dir)[idx].meta = 0;
			idx++;
		}
	}
	
	free(FST);
	free(diskHeader);
	return numFiles;
}
