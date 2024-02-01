/* boxart.c 
	- Boxart texture loading from a binary compilation
	by emu_kidid
 */

#include <string.h>
#include <stdio.h>
#include <fat.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/dir.h>
#include "boxart.h"
#include "files.h"
#include "util.h"
#include "../devices/deviceHandler.h"


#define MAX_HEADER_SIZE 0x4000
file_handle* boxartFile = NULL;
char *boxartHeader = NULL;
extern u32 missBoxArt_length;
extern u8 missBoxArt[];	//default "no boxart" texture, TODO make this nicer.

/* Call this in the menu to set things up */
void BOXART_Init()
{
	// open boxart file
	if(!boxartFile) {
		boxartFile = (file_handle*)calloc(1, sizeof(file_handle));
		// TODO config device is broken when switching device, should we rely on it being on the CUR device?
		concat_path(boxartFile->name, devices[DEVICE_CONFIG]->initial->name, "swiss/boxart_hires.bin");
	
		if(devices[DEVICE_CONFIG]->readFile(boxartFile, NULL, 0) != 0) {
			print_gecko("boxart_hires.bin not found\r\n");
			free(boxartFile);
			boxartFile = NULL;
		}
		else {
			print_gecko("boxart_hires.bin found, %d in bytes\r\n", boxartFile->size);
		}
		
	}
	
	// file wasn't found	
	if(!boxartFile)
		return;
	
	// allocate header space and read header
	if(!boxartHeader)
	{
		boxartHeader = (char*)memalign(32,MAX_HEADER_SIZE);
		devices[DEVICE_CONFIG]->readFile(boxartFile, boxartHeader, MAX_HEADER_SIZE);
	}
}

/* Call this when we launch a game to free up resources */
void BOXART_DeInit()
{
	//close boxart file
	if(boxartFile)
	{
		devices[DEVICE_CONFIG]->closeFile(boxartFile);
		boxartFile = NULL;
	}
	//free header data
	if(boxartHeader)
	{
		free(boxartHeader);
		boxartHeader = NULL;
	}
}

/* Pass In a gameID and an aligned buffer of size BOXART_TEX_SIZE
	- a RGB565 image will be returned */
void BOXART_LoadTexture(char *gameID, char *buffer, int type)
{
	// 1392640
	int textureSize, offset;
	switch(type) {
		case BOX_FRONT:
			textureSize = BOXART_TEX_FRONT_SIZE;	//660960
			offset = 0;
		break;
			
		case BOX_SPINE:
			textureSize = BOXART_TEX_SPINE_SIZE;	//65280
			offset = BOXART_TEX_FRONT_SIZE;			//660960
		break;
		
		case BOX_BACK:
			textureSize = BOXART_TEX_BACK_SIZE;		//666400
			offset = BOXART_TEX_FRONT_SIZE + BOXART_TEX_SPINE_SIZE;
		break;
	
		case BOX_ALL:
		default:
			textureSize = BOXART_TEX_SIZE;
			offset = 0;
		break;
	}
		
	if(!boxartFile)
	{
		memset(buffer, 0x5A, textureSize);
		//memcpy(buffer,&missBoxArt,missBoxArt_length);
		return;
	}
		
	int i = 0;
	for(i = 0; i < MAX_HEADER_SIZE; i+=10)
	{
		if(*(u32*)(&boxartHeader[i]) == 0xFFFFFFFF) break;	// end of the list
		if(!strncmp(gameID, &boxartHeader[i], 6))
		{
			devices[DEVICE_CONFIG]->seekFile(boxartFile, (*(u32*)(&boxartHeader[i+6]))+offset, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CONFIG]->readFile(boxartFile, buffer, textureSize);
			print_gecko("Compared & found [%s] to [%.*s]. Offset for this one is: %08X\r\n", gameID, 6, &boxartHeader[i], (*(u32*)(&boxartHeader[i+6])) + offset);
			return;
		}
	}
	// if we didn't find it, then return the default image
	//memcpy(buffer,&missBoxArt,missBoxArt_length);
	memset(buffer, 0x5A, textureSize);
}