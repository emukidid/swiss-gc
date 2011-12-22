#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "gcm.h"
#include "swiss.h"
#include "patcher.h"
#include "devices/deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

#define FST_ENTRY_SIZE 12

// Returns the number of filesToPatch and fills out the filesToPatch array passed in (pre-allocated)
int parse_gcm(file_handle *file, ExecutableFile *filesToPatch) {

	DiskHeader header;
	char	*FST; 
	char	filename[256];
	int		numFiles = 0;
		
	// Grab disc header
	memset(&header,0,sizeof(DiskHeader));
 	deviceHandler_seekFile(file,0,DEVICE_HANDLER_SEEK_SET);
 	if(deviceHandler_readFile(file,&header,sizeof(DiskHeader)) != sizeof(DiskHeader)) {
		return -1;
	}
 	  
 	// Alloc and read FST
	FST=(char*)memalign(32,header.FSTSize); 
	if(!FST) {
		return -1;
	}
	deviceHandler_seekFile(file,header.FSTOffset,DEVICE_HANDLER_SEEK_SET);
 	if(deviceHandler_readFile(file,FST,header.FSTSize) != header.FSTSize) {
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
			if(	(strstr(filename,".dol")) || (strstr(filename,".DOL")) || 
				(strstr(filename,".elf")) || (strstr(filename,".ELF")) || (strstr(filename,"execD.img"))) {
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if((strstr(filename,".tgc")) || (strstr(filename,".TGC"))) {
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
	
	// Some games contain a single "default.dol", these do not need pre-patching.
	if(numFiles==1 && (!strcmp(&filesToPatch[0].name[0],"default.dol"))) {
		numFiles = 0;
	}
	
	// if we have any to pre-patch, we must patch the main dol too
	if(numFiles) {
		DiskHeader *header = memalign(32,sizeof(DiskHeader));
		deviceHandler_seekFile(file,0,DEVICE_HANDLER_SEEK_SET);
		deviceHandler_readFile(file,header,sizeof(DiskHeader));
		filesToPatch[numFiles].offset = header->DOLOffset;
		filesToPatch[numFiles].size = 8*1024*1024;
		sprintf(&filesToPatch[numFiles].name[0],"Main DOL File"); 
		numFiles++;
		free(header);
	}
	return numFiles;
}

int parse_tgc(file_handle *file, ExecutableFile *filesToPatch, u32 tgc_base) {
	char	*FST; 
	char	filename[256];
	u32 fileAreaStart, fakeAmount, offset, size, numFiles = 0;
	
	// add this embedded GCM's main DOL
	deviceHandler_seekFile(file,tgc_base+0x1C,DEVICE_HANDLER_SEEK_SET);
	deviceHandler_readFile(file,&offset,4);
	deviceHandler_readFile(file,&size,4);
	deviceHandler_readFile(file, &fileAreaStart, 4);
	deviceHandler_seekFile(file,tgc_base+0x34,DEVICE_HANDLER_SEEK_SET);
	deviceHandler_readFile(file, &fakeAmount, 4);
	filesToPatch[numFiles].offset = offset+tgc_base;
	filesToPatch[numFiles].size = size;
	strcpy(&filesToPatch[numFiles].name[0],"TGC Main DOL");
	numFiles++;
	
	// Grab FST Offset & Size
	u32 fstOfsAndSize[2];
 	deviceHandler_seekFile(file,tgc_base+0x10,DEVICE_HANDLER_SEEK_SET);
 	deviceHandler_readFile(file,&fstOfsAndSize,2*sizeof(u32));

 	// Alloc and read FST
	FST=(char*)memalign(32,fstOfsAndSize[1]); 
	deviceHandler_seekFile(file,tgc_base+fstOfsAndSize[0],DEVICE_HANDLER_SEEK_SET);
 	deviceHandler_readFile(file,FST,fstOfsAndSize[1]);

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
			if(	(strstr(filename,".dol")) || (strstr(filename,".DOL")) || 
				(strstr(filename,".elf")) || (strstr(filename,".ELF")) || (strstr(filename,"execD.img"))) {
				filesToPatch[numFiles].offset = (file_offset-fakeAmount)+(tgc_base+fileAreaStart);
				filesToPatch[numFiles].size = size;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
		} 
	}
	free(FST);
	return numFiles;
	
}

int patch_gcm(file_handle *file, ExecutableFile *filesToPatch, int numToPatch) {
	int i, pre_patched = 0;
	for(i = 0; i < numToPatch; i++) {
		sprintf(txtbuffer, "Patching: %s %iMb", filesToPatch[i].name, filesToPatch[i].size/1024/1024);
		DrawFrameStart();
		DrawMessageBox(D_INFO,txtbuffer);
		DrawFrameFinish();
		u8 *buffer = (u8*)0x80003100;	// This is free
		deviceHandler_seekFile(file,filesToPatch[i].offset,DEVICE_HANDLER_SEEK_SET);
		deviceHandler_readFile(file,buffer,filesToPatch[i].size);
		int patched = 0;
		sprintf(txtbuffer, "Patch %s mem %08X ofs %08X size %08X\r\n",
				filesToPatch[i].name,(u32)buffer,filesToPatch[i].offset,filesToPatch[i].size);
		print_gecko(txtbuffer);
		if(swissSettings.useHiLevelPatch) {
			patched += Patch_DVDHighLevelRead(buffer, filesToPatch[i].size);
		} else {
			patched += Patch_DVDLowLevelRead(buffer, filesToPatch[i].size);
		}
		patched += Patch_480pVideo(buffer, filesToPatch[i].size);
		patched += Patch_DVDAudioStreaming(buffer, filesToPatch[i].size);
		if(patched) {
			print_gecko("Seeking in the file..\r\n");
			deviceHandler_seekFile(file,filesToPatch[i].offset,DEVICE_HANDLER_SEEK_SET);
			print_gecko("Started writing the patch\r\n");
			deviceHandler_writeFile(file,buffer,filesToPatch[i].size);
			print_gecko("Finished writing the patch\r\n");
			pre_patched = 1;
		}
	}
	if(pre_patched) {
		sprintf(txtbuffer, PRE_PATCHER_MAGIC);
		deviceHandler_seekFile(file,0x100,DEVICE_HANDLER_SEEK_SET);
		deviceHandler_writeFile(file,txtbuffer,strlen(txtbuffer));
		deviceHandler_seekFile(file,0x120,DEVICE_HANDLER_SEEK_SET);
		deviceHandler_writeFile(file,&swissSettings.useHiLevelPatch,4);
		deviceHandler_writeFile(file,&swissSettings.useHiMemArea,4);
		deviceHandler_writeFile(file,&swissSettings.disableInterrupts,4);
		return 1;
	}
	return 0;
}


