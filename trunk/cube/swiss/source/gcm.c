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
#include "patcher.h"
#include "sidestep.h"
#include "devices/deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include <sys/stat.h>
#include <errno.h>

#define PATCH_CHUNK_SIZE 8*1024*1024
#define FST_ENTRY_SIZE 12

//Lets parse the entire game FST in search for the banner
unsigned int getBannerOffset(file_handle *file) {
	unsigned int   	buffer[2]; 
	unsigned long   fst_offset=0,filename_offset=0,entries=0,string_table_offset=0,fst_size=0; 
	unsigned long   i,offset; 
	char   filename[256]; 
	
	// lets just quickly check if this is a valid GCM (contains magic)
	deviceHandler_seekFile(file,0x1C,DEVICE_HANDLER_SEEK_SET);
	if((deviceHandler_readFile(file,(unsigned char*)buffer,sizeof(int))!=sizeof(int)) || (buffer[0]!=0xC2339F3D)) {
		return 0;
	}

	// get FST offset and size
	deviceHandler_seekFile(file,0x424,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(file,(unsigned char*)buffer,sizeof(int)*2)!=sizeof(int)*2) {
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
	deviceHandler_seekFile(file,fst_offset,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(file,FST,fst_size)!=fst_size) {
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
			if((!stricmp(filename,"opening.bnr")) || (!stricmp(filename,"OPENING.BNR"))) 
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
			if(endsWith(filename,".dol") || endsWith(filename,".DOL")) {
				filesToPatch[numFiles].offset = file_offset;
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_DOL;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if((endsWith(filename,".elf") || endsWith(filename,".ELF")) && size < 12*1024*1024) {
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
			if(endsWith(filename,".tgc") || endsWith(filename,".TGC")) {
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
			if(endsWith(filename,".dol") || endsWith(filename,".DOL")) {
				filesToPatch[numFiles].offset = (file_offset-fakeAmount)+(tgc_base+fileAreaStart);
				filesToPatch[numFiles].size = size;
				filesToPatch[numFiles].type = PATCH_DOL;
				memcpy(&filesToPatch[numFiles].name,&filename[0],64); 
				numFiles++;
			}
			if((endsWith(filename,".elf") || endsWith(filename,".ELF")) && size < 12*1024*1024) {
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
	int i, patched_buf_num = 0;
	
	// Go through all the possible files we think need patching..
	for(i = 0; i < numToPatch; i++) {	
		int patched = 0;
		// File handle for a patch we might need to write
		file_handle patchFile;
		memset(&patchFile, 0, sizeof(file_handle));
		
		// Round up to 32 bytes
		if(filesToPatch[i].size % 0x20) {
			filesToPatch[i].size += (0x20-(filesToPatch[i].size%0x20));
		}
		
		// Chunk the file out in 8mb chunks or less
		int ofs;
		for(ofs = 0; ofs < filesToPatch[i].size; ofs+=PATCH_CHUNK_SIZE) {
			print_gecko("Checking %s %iKb\r\n", filesToPatch[i].name, filesToPatch[i].size/1024);
			int sizeToRead = (ofs+PATCH_CHUNK_SIZE > filesToPatch[i].size) ? (filesToPatch[i].size-ofs):PATCH_CHUNK_SIZE;
			u8 *buffer = (u8*)memalign(32, sizeToRead);
			
			deviceHandler_seekFile(file,filesToPatch[i].offset+ofs,DEVICE_HANDLER_SEEK_SET);
			if(deviceHandler_readFile(file,buffer,sizeToRead)!= sizeToRead) {
				DrawFrameStart();
				DrawMessageBox(D_FAIL, "Failed to read!");
				DrawFrameFinish();
				sleep(5);
				return 0;
			}

			u32 ret = Patch_DVDLowLevelRead(buffer, sizeToRead, filesToPatch[i].type, multiDol);
			if(READ_PATCHED_ALL != ret)	{
				DrawFrameStart();
				DrawMessageBox(D_FAIL, "Failed to find necessary functions for patching!");
				DrawFrameFinish();
				//sleep(5);
			}
			else
				patched += 1;
			if(swissSettings.debugUSB && usb_isgeckoalive(1)) {
				Patch_Fwrite(buffer, sizeToRead);
			}
			patched += Patch_DVDCompareDiskId(buffer, sizeToRead);
			patched += Patch_VidMode(buffer, sizeToRead, filesToPatch[i].type);
			patched += Patch_FontEnc(buffer, sizeToRead);
			patched += Patch_DVDAudioStreaming(buffer, sizeToRead);
			if(swissSettings.forceWidescreen)
				Patch_WideAspect(buffer, sizeToRead, filesToPatch[i].type);
			if(swissSettings.forceAnisotropy)
				Patch_TexFilt(buffer, sizeToRead, filesToPatch[i].type);
			if(patched) {
				sprintf(txtbuffer, "Writing patch for %s %iKb", filesToPatch[i].name, filesToPatch[i].size/1024);
				DrawFrameStart();
				DrawMessageBox(D_INFO,txtbuffer);
				DrawFrameFinish();

				char patchDirName[1024];
				memset(&patchDirName, 0, 1024);
				sprintf(&patchDirName[0],"%s.patches",file->name);
				
				// Make a patches dir if we don't have one already
				if(mkdir(&patchDirName[0], 0777) != 0) {
					if(errno != EEXIST) {
						return -2;
					}
				}
					
				// Write a .patchX file out for this game with the patched buffer inside.
				print_gecko("Writing patch file: %s.patches/%i %i bytes (disc offset %08X)\r\n", file->name, patched_buf_num, sizeToRead, filesToPatch[i].offset+ofs);
				sprintf(&patchFile.name[0], "%s/%i",patchDirName, patched_buf_num);
				deviceHandler_deleteFile(&patchFile);
				deviceHandler_writeFile(&patchFile,buffer,sizeToRead);
				

			}
			free(buffer);
		}
		if(patchFile.name[0]) {
			u32 magic = SWISS_MAGIC;
			deviceHandler_writeFile(&patchFile,&filesToPatch[i].offset,4);
			deviceHandler_writeFile(&patchFile,&filesToPatch[i].size,4);
			deviceHandler_writeFile(&patchFile,&magic,4);
			fclose(patchFile.fp);
			patched_buf_num++;
		}
	}
	return 0;
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
	char	filename[256];
	int		numFiles = 1, idx = 0;
	int		isRoot = (file->name[0] == '\\');
	
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
	
	u32 filename_offset=((*(u32*)&FST[parent_dir_offset*0x0C]) & 0x00FFFFFF); 
	memset(&filename[0],0,256);
	memcpy(&filename[0],&FST[string_table_offset+filename_offset],255); 
	dir_end_offset = *(u32*)&FST[(parent_dir_offset*0x0C) + 8];
	
	if(!isRoot) {
		// Add a special ".." dir which will take us back up a dir
		if(idx == numFiles){
			++numFiles;
			*dir = realloc( *dir, numFiles * sizeof(file_handle) ); 
		}
		strcpy((*dir)[idx].name, "..");
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
		memset(&filename[0],0,256);
		memcpy(&filename[0],&FST[string_table_offset+filename_offset],255); 
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
				memcpy((*dir)[idx].name, &filename[0], 255);
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
			memcpy((*dir)[idx].name, &filename[0], 255);
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
