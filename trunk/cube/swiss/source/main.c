#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <ogc/color.h>
#include <ogc/exi.h>
#include <ogc/lwp.h>
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
#include "settings.h"
#include "info.h"
#include "swiss.h"
#include "bba.h"
#include "exi.h"
#include "dvd.h"
#include "wkf.h"
#include "httpd.h"
#include "config.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "aram/sidestep.h"

#define DEFAULT_FIFO_SIZE    (256*1024)//(64*1024) minimum

extern void __libogc_exit(int status);

GXRModeObj *vmode = NULL;				//Graphics Mode Object
u32 *xfb[2] = { NULL, NULL };   //Framebuffers
int whichfb = 0;       		 	    //Frame buffer toggle
u8 driveVersion[8];
file_handle* allFiles;   		//all the files in the current dir
int curMenuLocation = ON_FILLIST; //where are we on the screen?
int files = 0;                  //number of files in a directory
int curMenuSelection = 0;	      //menu selection
int curSelection = 0;		        //game selection
int needsDeviceChange = 0;
int needsRefresh = 0;
SwissSettings swissSettings;

static void ProperScanPADS()	{
	PAD_ScanPads(); 
}

void populateVideoStr(GXRModeObj *vmode) {
	if(vmode == &TVPal576ProgScale) {
		videoStr = Prog576Str;
	}
	else if(vmode == &TVNtsc480Prog) {
		videoStr = Prog480Str;
	}
	else if(vmode == &TVPal576IntDfScale) {
		videoStr = PALStr;
	}
	else if(vmode == &TVNtsc480IntDf) {
		videoStr = NTSCStr;
	}
	else {
		videoStr = UnkStr;
	}
}

/* Initialise Video, PAD, DVD, Font */
void* Initialise (void)
{
	VIDEO_Init ();
	PAD_Init ();  
	DVD_Init(); 
	*(volatile unsigned long*)0xcc00643c = 0x00000000; //allow 32mhz exi bus
	
	// Disable IPL modchips to allow access to IPL ROM fonts
	ipl_set_config(6); 
	usleep(1000); //wait for modchip to disable (overkill)
	
	
	__SYS_ReadROM(IPLInfo,256,0);	// Read IPL tag

	// Wii has no IPL tags for "PAL" so let libOGC figure out the video mode
	if(!is_gamecube()) {
		vmode = VIDEO_GetPreferredMode(NULL); //Last mode used
	}
	else {	// Gamecube, determine based on IPL
		PAD_ScanPads();
		// L Trigger held down ignores the fact that there's a component cable plugged in.
		if(VIDEO_HaveComponentCable() && !(PAD_ButtonsDown(0) & PAD_TRIGGER_L)) {
			if((strstr(IPLInfo,"PAL")!=NULL)) {
				vmode = &TVPal576ProgScale; //Progressive 576p
			}
			else {
				vmode = &TVNtsc480Prog; //Progressive 480p
			}
		}
		else {
			//try to use the IPL region
			if(strstr(IPLInfo,"PAL")!=NULL) {
				vmode = &TVPal576IntDfScale;         //PAL
			}
			else if(strstr(IPLInfo,"NTSC")!=NULL) {
				vmode = &TVNtsc480IntDf;        //NTSC
			}
			else {
				vmode = VIDEO_GetPreferredMode(NULL); //Last mode used
			}
		}
	}
	populateVideoStr(vmode);

	VIDEO_Configure (vmode);
	xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
	xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
	VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);
	VIDEO_SetPostRetraceCallback (ProperScanPADS);
	VIDEO_SetBlack (0);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
	if (vmode->viTVMode & VI_NON_INTERLACE) {
		VIDEO_WaitVSync ();
	}
	
	// setup the fifo and then init GX
	void *gp_fifo = NULL;
	gp_fifo = MEM_K0_TO_K1 (memalign (32, DEFAULT_FIFO_SIZE));
	memset (gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);
	// clears the bg to color and clears the z buffer
	GX_SetCopyClear ((GXColor){0,0,0,255}, 0x00000000);
	// init viewport
	GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
	// Set the correct y scaling for efb->xfb copy operation
	GX_SetDispCopyYScale ((f32) vmode->xfbHeight / (f32) vmode->efbHeight);
	GX_SetDispCopyDst (vmode->fbWidth, vmode->xfbHeight);
	GX_SetCullMode (GX_CULL_NONE); // default in rsp init
	GX_CopyDisp (xfb[0], GX_TRUE); // This clears the efb
	GX_CopyDisp (xfb[0], GX_TRUE); // This clears the xfb

	init_font();
	init_textures();
	whichfb = 0;
	
	drive_version(&driveVersion[0]);
	swissSettings.hasDVDDrive = *(u32*)&driveVersion[0] ? 1 : 0;
	
	if(!driveVersion[0]) {
		// Reset DVD if there was a modchip
		DrawFrameStart();
		WriteFontStyled(640/2, 250, "Initialise DVD .. (HOLD B if NO DVD Drive)", 0.8f, true, defaultColor);
		DrawFrameFinish();
		dvd_reset();	// low-level, basic
		dvd_read_id();
		drive_version(&driveVersion[0]);
		swissSettings.hasDVDDrive = *(u32*)&driveVersion[0] ? 1 : 0;
		if(!swissSettings.hasDVDDrive) {
			DrawFrameStart();
			DrawMessageBox(D_INFO, "No DVD Drive Detected !!");
			DrawFrameFinish();
			sleep(2);
		}
	}
	
	return xfb[0];
}

void load_auto_dol() {
	sprintf(txtbuffer, "%sboot.dol", deviceHandler_initial->name);
	FILE *fp = fopen(txtbuffer, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if ((size > 0) && (size < (AR_GetSize() - (64*1024)))) {
			u8 *dol = (u8*) memalign(32, size);
			if (dol) {
				fread(dol, 1, size, fp);
				DOLtoARAM(dol);
			}
		}
		fclose(fp);
	}
}

void load_config() {

	// Try to open up the config .ini in case it hasn't been opened already (SD, IDE-EXI only)
	if(!config_init()) {
		if(!config_create()) {
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Failed to create configuration file!");
			DrawFrameFinish();
			sleep(1);
		}
	}
	else {
		DrawFrameStart();
		sprintf(txtbuffer,"Loaded %i entries from the config file",config_get_count());
		DrawMessageBox(D_INFO,txtbuffer);
		DrawFrameFinish();
		memcpy(&swissSettings, config_get_swiss_settings(), sizeof(SwissSettings));
	}
}

int comp(const void *a1, const void *b1)
{
	const file_handle* a = a1;
	const file_handle* b = b1;
	
	if(!a && b) return 1;
	if(a && !b) return -1;
	if(!a && !b) return 0;
	
	if((curDevice == DVD_DISC) && ((dvdDiscTypeInt == GAMECUBE_DISC) || (dvdDiscTypeInt == MULTIDISC_DISC)))
	{
		if(a->size == DISC_SIZE && a->fileBase == 0)
			return -1;
		if(b->size == DISC_SIZE && b->fileBase == 0)
			return 1;
	}
	
	if(a->fileAttrib == IS_DIR && b->fileAttrib == IS_FILE)
		return -1;
	if(a->fileAttrib == IS_FILE && b->fileAttrib == IS_DIR)
		return 1;

	return stricmp(a->name, b->name);
}

void sortFiles(file_handle* dir, int num_files)
{
	if(num_files > 0) {
		qsort(&dir[0],num_files,sizeof(file_handle),comp);
	}
}

void main_loop()
{ 
	int i = 0,max,j;	
	file_handle curDir;
	
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
	// We don't care if a subsequent device is "default"
	if(needsDeviceChange) {
		swissSettings.defaultDevice = 0;
		needsDeviceChange = 0;
		deviceHandler_initial = NULL;
		needsRefresh = 1;
	}

	select_device();	// Will automatically return if we have defaultDevice = 1
	needsRefresh = 1;
	
	if(deviceHandler_initial) {
		// If the user selected a device, make sure it's ready before we browse the filesystem
		deviceHandler_deinit( deviceHandler_initial );
		if(!deviceHandler_init( deviceHandler_initial )) {
			print_gecko("Device Failed to initialize!\r\n",files);
			needsDeviceChange = 1;
			return;
		}
		if(curDevice==SD_CARD && !swissSettings.defaultDevice) { 
			load_config();
		}
	}
	else {
		curMenuLocation=ON_OPTIONS;
	}
  
	while(1) {
		if(deviceHandler_initial && needsRefresh) {
			curMenuLocation=ON_OPTIONS;
			curSelection=0; files=0; curMenuSelection=0;
			// Read the directory/device TOC
			if(allFiles){ free(allFiles); allFiles = NULL; }
			print_gecko("Reading directory: %s\r\n",curFile.name);
			files = deviceHandler_readDir(&curFile, &allFiles, -1);
			memcpy(&curDir, &curFile, sizeof(file_handle));
			sortFiles(allFiles, files);
			print_gecko("Found %i entries\r\n",files);
			if(files<1) { break;}
			needsRefresh = 0;
			curMenuLocation=ON_FILLIST;
		}
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
		doBackdrop();
		// print files
		i = MIN(MAX(0,curSelection-FILES_PER_PAGE/2),MAX(0,files-FILES_PER_PAGE));
		max = MIN(files, MAX(curSelection+FILES_PER_PAGE/2,FILES_PER_PAGE));
		for(j = 0; i<max; ++i,++j){
			DrawFileBrowserButton(50,90+(j*50), 550, 90+(j*50)+50, getRelativeName(&allFiles[i].name[0]), &allFiles[i], (i == curSelection)?((curMenuLocation==ON_FILLIST)?B_SELECTED:B_NOSELECT):B_NOSELECT, -1);
		}
		// print menu
		DrawMenuButtons((curMenuLocation==ON_OPTIONS)?curMenuSelection:-1);
		DrawFrameFinish();

		u16 btns = PAD_ButtonsHeld(0);
		if(curMenuLocation==ON_OPTIONS) {
			if(btns & PAD_BUTTON_LEFT){	curMenuSelection = (--curMenuSelection < 0) ? (MENU_MAX-1) : curMenuSelection;}
			else if(btns & PAD_BUTTON_RIGHT){curMenuSelection = (curMenuSelection + 1) % MENU_MAX;	}
		}
		if(deviceHandler_initial && ((btns & PAD_BUTTON_B)||(curMenuLocation==ON_FILLIST)))	{
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_B){ VIDEO_WaitVSync (); }
			curMenuLocation=ON_FILLIST;
			textFileBrowser(&allFiles, files);
		}
		else if(btns & PAD_BUTTON_A) {
			//handle menu event
			switch(curMenuSelection) {
				case 0:		// Device change
					deviceHandler_initial = NULL;
					needsDeviceChange = 1;  //Change from SD->DVD or vice versa
					needsRefresh = 1;
					break;
				case 1:		// Settings
					show_settings(NULL, NULL);
					break;
				case 2:		// Credits
					show_info();
					break;
				case 3:
					memcpy(&curFile, &curDir, sizeof(file_handle));
					needsRefresh=1;
					break;
				case 4:
					__libogc_exit(0);
					break;
			}
			
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT))) {
			VIDEO_WaitVSync();
		}
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
	// Setup defaults (if no config is found)
	memset(&swissSettings, 0 , sizeof(SwissSettings));
	
	void *fb;
	fb = Initialise();
	if(!fb) {
		return -1;
	}

	// Sane defaults
	refreshSRAM();
	swissSettings.debugUSB = 0;
	swissSettings.gameVMode = 4;	// Auto video mode
	swissSettings.exiSpeed = 1;		// 32MHz
	swissSettings.uiVMode = 4; 		// Auto UI mode
	config_copy_swiss_settings(&swissSettings);
	// Start up the BBA if it exists
	init_network_thread();
	init_httpd_thread();

	//debugging stuff
	if(swissSettings.debugUSB) {
		if(usb_isgeckoalive(1)) {
			usb_flush(1);
		}
		print_gecko("Arena Size: %iKb\r\n",(SYS_GetArena1Hi()-SYS_GetArena1Lo())/1024);
		print_gecko("DVD Drive Present? %s\r\n",swissSettings.hasDVDDrive?"Yes":"No");
		print_gecko("SVN Revision: %s\r\n", SVNREVISION);
	}
	
	// Are we working with a Wiikey Fusion?
	if(__wkfSpiReadId() != 0 && __wkfSpiReadId() != 0xFFFFFFFF) {
		print_gecko("Detected Wiikey Fusion with SPI Flash ID: %08X\r\n",__wkfSpiReadId());
		swissSettings.defaultDevice = 1;
		curDevice = WKF;
	}
	else {
		// Try to init SD cards here and load config
		deviceHandler_initial = &initial_SD0;
		deviceHandler_init     =  deviceHandler_FAT_init;
		deviceHandler_deinit     =  deviceHandler_FAT_deinit;
		if(deviceHandler_init(deviceHandler_initial)) {
			print_gecko("Detected SDGecko in Slot A\r\n");
			load_auto_dol();
			load_config();
			print_gecko("Autoselecting SDA %s\r\n", swissSettings.defaultDevice ? "Yes":"No");
			if(swissSettings.defaultDevice)
				curDevice = SD_CARD;
		}
		else {
			deviceHandler_deinit(deviceHandler_initial);
			deviceHandler_initial = &initial_SD1;
			if(deviceHandler_init(deviceHandler_initial)) {
				print_gecko("Detected SDGecko in Slot B\r\n");
				load_auto_dol();
				load_config();
				print_gecko("Autoselecting SDB %s\r\n", swissSettings.defaultDevice ? "Yes":"No");
				if(swissSettings.defaultDevice)
					curDevice = SD_CARD;
			}
		}
	}
	
	// If no device has been selected yet to browse ..
	if(!swissSettings.defaultDevice) {
		print_gecko("No default boot device detected, trying DVD!\r\n");
		// Do we have a DVD drive with a ready medium we can perhaps browse then?
		u8 driveReallyExists[8];
		drive_version(&driveReallyExists[0]);
		if(*(u32*)&driveReallyExists[0]) {
			dvd_read_id();
			if(!dvd_get_error()) {
				print_gecko("DVD Medium is up, using it as default device\r\n");
				swissSettings.defaultDevice = 1;
				curDevice = DVD_DISC;
				
				// If we have a GameCube (single image) bootable disc, show the banner screen here
				dvdDiscTypeInt = gettype_disc();
				if(dvdDiscTypeInt == GAMECUBE_DISC) {
					select_device();
					// Setup curFile and load it
					memset(&curFile, 0, sizeof(file_handle));
					curFile.size = DISC_SIZE;
					curFile.fileAttrib = IS_FILE;
					load_file();
					swissSettings.defaultDevice = 0;
					deviceHandler_initial = NULL;
				}
			}
		}
	}

	deviceHandler_initial = !swissSettings.defaultDevice ? NULL:deviceHandler_initial;
	needsRefresh = (swissSettings.defaultDevice != 0);
	while(1) {
		if(needsRefresh || needsDeviceChange) {
			if(allFiles)
				free(allFiles); 
			allFiles = NULL;
			files = 0;
		}
		main_loop();
	}
	return 0;
}
