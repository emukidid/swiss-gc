#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "gcm.h"
#include "main.h"
#include "dvd.h"
#include "swiss.h"
#include "cheats.h"
#include "patcher.h"
#include "sidestep.h"
#include "crc32/crc32.h"
#include "devices/deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "../../reservedarea.h"
#include <sys/stat.h>
#include <errno.h>

#define FST_ENTRY_SIZE 12

//Lets parse the entire game FST in search for the banner
unsigned int getBannerOffset(file_handle *file) {
	unsigned int   	buffer[2]; 
	unsigned long   fst_offset=0,filename_offset=0,entries=0,string_table_offset=0,fst_size=0; 
	unsigned long   i,offset; 
	char   filename[256]; 
	
	// lets just quickly check if this is a valid GCM (contains magic)
	devices[DEVICE_CUR]->seekFile(file,0x1C,DEVICE_HANDLER_SEEK_SET);
	if((devices[DEVICE_CUR]->readFile(file,(unsigned char*)buffer,sizeof(int))!=sizeof(int)) || (buffer[0]!=0xC2339F3D)) {
		return 0;
	}

	// get FST offset and size
	devices[DEVICE_CUR]->seekFile(file,0x424,DEVICE_HANDLER_SEEK_SET);
	if(devices[DEVICE_CUR]->readFile(file,(unsigned char*)buffer,sizeof(int)*2)!=sizeof(int)*2) {
		return 0;
	}  
  
	fst_offset	=buffer[0];
	fst_size	=buffer[1];

	if((!fst_offset) || (!fst_size)) {
		return 0;
	}
      
	// allocate space for FST table
	char* FST = (char*)memalign(32,fst_size);		//malloc dies on NTSC for some reason.. bad libogc! 
	if(!FST) {
		return 0;	//didn't fit, doesn't matter it's just for a banner
	}
	
	// read the FST
	devices[DEVICE_CUR]->seekFile(file,fst_offset,DEVICE_HANDLER_SEEK_SET);
	if(devices[DEVICE_CUR]->readFile(file,FST,fst_size)!=fst_size) {
		free(FST);
		return 0;
	}
  
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
			unsigned int loc = 0;
			memcpy(&loc,&FST[offset+4],4);
			if((!strcasecmp(filename,"opening.bnr")) || (!strcasecmp(filename,"OPENING.BNR"))) 
			{
				free(FST);
				return loc;
			}		
		} 
	}
	free(FST);
	return 0;
}

// Returns the number of filesToPatch and fills out the filesToPatch array passed in (pre-allocated)
int parse_gcm(file_handle *file, ExecutableFile *filesToPatch) {

	DiskHeader header;
	char	*FST; 
	char	filename[256];
	int		numFiles = 0;
		
	// Grab disc header
	memset(&header,0,sizeof(DiskHeader));
 	devices[DEVICE_CUR]->seekFile(file,0,DEVICE_HANDLER_SEEK_SET);
 	if(devices[DEVICE_CUR]->readFile(file,&header,sizeof(DiskHeader)) != sizeof(DiskHeader)) {
		return -1;
	}
 	  
 	// Alloc and read FST
	FST=(char*)memalign(32,header.FSTSize); 
	if(!FST) {
		return -1;
	}
	devices[DEVICE_CUR]->seekFile(file,header.FSTOffset,DEVICE_HANDLER_SEEK_SET);
 	if(devices[DEVICE_CUR]->readFile(file,FST,header.FSTSize) != header.FSTSize) {
		free(FST); 
		return -1;
	}

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
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_DOL;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(endsWith(filename,".elf") && size < 12*1024*1024) {
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_ELF;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(strstr(filename,"execD.img")) {
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_LOADER;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(endsWith(filename,".tgc")) {
				// Go through all the TGC's internal files
				ExecutableFile *filesInTGCToPatch = memalign(32, sizeof(ExecutableFile)*32);
				int numTGCFilesToPatch = parse_tgc(file, filesInTGCToPatch, file_offset), j;
				for(j=0; j<numTGCFilesToPatch; j++) {
					memcpy(&filesToPatch[numFiles], &filesInTGCToPatch[j], sizeof(ExecutableFile));
					numFiles++;
				}
				free(filesInTGCToPatch);
			}
		} 
	}
	free(FST);
	
	// Some games contain a single "default.dol", these do not need 
	// pre-patching because they are what is actually pointed to by the apploader (and loaded by us)
	if(numFiles==1 && (!strcmp(&filesToPatch[0].name[0],"default.dol"))) {
		numFiles = 0;
	}
	// Multi-DOL games may re-load the main DOL, so make sure we patch it too.
	if(numFiles>0) {
		DOLHEADER dolhdr;
		u32 main_dol_size = 0;
		// Calc size
		devices[DEVICE_CUR]->seekFile(file,GCMDisk.DOLOffset,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(file,&dolhdr,DOLHDRLENGTH) != DOLHDRLENGTH) {
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Failed to read Main DOL Header");
			DrawFrameFinish();
			while(1);
		}
		for (i = 0; i < MAXTEXTSECTION; i++) {
			if (dolhdr.textLength[i] + dolhdr.textOffset[i] > main_dol_size)
				main_dol_size = dolhdr.textLength[i] + dolhdr.textOffset[i];
		}
		for (i = 0; i < MAXDATASECTION; i++) {
			if (dolhdr.dataLength[i] + dolhdr.dataOffset[i] > main_dol_size)
				main_dol_size = dolhdr.dataLength[i] + dolhdr.dataOffset[i];
		}
		filesToPatch[numFiles].offset = GCMDisk.DOLOffset;
		filesToPatch[numFiles].size = main_dol_size;
		filesToPatch[numFiles].type = PATCH_DOL;
		sprintf(filesToPatch[numFiles].name, "Main DOL");
		numFiles++;
		
		// Patch the apploader too!
		// Calc Apploader trailer size
		u32 appldr_info[2];
		devices[DEVICE_CUR]->seekFile(file,0x2454,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(file,&appldr_info,8) != 8) {
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Failed to read Apploader info");
			DrawFrameFinish();
			while(1);
		}
		filesToPatch[numFiles].size = appldr_info[1];
		filesToPatch[numFiles].offset = 0x2460 + appldr_info[0];
		filesToPatch[numFiles].type = PATCH_LOADER;
		sprintf(filesToPatch[numFiles].name, "Apploader Trailer");
		numFiles++;
	}
	return numFiles;
}

int parse_tgc(file_handle *file, ExecutableFile *filesToPatch, u32 tgc_base) {
	char	*FST; 
	char	filename[256];
	u32 fileAreaStart, fakeAmount, offset, size, numFiles = 0;
	
	// add this embedded GCM's main DOL
	devices[DEVICE_CUR]->seekFile(file,tgc_base+0x1C,DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file,&offset,4);
	devices[DEVICE_CUR]->readFile(file,&size,4);
	devices[DEVICE_CUR]->readFile(file, &fileAreaStart, 4);
	devices[DEVICE_CUR]->seekFile(file,tgc_base+0x34,DEVICE_HANDLER_SEEK_SET);
	devices[DEVICE_CUR]->readFile(file, &fakeAmount, 4);
	filesToPatch[numFiles].offset = offset+tgc_base;
	filesToPatch[numFiles].size = size;
	filesToPatch[numFiles].type = PATCH_DOL;
	strcpy(&filesToPatch[numFiles].name[0],"TGC Main DOL");
	numFiles++;
	
	// Grab FST Offset & Size
	u32 fstOfsAndSize[2];
 	devices[DEVICE_CUR]->seekFile(file,tgc_base+0x10,DEVICE_HANDLER_SEEK_SET);
 	devices[DEVICE_CUR]->readFile(file,&fstOfsAndSize,2*sizeof(u32));

 	// Alloc and read FST
	FST=(char*)memalign(32,fstOfsAndSize[1]); 
	devices[DEVICE_CUR]->seekFile(file,tgc_base+fstOfsAndSize[0],DEVICE_HANDLER_SEEK_SET);
 	devices[DEVICE_CUR]->readFile(file,FST,fstOfsAndSize[1]);

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
				filesToPatch[numFiles].offset = (file_offset-fakeAmount)+(tgc_base+fileAreaStart);
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_DOL;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(endsWith(filename,".elf") && size < 12*1024*1024) {
				filesToPatch[numFiles].offset = (file_offset-fakeAmount)+(tgc_base+fileAreaStart);
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_ELF;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if(strstr(filename,"execD.img")) {
				filesToPatch[numFiles].offset = (file_offset-fakeAmount)+(tgc_base+fileAreaStart);
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_LOADER;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
		} 
	}
	free(FST);
	return numFiles;
	
}

int patch_gcm(file_handle *file, ExecutableFile *filesToPatch, int numToPatch, int multiDol) {
	int i, num_patched = 0;

	// If the current device isn't SD Gecko, init one slot to write patches.
	// TODO expand this to support IDE-EXI and other writable devices (requires dvd patch re-write/modularity)
	bool patchDeviceReady = false;
	if(devices[DEVICE_CUR] != &__device_sd_a && devices[DEVICE_CUR] != &__device_sd_b) {
		if(deviceHandler_test(&__device_sd_a)) {
			devices[DEVICE_PATCHES] = &__device_sd_a;
		}
		else if(deviceHandler_test(&__device_sd_b)) {
			devices[DEVICE_PATCHES] = &__device_sd_b;
		}
	}
	else {
		devices[DEVICE_PATCHES] = devices[DEVICE_CUR];
		patchDeviceReady = true;
	}
		
	if(devices[DEVICE_PATCHES] == NULL) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "No writable device present\nA SD Gecko must be inserted in\n order to utilise patches for this game.");
		DrawFrameFinish();
		sleep(5);
		return 0;
	}

	char patchFileName[256];
	char patchDirName[256];
	char patchBaseDirName[256];
	char gameID[8];
	memset(&gameID, 0, 8);
	memset(&patchDirName, 0, 256);
	memset(&patchBaseDirName, 0, 256);
	strncpy((char*)&gameID, (char*)&GCMDisk, 4);
	sprintf(&patchDirName[0],"%sswiss_patches/%s",devices[DEVICE_PATCHES]->initial->name, &gameID[0]);
	sprintf(&patchBaseDirName[0],"%sswiss_patches",devices[DEVICE_PATCHES]->initial->name);
	print_gecko("Patch dir will be: %s if required\r\n", patchDirName);
	*(vu32*)VAR_EXECD_OFFSET = 0xFFFFFFFF;
	// Go through all the possible files we think need patching..
	for(i = 0; i < numToPatch; i++) {
		u32 patched = 0, crc32 = 0;

		sprintf(txtbuffer, "Patching File %i/%i",i+1,numToPatch);
		DrawFrameStart();
		DrawProgressBar((int)(((float)(i+1)/(float)numToPatch)*100), txtbuffer);
		DrawFrameFinish();
			
		// Round up to 32 bytes
		if(filesToPatch[i].size % 0x20) {
			filesToPatch[i].size += (0x20-(filesToPatch[i].size%0x20));
		}
		
		if(filesToPatch[i].size > 8*1024*1024) {
			print_gecko("Skipping %s %iKB too large\r\n", filesToPatch[i].name, filesToPatch[i].size/1024);
			continue;
		}
		print_gecko("Checking %s %iKb\r\n", filesToPatch[i].name, filesToPatch[i].size/1024);
		if(strstr(filesToPatch[i].name, "execD.")) {
			*(vu32*)VAR_EXECD_OFFSET = filesToPatch[i].offset;
		}
		if(strstr(filesToPatch[i].name, "iwanagaD.dol") || strstr(filesToPatch[i].name, "switcherD.dol")) {
			continue;	// skip unused PSO files
		}
		int sizeToRead = filesToPatch[i].size, ret = 0;
		u8 *buffer = (u8*)memalign(32, sizeToRead);
		
		devices[DEVICE_CUR]->seekFile(file,filesToPatch[i].offset, DEVICE_HANDLER_SEEK_SET);
		ret = devices[DEVICE_CUR]->readFile(file,buffer,sizeToRead);
		print_gecko("Read from %08X Size %08X - Result: %08X\r\n", filesToPatch[i].offset, sizeToRead, ret);
		if(ret != sizeToRead) {
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Failed to read!");
			DrawFrameFinish();
			sleep(5);
			return 0;
		}
		
		if(devices[DEVICE_CUR] != &__device_dvd && devices[DEVICE_CUR] != &__device_wkf) {
			ret = Patch_DVDLowLevelRead(buffer, sizeToRead, filesToPatch[i].type);
			if(READ_PATCHED_ALL != ret)	{
				DrawFrameStart();
				DrawMessageBox(D_FAIL, "Failed to find necessary functions for patching!");
				DrawFrameFinish();
				sleep(5);
			}
			else
				patched += 1;
		}
		
		// Patch specific game hacks
		if(devices[DEVICE_CUR] != &__device_dvd && devices[DEVICE_CUR] != &__device_wkf) {
			patched += Patch_GameSpecific(buffer, sizeToRead, &gameID[0], filesToPatch[i].type);
		}
		
		// Patch IGR
		if(swissSettings.igrType != IGR_OFF) {
			patched += Patch_IGR(buffer, sizeToRead, filesToPatch[i].type);
		}
				
		if(swissSettings.debugUSB && usb_isgeckoalive(1) && !swissSettings.wiirdDebug) {
			patched += Patch_Fwrite(buffer, sizeToRead);
		}
		
		if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
			Patch_CheatsHook(buffer, sizeToRead, filesToPatch[i].type);
		}
		
		if(devices[DEVICE_CUR] == &__device_dvd && is_gamecube()) {
			patched += Patch_DVDLowLevelReadForDVD(buffer, sizeToRead, filesToPatch[i].type);
			patched += Patch_DVDReset(buffer, sizeToRead);
		}
		
		if(devices[DEVICE_CUR] == &__device_wkf) {
			patched += Patch_DVDLowLevelReadForWKF(buffer, sizeToRead, filesToPatch[i].type);
		}
		
		patched += Patch_FontEnc(buffer, sizeToRead);
		if(swissSettings.gameVMode > 0)
			Patch_VidMode(buffer, sizeToRead, filesToPatch[i].type);
		if(swissSettings.forceWidescreen)
			Patch_WideAspect(buffer, sizeToRead, filesToPatch[i].type);
		if(swissSettings.forceAnisotropy)
			Patch_TexFilt(buffer, sizeToRead, filesToPatch[i].type);
		if(patched) {
			if(!patchDeviceReady) {
				deviceHandler_setStatEnabled(0);
				if(!devices[DEVICE_PATCHES]->init(devices[DEVICE_PATCHES]->initial)) {
					deviceHandler_setStatEnabled(1);
					return false;
				}
				deviceHandler_setStatEnabled(1);
				patchDeviceReady = true;
			}
			// File handle for a patch we might need to write
			FILE *patchFile = 0;
			memset(patchFileName, 0, 256);
		
			// Make a base patches dir if we don't have one already
			if(mkdir(&patchBaseDirName[0], 0777) != 0) {
				if(errno != EEXIST) {
					return -2;
				}
			}
			if(mkdir(&patchDirName[0], 0777) != 0) {
				if(errno != EEXIST) {
					return -2;
				}
			}
			sprintf(patchFileName, "%s/%i",patchDirName, num_patched);

			// Work out the crc32
			crc32 = Crc32_ComputeBuf( 0, buffer, (u32) sizeToRead);

			// See if this file already exists, if it does, match crc
			patchFile = fopen( patchFileName, "rb" );
			if(patchFile) {
				//print_gecko("Old Patch exists\r\n");
				u32 oldCrc32 = 0;
				fseek(patchFile, 0L, SEEK_END);
				u32 file_size = ftell(patchFile);
				fseek(patchFile, file_size-4, SEEK_SET);
				fread(&oldCrc32, 1, 4, patchFile);
				if(oldCrc32 == crc32) {
					num_patched++;
					fclose(patchFile);
					free(buffer);
					print_gecko("CRC matched, no need to patch again\r\n");
					continue;
				}
				else {
					remove(patchFileName);
					fclose(patchFile);
					print_gecko("CRC mismatch, writing patch again\r\n");
				}
			}
			// Otherwise, write a file out for this game with the patched buffer inside.
			print_gecko("Writing patch file: %s %i bytes (disc offset %08X)\r\n", patchFileName, sizeToRead, filesToPatch[i].offset);
			patchFile = fopen(patchFileName, "wb");
			fwrite(buffer, 1, sizeToRead, patchFile);
			u32 magic = SWISS_MAGIC;
			fwrite(&filesToPatch[i].offset, 1, 4, patchFile);
			fwrite(&filesToPatch[i].size, 1, 4, patchFile);
			fwrite(&magic, 1, 4, patchFile);
			fwrite(&crc32, 1, 4, patchFile);
			fclose(patchFile);
			num_patched++;
		}
		free(buffer);
	}

	return num_patched;
}

u32 calc_fst_entries_size(char *FST) {
	
	u32 totalSize = 0;
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
int read_fst(file_handle *file, file_handle** dir, u32 *usedSpace) {

	print_gecko("Read dir for directory: %s\r\n",file->name);
	DiskHeader header;
	char	*FST; 
	char	filename[1024];
	int		numFiles = 1, idx = 0;
	int		isRoot = !strcmp((const char*)&file->name[0], "dvd:/");
	
	// Grab disc header
	memset(&header,0,sizeof(DiskHeader));
	DVD_Read(&header,0,sizeof(DiskHeader));
 	// Alloc and read FST
	FST=(char*)memalign(32,header.FSTSize); 
	if(!FST) {
		return -1;
	}
	// Read the FST
 	DVD_Read(FST,header.FSTOffset,header.FSTSize);
	
	// Get the space taken up by this disc
	if(usedSpace) *usedSpace = calc_fst_entries_size(FST);
		
	// Add the disc itself as a "file"
	*dir = malloc( numFiles * sizeof(file_handle) );
	DVD_Read((*dir)[idx].name, 32, 128);
	strcat((*dir)[idx].name, ".gcm");
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
	memset(&filename[0],0,1024);
	sprintf(&filename[0], "%s/%s", file->name, &FST[string_table_offset+filename_offset]);
	dir_end_offset = *(u32*)&FST[(parent_dir_offset*0x0C) + 8];
	
	if(!isRoot) {
		// Add a special ".." dir which will take us back up a dir
		if(idx == numFiles){
			++numFiles;
			*dir = realloc( *dir, numFiles * sizeof(file_handle) ); 
		}
		sprintf((*dir)[idx].name, "%s/..", file->name);
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
		memset(&filename[0],0,1024);
		sprintf(&filename[0], "%s/%s", file->name, &FST[string_table_offset+filename_offset]);
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
				memcpy((*dir)[idx].name, &filename[0], 1024);
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
			memcpy((*dir)[idx].name, &filename[0], 1024);
			(*dir)[idx].fileBase = file_offset;
			(*dir)[idx].offset = 0;
			(*dir)[idx].size = size;
			(*dir)[idx].fileAttrib = IS_FILE;
			(*dir)[idx].meta = 0;
			idx++;
		}
	}
	
	free(FST);

	return numFiles;
}
