/*
*
*   Swiss - The Gamecube IPL replacement
*
*/

#include <stdio.h>
#include <stdarg.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <ogc/color.h>
#include <ogc/exi.h>
#include <ogc/usbgecko.h>
#include <ogc/video_types.h>
#include <sdcard/card_cmn.h>
#include <ogc/lwp_threads.h>
#include <ogc/machine/processor.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include "swiss.h"
#include "main.h"
#include "util.h"
#include "info.h"
#include "httpd.h"
#include "exi.h"
#include "patcher.h"
#include "dvd.h"
#include "gcm.h"
#include "mp3.h"
#include "wkf.h"
#include "cheats.h"
#include "settings.h"
#include "aram/sidestep.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "devices/filemeta.h"
#include "dolparameters.h"
#include "../../reservedarea.h"

DiskHeader GCMDisk;      //Gamecube Disc Header struct
char IPLInfo[256] __attribute__((aligned(32)));
GXRModeObj *newmode = NULL;
char txtbuffer[2048];           //temporary text buffer
file_handle curFile;    //filedescriptor for current file
file_handle curDir;     //filedescriptor for current directory
extern int execdpatch_bin_size;
extern u8 execdpatch_bin[];

// Menu related variables
int curMenuLocation = ON_FILLIST; //where are we on the screen?
int curMenuSelection = 0;	      //menu selection
int curSelection = 0;		      //entry selection
int needsDeviceChange = 0;
int needsRefresh = 0;
int current_view_start = 0;
int current_view_end = 0;

/* The default boot up MEM1 lowmem values (from ipl when booting a game) */
static const u32 GC_DefaultConfig[56] =
{
	0x0D15EA5E, 0x00000001, 0x01800000, 0x00000003, //  0.. 3 80000020
	0x00000000, 0x00000000, 0x00000000, 0x00000000, //  4.. 7 80000030
	0x00000000, 0x00000000, 0x00000000, 0x00000000, //  8..11 80000040
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 12..15 80000050
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 16..19 80000060
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 20..23 80000070
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 24..27 80000080
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 28..31 80000090
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 32..35 800000A0
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 36..39 800000B0
	0x015D47F8, 0xF8248360, 0x00000000, 0x00000000, // 40..43 800000C0
	0x01000000, 0x00000000, 0x00000000, 0x00000000, // 44..47 800000D0
	0x00000000, 0x00000000, 0x00000000, 0x81800000, // 48..51 800000E0
	0x01800000, 0x00000000, 0x09A7EC80, 0x1CF7C580  // 52..55 800000F0
};

char *DiscIDNoNTSC[] = {"DLSP64", "G3FD69", "G3FF69", "G3FP69", "G3FS69", "GFZP01", "GLRD64", "GLRF64", "GLRP64", "GM8P01", "GMSP01", "GSWD64", "GSWF64", "GSWI64", "GSWP64", "GSWS64"};

/* re-init video for a given game */
void ogc_video__reset()
{
	char* gameID = (char*)&GCMDisk;
	char region = wodeRegionToChar(GCMDisk.RegionCode);
	int i;
	
	if(!swissSettings.forceEncoding) {
		if(region == 'J')
			swissSettings.forceEncoding = 2;
		else
			swissSettings.forceEncoding = 1;
	}
	
	if(!swissSettings.gameVMode || swissSettings.disableVideoPatches) {
		if(region == 'P')
			swissSettings.gameVMode = -2;
		else
			swissSettings.gameVMode = -1;
		
		if(swissSettings.uiVMode > 0) {
			swissSettings.sram60Hz = (swissSettings.uiVMode >= 1) && (swissSettings.uiVMode <= 2);
			swissSettings.sramProgressive = (swissSettings.uiVMode == 2) || (swissSettings.uiVMode == 4);
		}
	} else {
		swissSettings.sram60Hz = (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7);
		swissSettings.sramProgressive = (swissSettings.gameVMode >= 4 && swissSettings.gameVMode <= 7) ||
										(swissSettings.gameVMode >= 11 && swissSettings.gameVMode <= 14);
	}
	
	if(!strncmp(gameID, "GB3E51", 6) || (!strncmp(gameID, "G2OE41", 6) && swissSettings.sramLanguage == 3) || !strncmp(gameID, "GMXP70", 6))
		swissSettings.sramProgressive = 0;
	
	syssram* sram = __SYS_LockSram();
	sram->ntd = swissSettings.sram60Hz ? (sram->ntd|0x40):(sram->ntd&~0x40);
	sram->flags = swissSettings.sramProgressive ? (sram->flags|0x80):(sram->flags&~0x80);
	__SYS_UnlockSram(1);
	while(!__SYS_SyncSram());
	
	if(region == 'P' && swissSettings.bs2Boot) {
		if(swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7)
			swissSettings.gameVMode += 7;
	}
	
	for(i = 0; i < sizeof(DiscIDNoNTSC)/sizeof(char*); i++) {
		if(!strncmp(gameID, DiscIDNoNTSC[i], 6)) {
			if(swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7)
				swissSettings.gameVMode += 7;
			break;
		}
	}
	
	/* set TV mode for current game */
	uiDrawObj_t *msgBox = NULL;
	switch(swissSettings.gameVMode) {
		case -1:
			if(VIDEO_GetCurrentTvMode() == VI_MPAL) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL-M 480i");
				newmode = &TVMpal480IntDf;
			} else {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 480i");
				newmode = &TVNtsc480IntDf;
			}
			break;
		case -2:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 576i");
			newmode = &TVPal576IntDfScale;
			break;
		case 1:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 480i");
			newmode = region == 'P' ? &TVPal576IntDfScale : &TVNtsc480IntDf;
			break;
		case 2:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 480sf");
			newmode = region == 'P' ? &TVPal576IntDfScale : &TVNtsc480IntDf;
			break;
		case 3:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 240p");
			newmode = region == 'P' ? &TVPal576IntDfScale : &TVNtsc480IntDf;
			break;
		case 4:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 960i");
				newmode = region == 'P' ? &TVPal576ProgScale : &TVNtsc480Prog;
			} else {
				swissSettings.gameVMode = 1;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: NTSC 480i\nComponent Cable not detected.");
				newmode = region == 'P' ? &TVPal576IntDfScale : &TVNtsc480IntDf;
			}
			break;
		case 5:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 480p");
				newmode = region == 'P' ? &TVPal576ProgScale : &TVNtsc480Prog;
			} else {
				swissSettings.gameVMode = 3;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: NTSC 240p\nComponent Cable not detected.");
				newmode = region == 'P' ? &TVPal576IntDfScale : &TVNtsc480IntDf;
			}
			break;
		case 6:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 1080i");
				newmode = region == 'P' ? &TVPal576ProgScale : &TVNtsc480Prog;
			} else {
				swissSettings.gameVMode = 1;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: NTSC 480i\nComponent Cable not detected.");
				newmode = region == 'P' ? &TVPal576IntDfScale : &TVNtsc480IntDf;
			}
			break;
		case 7:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 540p");
				newmode = region == 'P' ? &TVPal576ProgScale : &TVNtsc480Prog;
			} else {
				swissSettings.gameVMode = 3;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: NTSC 240p\nComponent Cable not detected.");
				newmode = region == 'P' ? &TVPal576IntDfScale : &TVNtsc480IntDf;
			}
			break;
		case 8:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 576i");
			newmode = &TVPal576IntDfScale;
			break;
		case 9:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 576sf");
			newmode = &TVPal576IntDfScale;
			break;
		case 10:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 288p");
			newmode = &TVPal576IntDfScale;
			break;
		case 11:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 1152i");
				newmode = &TVPal576ProgScale;
			} else {
				swissSettings.gameVMode = 8;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: PAL 576i\nComponent Cable not detected.");
				newmode = &TVPal576IntDfScale;
			}
			break;
		case 12:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 576p");
				newmode = &TVPal576ProgScale;
			} else {
				swissSettings.gameVMode = 10;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: PAL 288p\nComponent Cable not detected.");
				newmode = &TVPal576IntDfScale;
			}
			break;
		case 13:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 1080i");
				newmode = &TVPal576ProgScale;
			} else {
				swissSettings.gameVMode = 8;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: PAL 576i\nComponent Cable not detected.");
				newmode = &TVPal576IntDfScale;
			}
			break;
		case 14:
			if(swissSettings.forceDTVStatus || VIDEO_HaveComponentCable()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 540p");
				newmode = &TVPal576ProgScale;
			} else {
				swissSettings.gameVMode = 10;
				msgBox = DrawMessageBox(D_WARN, "Video Mode: PAL 288p\nComponent Cable not detected.");
				newmode = &TVPal576IntDfScale;
			}
			break;
	}
	if(msgBox != NULL) {
		DrawPublish(msgBox);
		sleep(2);
		DrawDispose(msgBox);
	}
}

void do_videomode_swap() {
	if((newmode != NULL) && (newmode != getVideoMode())) {
		setVideoMode(newmode);
	}
}

void drawCurrentDevice(uiDrawObj_t *containerPanel) {
	if(devices[DEVICE_CUR] == NULL)
		return;
	device_info* info = (device_info*)devices[DEVICE_CUR]->info(devices[DEVICE_CUR]->initial);

	uiDrawObj_t *bgBox = DrawTransparentBox(30, 100, 135, 200);	// Device icon + slot box
	DrawAddChild(containerPanel, bgBox);
	// Draw the device image
	float scale = 1;
	if (devices[DEVICE_CUR]->deviceTexture.width > devices[DEVICE_CUR]->deviceTexture.height) {
		scale = 104.0f / devices[DEVICE_CUR]->deviceTexture.width;
	} else {
		scale = 84.0f / devices[DEVICE_CUR]->deviceTexture.height;
	}
	int scaledWidth = devices[DEVICE_CUR]->deviceTexture.width*scale;
	int scaledHeight = devices[DEVICE_CUR]->deviceTexture.height*scale;
	uiDrawObj_t *devImageLabel = DrawImage(devices[DEVICE_CUR]->deviceTexture.textureId
				, 30 + ((135-30) / 2) - (scaledWidth/2), 92 + ((200-100) /2) - (scaledHeight/2)	// center x,y
				, scaledWidth, scaledHeight, // scaled image
				0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
	DrawAddChild(containerPanel, devImageLabel);
	if(devices[DEVICE_CUR]->location == LOC_MEMCARD_SLOT_A)
		sprintf(txtbuffer, "%s", "Slot A");
	else if(devices[DEVICE_CUR]->location == LOC_MEMCARD_SLOT_B)
		sprintf(txtbuffer, "%s", "Slot B");
	else if(devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR)
		sprintf(txtbuffer, "%s", "DVD Device");
	else if(devices[DEVICE_CUR]->location == LOC_SERIAL_PORT_1)
		sprintf(txtbuffer, "%s", "Serial Port 1");
	else if(devices[DEVICE_CUR]->location == LOC_SERIAL_PORT_2)
		sprintf(txtbuffer, "%s", "Serial Port 2");
	else if(devices[DEVICE_CUR]->location == LOC_SYSTEM)
		sprintf(txtbuffer, "%s", "System");
	
	// Info labels
	uiDrawObj_t *devNameLabel = DrawStyledLabel(30 + ((135-30) / 2), 195, txtbuffer, 0.65f, true, defaultColor);
	uiDrawObj_t *devInfoBox = DrawTransparentBox(30, 225, 135, 330);	// Device size/extra info box
	DrawAddChild(containerPanel, devNameLabel);
	DrawAddChild(containerPanel, devInfoBox);
	
	// Total space
	uiDrawObj_t *devTotalLabel = DrawStyledLabel(83, 233, "Total:", 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devTotalLabel);
	if(info->totalSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%ulKB", info->totalSpaceInKB);
	if(info->totalSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)info->totalSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)info->totalSpaceInKB/(1024*1024));
	uiDrawObj_t *devTotalSizeLabel = DrawStyledLabel(83, 248, txtbuffer, 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devTotalSizeLabel);
	
	// Free space
	uiDrawObj_t *devFreeLabel = DrawStyledLabel(83, 268, "Free:", 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devFreeLabel);
	if(info->freeSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%ulKB", info->freeSpaceInKB);
	if(info->freeSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)info->freeSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)info->freeSpaceInKB/(1024*1024));
	uiDrawObj_t *devFreeSizeLabel = DrawStyledLabel(83, 283, txtbuffer, 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devFreeSizeLabel);
	
	// Used space
	uiDrawObj_t *devUsedLabel = DrawStyledLabel(83, 303, "Used:", 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devUsedLabel);
	u32 usedSpaceInKB = (info->totalSpaceInKB)-(info->freeSpaceInKB);
	if(usedSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%ulKB", usedSpaceInKB);
	if(usedSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)usedSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)usedSpaceInKB/(1024*1024));
	uiDrawObj_t *devUsedSizeLabel = DrawStyledLabel(83, 318, txtbuffer, 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devUsedSizeLabel);
}

// Draws all the files in the current dir.
void drawFiles(file_handle** directory, int num_files, uiDrawObj_t *containerPanel) {
	int j = 0;
	current_view_start = MIN(MAX(0,curSelection-FILES_PER_PAGE/2),MAX(0,num_files-FILES_PER_PAGE));
	current_view_end = MIN(num_files, MAX(curSelection+FILES_PER_PAGE/2,FILES_PER_PAGE));
	drawCurrentDevice(containerPanel);
	int fileListBase = 102;
	int scrollBarHeight = fileListBase+(FILES_PER_PAGE*20)+50;
	int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
	if(num_files > 0) {
		// Draw which directory we're in
		sprintf(txtbuffer, "%s", &curFile.name[0]);
		float scale = GetTextScaleToFitInWidthWithMax(txtbuffer, ((getVideoMode()->fbWidth-150)-20), .85);
		DrawAddChild(containerPanel, DrawStyledLabel(150, 80, txtbuffer, scale, false, defaultColor));
		if(num_files > FILES_PER_PAGE) {
			uiDrawObj_t *scrollBar = DrawVertScrollBar(getVideoMode()->fbWidth-25, fileListBase, 16, scrollBarHeight, (float)((float)curSelection/(float)(num_files-1)),scrollBarTabHeight);
			DrawAddChild(containerPanel, scrollBar);
		}
		for(j = 0; current_view_start<current_view_end; ++current_view_start,++j) {
			populate_meta(&((*directory)[current_view_start]));
			uiDrawObj_t *browserButton = DrawFileBrowserButton(150, fileListBase+(j*40), 
									getVideoMode()->fbWidth-30, fileListBase+(j*40)+40, 
									getRelativeName((*directory)[current_view_start].name),
									&((*directory)[current_view_start]), 
									(current_view_start == curSelection) ? B_SELECTED:B_NOSELECT);
			((*directory)[current_view_start]).uiObj = browserButton;
			DrawAddChild(containerPanel, browserButton);
		}
	}
}

uiDrawObj_t* loadingBox = NULL;
uiDrawObj_t* renderFileBrowser(file_handle** directory, int num_files, uiDrawObj_t* filePanel)
{
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) return NULL;
  
	while(1) {
		if(loadingBox != NULL) {
			DrawDispose(loadingBox);
		}
		loadingBox = DrawProgressLoading(PROGRESS_BOX_BOTTOMLEFT);
		DrawPublish(loadingBox);
		uiDrawObj_t *newPanel = DrawContainer();
		drawFiles(directory, num_files, newPanel);
		if(filePanel != NULL) {
			DrawDispose(filePanel);
		}
		filePanel = newPanel;
		DrawPublish(filePanel);
		DrawDispose(loadingBox);
		
		u32 waitButtons = PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_TRIGGER_L|PAD_TRIGGER_R;
		while ((PAD_StickY(0) > -16 && PAD_StickY(0) < 16) && !(PAD_ButtonsHeld(0) & waitButtons))
			{ VIDEO_WaitVSync (); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_UP) || PAD_StickY(0) >= 16){	curSelection = (--curSelection < 0) ? num_files-1 : curSelection;}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || PAD_StickY(0) <= -16) {curSelection = (curSelection + 1) % num_files;	}
		if(PAD_ButtonsHeld(0) & (PAD_BUTTON_LEFT|PAD_TRIGGER_L)) {
			if(curSelection == 0) {
				curSelection = num_files-1;
			}
			else {
				curSelection = (curSelection - FILES_PER_PAGE < 0) ? 0 : curSelection - FILES_PER_PAGE;
			}
		}
		if(PAD_ButtonsHeld(0) & (PAD_BUTTON_RIGHT|PAD_TRIGGER_R)) {
			if(curSelection == num_files-1) {
				curSelection = 0;
			}
			else {
				curSelection = (curSelection + FILES_PER_PAGE > num_files-1) ? num_files-1 : (curSelection + FILES_PER_PAGE) % num_files;
			}
		}
		
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A))	{
			//go into a folder or select a file
			if((*directory)[curSelection].fileAttrib==IS_DIR) {
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_SPECIAL){
				int len = strlen(&curFile.name[0]);
				
				// Go up a folder
				while(len && curFile.name[len-1]!='/')
      				len--;
				if(len != strlen(&curFile.name[0]))
					curFile.name[len-1] = '\0';

				// If we're a file, go up to the parent of the file
				if(curFile.fileAttrib == IS_FILE) {
					while(len && curFile.name[len-1]!='/')
						len--;
				}
				if(len != strlen(&curFile.name[0]))
					curFile.name[len-1] = '\0';
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_FILE){
				memcpy(&curDir, &curFile, sizeof(file_handle));
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh= manage_file() ? 1:0;
				// If we return from doing something with a file, refresh the device in the same dir we were at
				memcpy(&curFile, &curDir, sizeof(file_handle));
			}
			return filePanel;
		}
		
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B)	{
			curMenuLocation=ON_OPTIONS;
			return filePanel;
		}
		if(PAD_StickY(0) <= -16 || PAD_StickY(0) >= 16) {
			usleep((abs(PAD_StickY(0)) > 64 ? 50000:100000) - abs(PAD_StickY(0)*64));
		}
		else {
			while (PAD_ButtonsHeld(0) & waitButtons)
				{ VIDEO_WaitVSync (); }
		}
	}
	return filePanel;
}

void drawCurrentDeviceCarousel(uiDrawObj_t *containerPanel) {
	if(devices[DEVICE_CUR] == NULL)
		return;
	device_info* info = (device_info*)devices[DEVICE_CUR]->info(devices[DEVICE_CUR]->initial);

	uiDrawObj_t *bgBox = DrawTransparentBox(30, 395, getVideoMode()->fbWidth-30, 420);
	DrawAddChild(containerPanel, bgBox);
	// Device name
	uiDrawObj_t *devNameLabel = DrawStyledLabel(50, 400, devices[DEVICE_CUR]->deviceName, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devNameLabel);
	
	// Used space
	u32 usedSpaceInKB = (info->totalSpaceInKB)-(info->freeSpaceInKB);
	if(usedSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%ulKB", usedSpaceInKB);
	if(usedSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)usedSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)usedSpaceInKB/(1024*1024));
	int starting_pos = getVideoMode()->fbWidth-50-GetTextSizeInPixels(txtbuffer)*0.5;
	uiDrawObj_t *devUsedSizeLabel = DrawStyledLabel(starting_pos, 400, txtbuffer, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devUsedSizeLabel);

	starting_pos -= GetTextSizeInPixels(" | Used:")*0.5;
	uiDrawObj_t *devUsedLabel = DrawStyledLabel(starting_pos, 400, " | Used:", 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devUsedLabel);
	
	// Free space
	if(info->freeSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%ulKB", info->freeSpaceInKB);
	if(info->freeSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)info->freeSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)info->freeSpaceInKB/(1024*1024));
	starting_pos -= GetTextSizeInPixels(txtbuffer)*0.5;
	uiDrawObj_t *devFreeSizeLabel = DrawStyledLabel(starting_pos, 400, txtbuffer, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devFreeSizeLabel);

	starting_pos -= GetTextSizeInPixels(" | Free:")*0.5;
	uiDrawObj_t *devFreeLabel = DrawStyledLabel(starting_pos, 400, " | Free:", 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devFreeLabel);
	
	// Total space
	if(info->totalSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%ulKB", info->totalSpaceInKB);
	if(info->totalSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)info->totalSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)info->totalSpaceInKB/(1024*1024));
	starting_pos -= GetTextSizeInPixels(txtbuffer)*0.5;
	uiDrawObj_t *devTotalSizeLabel = DrawStyledLabel(starting_pos, 400, txtbuffer, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devTotalSizeLabel);

	starting_pos -= GetTextSizeInPixels("Total:")*0.5;
	uiDrawObj_t *devTotalLabel = DrawStyledLabel(starting_pos, 400, "Total:", 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devTotalLabel);
}

// Draws all the files in the current dir.
void drawFilesCarousel(file_handle** directory, int num_files, uiDrawObj_t *containerPanel) {
	int j = 0;
	current_view_start = MIN(MAX(0,curSelection-FILES_PER_PAGE_CAROUSEL/2),MAX(0,num_files-FILES_PER_PAGE_CAROUSEL));
	current_view_end = MIN(num_files, MAX(curSelection+FILES_PER_PAGE_CAROUSEL/2,FILES_PER_PAGE_CAROUSEL));
	drawCurrentDeviceCarousel(containerPanel);
	if(num_files > 0) {
		// Draw which directory we're in
		sprintf(txtbuffer, "%s", &curFile.name[0]);
		float scale = GetTextScaleToFitInWidthWithMax(txtbuffer, (getVideoMode()->fbWidth-60), .85);
		DrawAddChild(containerPanel, DrawStyledLabel(30, 85, txtbuffer, scale, false, defaultColor));
		int left_num = curSelection - current_view_start; // Number of entries to the left
		int right_num = (current_view_end - curSelection)-1;
		//print_gecko("%i entries to the left, %i to the right in this view\r\n", left_num, right_num);
		//print_gecko("%i cur sel, %i start, %i end\r\n", curSelection, current_view_start, current_view_end);
		
		bool parentLink = ((*directory)[curSelection].fileAttrib==IS_SPECIAL);
		int y_base = 105; // top most point
		int sub_entry_width = 40;
		int sub_entry_height = 270;
		int main_entry_width = 320;
		int main_entry_height = parentLink ? 40 : 280;
		int left_x_base = ((getVideoMode()->fbWidth / 2) - (main_entry_width / 2));  // left x entry
		int right_x_base = ((getVideoMode()->fbWidth / 2) + (main_entry_width / 2));  // right x entry
		
		uiDrawObj_t *browserObject = NULL;
		// TODO scale and position based on how far from the middle these are (banner and text too)
		// Left spineart entries
		for(j = 0; j < left_num; j++) {
			populate_meta(&((*directory)[current_view_start]));
			browserObject = DrawFileCarouselEntry(left_x_base - ((sub_entry_width*(left_num-j-1))+sub_entry_width), y_base + 10, 
									left_x_base - ((sub_entry_width*(left_num-j-1))), y_base + 10 + sub_entry_height, 
									getRelativeName((*directory)[current_view_start].name),
									&((*directory)[current_view_start]), j - left_num);
			((*directory)[current_view_start]).uiObj = browserObject;
			DrawAddChild(containerPanel, browserObject);
			current_view_start++;
		}
		
		// Main entry
		populate_meta(&((*directory)[current_view_start]));
		browserObject = DrawFileCarouselEntry(((getVideoMode()->fbWidth / 2) - (main_entry_width / 2)), y_base, 
								((getVideoMode()->fbWidth / 2) + (main_entry_width / 2)), y_base + main_entry_height, 
								parentLink ? "Up to parent directory" : getRelativeName((*directory)[current_view_start].name),
								&((*directory)[current_view_start]), 0);
		((*directory)[current_view_start]).uiObj = browserObject;
		DrawAddChild(containerPanel, browserObject);
		current_view_start++;
		
		// Right spineart entries
		for(j = 0; j < right_num; j++) {
			populate_meta(&((*directory)[current_view_start]));
			browserObject = DrawFileCarouselEntry(right_x_base + ((sub_entry_width*j)), y_base + 10, 
									right_x_base + ((sub_entry_width*j)+sub_entry_width), y_base + 10 + sub_entry_height,
									getRelativeName((*directory)[current_view_start].name),
									&((*directory)[current_view_start]), j+1);
			((*directory)[current_view_start]).uiObj = browserObject;
			DrawAddChild(containerPanel, browserObject);
			current_view_start++;
		}
	}
}

// Carousel (one main file in the middle, entries to either side)
uiDrawObj_t* renderFileCarousel(file_handle** directory, int num_files, uiDrawObj_t* filePanel)
{
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) return NULL;
	if(curSelection == 0 && num_files > 1 && (*directory)[0].fileAttrib==IS_SPECIAL) {
		curSelection = 1; // skip the ".." by default
	}
	while(1) {
		if(loadingBox != NULL) {
			DrawDispose(loadingBox);
		}
		loadingBox = DrawProgressLoading(PROGRESS_BOX_TOPRIGHT);
		DrawPublish(loadingBox);
		uiDrawObj_t *newPanel = DrawContainer();
		drawFilesCarousel(directory, num_files, newPanel);
		if(filePanel != NULL) {
			DrawDispose(filePanel);
		}
		filePanel = newPanel;
		DrawPublish(filePanel);
		DrawDispose(loadingBox);
		
		u32 waitButtons = PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_TRIGGER_L|PAD_TRIGGER_R;
		while ((PAD_StickX(0) > -16 && PAD_StickX(0) < 16) && !(PAD_ButtonsHeld(0) & waitButtons))
			{ VIDEO_WaitVSync (); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) || PAD_StickX(0) <= -16){	curSelection = (--curSelection < 0) ? num_files-1 : curSelection;}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) || PAD_StickX(0) >= 16) {curSelection = (curSelection + 1) % num_files;	}
		if(PAD_ButtonsHeld(0) & (PAD_BUTTON_UP|PAD_TRIGGER_L)) {
			if(curSelection == 0) {
				curSelection = num_files-1;
			}
			else {
				curSelection = (curSelection - FILES_PER_PAGE_CAROUSEL < 0) ? 0 : curSelection - FILES_PER_PAGE_CAROUSEL;
			}
		}
		if(PAD_ButtonsHeld(0) & (PAD_BUTTON_DOWN|PAD_TRIGGER_R)) {
			if(curSelection == num_files-1) {
				curSelection = 0;
			}
			else {
				curSelection = (curSelection + FILES_PER_PAGE_CAROUSEL > num_files-1) ? num_files-1 : (curSelection + FILES_PER_PAGE_CAROUSEL) % num_files;
			}
		}
		
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A))	{
			//go into a folder or select a file
			if((*directory)[curSelection].fileAttrib==IS_DIR) {
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_SPECIAL){
				int len = strlen(&curFile.name[0]);
				
				// Go up a folder
				while(len && curFile.name[len-1]!='/')
      				len--;
				if(len != strlen(&curFile.name[0]))
					curFile.name[len-1] = '\0';

				// If we're a file, go up to the parent of the file
				if(curFile.fileAttrib == IS_FILE) {
					while(len && curFile.name[len-1]!='/')
						len--;
				}
				if(len != strlen(&curFile.name[0]))
					curFile.name[len-1] = '\0';
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_FILE){
				memcpy(&curDir, &curFile, sizeof(file_handle));
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh= manage_file() ? 1:0;
				// If we return from doing something with a file, refresh the device in the same dir we were at
				memcpy(&curFile, &curDir, sizeof(file_handle));
			}
			return filePanel;
		}
		
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B)	{
			curMenuLocation=ON_OPTIONS;
			return filePanel;
		}
		if(PAD_StickX(0) <= -16 || PAD_StickX(0) >= 16) {
			usleep((abs(PAD_StickX(0)) > 64 ? 50000:100000) - abs(PAD_StickX(0)*64));
		}
		else {
			while (PAD_ButtonsHeld(0) & waitButtons)
				{ VIDEO_WaitVSync (); }
		}
	}
	return filePanel;
}

void select_dest_dir(file_handle* directory, file_handle* selection)
{
	file_handle *directories = NULL;
	file_handle curDir;
	memcpy(&curDir, directory, sizeof(file_entry));
	int i = 0, j = 0, max = 0, refresh = 1, num_files =0, idx = 0;
	
	int fileListBase = 90;
	int scrollBarHeight = (FILES_PER_PAGE*40);
	int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
	uiDrawObj_t* destDirBox = NULL;
	while(1){
		// Read the directory
		if(refresh) {
			num_files = devices[DEVICE_DEST]->readDir(&curDir, &directories, IS_DIR);
			sortFiles(directories, num_files);
			refresh = idx = 0;
			scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
		}
		uiDrawObj_t* tempBox = DrawEmptyBox(20,40, getVideoMode()->fbWidth-20, 450);
		DrawAddChild(tempBox, DrawLabel(50, 55, "Enter directory and press X"));
		i = MIN(MAX(0,idx-FILES_PER_PAGE/2),MAX(0,num_files-FILES_PER_PAGE));
		max = MIN(num_files, MAX(idx+FILES_PER_PAGE/2,FILES_PER_PAGE));
		if(num_files > FILES_PER_PAGE)
			DrawAddChild(tempBox, DrawVertScrollBar(getVideoMode()->fbWidth-30, fileListBase, 16, scrollBarHeight, (float)((float)idx/(float)(num_files-1)),scrollBarTabHeight));
		for(j = 0; i<max; ++i,++j) {
			DrawAddChild(tempBox, DrawSelectableButton(50,fileListBase+(j*40), getVideoMode()->fbWidth-35, fileListBase+(j*40)+40, getRelativeName((directories)[i].name), (i == idx) ? B_SELECTED:B_NOSELECT));
		}
		if(destDirBox) {
			DrawDispose(destDirBox);
		}
		destDirBox = tempBox;
		DrawPublish(destDirBox);
		while ((PAD_StickY(0) > -16 && PAD_StickY(0) < 16) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN))
			{ VIDEO_WaitVSync (); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_UP) || PAD_StickY(0) > 16){	idx = (--idx < 0) ? num_files-1 : idx;}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || PAD_StickY(0) < -16) {idx = (idx + 1) % num_files;	}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A))	{
			//go into a folder or select a file
			if((directories)[idx].fileAttrib==IS_DIR) {
				memcpy(&curDir, &(directories)[idx], sizeof(file_handle));
				refresh=1;
			}
			else if(directories[idx].fileAttrib==IS_SPECIAL){
				//go up a folder
				int len = strlen(&curDir.name[0]);
				while(len && curDir.name[len-1]!='/') {
      				len--;
				}
				if(len != strlen(&curDir.name[0])) {
					curDir.name[len-1] = '\0';
					refresh=1;
				}
			}
		}
		if(PAD_StickY(0) < -16 || PAD_StickY(0) > 16) {
			usleep(50000 - abs(PAD_StickY(0)*256));
		}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_X))	{
			memcpy(selection, &curDir, sizeof(file_handle));
			break;
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)))
			{ VIDEO_WaitVSync (); }
	}
	DrawDispose(destDirBox);
	free(directories);
}

// Alt DOL sorting/selecting code
int exccomp(const void *a1, const void *b1)
{
	const ExecutableFile* a = a1;
	const ExecutableFile* b = b1;
	
	if(!a && b) return 1;
	if(a && !b) return -1;
	if(!a && !b) return 0;
	
	if(a->type == PATCH_DOL && b->type != PATCH_DOL)
		return -1;
	if(a->type != PATCH_DOL && b->type == PATCH_DOL)
		return 1;

	return strcasecmp(a->name, b->name);
}

void sortDols(ExecutableFile *filesToPatch, int num_files)
{
	if(num_files > 0) {
		qsort(filesToPatch, num_files, sizeof(ExecutableFile), exccomp);
	}
}

// Allow the user to select an alternate DOL
ExecutableFile* select_alt_dol(ExecutableFile *filesToPatch) {
	int i = 0, j = 0, max = 0, idx = 0, page = 4;
	for(i = 0; i < 64; i++) if(filesToPatch[i].offset == 0) break;
	sortDols(filesToPatch, i);	// Sort DOL to the top
	for(i = 0; i < 64; i++) if(filesToPatch[i].offset == 0 || filesToPatch[i].type != PATCH_DOL) break;
	int num_files = i;
	if(num_files < 2) return 0;
	
	int fileListBase = 175;
	int scrollBarHeight = (page*40);
	int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
	uiDrawObj_t *container = NULL;
	while(1) {
		uiDrawObj_t *newPanel = DrawEmptyBox(20,fileListBase-30, getVideoMode()->fbWidth-20, 340);
		DrawAddChild(newPanel, DrawLabel(50, fileListBase-30, "Select DOL or Press B to boot normally"));
		i = MIN(MAX(0,idx-(page/2)),MAX(0,num_files-page));
		max = MIN(num_files, MAX(idx+(page/2),page));
		if(num_files > page)
			DrawAddChild(newPanel, DrawVertScrollBar(getVideoMode()->fbWidth-30, fileListBase, 16, scrollBarHeight, (float)((float)idx/(float)(num_files-1)),scrollBarTabHeight));
		for(j = 0; i<max; ++i,++j) {
			DrawAddChild(newPanel, DrawSelectableButton(50,fileListBase+(j*40), getVideoMode()->fbWidth-35, fileListBase+(j*40)+40, filesToPatch[i].name, (i == idx) ? B_SELECTED:B_NOSELECT));
		}
		if(container) {
			DrawDispose(container);
		}
		DrawPublish(newPanel);
		container = newPanel;
		while ((PAD_StickY(0) > -16 && PAD_StickY(0) < 16) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN))
			{ VIDEO_WaitVSync (); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_UP) || PAD_StickY(0) > 16){	idx = (--idx < 0) ? num_files-1 : idx;}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || PAD_StickY(0) < -16) {idx = (idx + 1) % num_files;	}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A))	break;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_B))	{ idx = -1; break; }
		if(PAD_StickY(0) < -16 || PAD_StickY(0) > 16) {
			usleep(50000 - abs(PAD_StickY(0)*256));
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)))
			{ VIDEO_WaitVSync (); }
	}
	DrawDispose(container);
	return idx >= 0 ? &filesToPatch[idx] : NULL;
	
}

unsigned int load_app(ExecutableFile *filesToPatch, int numToPatch)
{
	uiDrawObj_t* progBox = NULL;
	char* gameID = (char*)0x80000000;
	int sizeToRead, type;
	void *buffer;
	
	if(swissSettings.bs2Boot) {
		progBox = DrawPublish(DrawProgressBar(true, 0, "Loading BS2"));
		
		// Read BS2
		sizeToRead = 0x1AFF00-0x820;
		type = PATCH_APPLOADER;
		buffer = memalign(32,sizeToRead);
		if(!buffer) {
			return 0;
		}
		read_rom_ipl_clear(0x820,buffer,sizeToRead);
		
		// Guess BS2's size
		if(!IPLInfo[0x55]) {
			sizeToRead = 1435168;
		}
		else if(!strncmp(&IPLInfo[0x55], "NTSC Revision 1.1", 17)) {
			sizeToRead = 1583056;
		}
		else if(!strncmp(&IPLInfo[0x55], "NTSC Revision 1.2", 17)) {
			sizeToRead = 1587472;
		}
		else if(!strncmp(&IPLInfo[0x55], "PAL  Revision 1.0", 17)) {
			sizeToRead = 1763016;
		}
		else if(!strncmp(&IPLInfo[0x55], "MPAL Revision 1.1", 17)) {
			sizeToRead = 1561744;
		}
		else if(!strncmp(&IPLInfo[0x55], "PAL  Revision 1.2", 17)) {
			sizeToRead = 1766736;
		}
	}
	else {
		memcpy((void*)0x80000020,GC_DefaultConfig,0xE0);
		
		// Read the game header to 0x80000000 & apploader header
		devices[DEVICE_CUR]->seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,(unsigned char*)0x80000000,32) != 32) {
			DrawPublish(DrawMessageBox(D_FAIL, "Game Header Failed to read"));
			while(1);
		}
		
		// Prompt for DOL selection if multi-dol
		ExecutableFile* altDol = NULL;
		if(filesToPatch != NULL && numToPatch > 0) {
			altDol = select_alt_dol(filesToPatch);
			if(altDol != NULL) {
				print_gecko("Alt DOL selected :%08X\r\n", altDol->offset);
				GCMDisk.DOLOffset = altDol->offset;
				// For a DOL from a TGC, redirect the FST to the TGC FST.
				if(altDol->tgcFstSize > 0) {
					GCMDisk.MaxFSTSize = altDol->tgcFstSize;
					GCMDisk.FSTOffset = altDol->tgcFstOffset;
				}
			}
		}

		progBox = DrawPublish(DrawProgressBar(true, 0, "Loading DOL"));
		
		// Adjust top of memory
		u32 top_of_main_ram = 0x81800000;
		
		// Native devices shouldn't steal memory for AS redirection
		if(GCMDisk.AudioStreaming && (devices[DEVICE_CUR]->features & FEAT_REPLACES_DVD_FUNCS) && !((devices[DEVICE_CUR]->features & FEAT_ALT_READ_PATCHES) || !swissSettings.emulateAudioStreaming)) {
			top_of_main_ram = DECODED_BUFFER_0;
		}
		// Steal even more if there's cheats!
		if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
			top_of_main_ram = WIIRD_ENGINE;
		}
		// execD handler lives at the top of mem and is branched to via lowmem code.
		if(*(vu32*)VAR_EXECD_OFFSET != 0 && *(vu32*)VAR_EXECD_OFFSET != -1) {
			top_of_main_ram = EXECD_RUNNER;
		}

		print_gecko("Top of RAM simulated as: 0x%08X\r\n", top_of_main_ram);
		
		// Read FST to top of Main Memory (round to 32 byte boundary)
		u32 fstSizeAligned = GCMDisk.MaxFSTSize + (32-(GCMDisk.MaxFSTSize%32));
		devices[DEVICE_CUR]->seekFile(&curFile,GCMDisk.FSTOffset,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,(void*)(top_of_main_ram-fstSizeAligned),GCMDisk.MaxFSTSize) != GCMDisk.MaxFSTSize) {
			DrawPublish(DrawMessageBox(D_FAIL, "Failed to read fst.bin"));
			while(1);
		}
		if(altDol != NULL && altDol->tgcFstSize > 0) {
			adjust_tgc_fst((void*)(top_of_main_ram-fstSizeAligned), altDol->tgcBase, altDol->tgcFileStartArea, altDol->tgcFakeOffset);
		}
		
		// Read bi2.bin (Disk Header Information) to just under the FST
		devices[DEVICE_CUR]->seekFile(&curFile,0x440,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,(void*)(top_of_main_ram-fstSizeAligned-0x2000),0x2000) != 0x2000) {
			DrawPublish(DrawMessageBox(D_FAIL, "Failed to read bi2.bin"));
			while(1);
		}

		*(volatile u32*)0x8000002C = (*(volatile u32*)0xCC00302C >> 28) + (swissSettings.debugUSB ? 0x10000004:0x00000001);
		*(volatile u32*)0x800000F4 = top_of_main_ram-fstSizeAligned-0x2000;	// bi2.bin location
		*(volatile u32*)0x80000038 = top_of_main_ram-fstSizeAligned;		// FST Location in ram
		*(volatile u32*)0x8000003C = GCMDisk.MaxFSTSize;					// FST Max Length
		*(volatile u32*)0x80000034 = *(volatile u32*)0x80000038;			// Arena Hi
		*(volatile u32*)0x80000028 = top_of_main_ram & 0x01FFFFFF;			// Physical Memory Size
		*(volatile u32*)0x800000F0 = top_of_main_ram & 0x01FFFFFF;			// Console Simulated Mem size
		u32* osctxblock = (u32*)memalign(32, 1024);
		*(volatile u32*)0x800000C0 = (u32)osctxblock & 0x7FFFFFFF;
		*(volatile u32*)0x800000D4 = (u32)osctxblock;
		memset(osctxblock, 0, 1024);

		print_gecko("DOL Lives at %08X\r\n", GCMDisk.DOLOffset);
		
		// Read the Main DOL header
		DOLHEADER dolhdr;
		devices[DEVICE_CUR]->seekFile(&curFile,GCMDisk.DOLOffset,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,&dolhdr,DOLHDRLENGTH) != DOLHDRLENGTH) {
			DrawPublish(DrawMessageBox(D_FAIL, "Failed to read DOL Header"));
			while(1);
		}
		
		// Figure out the size of the Main DOL so that we can read it all
		sizeToRead = DOLSize(&dolhdr);
		type = PATCH_DOL;
		print_gecko("DOL size %i\r\n", sizeToRead);

		// Read the entire Main DOL
		buffer = memalign(32,sizeToRead);
		print_gecko("DOL buffer %08X\r\n", (u32)buffer);
		if(!buffer) {
			return 0;
		}
		devices[DEVICE_CUR]->seekFile(&curFile,GCMDisk.DOLOffset,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,buffer,sizeToRead) != sizeToRead) {
			DrawPublish(DrawMessageBox(D_FAIL, "Failed to read DOL"));
			while(1);
		}
	}

	// Patch to read from SD/HDD/USBGecko/WKF (if frag req or audio streaming)
	if(devices[DEVICE_CUR]->features & FEAT_REPLACES_DVD_FUNCS) {
		if((devices[DEVICE_CUR]->features & FEAT_ALT_READ_PATCHES) || !swissSettings.emulateAudioStreaming) {
			Patch_DVDLowLevelReadAlt(buffer, sizeToRead, gameID, type);
			Patch_GameSpecificReadAlt(buffer, sizeToRead, gameID, type);
		}
		else {
			u32 ret = Patch_DVDLowLevelRead(buffer, sizeToRead, type);
			if(READ_PATCHED_ALL != ret)	{
				uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "Failed to find necessary functions for patching!");
				DrawPublish(msgBox);
				sleep(5);
				DrawDispose(msgBox);
			}
			else {
				Patch_GameSpecificRead(buffer, sizeToRead, gameID, type);
			}
		}
	}
	
	// Patch specific game hacks
	Patch_GameSpecific(buffer, sizeToRead, gameID, type);
	
	// Patch IGR
	if(swissSettings.igrType != IGR_OFF || swissSettings.invertCStick) {
		Patch_PADStatus(buffer, sizeToRead, type);
	}
	
	// Patch OSReport to print out over USBGecko
	if(swissSettings.debugUSB && usb_isgeckoalive(1) && !swissSettings.wiirdDebug) {
		Patch_Fwrite(buffer, sizeToRead);
	}
	// Force Video Mode
	if(!swissSettings.disableVideoPatches) {
		Patch_GameSpecificVideo(buffer, sizeToRead, gameID, type);
		Patch_VideoMode(buffer, sizeToRead, type);
	}
	// Force Widescreen
	if(swissSettings.forceWidescreen) {
		Patch_Widescreen(buffer, sizeToRead, type);
	}
	// Force Anisotropy
	if(swissSettings.forceAnisotropy) {
		Patch_TexFilt(buffer, sizeToRead, type);
	}
	// Force Text Encoding
	Patch_FontEncode(buffer, sizeToRead);
	
	// Cheats
	if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
		Patch_CheatsHook(buffer, sizeToRead, type);
	}

	DCFlushRange(buffer, sizeToRead);
	ICInvalidateRange(buffer, sizeToRead);
	
	// See if the combination of our patches has exhausted our play area.
	if(!install_code(0)) {
		DrawDispose(progBox);
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "Too many patches enabled, memory limit reached!");
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return 0;
	}
	
	// Don't spin down the drive when running something from it...
	if(devices[DEVICE_CUR] != &__device_dvd) {
		devices[DEVICE_CUR]->deinit(&curFile);
	}
	
	DrawDispose(progBox);
	DrawShutdown();
	
	do_videomode_swap();
	VIDEO_SetPostRetraceCallback (NULL);
	*(vu32*)VAR_TVMODE = VIDEO_GetCurrentTvMode();
	DCFlushRange((void*)0x80000000, 0x3100);
	ICInvalidateRange((void*)0x80000000, 0x3100);
	
	// Try a device speed test using the actual in-game read code
	if((devices[DEVICE_CUR]->features & FEAT_REPLACES_DVD_FUNCS) && !((devices[DEVICE_CUR]->features & FEAT_ALT_READ_PATCHES) || !swissSettings.emulateAudioStreaming)) {
		install_code(1);
		print_gecko("Attempting speed test\r\n");
		char *buffer = memalign(32,1024*1024);
		typedef u32 (*_calc_speed) (void* dst, u32 len, u32 *speed);
		_calc_speed calculate_speed = (_calc_speed) (void*)(CALC_SPEED);
		u32 speed = 0;
		if(devices[DEVICE_CUR] == &__device_ide_a || devices[DEVICE_CUR] == &__device_ide_b) {
			calculate_speed(buffer, 1024*1024, &speed);	//Once more for HDD seek
			speed = 0;
		}
		calculate_speed(buffer, 1024*1024, &speed);
		float timeTakenInSec = (float)speed/1000000;
		print_gecko("Speed is %i usec %.2f sec for 1MB\r\n",speed,timeTakenInSec);
		float bytePerSec = (1024*1024) / timeTakenInSec;
		print_gecko("Speed is %.2f KB/s\r\n",bytePerSec/1024);
		print_gecko("Speed for 1024 bytes is: %i usec\r\n",speed/1024);
		*(vu32*)VAR_DEVICE_SPEED = speed/1024;
		free(buffer);
	}
	
	if(swissSettings.hasDVDDrive) {
		// Check DVD Status, make sure it's error code 0
		print_gecko("DVD: %08X\r\n",dvd_get_error());
	}
	// Clear out some patch variables
	*(vu32*)VAR_TMP1 = 0;
	*(vu32*)VAR_TMP2 = 0;
	*(vu32*)VAR_FAKE_IRQ_SET = 0;
	*(vu32*)VAR_INTERRUPT_TIMES = 0;
	*(vu32*)VAR_READS_IN_AS = 0;
	*(vu32*)VAR_DISC_CHANGING = 0;
	*(vu32*)VAR_LAST_OFFSET = 0xCAFEBABE;
	*(vu32*)VAR_AS_ENABLED = GCMDisk.AudioStreaming;
	*(vu8*)VAR_IGR_EXIT_TYPE = swissSettings.igrType;
	*(vu32*)VAR_EMU_READ_SPEED = swissSettings.emulateReadSpeed;
	memset(VAR_DI_REGS, 0, 0x24);
	memset(VAR_STREAM_START, 0, 0xA0);
	print_gecko("Audio Streaming is %s\r\n",*(vu32*)VAR_AS_ENABLED?"Enabled":"Disabled");
	if(*(vu32*)VAR_EXECD_OFFSET != 0 && *(vu32*)VAR_EXECD_OFFSET != -1) {
		print_gecko("execD offset: %08X\r\n", *(vu32*)VAR_EXECD_OFFSET);
		memcpy((void*)EXECD_RUNNER, execdpatch_bin, execdpatch_bin_size);
		DCFlushRange((void*)EXECD_RUNNER, EXECD_RUNNER_SPACE);
		ICInvalidateRange((void*)EXECD_RUNNER, EXECD_RUNNER_SPACE);
		print_gecko("Installed execD handler to %08X (%i bytes)\r\n", EXECD_RUNNER, execdpatch_bin_size);
	}
	
	if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
		kenobi_install_engine();
	}
	
	print_gecko("libogc shutdown and boot game!\r\n");
	if(devices[DEVICE_CUR] == &__device_sd_a || devices[DEVICE_CUR] == &__device_sd_b || devices[DEVICE_CUR] == &__device_sd_c) {
		print_gecko("set size\r\n");
		sdgecko_setPageSize(devices[DEVICE_CUR] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_CUR] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2), 512);
	}
	else if(devices[DEVICE_PATCHES] == &__device_sd_a || devices[DEVICE_PATCHES] == &__device_sd_b || devices[DEVICE_PATCHES] == &__device_sd_c) {
		print_gecko("set size\r\n");
		sdgecko_setPageSize(devices[DEVICE_PATCHES] == &__device_sd_a ? EXI_CHANNEL_0:(devices[DEVICE_PATCHES] == &__device_sd_b ? EXI_CHANNEL_1:EXI_CHANNEL_2), 512);
	}
	if(type == PATCH_APPLOADER) {
		BINtoARAM(buffer, sizeToRead, 0x81300000);
	}
	else if(type == PATCH_DOL) {
		DOLtoARAM(buffer, 0, NULL);
	}
	return 0;
}

void boot_dol()
{ 
	unsigned char *dol_buffer;
	unsigned char *ptr;
  
	dol_buffer = (unsigned char*)memalign(32,curFile.size);
	if(!dol_buffer) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"DOL is too big. Press A.");
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return;
	}
		
	int i=0;
	ptr = dol_buffer;
	uiDrawObj_t* progBar = DrawProgressBar(false, 0, "Loading DOL");
	DrawPublish(progBar);
	for(i = 0; i < curFile.size; i+= 131072) {
		DrawUpdateProgressBar(progBar, (int)((float)((float)i/(float)curFile.size)*100));
		
		devices[DEVICE_CUR]->seekFile(&curFile,i,DEVICE_HANDLER_SEEK_SET);
		int size = i+131072 > curFile.size ? curFile.size-i : 131072; 
		if(devices[DEVICE_CUR]->readFile(&curFile,ptr,size)!=size) {
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"Failed to read DOL. Press A.");
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
			return;
		}
  		ptr+=size;
	}
	DrawDispose(progBar);
	
	// Build a command line to pass to the DOL
	int argc = 0;
	char *argv[1024];

	u32 readTest;
	char fileName[PATHNAME_MAX];
	memset(&fileName[0], 0, PATHNAME_MAX);
	strncpy(&fileName[0], &curFile.name[0], strrchr(&curFile.name[0], '.') - &curFile.name[0]);
	print_gecko("DOL file name without extension [%s]\r\n", fileName);
	
	file_handle *cliArgFile = calloc(1, sizeof(file_handle));
	
	// .cli argument file
	sprintf(cliArgFile->name, "%s.cli", fileName);
	if(devices[DEVICE_CUR]->readFile(cliArgFile, &readTest, 4) != 4) {
		free(cliArgFile);
		cliArgFile = NULL;
	}
	
	// we found something, use parameters (.cli)
	if(cliArgFile) {
		print_gecko("Argument file found [%s]\r\n", cliArgFile->name);
		char *cli_buffer = memalign(32, cliArgFile->size);
		if(cli_buffer) {
			devices[DEVICE_CUR]->seekFile(cliArgFile, 0, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CUR]->readFile(cliArgFile, cli_buffer, cliArgFile->size);

			// Parse CLI
			argv[argc] = (char*)&curFile.name;
			argc++;
			// First argument is at the beginning of the file
			if(cli_buffer[0] != '\r' && cli_buffer[0] != '\n') {
				argv[argc] = cli_buffer;
				argc++;
			}

			// Search for the others after each newline
			for(i = 0; i < cliArgFile->size; i++) {
				if(cli_buffer[i] == '\r' || cli_buffer[i] == '\n') {
					cli_buffer[i] = '\0';
				}
				else if(cli_buffer[i - 1] == '\0') {
					argv[argc] = cli_buffer + i;
					argc++;

					if(argc >= 1024)
						break;
				}
			}
		}
	}

	free(cliArgFile);

	file_handle *dcpArgFile = calloc(1, sizeof(file_handle));
	
	// .dcp parameter file
	sprintf(dcpArgFile->name, "%s.dcp", fileName);
	if(devices[DEVICE_CUR]->readFile(dcpArgFile, &readTest, 4) != 4) {
		free(dcpArgFile);
		dcpArgFile = NULL;
	}
	
	// we found something, parse and display parameters for selection (.dcp)
	if(dcpArgFile) {
		print_gecko("Argument file found [%s]\r\n", dcpArgFile->name);
		char *dcp_buffer = memalign(32, dcpArgFile->size);
		if(dcp_buffer) {
			devices[DEVICE_CUR]->seekFile(dcpArgFile, 0, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CUR]->readFile(dcpArgFile, dcp_buffer, dcpArgFile->size);

			// Parse DCP
			parseParameters(dcp_buffer);
			Parameters *params = (Parameters*)getParameters();
			if(params->num_params > 0) {
				DrawArgsSelector(getRelativeName(&curFile.name[0]));
				// Get an argv back or none.
				populateArgv(&argc, argv, (char*)&curFile.name);
			}
		}
	}

	free(dcpArgFile);

	if(devices[DEVICE_CUR] != NULL) devices[DEVICE_CUR]->deinit( devices[DEVICE_CUR]->initial );
	// Boot
	DOLtoARAM(dol_buffer, argc, argc == 0 ? NULL : argv);
}

/* Manage file  - The user will be asked what they want to do with the currently selected file - copy/move/delete*/
bool manage_file() {
	// If it's a file
	if(curFile.fileAttrib == IS_FILE) {
		if(!swissSettings.enableFileManagement) {
			load_file();
			return false;
		}
		// Ask the user what they want to do with it
		uiDrawObj_t* manageFileBox = DrawEmptyBox(10,150, getVideoMode()->fbWidth-10, 350);
		DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 160, "Manage File:", 1.0f, true, defaultColor));
		float scale = GetTextScaleToFitInWidth(getRelativeName(curFile.name), getVideoMode()->fbWidth-10-10);
		DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 200, getRelativeName(curFile.name), scale, true, defaultColor));
		if(devices[DEVICE_CUR]->features & FEAT_WRITE) {
			DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 230, "(A) Load (X) Copy (Y) Move (Z) Delete", 1.0f, true, defaultColor));
		}
		else {
			DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 230, "(A) Load (X) Copy", 1.0f, true, defaultColor));
		}
		DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 300, "Press an option to continue, or B to return", 1.0f, true, defaultColor));
		DrawPublish(manageFileBox);
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
		int option = 0;
		while(1) {
			u32 buttons = PAD_ButtonsHeld(0);
			if(buttons & PAD_BUTTON_X) {
				option = COPY_OPTION;
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_X){ VIDEO_WaitVSync (); }
				break;
			}
			if((devices[DEVICE_CUR]->features & FEAT_WRITE) && (buttons & PAD_BUTTON_Y)) {
				option = MOVE_OPTION;
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
				break;
			}
			if((devices[DEVICE_CUR]->features & FEAT_WRITE) && (buttons & PAD_TRIGGER_Z)) {
				option = DELETE_OPTION;
				while(PAD_ButtonsHeld(0) & PAD_TRIGGER_Z){ VIDEO_WaitVSync (); }
				break;
			}
			if(buttons & PAD_BUTTON_A) {
				DrawDispose(manageFileBox);
				load_file();
				return false;
			}
			if(buttons & PAD_BUTTON_B) {
				DrawDispose(manageFileBox);
				return false;
			}
		}
		DrawDispose(manageFileBox);
	
		// If delete, delete it + refresh the device
		if(option == DELETE_OPTION) {
			if(!devices[DEVICE_CUR]->deleteFile(&curFile)) {
				//go up a folder
				int len = strlen(&curFile.name[0]);
				while(len && curFile.name[len-1]!='/') {
      				len--;
				}
				curFile.name[len-1] = '\0';
				uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,"File deleted! Press A to continue");
				DrawPublish(msgBox);
				wait_press_A();
				DrawDispose(msgBox);
				needsRefresh=1;
			}
			else {
				uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,"Delete Failed! Press A to continue");
				DrawPublish(msgBox);
				wait_press_A();
				DrawDispose(msgBox);
			}
		}
		// If copy, ask which device is the destination device and copy
		else if((option == COPY_OPTION) || (option == MOVE_OPTION)) {
			u32 ret = 0;
			// Show a list of destination devices (the same device is also a possibility)
			select_device(DEVICE_DEST);
			if(devices[DEVICE_DEST] == NULL) return false;

			// If the devices are not the same, init the destination, fail on non-existing device/etc
			if(devices[DEVICE_CUR] != devices[DEVICE_DEST]) {
				devices[DEVICE_DEST]->deinit( devices[DEVICE_DEST]->initial );	
				deviceHandler_setStatEnabled(0);
				if(!devices[DEVICE_DEST]->init( devices[DEVICE_DEST]->initial )) {
					sprintf(txtbuffer, "Failed to init destination device! (%ul)\nPress A to continue.",ret);
					uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
					DrawPublish(msgBox);
					wait_press_A();
					DrawDispose(msgBox);
					deviceHandler_setStatEnabled(1);
					return false;
				}
				deviceHandler_setStatEnabled(1);
			}
			// Traverse this destination device and let the user select a directory to dump the file in
			file_handle *destFile = memalign(32,sizeof(file_handle));
			memset(destFile, 0, sizeof(file_handle));
			
			// Show a directory only browser and get the destination file location
			select_dest_dir(devices[DEVICE_DEST]->initial, destFile);
			
			u32 isDestCard = devices[DEVICE_DEST] == &__device_card_a || devices[DEVICE_DEST] == &__device_card_b;
			u32 isSrcCard = devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b;
			
			sprintf(destFile->name, "%s/%s",destFile->name,stripInvalidChars(getRelativeName(&curFile.name[0])));
			destFile->fp = 0;
			destFile->ffsFp = 0;
			destFile->fileBase = 0;
			destFile->size = 0;
			destFile->fileAttrib = IS_FILE;
			// Create a GCI if something is coming out from CARD to another device
			if(isSrcCard && !isDestCard) {
				sprintf(destFile->name, "%s.gci",destFile->name);
			}

			// If the destination file already exists, ask the user what to do
			u8 nothing[1];
			if(devices[DEVICE_DEST]->readFile(destFile, nothing, 1) >= 0) {
				uiDrawObj_t* dupeBox = DrawEmptyBox(10,150, getVideoMode()->fbWidth-10, 350);
				DrawAddChild(dupeBox, DrawStyledLabel(640/2, 160, "File exists:", 1.0f, true, defaultColor));
				float scale = GetTextScaleToFitInWidth(getRelativeName(curFile.name), getVideoMode()->fbWidth-10-10);
				DrawAddChild(dupeBox, DrawStyledLabel(640/2, 200, getRelativeName(curFile.name), scale, true, defaultColor));
				DrawAddChild(dupeBox, DrawStyledLabel(640/2, 230, "(A) Rename (Z) Overwrite", 1.0f, true, defaultColor));
				DrawAddChild(dupeBox, DrawStyledLabel(640/2, 300, "Press an option to continue, or B to return", 1.0f, true, defaultColor));
				DrawPublish(dupeBox);
				while(PAD_ButtonsHeld(0) & (PAD_BUTTON_A | PAD_TRIGGER_Z)) { VIDEO_WaitVSync (); }
				while(1) {
					u32 buttons = PAD_ButtonsHeld(0);
					if(buttons & PAD_TRIGGER_Z) {
						if(!strcmp(curFile.name, destFile->name)) {
							DrawDispose(dupeBox);
							uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Can't overwrite a file with itself!");
							DrawPublish(msgBox);
							wait_press_A();
							DrawDispose(msgBox);
							return false; 
						}
						else {
							devices[DEVICE_DEST]->deleteFile(destFile);
						}

						while(PAD_ButtonsHeld(0) & PAD_TRIGGER_Z){ VIDEO_WaitVSync (); }
						break;
					}
					if(buttons & PAD_BUTTON_A) {
						int cursor, extension_start = -1, copy_num = 0;
						char name_backup[1024];
						for(cursor = 0; destFile->name[cursor]; cursor++) {
							if(destFile->name[cursor] == '.' && destFile->name[cursor - 1] != '/'
								&& cursor > 0)
								extension_start = cursor;
							if(destFile->name[cursor] == '/')
								extension_start = -1;
							name_backup[cursor] = destFile->name[cursor];
						}

						devices[DEVICE_DEST]->closeFile(destFile);

						if(extension_start >= 0) {
							destFile->name[extension_start] = 0;
							cursor = extension_start;
						}

						if(destFile->name[cursor - 3] == '_' && in_range(destFile->name[cursor - 2], '0', '9')
							    && in_range(destFile->name[cursor - 1], '0', '9')) {
							copy_num = (int) strtol(destFile->name + cursor - 2, 0, 10);
						}
						else {
							cursor += 3;
							if((strlen(name_backup) + 4) >= 1024) {
								DrawDispose(dupeBox);
								uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "File name too long!");
								DrawPublish(msgBox);
								wait_press_A();
								DrawDispose(msgBox);
								return false;
							}
							destFile->name[cursor - 3] = '_';
							sprintf(destFile->name + cursor - 2, "%02i", copy_num);
						}

						if(extension_start >= 0) {
							strcpy(destFile->name + cursor, name_backup + extension_start);
						}

						while(devices[DEVICE_DEST]->readFile(destFile, nothing, 1) >= 0) {
							devices[DEVICE_DEST]->closeFile(destFile);
							copy_num++;
							if(copy_num > 99) {
								DrawDispose(dupeBox);
								uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Too many copies!");
								DrawPublish(msgBox);
								wait_press_A();
								DrawDispose(msgBox);
								return false;
							}
							sprintf(destFile->name + cursor - 2, "%02i", copy_num);
							if(extension_start >= 0) {
								strcpy(destFile->name + cursor, name_backup + extension_start);
							}
						}

						while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
						break;
					}
					if(buttons & PAD_BUTTON_B) {
						DrawDispose(dupeBox);
						return false;
					}
				}
				DrawDispose(dupeBox);
			}

			// Seek back to 0 after all these reads
			devices[DEVICE_DEST]->seekFile(destFile, 0, DEVICE_HANDLER_SEEK_SET);
			
			// Same (fat based) device and user wants to move the file, just rename ;)
			if(devices[DEVICE_CUR] == devices[DEVICE_DEST]
				&& option == MOVE_OPTION 
				&& (devices[DEVICE_CUR]->features & FEAT_FAT_FUNCS)) {
				ret = f_rename(&curFile.name[0], &destFile->name[0]);
				//go up a folder
				int len = strlen(&curFile.name[0]);
				while(len && curFile.name[len-1]!='/') {
					len--;
				}
				curFile.name[len-1] = '\0';
				needsRefresh=1;
				uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,ret ? "Move Failed! Press A to continue":"File moved! Press A to continue");
				DrawPublish(msgBox);
				wait_press_A();
				DrawDispose(msgBox);
			}
			else {
				// If we're copying out from memory card, make a .GCI
				if(isSrcCard) {
					setCopyGCIMode(TRUE);
					curFile.size += sizeof(GCI);
				}
				// If we're copying a .gci to a memory card, do it properly
				if(isDestCard && (endsWith(destFile->name,".gci"))) {
					// Read the header
					char *gciHeader = memalign(32, sizeof(GCI));
					devices[DEVICE_CUR]->seekFile(&curFile, 0, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_CUR]->readFile(&curFile, gciHeader, sizeof(GCI));
					devices[DEVICE_CUR]->seekFile(&curFile, 0, DEVICE_HANDLER_SEEK_SET);
					setGCIInfo(gciHeader);
					free(gciHeader);
				}
				
				// Read from one file and write to the new directory
				u32 isCard = devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b;
				u32 curOffset = 0, cancelled = 0, chunkSize = (isCard||isDestCard) ? curFile.size : (256*1024);
				char *readBuffer = (char*)memalign(32,chunkSize);
				sprintf(txtbuffer, "Copying to: %s",getRelativeName(destFile->name));
				uiDrawObj_t* progBar = DrawProgressBar(false, 0, txtbuffer);
				DrawPublish(progBar);
				
				u64 startTime = gettime();
				u64 lastTime = gettime();
				u32 lastOffset = 0;
				int speed = 0;
				int timeremain = 0;
				while(curOffset < curFile.size) {
					u32 buttons = PAD_ButtonsHeld(0);
					if(buttons & PAD_BUTTON_B) {
						cancelled = 1;
						break;
					}
					u32 timeDiff = diff_msec(lastTime, gettime());
					u32 timeStart = diff_msec(startTime, gettime());
					if(timeDiff >= 1000) {
						speed = (int)((float)(curOffset-lastOffset) / (float)(timeDiff/1000.0f));
						timeremain = (curFile.size - curOffset) / speed;
						lastTime = gettime();
						lastOffset = curOffset;
					}
					DrawUpdateProgressBarDetail(progBar, (int)((float)((float)curOffset/(float)curFile.size)*100), speed, timeStart/1000, timeremain);
					u32 amountToCopy = curOffset + chunkSize > curFile.size ? curFile.size - curOffset : chunkSize;
					ret = devices[DEVICE_CUR]->readFile(&curFile, readBuffer, amountToCopy);
					if(ret != amountToCopy) {	// Retry the read.
						devices[DEVICE_CUR]->seekFile(&curFile, curFile.offset-ret, DEVICE_HANDLER_SEEK_SET);
						ret = devices[DEVICE_CUR]->readFile(&curFile, readBuffer, amountToCopy);
						if(ret != amountToCopy) {
							DrawDispose(progBar);
							free(readBuffer);
							devices[DEVICE_CUR]->closeFile(&curFile);
							devices[DEVICE_DEST]->closeFile(destFile);
							sprintf(txtbuffer, "Failed to Read! (%d %d)\n%s",amountToCopy,ret, &curFile.name[0]);
							uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
							DrawPublish(msgBox);
							wait_press_A();
							setGCIInfo(NULL);
							setCopyGCIMode(FALSE);
							DrawDispose(msgBox);
							return true;
						}
					}
					ret = devices[DEVICE_DEST]->writeFile(destFile, readBuffer, amountToCopy);
					if(ret != amountToCopy) {
						DrawDispose(progBar);
						free(readBuffer);
						devices[DEVICE_CUR]->closeFile(&curFile);
						devices[DEVICE_DEST]->closeFile(destFile);
						sprintf(txtbuffer, "Failed to Write! (%ul %ul)\n%s",amountToCopy,ret,destFile->name);
						uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
						DrawPublish(msgBox);
						wait_press_A();
						setGCIInfo(NULL);
						setCopyGCIMode(FALSE);
						DrawDispose(msgBox);
						return true;
					}
					curOffset+=amountToCopy;
				}
				DrawDispose(progBar);

				// Handle empty files as a special case
				if(curFile.size == 0) {
					ret = devices[DEVICE_DEST]->writeFile(destFile, readBuffer, 0);
					if(ret != 0) {
						free(readBuffer);
						sprintf(txtbuffer, "Failed to Write! (%ul)\n%s",ret,destFile->name);
						uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
						DrawPublish(msgBox);
						wait_press_A();
						setGCIInfo(NULL);
						setCopyGCIMode(FALSE);
						DrawDispose(msgBox);
						return true;
					}
				}
				free(readBuffer);
				devices[DEVICE_CUR]->closeFile(&curFile);
				devices[DEVICE_DEST]->closeFile(destFile);
				setGCIInfo(NULL);
				setCopyGCIMode(FALSE);
				free(destFile);
				uiDrawObj_t *msgBox = NULL;
				if(!cancelled) {
					// If cut, delete from source device
					if(option == MOVE_OPTION) {
						devices[DEVICE_CUR]->deleteFile(&curFile);
						needsRefresh=1;
						msgBox = DrawMessageBox(D_INFO,"Move Complete!");
					}
					else {
						msgBox = DrawMessageBox(D_INFO,"Copy Complete! Press A to continue");
					}
				} 
				else {
					msgBox = DrawMessageBox(D_INFO,"Operation Cancelled! Press A to continue");
				}
				DrawPublish(msgBox);
				wait_press_A();
				DrawDispose(msgBox);
			}
		}
	}
	// Else if directory, mention support not yet implemented.
	else {
		uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,"Directory support not implemented");
		DrawPublish(msgBox);
		sleep(2);
		DrawDispose(msgBox);
	}
	return true;
}

void load_game() {
	if(devices[DEVICE_CUR] == &__device_wode) {
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Setup base offset please Wait .."));
		devices[DEVICE_CUR]->setupFile(&curFile, NULL, 0);
		DrawDispose(msgBox);
	}
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading ..."));
	
	// boot the GCM/ISO file, gamecube disc or multigame selected entry
	devices[DEVICE_CUR]->seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
	if(devices[DEVICE_CUR]->readFile(&curFile,&GCMDisk,sizeof(DiskHeader)) != sizeof(DiskHeader)) {
		DrawDispose(msgBox);
		msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File!"));
		sleep(2);
		DrawDispose(msgBox);
		return;
	}

	if(GCMDisk.DVDMagicWord != DVD_MAGIC) {
		DrawDispose(msgBox);
		msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Disc does not contain valid word at 0x1C"));
		sleep(2);
		DrawDispose(msgBox);
		return;
	}
	
	if(curFile.size < DISC_SIZE && strncmp(&GCMDisk.GameName[0x1E0], "NKIT", 4)) {
		DrawDispose(msgBox);
		msgBox = DrawPublish(DrawMessageBox(D_WARN, "Disc shrunk using an unsupported tool.\nPlease use NKit."));
		sleep(5);
		DrawDispose(msgBox);
	}
	
	// Check that Audio streaming is really necessary before we patch anything for it
	if(GCMDisk.AudioStreaming) {
		print_gecko("Checking game for 32K files\r\n");
		int numAdpFiles = parse_gcm_for_ext(&curFile, NULL, true);
		if(!numAdpFiles) {
			print_gecko("No 32K files detected in FST, disabling AudioStreaming flag\r\n");
			GCMDisk.AudioStreaming = 0;
		}
		else {
			print_gecko("Found %i 32K files\r\n", numAdpFiles);
		}
	}
	
	DrawDispose(msgBox);
	// Show game info or return to the menu
	if(!info_game()) {
		return;
	}
	
	// setup the video mode before we kill libOGC kernel
	ogc_video__reset();
	
	// Auto load cheats if the set to auto load and if any are found
	if(swissSettings.autoCheats) {
		if(findCheats(true) > 0) {
			int appliedCount = applyAllCheats();
			sprintf(txtbuffer, "Applied %i cheats", appliedCount);
			msgBox = DrawPublish(DrawMessageBox(D_INFO, txtbuffer));
			sleep(1);
			DrawDispose(msgBox);
		}
	}
	
	int numToPatch = 0;
	ExecutableFile *filesToPatch = memalign(32, sizeof(ExecutableFile)*512);
	memset(filesToPatch, 0, sizeof(ExecutableFile)*512);

	// Report to the user the patch status of this GCM/ISO file
	if(devices[DEVICE_CUR]->features & FEAT_CAN_READ_PATCHES) {
		numToPatch = check_game(filesToPatch);
	}
	
  	if(devices[DEVICE_CUR] != &__device_wode) {
		file_handle *secondDisc = meta_find_disk2(&curFile);
		*(vu32*)VAR_SECOND_DISC = secondDisc ? 1:0;
		*(vu32*)VAR_CURRENT_DISC = 0;
		*(vu32*)VAR_IGR_DOL_SIZE = 0;
		// Call the special setup for each device (e.g. SD will set the sector(s))
		if(!devices[DEVICE_CUR]->setupFile(&curFile, secondDisc, numToPatch)) {
			msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Failed to setup the file (too fragmented?)"));
			wait_press_A();
			free(filesToPatch);
			DrawDispose(msgBox);
			config_unload_current();
			return;
		}
	}

	load_app(filesToPatch, numToPatch);
	config_unload_current();
}

/* Execute/Load/Parse the currently selected file */
void load_file()
{
	char *fileName = &curFile.name[0];
		
	//if it's a DOL, boot it
	if(strlen(fileName)>4) {
		if(endsWith(fileName,".dol") || endsWith(fileName,".dol+cli")) {
			boot_dol();
			// if it was invalid (overlaps sections, too large, etc..) it'll return
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid DOL"));
			sleep(2);
			DrawDispose(msgBox);
			return;
		}
		if(endsWith(fileName,".mp3")) {
			mp3_player(getCurrentDirEntries(), getCurrentDirEntryCount(), &curFile);
			return;
		}
		if(endsWith(fileName,".fzn")) {
			if(curFile.size != 0x1D0000) {
				uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "File Size must be 0x1D0000 bytes!"));
				sleep(2);
				DrawDispose(msgBox);
				return;
			}
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading Flash File ..."));
			u8 *flash = (u8*)memalign(32,0x1D0000);
			devices[DEVICE_CUR]->seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CUR]->readFile(&curFile,flash,0x1D0000);
			// Try to read a .fw file too.
			file_handle fwFile;
			memset(&fwFile, 0, sizeof(file_handle));
			sprintf(&fwFile.name[0],"%s.fw", &curFile.name[0]);
			u8 *firmware = (u8*)memalign(32, 0x3000);
			DrawDispose(msgBox);
			if(devices[DEVICE_CUR] == &__device_dvd || devices[DEVICE_CUR]->readFile(&fwFile,firmware,0x3000) != 0x3000) {
				free(firmware); firmware = NULL;
				msgBox = DrawPublish(DrawMessageBox(D_WARN, "Didn't find a firmware file, flashing menu only."));
			}
			else {
				msgBox = DrawPublish(DrawMessageBox(D_INFO, "Found firmware file, this will be flashed too."));
			}
			sleep(1);
			DrawDispose(msgBox);
			wkfWriteFlash(flash, firmware);
			msgBox = DrawPublish(DrawMessageBox(D_INFO, "Flashing Complete !!"));
			sleep(2);
			DrawDispose(msgBox);
			return;
		}
		if(endsWith(fileName,".iso") || endsWith(fileName,".gcm")) {
			if(devices[DEVICE_CUR]->features & FEAT_BOOT_GCM)
				load_game();
			else {
				uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "This device does not support booting of images."));
				sleep(2);
				DrawDispose(msgBox);
			}
			return;
		}
		uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "Unknown File Type\nEnable file management to manage this file."));
		sleep(1);
		DrawDispose(msgBox);
		return;			
	}

}

int check_game(ExecutableFile *filesToPatch)
{ 	
	char* gameID = (char*)&GCMDisk;
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Checking Game .."));
	
	u32 numToPatch = parse_gcm(&curFile, filesToPatch);
	
	if(!swissSettings.disableVideoPatches) {
		if(!strncmp(gameID, "GS8P7D", 6)) {
			parse_gcm_add(&curFile, filesToPatch, &numToPatch, "SPYROCFG_NGC.CFG");
		}
	}
	DrawDispose(msgBox);
	return patch_gcm(&curFile, filesToPatch, numToPatch);
}

uiDrawObj_t* draw_game_info() {
	uiDrawObj_t *container = DrawEmptyBox(75,120, getVideoMode()->fbWidth-78, 400);

	sprintf(txtbuffer,"%s",(GCMDisk.DVDMagicWord != DVD_MAGIC)?getRelativeName(&curFile.name[0]):GCMDisk.GameName);
	float scale = GetTextScaleToFitInWidth(txtbuffer,(getVideoMode()->fbWidth-78)-75);
	DrawAddChild(container, DrawStyledLabel(640/2, 130, txtbuffer, scale, true, defaultColor));

	if(devices[DEVICE_CUR] == &__device_qoob) {
		sprintf(txtbuffer,"Size: %.2fKB (%ul blocks)", (float)curFile.size/1024, curFile.size/0x10000);
		DrawAddChild(container, DrawStyledLabel(640/2, 160, txtbuffer, 0.8f, true, defaultColor));
		sprintf(txtbuffer,"Position on Flash: %08X",(u32)(curFile.fileBase&0xFFFFFFFF));
		DrawAddChild(container, DrawStyledLabel(640/2, 180, txtbuffer, 0.8f, true, defaultColor));
	}
	else if(devices[DEVICE_CUR] == &__device_wode) {
		ISOInfo_t* isoInfo = (ISOInfo_t*)&curFile.other;
		sprintf(txtbuffer,"Partition: %i, ISO: %i", isoInfo->iso_partition,isoInfo->iso_number);
		DrawAddChild(container, DrawStyledLabel(640/2, 160, txtbuffer, 0.8f, true, defaultColor));
	}
	else if(devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b) {
		sprintf(txtbuffer,"Size: %.2fKB (%ul blocks)", (float)curFile.size/1024, curFile.size/8192);
		DrawAddChild(container, DrawStyledLabel(640/2, 160, txtbuffer, 0.8f, true, defaultColor));
		sprintf(txtbuffer,"Position on Card: %08X",curFile.offset);
		DrawAddChild(container, DrawStyledLabel(640/2, 180, txtbuffer, 0.8f, true, defaultColor));
	}
	else {
		sprintf(txtbuffer,"Size: %.2f%s", (float)curFile.size > (1024*1024*1024) ? (float)curFile.size/(1024*1024*1024) : (float)curFile.size > (1024*1024) ? (float)curFile.size/(1024*1024) : (float)curFile.size/1024, (float)curFile.size > (1024*1024*1024) ? "GB" : (float)curFile.size > (1024*1024) ? "MB" : "KB");
		DrawAddChild(container, DrawStyledLabel(640/2, 160, txtbuffer, 0.8f, true, defaultColor));
		if(curFile.meta) {
			if(curFile.meta->banner)
				DrawAddChild(container, DrawTexObj(&curFile.meta->bannerTexObj, 215, 240, 192, 64, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
			if(curFile.meta->regionTexId != -1 && curFile.meta->regionTexId != 0)
				DrawAddChild(container, DrawImage(curFile.meta->regionTexId, 450, 262, 30,20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));

			sprintf(txtbuffer, "%s", curFile.meta->bnrDescription.description);
			char* rest = &txtbuffer[0]; 
			char* tok;
			int line = 0;
			while ((tok = strtok_r (rest,"\r\n", &rest))) {
				float scale = GetTextScaleToFitInWidth(tok,(getVideoMode()->fbWidth-78)-75);
				DrawAddChild(container, DrawStyledLabel(640/2, 315+(line*scale*24), tok, scale, true, defaultColor));
				line++;
			}
		}
	}
	if(GCMDisk.DVDMagicWord == DVD_MAGIC) {
		sprintf(txtbuffer, "Game ID: [%.6s] Audio Streaming: [%s]", (char*)&GCMDisk, (GCMDisk.AudioStreaming ? "Yes":"No"));
		DrawAddChild(container, DrawStyledLabel(640/2, 200, txtbuffer, 0.8f, true, defaultColor));

		if(meta_find_disk2(&curFile)) {
			sprintf(txtbuffer, "Disc %i", GCMDisk.DiscID+1);
			DrawAddChild(container, DrawStyledLabel(640/2, 220, txtbuffer, 0.8f, true, defaultColor));
		}
	}
	if(devices[DEVICE_CUR] == &__device_wode) {
		DrawAddChild(container, DrawStyledLabel(640/2, 380, "Settings (X) - Cheats (Y) - Boot (A)", 0.75f, true, defaultColor));
	}
	else {
		DrawAddChild(container, DrawStyledLabel(640/2, 380, "Settings (X) - Cheats (Y) - Exit (B) - Boot (A)", 0.75f, true, defaultColor));
	}
	return container;
}

/* Show info about the game - and also load the config for it */
int info_game()
{
	int ret = -1;
	ConfigEntry *config = NULL;
	// Find the config for this game, or default if we don't know about it
	config = calloc(1, sizeof(ConfigEntry));
	memcpy(config->game_id, &GCMDisk.ConsoleID, 4);
	strncpy(&config->game_name[0],&GCMDisk.GameName[0],32);
	config_find(config);	// populate
	uiDrawObj_t *infoPanel = DrawPublish(draw_game_info());
	while(1) {
		while(PAD_ButtonsHeld(0) & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y)){ VIDEO_WaitVSync (); }
		while(!(PAD_ButtonsHeld(0) & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y))){ VIDEO_WaitVSync (); }
		if(PAD_ButtonsHeld(0) & (PAD_BUTTON_B|PAD_BUTTON_A)){
			ret = (PAD_ButtonsHeld(0) & PAD_BUTTON_A) ? 1:0;
			// WODE can't return from here.
			if(devices[DEVICE_CUR] == &__device_wode && !ret) {
				continue;
			}
			break;
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) {
			show_settings((GCMDisk.DVDMagicWord == DVD_MAGIC) ? &curFile : NULL, config);
		}
		// Look for a cheats file based on the GameID
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_Y) {
			int num_cheats = findCheats(false);
			if(num_cheats != 0) {
				DrawCheatsSelector(getRelativeName(getCurrentDirEntries()[curSelection].name));
			}
		}
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	}
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	DrawDispose(infoPanel);
	// Load config for this game into our current settings
	if(ret) 
		config_load_current(config);
	free(config);
	return ret;
}

void select_device(int type)
{
	u32 requiredFeatures = (type == DEVICE_DEST) ? FEAT_WRITE:FEAT_READ;

	if(is_httpd_in_use()) {
		uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_INFO,"Can't load device while HTTP is processing!"));
		sleep(5);
		DrawDispose(msgBox);
		return;
	}

	int curDevice = 0;
	int inAdvanced = 0, showAllDevices = 0;
	int direction = 0;

	// Find the first device that meets the requiredFeatures and is available
	while((allDevices[curDevice] == NULL) || !(deviceHandler_getDeviceAvailable(allDevices[curDevice])||showAllDevices) || !(allDevices[curDevice]->features & requiredFeatures)) {
		curDevice ++;
		if( (curDevice >= MAX_DEVICES)){
			curDevice = 0;
		}
	}
	
	
	uiDrawObj_t *deviceSelectBox = NULL;
	while(1) {
		// Device selector
		deviceSelectBox = DrawEmptyBox(20,190, getVideoMode()->fbWidth-20, 410);
		uiDrawObj_t *selectLabel = DrawStyledLabel(640/2, 195
													, type == DEVICE_DEST ? "Destination Device" : "Device Selection"
													, 1.0f, true, defaultColor);
		uiDrawObj_t *fwdLabel = DrawLabel(520, 270, "->");
		uiDrawObj_t *backLabel = DrawLabel(100, 270, "<-");
		uiDrawObj_t *showAllLabel = DrawStyledLabel(20, 400, "(Z) Show all devices", 0.65f, false, showAllDevices ? defaultColor:deSelectedColor);
		DrawAddChild(deviceSelectBox, selectLabel);
		DrawAddChild(deviceSelectBox, fwdLabel);
		DrawAddChild(deviceSelectBox, backLabel);
		DrawAddChild(deviceSelectBox, showAllLabel);
		
		if(direction != 0) {
			if(direction > 0) {
				curDevice = allDevices[curDevice+1] == NULL ? 0 : curDevice+1;
			}
			else {
				curDevice = curDevice > 0 ? curDevice-1 : MAX_DEVICES-1;
			}
			// Go to next available device that meets the requiredFeatures
			while((allDevices[curDevice] == NULL) || !(deviceHandler_getDeviceAvailable(allDevices[curDevice])) || !(allDevices[curDevice]->features & requiredFeatures)) {
				if(allDevices[curDevice] != NULL && showAllDevices && (allDevices[curDevice]->features & requiredFeatures)) {
					break;	// Show all devices? then continue
				}
				curDevice += direction;
				if((curDevice < 0) || (curDevice >= MAX_DEVICES)){
					curDevice = direction > 0 ? 0 : MAX_DEVICES-1;
				}
			}
			direction = 0;
		}

		textureImage *devImage = &allDevices[curDevice]->deviceTexture;
		uiDrawObj_t *deviceImage = DrawImage(devImage->textureId, 640/2, 270-(devImage->height/2), devImage->width, devImage->height, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
		uiDrawObj_t *deviceNameLabel = DrawStyledLabel(640/2, 330, (char*)allDevices[curDevice]->deviceName, 0.85f, true, defaultColor);
		uiDrawObj_t *deviceDescLabel = DrawStyledLabel(640/2, 350, (char*)allDevices[curDevice]->deviceDescription, 0.65f, true, defaultColor);
		DrawAddChild(deviceSelectBox, deviceImage);
		DrawAddChild(deviceSelectBox, deviceNameLabel);
		DrawAddChild(deviceSelectBox, deviceDescLabel);
		if(allDevices[curDevice]->features & FEAT_BOOT_GCM) {
			uiDrawObj_t *gameBootLabel = DrawStyledLabel(640/2, 365, "Supports Game Boot", 0.65f, true, defaultColor);
			DrawAddChild(deviceSelectBox, gameBootLabel);
		}
		// Memory card port devices, allow for speed selection
		if(allDevices[curDevice]->location & (LOC_MEMCARD_SLOT_A | LOC_MEMCARD_SLOT_B | LOC_SERIAL_PORT_2)) {
			uiDrawObj_t *exiOptionsLabel = DrawStyledLabel(getVideoMode()->fbWidth-190, 400, "(X) EXI Options", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			DrawAddChild(deviceSelectBox, exiOptionsLabel);
			if(inAdvanced) {
				// Draw speed selection if advanced menu is showing.
				uiDrawObj_t *exiSpeedLabel = DrawStyledLabel(getVideoMode()->fbWidth-160, 385, swissSettings.exiSpeed ? "Speed: 32 MHz":"Speed: 16 MHz", 0.65f, false, defaultColor);
				DrawAddChild(deviceSelectBox, exiSpeedLabel);
			}
		}
		DrawPublish(deviceSelectBox);	
		while (!(PAD_ButtonsHeld(0) & 
			(PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X|PAD_TRIGGER_Z) ))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if((btns & PAD_BUTTON_X) && (allDevices[curDevice]->location & (LOC_MEMCARD_SLOT_A | LOC_MEMCARD_SLOT_B | LOC_SERIAL_PORT_2)))
			inAdvanced ^= 1;
		if(btns & PAD_TRIGGER_Z) {
			showAllDevices ^= 1;
			if(!showAllDevices && !deviceHandler_getDeviceAvailable(allDevices[curDevice])) {
				inAdvanced = 0;
				direction = 1;
			}
		}
		if(inAdvanced) {
			if((btns & PAD_BUTTON_RIGHT) || (btns & PAD_BUTTON_LEFT)) {
				swissSettings.exiSpeed^=1;
			}
		}
		else {
			if(btns & PAD_BUTTON_RIGHT) {
				direction = 1;
			}
			if(btns & PAD_BUTTON_LEFT) {
				direction = -1;
			}
		}
			
		if(btns & PAD_BUTTON_A) {
			if(!inAdvanced) {
				if(type == DEVICE_CUR)
					break;
				else {
					if(!(allDevices[curDevice]->features & FEAT_WRITE)) {
						// TODO don't break cause read only device
					}
					else {
						break;
					}
				}
			}
			else 
				inAdvanced = 0;
		}
		if(btns & PAD_BUTTON_B) {
			devices[type] = NULL;
			DrawDispose(deviceSelectBox);
			return;
		}
		while ((PAD_ButtonsHeld(0) & 
			(PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_B|PAD_BUTTON_A
			|PAD_BUTTON_X|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_TRIGGER_Z) ))
			{ VIDEO_WaitVSync (); }
		DrawDispose(deviceSelectBox);
	}
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	// Deinit any existing device
	if(devices[type] != NULL) devices[type]->deinit( devices[type]->initial );
	DrawDispose(deviceSelectBox);
	devices[type] = allDevices[curDevice];
}

void menu_loop()
{ 
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
	// We don't care if a subsequent device is "default"
	if(needsDeviceChange) {
		freeFiles();
		if(devices[DEVICE_CUR]) {
			devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
		}
		devices[DEVICE_CUR] = NULL;
		needsDeviceChange = 0;
		needsRefresh = 1;
		curMenuLocation = ON_FILLIST;
		select_device(DEVICE_CUR);
		if(devices[DEVICE_CUR] != NULL) {
			memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Setting up device"));
			// If the user selected a device, make sure it's ready before we browse the filesystem
			if(!devices[DEVICE_CUR]->init( devices[DEVICE_CUR]->initial )) {
				needsDeviceChange = 1;
				deviceHandler_setDeviceAvailable(devices[DEVICE_CUR], false);
				DrawDispose(msgBox);
				return;
			}
			DrawDispose(msgBox);
			deviceHandler_setDeviceAvailable(devices[DEVICE_CUR], true);	
		}
		else {
			curMenuLocation=ON_OPTIONS;
		}
	}

	uiDrawObj_t *filePanel = NULL;
	while(1) {
		DrawUpdateMenuButtons((curMenuLocation==ON_OPTIONS)?curMenuSelection:-1);
		if(devices[DEVICE_CUR] != NULL && needsRefresh) {
			curMenuLocation=ON_OPTIONS;
			curSelection=0; curMenuSelection=0;
			scanFiles();
			if(getCurrentDirEntryCount()<1) { devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial); needsDeviceChange=1; break;}
			needsRefresh = 0;
			curMenuLocation=ON_FILLIST;
			DrawUpdateMenuButtons((curMenuLocation==ON_OPTIONS)?curMenuSelection:-1);
		}
		if(devices[DEVICE_CUR] != NULL && curMenuLocation==ON_FILLIST) {
			file_handle* curDirFiles = getCurrentDirEntries();
			if(swissSettings.fileBrowserType == BROWSER_CAROUSEL) {
				filePanel = renderFileCarousel(&curDirFiles, getCurrentDirEntryCount(), filePanel);
			}
			else {
				filePanel = renderFileBrowser(&curDirFiles, getCurrentDirEntryCount(), filePanel);
			}
			while(PAD_ButtonsHeld(0) & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT)) {
				VIDEO_WaitVSync (); 
			}
		}
		else if (curMenuLocation==ON_OPTIONS) {
			u16 btns = PAD_ButtonsHeld(0);
			while (!((btns=PAD_ButtonsHeld(0)) & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT))) {
				VIDEO_WaitVSync();
			}
			
			if(btns & PAD_BUTTON_LEFT){	curMenuSelection = (--curMenuSelection < 0) ? (MENU_MAX-1) : curMenuSelection;}
			else if(btns & PAD_BUTTON_RIGHT){curMenuSelection = (curMenuSelection + 1) % MENU_MAX;	}

			if(btns & PAD_BUTTON_A) {
				//handle menu event
				switch(curMenuSelection) {
					case 0:		// Device change
						needsDeviceChange = 1;  //Change from SD->DVD or vice versa
						break;
					case 1:		// Settings
						show_settings(NULL, NULL);
						break;
					case 2:		// Credits
						show_info();
						break;
					case 3:
						if(devices[DEVICE_CUR] != NULL) {
							memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
							if(devices[DEVICE_CUR] == &__device_wkf) { 
								wkfReinit(); devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
							}
						}
						needsRefresh=1;
						break;
					case 4:
						SYS_ResetSystem(SYS_HOTRESET, 0, TRUE);
						break;
				}
			}
			if((btns & PAD_BUTTON_B) && devices[DEVICE_CUR] != NULL) {
				curMenuLocation = ON_FILLIST;
			}
			while(PAD_ButtonsHeld(0) & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT)) {
				VIDEO_WaitVSync (); 
			}
		}
		if(needsDeviceChange) {
			break;
		}
	}
	if(filePanel != NULL) {
		DrawDispose(filePanel);
	}
	DrawUpdateMenuButtons((curMenuLocation==ON_OPTIONS)?curMenuSelection:-1);
}
