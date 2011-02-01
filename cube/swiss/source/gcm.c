#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "gcm.h"
#include "devices/deviceHandler.h"

#define FST_ENTRY_SIZE 12

int numExecutables=0;
int numEmbeddedGCMs=0;

void parse_gcm(file_handle *file) {

	DiskHeader header;
	char   *FST; 
	char   filename[256]; 
	unsigned int   filename_offset=0,entries=0,string_table_offset=0; 
	unsigned int   i=0,offset=0; 
	
	// Initialise the vars
	numExecutables=0;
  numEmbeddedGCMs=0;
		
  // Grab disc header
  memset(&header,0,sizeof(DiskHeader));
 	deviceHandler_seekFile(file,0,DEVICE_HANDLER_SEEK_SET);
 	if(deviceHandler_readFile(file,&header,sizeof(DiskHeader)) != sizeof(DiskHeader)) {
 	  return;
  }
 	  
 	// Alloc and read FST
	FST=(char*)memalign(32,header.FSTSize); 
	if(!FST) {
  	return;
	}
	deviceHandler_seekFile(file,header.FSTOffset,DEVICE_HANDLER_SEEK_SET);
 	if(deviceHandler_readFile(file,FST,header.FSTSize) != header.FSTSize) {
  	free(FST); 
  	return;
	}

	entries=*(unsigned int*)&FST[8];
	string_table_offset=FST_ENTRY_SIZE*entries;
		
	// go through every entry
	for (i=1;i<entries;i++) 
	{ 
		offset=i*0x0c; 
		if(FST[offset]==0) //skip directories
		{ 
			filename_offset=((*(unsigned int*)&FST[offset]) & 0x00FFFFFF); 
			memset(&filename[0],0,256);
			memcpy(&filename[0],&FST[string_table_offset+filename_offset],255); 
      if((strstr(filename,".elf")) || (strstr(filename,".ELF")) || (strstr(filename,".dol")) || (strstr(filename,".DOL"))) 
				numExecutables++;
			if((strstr(filename,".tgc")) || (strstr(filename,".TGC")))
				numEmbeddedGCMs++;	
		} 
	}
	free(FST);
}

