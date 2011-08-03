/* DOL Loading code */


#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogc/machine/processor.h>
#include "devices/deviceHandler.h"
#include <malloc.h>

#define DOL_HEADER_SIZE 256
#define DOL_OFFSET_LOC  0x420

/*
 * check a DOL file to see if it contains certain signatures
 * returns 1 on success, 0 on fail, -1 if unsure (wasn't able to scan)
*/
int check_dol(file_handle* disc, unsigned int *sig, int size)
{
	unsigned int dolOffset = 0, i;

	deviceHandler_seekFile(disc,DOL_OFFSET_LOC,DEVICE_HANDLER_SEEK_SET);
	if((deviceHandler_readFile(disc,&dolOffset,sizeof(unsigned int)) != sizeof(unsigned int)) || (!dolOffset)) {
	 return -1;
  }

	char *dolheader = (char*)memalign(32,DOL_HEADER_SIZE);
	if(!dolheader) {
  	return -1;	
	}
	deviceHandler_seekFile(disc,dolOffset,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(disc,dolheader,DOL_HEADER_SIZE) != DOL_HEADER_SIZE) {
  	free(dolheader);
	  return -1;     
  }
	
	struct dol_s
	{
		unsigned long sec_pos[18];
		unsigned long sec_address[18];
		unsigned long sec_size[18];
		unsigned long bss_address, bss_size, entry_point;
	} *d = (struct dol_s *)dolheader;
	
	for (i = 0; i < 18; ++i)
	{
		if (!d->sec_size[i]) {
			continue;
		}
		char *dolsection = memalign(32,d->sec_size[i]); //malloc section
		if(!dolsection)	{
  		free(dolheader); 
  		return -1;
		}
		deviceHandler_seekFile(disc,dolOffset+d->sec_pos[i],DEVICE_HANDLER_SEEK_SET);
    if(deviceHandler_readFile(disc,dolsection,d->sec_size[i]) != d->sec_size[i]) {
  		free(dolsection); 
  		free(dolheader); 
  		return -1;
		}
		void *addr_start = dolsection;
	  void *addr_end = dolsection+d->sec_size[i];	
	
	  while(addr_start<addr_end) 
	  {
		  if(memcmp(addr_start,sig,size)==0) //check
		  {
  		  free(dolsection); free(dolheader);
		    return 1;                       //found it
	    }
	    addr_start += 4;
	  }
		free(dolsection);
	}
	free(dolheader);
	return 0; //failed to find anything
}
