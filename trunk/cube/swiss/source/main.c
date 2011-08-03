#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <ogc/color.h>
#include <ogc/exi.h>
#include <ogc/usbgecko.h>
#include <ogc/video_types.h>
#include <sdcard/card_cmn.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include "main.h"
#include "swiss.h"
#include "font.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"

/* SD-Boot vars */
GXRModeObj *vmode = NULL;				//Graphics Mode Object
u32 *xfb[2] = { NULL, NULL };   //Framebuffers
int whichfb = 0;       		 	    //Frame buffer toggle

static file_handle* allFiles;   //all the files
int curMenuLocation = ON_FILLIST; //where are we on the screen?
int files = 0;                  //number of files in a directory
int curMenuSelection = 0;	      //menu selection
int curSelection = 0;		        //game selection
int needsDeviceChange = 0;
int needsRefresh = 0;
SwissSettings swissSettings;

int cmpFileNames(const void *p1, const void *p2)
{
   return strcasecmp( (const char*) (((file_handle*)p1)->name), 
    (const char*) (((file_handle*)p2)->name));
}

void main_loop()
{ 
	int i = 0,max,j;	
	
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
	select_device();
	// Make sure the device is ready before we browse the filesystem
	deviceHandler_deinit( deviceHandler_initial );
	deviceHandler_init( deviceHandler_initial );
  
	while(1) {
		if(needsRefresh) {
			curSelection=0; files=0; curMenuSelection=0; needsRefresh = 0;
			
			// Read the directory/device TOC
			if(allFiles){ free(allFiles); allFiles = NULL; }
			files = deviceHandler_readDir(&curFile, &allFiles);
			if(files<1) { break;}
			curMenuLocation=ON_FILLIST;
			qsort(&allFiles[0], files, sizeof(file_handle *), cmpFileNames);
		}
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
		doBackdrop();
		WriteFont(50, 120, deviceHandler_initial->name);
		// print files
		i = MIN(MAX(0,curSelection-4),MAX(0,files-8));
		max = MIN(files, MAX(curSelection+4,8));
		for(j = 0; i<max; ++i,++j){
			DrawSelectableButton(50, 160+(j*30), 430, 160+(j*30)+30, getRelativeName(&allFiles[i].name[0]), (i == curSelection)?B_SELECTED:B_NOSELECT, -1);
		}
		// print menu
		for(i = 0; i<MENU_MAX; i++)	{
			if(i == curMenuSelection)	{
				DrawSelectableButton(458,130+(i*40),-1,140+(i*40)+24,_menu_array[i],(curMenuLocation==ON_OPTIONS)?B_SELECTED:B_NOSELECT, -1);
			}
			else {
				DrawSelectableButton(458,130+(i*40),-1,140+(i*40)+24,_menu_array[i],B_NOSELECT, -1);
			}
		}
		DrawFrameFinish();
			
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN));
		u16 btns = PAD_ButtonsHeld(0);
		if(curMenuLocation==ON_OPTIONS) {
			if(btns & PAD_BUTTON_UP){	curMenuSelection = (--curMenuSelection < 0) ? (MENU_MAX-1) : curMenuSelection;}
			else if(btns & PAD_BUTTON_DOWN){curMenuSelection = (curMenuSelection + 1) % MENU_MAX;	}
		}
		if((btns & PAD_BUTTON_LEFT)||(curMenuLocation==ON_FILLIST))	{
			curMenuLocation=ON_FILLIST;
			textFileBrowser(&allFiles, files);
		}
		else if(btns & PAD_BUTTON_A) {
			//handle menu event
			switch(curMenuSelection) {
				case 0:		// Boot Game or DOL
					boot_file();
					break;
				case 1:		
					needsDeviceChange = 1;  //Change from SD->DVD or vice versa
					break;
				case 2:		// Settings
					settings();
					break;
				case 3:
					install_game(); //Dump a game
					break;	
				case 4:		// Info
					info_game();
					break;
				case 5:		// Credits
					credits();
					break;
				case 6:
					needsRefresh=1;
					break;
			}
			
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)));
		if(needsDeviceChange) {
			break;
		}
	}
}

/****************************************************************************
* Main
****************************************************************************/
int main () 
{
	void *fb;   
	fb = Initialise();
	if(!fb) {
		return -1;
	}
	
	// Setup defaults
	swissSettings.useHiMemArea = 0;
	swissSettings.disableInterrupts = 0;
	swissSettings.useHiLevelPatch = 0;
	swissSettings.debugUSB = 1;
	swissSettings.curVideoSelection = AUTO;
	
	//debugging stuff
	if(swissSettings.debugUSB) {
		if(usb_isgeckoalive(1)) {
			usb_flush(1);
		}
		sprintf(txtbuffer, "Arena Size: %iKb\r\n",(SYS_GetArena1Hi()-SYS_GetArena1Lo())/1024);
	}

	while(1) {
		needsRefresh = 1;
		needsDeviceChange = 0;
		main_loop();
	}
	return 0;
}

                                                     

