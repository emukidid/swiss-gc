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
#include <zlib.h>

#include "swiss.h"
#include "main.h"
#include "util.h"
#include "info.h"
#include "httpd.h"
#include "exi.h"
#include "patcher.h"
#include "dvd.h"
#include "elf.h"
#include "gameid.h"
#include "gcm.h"
#include "mp3.h"
#include "nkit.h"
#include "wkf.h"
#include "cheats.h"
#include "settings.h"
#include "aram/sidestep.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "devices/filemeta.h"
#include "xxhash/xxhash.h"
#include "dolparameters.h"
#include "../../reservedarea.h"

DiskHeader GCMDisk;      //Gamecube Disc Header struct
TGCHeader tgcFile;
char IPLInfo[256] __attribute__((aligned(32)));
GXRModeObj *newmode = NULL;
char txtbuffer[2048];           //temporary text buffer
file_handle curFile;    //filedescriptor for current file
file_handle curDir;     //filedescriptor for current directory

// Menu related variables
int curMenuLocation = ON_FILLIST; //where are we on the screen?
int curMenuSelection = 0;	      //menu selection
int curSelection = 0;		      //entry selection
int needsDeviceChange = 0;
int needsRefresh = 0;
int current_view_start = 0;
int current_view_end = 0;

char *DiscIDNoNTSC[] = {"DLSP64", "G3FD69", "G3FF69", "G3FP69", "G3FS69", "GFZP01", "GLRD64", "GLRF64", "GLRP64", "GM8P01", "GMSP01", "GSWD64", "GSWF64", "GSWI64", "GSWP64", "GSWS64"};

/* re-init video for a given game */
void ogc_video__reset()
{
	char* gameID = (char*)&GCMDisk;
	char region = wodeRegionToChar(GCMDisk.RegionCode);
	int i;
	
	swissSettings.fontEncode = region == 'J';
	
	if(region == 'P')
		swissSettings.sramVideo = SYS_VIDEO_PAL;
	else if(swissSettings.sramVideo == SYS_VIDEO_PAL)
		swissSettings.sramVideo = SYS_VIDEO_NTSC;
	
	if(swissSettings.gameVMode > 0 && swissSettings.disableVideoPatches < 2) {
		swissSettings.sram60Hz = (swissSettings.gameVMode >= 1 && swissSettings.gameVMode <= 7);
		swissSettings.sramProgressive = (swissSettings.gameVMode >= 4 && swissSettings.gameVMode <= 7) ||
										(swissSettings.gameVMode >= 11 && swissSettings.gameVMode <= 14);
		
		if(swissSettings.sram60Hz) {
			for(i = 0; i < sizeof(DiscIDNoNTSC)/sizeof(char*); i++) {
				if(!strncmp(gameID, DiscIDNoNTSC[i], 6)) {
					swissSettings.gameVMode += 7;
					break;
				}
			}
		}
		if(swissSettings.sramProgressive && !getDTVStatus())
			swissSettings.gameVMode = 0;
	} else {
		swissSettings.sram60Hz = getTVFormat() != VI_PAL;
		swissSettings.sramProgressive = getScanMode() == VI_PROGRESSIVE;
		
		if(swissSettings.sramProgressive) {
			if(swissSettings.sramVideo == SYS_VIDEO_PAL)
				swissSettings.gameVMode = -2;
			else
				swissSettings.gameVMode = -1;
		} else
			swissSettings.gameVMode = 0;
	}
	
	if(!strncmp(gameID, "GB3E51", 6)
		|| (!strncmp(gameID, "G2OE41", 6) && swissSettings.sramLanguage == SYS_LANG_SPANISH)
		|| !strncmp(gameID, "GMXP70", 6))
		swissSettings.sramProgressive = 0;
	
	syssram* sram = __SYS_LockSram();
	sram->ntd = swissSettings.sram60Hz ? (sram->ntd|0x40):(sram->ntd&~0x40);
	sram->flags = swissSettings.sramProgressive ? (sram->flags|0x80):(sram->flags&~0x80);
	__SYS_UnlockSram(1);
	while(!__SYS_SyncSram());
	
	/* set TV mode for current game */
	uiDrawObj_t *msgBox = NULL;
	switch(swissSettings.gameVMode) {
		case -2:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 576p");
			newmode = &TVPal576ProgScale;
			break;
		case -1:
			msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 480p");
			newmode = &TVNtsc480Prog;
			break;
		case 0:
			if(swissSettings.sramVideo == SYS_VIDEO_MPAL && !getDTVStatus()) {
				msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL-M 480i");
				newmode = &TVMpal480IntDf;
			}
			else if(swissSettings.sramVideo == SYS_VIDEO_PAL) {
				if(swissSettings.sramProgressive && !getDTVStatus())
					msgBox = DrawMessageBox(D_WARN, "Video Mode: PAL 576i\nComponent Cable not detected.");
				else
					msgBox = DrawMessageBox(D_INFO, "Video Mode: PAL 576i");
				
				newmode = &TVPal576IntDfScale;
			}
			else {
				if(swissSettings.sramProgressive && !getDTVStatus())
					msgBox = DrawMessageBox(D_WARN, "Video Mode: NTSC 480i\nComponent Cable not detected.");
				else
					msgBox = DrawMessageBox(D_INFO, "Video Mode: NTSC 480i");
				
				newmode = &TVNtsc480IntDf;
			}
			break;
		case 1 ... 3:
			sprintf(txtbuffer, "Video Mode: %s %s", "NTSC", gameVModeStr[swissSettings.gameVMode]);
			msgBox = DrawMessageBox(D_INFO, txtbuffer);
			newmode = &TVNtsc480IntDf;
			break;
		case 4 ... 7:
			sprintf(txtbuffer, "Video Mode: %s %s", "NTSC", gameVModeStr[swissSettings.gameVMode]);
			msgBox = DrawMessageBox(D_INFO, txtbuffer);
			newmode = &TVNtsc480Prog;
			break;
		case 8 ... 10:
			sprintf(txtbuffer, "Video Mode: %s %s\n%s Mode selected.", "PAL", gameVModeStr[swissSettings.gameVMode], swissSettings.sram60Hz ? "60Hz":"50Hz");
			msgBox = DrawMessageBox(D_INFO, txtbuffer);
			newmode = &TVPal576IntDfScale;
			break;
		case 11 ... 14:
			sprintf(txtbuffer, "Video Mode: %s %s\n%s Mode selected.", "PAL", gameVModeStr[swissSettings.gameVMode], swissSettings.sram60Hz ? "60Hz":"50Hz");
			msgBox = DrawMessageBox(D_INFO, txtbuffer);
			newmode = &TVPal576ProgScale;
			break;
	}
	if((newmode != NULL) && (newmode != getVideoMode())) {
		DrawVideoMode(newmode);
	}
	if(msgBox != NULL) {
		DrawPublish(msgBox);
		sleep(2);
		DrawDispose(msgBox);
	}
}

void drawCurrentDevice(uiDrawObj_t *containerPanel) {
	uiDrawObj_t *bgBox = DrawTransparentBox(30, 100, 135, 200);	// Device icon + slot box
	DrawAddChild(containerPanel, bgBox);
	// Draw the device image
	float scale = 1.0f;
	if (devices[DEVICE_CUR]->deviceTexture.width > devices[DEVICE_CUR]->deviceTexture.height) {
		scale = MIN(1.0f, 104.0f / devices[DEVICE_CUR]->deviceTexture.width);
	} else {
		scale = MIN(1.0f, 84.0f / devices[DEVICE_CUR]->deviceTexture.height);
	}
	int scaledWidth = devices[DEVICE_CUR]->deviceTexture.realWidth*scale;
	int scaledHeight = devices[DEVICE_CUR]->deviceTexture.realHeight*scale;
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
	uiDrawObj_t *devLocationLabel = DrawStyledLabel(30 + ((135-30) / 2), 195, txtbuffer, 0.65f, true, defaultColor);
	DrawAddChild(containerPanel, devLocationLabel);
	
	device_info *info = devices[DEVICE_CUR]->info(devices[DEVICE_CUR]->initial);
	if(info == NULL) return;
	
	uiDrawObj_t *devInfoBox = DrawTransparentBox(30, 225, 135, 330);	// Device size/extra info box
	DrawAddChild(containerPanel, devInfoBox);
	
	// Total space
	uiDrawObj_t *devTotalLabel = DrawStyledLabel(83, 233, "Total:", 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devTotalLabel);
	formatBytes(txtbuffer, info->totalSpace, 0, info->metric);
	uiDrawObj_t *devTotalSizeLabel = DrawStyledLabel(83, 248, txtbuffer, 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devTotalSizeLabel);
	
	// Free space
	uiDrawObj_t *devFreeLabel = DrawStyledLabel(83, 268, "Free:", 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devFreeLabel);
	formatBytes(txtbuffer, info->freeSpace, 0, info->metric);
	uiDrawObj_t *devFreeSizeLabel = DrawStyledLabel(83, 283, txtbuffer, 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devFreeSizeLabel);
	
	// Used space
	uiDrawObj_t *devUsedLabel = DrawStyledLabel(83, 303, "Used:", 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devUsedLabel);
	formatBytes(txtbuffer, info->totalSpace - info->freeSpace, 0, info->metric);
	uiDrawObj_t *devUsedSizeLabel = DrawStyledLabel(83, 318, txtbuffer, 0.6f, true, defaultColor);
	DrawAddChild(containerPanel, devUsedSizeLabel);
}

void select_recent_entry() {
	if(swissSettings.recent[0][0] == 0) return;	// don't draw empty.
	int i = 0, idx = 0, max = RECENT_MAX;
	int rh = 22; // row height
	int fileListBase = 175;
	uiDrawObj_t *container = NULL;
	while(1) {
		uiDrawObj_t *newPanel = DrawEmptyBox(30,fileListBase-30, getVideoMode()->fbWidth-30, 380);
		DrawAddChild(newPanel, DrawLabel(45, fileListBase-30, "Recent:"));
		for(i = 0; i < RECENT_MAX; i++) {
			if(swissSettings.recent[i][0] == 0) {
				max = i;
				break;
			}
			DrawAddChild(newPanel, DrawSelectableButton(45,fileListBase+(i*(rh+2)), getVideoMode()->fbWidth-45, fileListBase+(i*(rh+2))+rh, getRelativeName(&swissSettings.recent[i][0]), (i == idx) ? B_SELECTED:B_NOSELECT));
		}
		if(container) {
			DrawDispose(container);
		}
		DrawPublish(newPanel);
		container = newPanel;
		while ((PAD_StickY(0) > -16 && PAD_StickY(0) < 16) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN))
			{ VIDEO_WaitVSync (); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_UP) || PAD_StickY(0) > 16){	idx = (--idx < 0) ? max-1 : idx;}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || PAD_StickY(0) < -16) {idx = (idx + 1) % max;	}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A))	break;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_B))	{ idx = -1; break; }
		if(PAD_StickY(0) < -16 || PAD_StickY(0) > 16) {
			usleep(50000 - abs(PAD_StickY(0)*16));
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)))
			{ VIDEO_WaitVSync (); }
	}
	do {VIDEO_WaitVSync();} while (PAD_ButtonsHeld(0) & PAD_BUTTON_B);
	DrawDispose(container);
	if(idx >= 0) {
		int res = load_existing_entry(&swissSettings.recent[idx][0]);
		if(res) {
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,res == RECENT_ERR_ENT_MISSING ? "Recent entry not found.\nPress A to continue." 
																					:	"Recent device not found.\nPress A to continue.");
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
		}
	}
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
		if(!strcmp(&swissSettings.autoload[0], &curFile.name[0])) {
			DrawAddChild(containerPanel, DrawImage(TEX_STAR, ((getVideoMode()->fbWidth-30)-16), 80, 16, 16, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		}
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

// Modify entry to go up a directory.
int upToParent(file_handle* entry)
{
	int len = strlen(entry->name);
	
	// Go up a folder
	while(len && entry->name[len-1]!='/')
		len--;
	if(len && len != strlen(entry->name))
		entry->name[len-1] = '\0';
	else
		return 1;

	// If we're a file, go up to the parent of the file
	if(entry->fileAttrib == IS_FILE) {
		while(len && entry->name[len-1]!='/')
			len--;
		if(len && len != strlen(entry->name))
			entry->name[len-1] = '\0';
		else
			return 1;
	}

	return 0;
}

uiDrawObj_t* loadingBox = NULL;
uiDrawObj_t* renderFileBrowser(file_handle** directory, int num_files, uiDrawObj_t* filePanel)
{
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) {
		memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
		needsRefresh=1;
		return filePanel;
	}
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
		
		u32 waitButtons = PAD_BUTTON_X|PAD_BUTTON_START|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_TRIGGER_Z;
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
		
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_A) {
			//go into a folder or select a file
			if((*directory)[curSelection].fileAttrib==IS_DIR) {
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_SPECIAL) {
				needsDeviceChange = upToParent(&curFile);
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_FILE) {
				memcpy(&curDir, &curFile, sizeof(file_handle));
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				if(canLoadFileType(&curFile.name[0])) {
					load_file();
				}
				else if(swissSettings.enableFileManagement) {
					needsRefresh = manage_file() ? 1:0;
				}
				memcpy(&curFile, &curDir, sizeof(file_handle));
			}
			return filePanel;
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) {
			needsDeviceChange = upToParent(&curFile);
			needsRefresh=1;
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_X) VIDEO_WaitVSync();
			return filePanel;
		}
		if((PAD_ButtonsHeld(0) & PAD_TRIGGER_Z) && swissSettings.enableFileManagement) {
			if((*directory)[curSelection].fileAttrib == IS_FILE || (*directory)[curSelection].fileAttrib == IS_DIR) {
				memcpy(&curDir, &curFile, sizeof(file_handle));
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh = manage_file() ? 1:0;
				memcpy(&curFile, &curDir, sizeof(file_handle));
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_B) VIDEO_WaitVSync();
				if(needsRefresh) {
					// If we return from doing something with a file, refresh the device in the same dir we were at
					return filePanel;
				}
			}
			else if((*directory)[curSelection].fileAttrib == IS_SPECIAL) {
				// Toggle autoload
				if(!strcmp(&swissSettings.autoload[0], &curFile.name[0])) {
					memset(&swissSettings.autoload[0], 0, PATHNAME_MAX);
				}
				else {
					strcpy(&swissSettings.autoload[0], &curFile.name[0]);
				}
				// Save config
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving autoload ..."));
				config_update_global(true);
				DrawDispose(msgBox);
			}
		}
		
		if((swissSettings.recentListLevel != 2) && (PAD_ButtonsHeld(0) & PAD_BUTTON_START)) {
			select_recent_entry();
			return filePanel;
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
			curMenuLocation = ON_OPTIONS;
			DrawUpdateFileBrowserButton((*directory)[curSelection].uiObj, (curMenuLocation == ON_FILLIST) ? B_SELECTED:B_NOSELECT);
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
	uiDrawObj_t *bgBox = DrawTransparentBox(30, 395, getVideoMode()->fbWidth-30, 420);
	DrawAddChild(containerPanel, bgBox);
	// Device name
	uiDrawObj_t *devNameLabel = DrawStyledLabel(50, 400, devices[DEVICE_CUR]->deviceName, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devNameLabel);
	
	device_info *info = devices[DEVICE_CUR]->info(devices[DEVICE_CUR]->initial);
	if(info == NULL) return;
	
	// Info labels
	char *textPtr = txtbuffer;
	textPtr = stpcpy(textPtr, "Total: ");
	textPtr += formatBytes(textPtr, info->totalSpace, 0, info->metric);
	textPtr = stpcpy(textPtr, " | Free: ");
	textPtr += formatBytes(textPtr, info->freeSpace, 0, info->metric);
	textPtr = stpcpy(textPtr, " | Used: ");
	textPtr += formatBytes(textPtr, info->totalSpace - info->freeSpace, 0, info->metric);
	
	int starting_pos = getVideoMode()->fbWidth-50-GetTextSizeInPixels(txtbuffer)*0.5;
	uiDrawObj_t *devInfoLabel = DrawStyledLabel(starting_pos, 400, txtbuffer, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devInfoLabel);
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
		if(!strcmp(&swissSettings.autoload[0], &curFile.name[0])) {
			DrawAddChild(containerPanel, DrawImage(TEX_STAR, ((getVideoMode()->fbWidth-30)-16), 85, 16, 16, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		}
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
	if(num_files<=0) {
		memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
		needsRefresh=1;
		return filePanel;
	}
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
		
		u32 waitButtons = PAD_BUTTON_X|PAD_BUTTON_START|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_TRIGGER_Z;
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
				needsDeviceChange = upToParent(&curFile);
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_FILE){
				memcpy(&curDir, &curFile, sizeof(file_handle));
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				if(canLoadFileType(&curFile.name[0])) {
					load_file();
				}
				else if(swissSettings.enableFileManagement) {
					needsRefresh = manage_file() ? 1:0;
				}
				memcpy(&curFile, &curDir, sizeof(file_handle));
			}
			return filePanel;
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) {
			needsDeviceChange = upToParent(&curFile);
			needsRefresh=1;
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_X) VIDEO_WaitVSync();
			return filePanel;
		}
		if((PAD_ButtonsHeld(0) & PAD_TRIGGER_Z) && swissSettings.enableFileManagement) {
			if((*directory)[curSelection].fileAttrib == IS_FILE || (*directory)[curSelection].fileAttrib == IS_DIR) {
				memcpy(&curDir, &curFile, sizeof(file_handle));
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh = manage_file() ? 1:0;
				memcpy(&curFile, &curDir, sizeof(file_handle));
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_B) VIDEO_WaitVSync();
				if(needsRefresh) {
					// If we return from doing something with a file, refresh the device in the same dir we were at
					return filePanel;
				}
			}
			else if((*directory)[curSelection].fileAttrib == IS_SPECIAL) {
				// Toggle autoload
				if(!strcmp(&swissSettings.autoload[0], &curFile.name[0])) {
					memset(&swissSettings.autoload[0], 0, PATHNAME_MAX);
				}
				else {
					strcpy(&swissSettings.autoload[0], &curFile.name[0]);
				}
				// Save config
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving autoload ..."));
				config_update_global(true);
				DrawDispose(msgBox);
			}
		}
		
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
			curMenuLocation = ON_OPTIONS;
			DrawUpdateFileBrowserButton((*directory)[curSelection].uiObj, (curMenuLocation == ON_FILLIST) ? B_SELECTED:B_NOSELECT);
			return filePanel;
		}
		if((swissSettings.recentListLevel != 2) && (PAD_ButtonsHeld(0) & PAD_BUTTON_START)) {
			select_recent_entry();
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

bool select_dest_dir(file_handle* directory, file_handle* selection)
{
	file_handle *directories = NULL;
	file_handle curDir;
	memcpy(&curDir, directory, sizeof(file_entry));
	int i = 0, j = 0, max = 0, refresh = 1, num_files =0, idx = 0;
	
	bool cancelled = false;
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
		while ((PAD_StickY(0) > -16 && PAD_StickY(0) < 16) && !(PAD_ButtonsHeld(0) & (PAD_BUTTON_X|PAD_BUTTON_A|PAD_BUTTON_B|PAD_BUTTON_UP|PAD_BUTTON_DOWN)))
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
				upToParent(&curDir);
				refresh=1;
			}
		}
		if(PAD_StickY(0) < -16 || PAD_StickY(0) > 16) {
			usleep(50000 - abs(PAD_StickY(0)*256));
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_X)	{
			memcpy(selection, &curDir, sizeof(file_handle));
			break;
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B)	{
			cancelled = true;
			break;
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & (PAD_BUTTON_X|PAD_BUTTON_A|PAD_BUTTON_B|PAD_BUTTON_UP|PAD_BUTTON_DOWN))))
			{ VIDEO_WaitVSync (); }
	}
	DrawDispose(destDirBox);
	free(directories);
	return cancelled;
}

// Alt DOL sorting/selecting code
int exccomp(const void *a1, const void *b1)
{
	const ExecutableFile* a = a1;
	const ExecutableFile* b = b1;
	
	if(!a && b) return 1;
	if(a && !b) return -1;
	if(!a && !b) return 0;
	
	if((a->type == PATCH_DOL || a->type == PATCH_ELF) && (b->type != PATCH_DOL && b->type != PATCH_ELF))
		return -1;
	if((a->type != PATCH_DOL && a->type != PATCH_ELF) && (b->type == PATCH_DOL || b->type == PATCH_ELF))
		return 1;
	
	if(a->offset == GCMDisk.DOLOffset && b->offset != GCMDisk.DOLOffset)
		return -1;
	if(a->offset != GCMDisk.DOLOffset && b->offset == GCMDisk.DOLOffset)
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
ExecutableFile* select_alt_dol(ExecutableFile *filesToPatch, int num_files) {
	if(swissSettings.autoBoot) return NULL;
	int i = 0, j = 0, max = 0, idx = 0, page = 4;
	sortDols(filesToPatch, num_files);	// Sort DOL to the top
	for(i = 0; i < num_files; i++) {
		if(filesToPatch[i].type != PATCH_DOL && filesToPatch[i].type != PATCH_ELF) {
			num_files = i;
			break;
		}
	}
	if(num_files < 2) return NULL;
	
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

void load_app(ExecutableFile *fileToPatch)
{
	uiDrawObj_t* progBox = NULL;
	char* gameID = (char*)0x80000000;
	void* buffer;
	u32 sizeToRead;
	int type;
	
	// Clear OSLoMem
	asm volatile("mtdabr %0" :: "r" (0));
	memset((void*)0x80000000,0,0x100);
	memset((void*)0x80003000,0,0x100);
	
	// Get top of memory
	u32 topAddr = getTopAddr();
	print_gecko("Top of RAM simulated as: 0x%08X\r\n", topAddr);
	
	*(volatile u32*)0x80000028 = 0x01800000;
	*(volatile u32*)0x8000002C = (swissSettings.debugUSB ? 0x10000004:0x00000001) + (*(volatile u32*)0xCC00302C >> 28);
	*(volatile u32*)0x800000CC = swissSettings.sramVideo;
	*(volatile u32*)0x800000D0 = 0x01000000;
	*(volatile u32*)0x800000E8 = 0x81800000 - topAddr;
	*(volatile u32*)0x800000EC = topAddr;
	if (topAddr != 0x81800000) asm volatile("mtdabr %0" :: "r" (0x800000E8 | 0b110));
	*(volatile u32*)0x800000F0 = 0x01800000;
	*(volatile u32*)0x800000F8 = TB_BUS_CLOCK;
	*(volatile u32*)0x800000FC = TB_CORE_CLOCK;
	
	// Copy the game header to 0x80000000
	memcpy(gameID,(char*)&GCMDisk,0x20);
	
	if(fileToPatch != NULL) {
		// For a DOL from a TGC, redirect the FST to the TGC FST.
		if(fileToPatch->tgcFstOffset != 0) {
			// Read FST to top of Main Memory (round to 32 byte boundary)
			u32 fstAddr = (topAddr-fileToPatch->tgcFstSize)&~31;
			devices[DEVICE_CUR]->seekFile(fileToPatch->file,fileToPatch->tgcFstOffset,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_CUR]->readFile(fileToPatch->file,(void*)fstAddr,fileToPatch->tgcFstSize) != fileToPatch->tgcFstSize) {
				DrawPublish(DrawMessageBox(D_FAIL, "Failed to read fst.bin"));
				while(1);
			}
			adjust_tgc_fst((void*)fstAddr, fileToPatch->tgcBase, fileToPatch->tgcFileStartArea, fileToPatch->tgcFakeOffset);
			
			// Copy bi2.bin (Disk Header Information) to just under the FST
			u32 bi2Addr = (fstAddr-0x2000)&~31;
			memcpy((void*)bi2Addr,(void*)&GCMDisk+0x440,0x2000);
			
			// Patch bi2.bin
			Patch_GameSpecificFile((void*)bi2Addr, 0x2000, gameID, "bi2.bin");
			
			*(volatile u32*)0x80000020 = 0x0D15EA5E;
			*(volatile u32*)0x80000024 = 1;
			*(volatile u32*)0x80000034 = fstAddr;								// Arena Hi
			*(volatile u32*)0x80000038 = fstAddr;								// FST Location in ram
			*(volatile u32*)0x8000003C = fileToPatch->tgcFstSize;				// FST Max Length
			*(volatile u32*)0x800000F4 = bi2Addr;								// bi2.bin location
			*(volatile u32*)0x800030F4 = fileToPatch->tgcBase;
		}
		else {
			// Read FST to top of Main Memory (round to 32 byte boundary)
			u32 fstAddr = (topAddr-GCMDisk.MaxFSTSize)&~31;
			devices[DEVICE_CUR]->seekFile(&curFile,GCMDisk.FSTOffset,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_CUR]->readFile(&curFile,(void*)fstAddr,GCMDisk.FSTSize) != GCMDisk.FSTSize) {
				DrawPublish(DrawMessageBox(D_FAIL, "Failed to read fst.bin"));
				while(1);
			}
			
			// Copy bi2.bin (Disk Header Information) to just under the FST
			u32 bi2Addr = (fstAddr-0x2000)&~31;
			memcpy((void*)bi2Addr,(void*)&GCMDisk+0x440,0x2000);
			
			// Patch bi2.bin
			Patch_GameSpecificFile((void*)bi2Addr, 0x2000, gameID, "bi2.bin");
			
			*(volatile u32*)0x80000020 = 0x0D15EA5E;
			*(volatile u32*)0x80000024 = 1;
			*(volatile u32*)0x80000034 = fstAddr;								// Arena Hi
			*(volatile u32*)0x80000038 = fstAddr;								// FST Location in ram
			*(volatile u32*)0x8000003C = GCMDisk.MaxFSTSize;					// FST Max Length
			*(volatile u32*)0x800000F4 = bi2Addr;								// bi2.bin location
		}
		
		if(devices[DEVICE_PATCHES] && devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
			sprintf(txtbuffer, "Loading DOL\nDo not remove %s", devices[DEVICE_PATCHES]->deviceName);
			progBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));
		}
		else {
			progBox = DrawPublish(DrawProgressBar(true, 0, "Loading DOL"));
		}
		
		print_gecko("DOL Lives at %08X\r\n", fileToPatch->offset);
		sizeToRead = (fileToPatch->size + 31) & ~31;
		type = fileToPatch->type;
		print_gecko("DOL size %i\r\n", sizeToRead);
		
		buffer = memalign(32, sizeToRead);
		print_gecko("DOL buffer %08X\r\n", (u32)buffer);
		if(buffer == NULL) return;
		
		if(fileToPatch->patchFile != NULL) {
			devices[DEVICE_PATCHES]->seekFile(fileToPatch->patchFile,0,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_PATCHES]->readFile(fileToPatch->patchFile,buffer,sizeToRead) != sizeToRead) {
				DrawPublish(DrawMessageBox(D_FAIL, "Failed to read DOL"));
				while(1);
			}
		}
		else {
			devices[DEVICE_CUR]->seekFile(fileToPatch->file,fileToPatch->offset,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_CUR]->readFile(fileToPatch->file,buffer,sizeToRead) != sizeToRead) {
				DrawPublish(DrawMessageBox(D_FAIL, "Failed to read DOL"));
				while(1);
			}
			fileToPatch->hash = XXH3_64bits(buffer, sizeToRead);
			gameID_set(&GCMDisk, fileToPatch->hash);
		}
	}
	else {
		if(devices[DEVICE_PATCHES] && devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
			sprintf(txtbuffer, "Loading BS2\nDo not remove %s", devices[DEVICE_PATCHES]->deviceName);
			progBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));

			if(!load_rom_ipl(devices[DEVICE_PATCHES], &buffer, &sizeToRead) &&
				!load_rom_ipl(devices[DEVICE_CUR], &buffer, &sizeToRead) &&
				!load_rom_ipl(&__device_sys, &buffer, &sizeToRead)) {
				return;
			}
		}
		else {
			progBox = DrawPublish(DrawProgressBar(true, 0, "Loading BS2"));

			if(!load_rom_ipl(devices[DEVICE_CUR], &buffer, &sizeToRead) &&
				!load_rom_ipl(&__device_sys, &buffer, &sizeToRead)) {
				return;
			}
		}
		type = PATCH_BS2;
	}
	
	setTopAddr((u32)VAR_PATCHES_BASE);
	
	if(fileToPatch == NULL || fileToPatch->patchFile == NULL) {
		Patch_ExecutableFile(&buffer, &sizeToRead, gameID, type);
	}
	
	DCFlushRange(buffer, sizeToRead);
	ICInvalidateRange(buffer, sizeToRead);
	
	// See if the combination of our patches has exhausted our play area.
	if(!install_code(0)) {
		DrawDispose(progBox);
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "Exhausted reserved memory.\nAn SD Card Adapter is necessary in order\nfor patches to reserve additional memory.");
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return;
	}
	
	// Don't spin down the drive when running something from it...
	if(devices[DEVICE_CUR] != &__device_dvd) {
		devices[DEVICE_CUR]->deinit(&curFile);
	}
	if(devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR) {
		// Check DVD Status, make sure it's error code 0
		print_gecko("DVD: %08X\r\n",dvd_get_error());
	}
	
	DrawDispose(progBox);
	DrawShutdown();
	
	VIDEO_SetPostRetraceCallback(NULL);
	VIDEO_SetBlack(true);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	
	DCFlushRange((void*)0x80000000, 0x3100);
	ICInvalidateRange((void*)0x80000000, 0x3100);
	
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
	if(type == PATCH_BS2) {
		BINtoARAM(buffer, sizeToRead, 0x81300000);
	}
	else if(type == PATCH_DOL) {
		DOLtoARAM(buffer, 0, NULL);
	}
	else if(type == PATCH_ELF) {
		ELFtoARAM(buffer, 0, NULL);
	}
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
	gameID_set(NULL, XXH3_64bits(dol_buffer, curFile.size));
	DrawDispose(progBar);
	
	if(devices[DEVICE_CONFIG] != NULL) {
		// Update the recent list.
		if(update_recent()) {
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving recent list ..."));
			config_update_recent(true);
			DrawDispose(msgBox);
		}
	}
	
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
	snprintf(cliArgFile->name, PATHNAME_MAX, "%s.cli", fileName);
	if(devices[DEVICE_CUR]->readFile(cliArgFile, &readTest, 4) != 4) {
		free(cliArgFile);
		cliArgFile = NULL;
	}
	
	// we found something, use parameters (.cli)
	if(cliArgFile) {
		print_gecko("Argument file found [%s]\r\n", cliArgFile->name);
		char *cli_buffer = calloc(1, cliArgFile->size + 1);
		if(cli_buffer) {
			devices[DEVICE_CUR]->seekFile(cliArgFile, 0, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CUR]->readFile(cliArgFile, cli_buffer, cliArgFile->size);

			// Parse CLI
			argv[argc] = getExternalPath(&curFile.name[0]);
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
	snprintf(dcpArgFile->name, PATHNAME_MAX, "%s.dcp", fileName);
	if(devices[DEVICE_CUR]->readFile(dcpArgFile, &readTest, 4) != 4) {
		free(dcpArgFile);
		dcpArgFile = NULL;
	}
	
	// we found something, parse and display parameters for selection (.dcp)
	if(dcpArgFile) {
		print_gecko("Argument file found [%s]\r\n", dcpArgFile->name);
		char *dcp_buffer = calloc(1, dcpArgFile->size + 1);
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
	if(!memcmp(dol_buffer, ELFMAG, SELFMAG)) {
		ELFtoARAM(dol_buffer, argc, argc == 0 ? NULL : argv);
	}
	else {
		DOLtoARAM(dol_buffer, argc, argc == 0 ? NULL : argv);
	}

	free(dol_buffer);
}

/* Manage file  - The user will be asked what they want to do with the currently selected file - copy/move/delete*/
bool manage_file() {
	bool isFile = curFile.fileAttrib == IS_FILE;
	bool canWrite = devices[DEVICE_CUR]->features & FEAT_WRITE;
	bool canMove = canWrite && isFile;
	bool canCopy = isFile;
	bool canDelete = canWrite;
	bool canRename = canWrite;
	
	// Ask the user what they want to do with the selected entry
	uiDrawObj_t* manageFileBox = DrawEmptyBox(10,150, getVideoMode()->fbWidth-10, 320);
	sprintf(txtbuffer, "Manage %s:", isFile ? "File" : "Directory");
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 160, txtbuffer, 1.0f, true, defaultColor));
	float scale = GetTextScaleToFitInWidth(getRelativeName(curFile.name), getVideoMode()->fbWidth-10-10);
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 190, getRelativeName(curFile.name), scale, true, defaultColor));
	sprintf(txtbuffer, "%s%s%s%s", 
					canCopy ? " (X) Copy " : "",
					canMove ? " (Y) Move " : "",
					canDelete ? " (Z) Delete " : "",
					canRename ? " (R) Rename" : "");
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 250, txtbuffer, 1.0f, true, defaultColor));
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 310, "Press an option to continue, or B to return", 1.0f, true, defaultColor));
	DrawPublish(manageFileBox);
	u32 waitButtons = PAD_BUTTON_X|PAD_BUTTON_Y|PAD_BUTTON_B|PAD_TRIGGER_Z|PAD_TRIGGER_R;
	do {VIDEO_WaitVSync();} while (PAD_ButtonsHeld(0) & waitButtons);
	int option = 0;
	while(1) {
		u32 buttons = PAD_ButtonsHeld(0);
		if(canCopy && (buttons & PAD_BUTTON_X)) {
			option = COPY_OPTION;
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_X){ VIDEO_WaitVSync (); }
			break;
		}
		if(canMove && (buttons & PAD_BUTTON_Y)) {
			option = MOVE_OPTION;
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
			break;
		}
		if(canDelete && (buttons & PAD_TRIGGER_Z)) {
			option = DELETE_OPTION;
			while(PAD_ButtonsHeld(0) & PAD_TRIGGER_Z){ VIDEO_WaitVSync (); }
			break;
		}
		if(canRename && (buttons & PAD_TRIGGER_R)) {
			option = RENAME_OPTION;
			while(PAD_ButtonsHeld(0) & PAD_TRIGGER_R){ VIDEO_WaitVSync (); }
			break;
		}
		if(buttons & PAD_BUTTON_B) {
			DrawDispose(manageFileBox);
			return false;
		}
	}
	do {VIDEO_WaitVSync();} while (PAD_ButtonsHeld(0) & waitButtons);
	DrawDispose(manageFileBox);
	
	// "Are you sure option" for deletes.
	if(option == DELETE_OPTION) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_WARN, "Delete confirmation required.\n \nPress L + A to continue, or B to cancel.");
		DrawPublish(msgBox);
		bool cancel = false;
		while(1) {
			u16 btns = PAD_ButtonsHeld(0);
			if ((btns & (PAD_BUTTON_A|PAD_TRIGGER_L)) == (PAD_BUTTON_A|PAD_TRIGGER_L)) {
				break;
			}
			else if (btns & PAD_BUTTON_B) {
				cancel = true;
				break;
			}
			VIDEO_WaitVSync();
		}
		do {VIDEO_WaitVSync();} while (PAD_ButtonsHeld(0) & (PAD_BUTTON_A|PAD_TRIGGER_L|PAD_BUTTON_B));
		DrawDispose(msgBox);
		if(cancel) {
			return false;
		}
	}

	// Handles renaming directories or files on FAT FS devices.
	if(canRename && option == RENAME_OPTION) {
		char *nameBuffer = calloc(1, sizeof(curFile.name));
		char *parentPath = calloc(1, sizeof(curFile.name));
		getParentPath(&curFile.name[0], parentPath);
		strcpy(nameBuffer, getRelativeName(&curFile.name[0]));
		DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_FILE, "Rename", nameBuffer, sizeof(curFile.name)-(strlen(parentPath)+1));
		concat_path(txtbuffer, parentPath, nameBuffer);
		bool modified = (strcmp(&curFile.name[0], txtbuffer) != 0) && strlen(nameBuffer) > 0;
		if(modified) {
			print_gecko("Renaming %s to %s\r\n", &curFile.name[0], txtbuffer);
			u32 ret = devices[DEVICE_CUR]->renameFile(&curFile, txtbuffer);
			sprintf(txtbuffer, "%s renamed!\nPress A to continue.", isFile ? "File" : "Directory");
			uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, ret ? "Move Failed! Press A to continue" : txtbuffer);
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
		}
		free(nameBuffer);
		free(parentPath);
		do {VIDEO_WaitVSync();} while (PAD_ButtonsHeld(0) & (PAD_BUTTON_B|PAD_BUTTON_START));
		return modified;
	}
	// Handle deletes (dir or file)
	else if(option == DELETE_OPTION) {
		uiDrawObj_t *progBar = DrawPublish(DrawProgressBar(true, 0, "Deleting ..."));
		bool deleted = deleteFileOrDir(&curFile);
		DrawDispose(progBar);
		sprintf(txtbuffer, "%s %s\nPress A to continue.", isFile ? "File" : "Directory", deleted ? "deleted successfully" : "failed to delete!");
		uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(deleted ? D_INFO : D_FAIL, txtbuffer));
		wait_press_A();
		DrawDispose(msgBox);
		return deleted;
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
			if(devices[DEVICE_DEST]->init( devices[DEVICE_DEST]->initial )) {
				sprintf(txtbuffer, "Failed to init destination device! (%u)\nPress A to continue.",ret);
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
		ret = select_dest_dir(devices[DEVICE_DEST]->initial, destFile);
		if(ret) {
			if(devices[DEVICE_DEST] != devices[DEVICE_CUR]) {
				devices[DEVICE_DEST]->deinit( devices[DEVICE_DEST]->initial );
			}
			return false;
		}
		
		u32 isDestCard = devices[DEVICE_DEST] == &__device_card_a || devices[DEVICE_DEST] == &__device_card_b;
		u32 isSrcCard = devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b;
		
		concat_path(destFile->name, destFile->name, stripInvalidChars(getRelativeName(curFile.name)));
		destFile->fp = 0;
		destFile->ffsFp = 0;
		destFile->fileBase = 0;
		destFile->offset = 0;
		destFile->size = 0;
		destFile->fileAttrib = IS_FILE;
		// Create a GCI if something is coming out from CARD to another device
		if(isSrcCard && !isDestCard) {
			strlcat(destFile->name, ".gci", PATHNAME_MAX);
		}

		// If the destination file already exists, ask the user what to do
		if(devices[DEVICE_DEST]->readFile(destFile, NULL, 0) == 0) {
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

					while(devices[DEVICE_DEST]->readFile(destFile, NULL, 0) == 0) {
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
		devices[DEVICE_CUR]->seekFile(&curFile, 0, DEVICE_HANDLER_SEEK_SET);
		devices[DEVICE_DEST]->seekFile(destFile, 0, DEVICE_HANDLER_SEEK_SET);
		
		// Same (fat based) device and user wants to move the file, just rename ;)
		if(devices[DEVICE_CUR] == devices[DEVICE_DEST]
			&& canRename && option == MOVE_OPTION) {
			ret = devices[DEVICE_CUR]->renameFile(&curFile, destFile->name);
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
			if(isDestCard && (endsWith(curFile.name,".gci") || endsWith(curFile.name,".gcs") || endsWith(curFile.name,".sav"))) {
				GCI gci;
				devices[DEVICE_CUR]->seekFile(&curFile, 0, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(&curFile, &gci, sizeof(GCI)) == sizeof(GCI)) {
					if(!memcmp(&gci, "DATELGC_SAVE", 12)) {
						devices[DEVICE_CUR]->seekFile(&curFile, 0x80, DEVICE_HANDLER_SEEK_SET);
						devices[DEVICE_CUR]->readFile(&curFile, &gci, sizeof(GCI));
						swab(&gci.reserved01, &gci.reserved01, 2);
						swab(&gci.icon_addr,  &gci.icon_addr, 20);
					}
					else if(!memcmp(&gci, "GCSAVE", 6)) {
						devices[DEVICE_CUR]->seekFile(&curFile, 0x110, DEVICE_HANDLER_SEEK_SET);
						devices[DEVICE_CUR]->readFile(&curFile, &gci, sizeof(GCI));
					}
					if(curFile.size - curFile.offset == gci.filesize8 * 8192) setGCIInfo(&gci);
					devices[DEVICE_CUR]->seekFile(&curFile, -sizeof(GCI), DEVICE_HANDLER_SEEK_CUR);
				}
			}
			
			// Read from one file and write to the new directory
			u32 isCard = devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b;
			u32 curOffset = curFile.offset, cancelled = 0, chunkSize = (isCard||isDestCard) ? curFile.size : (256*1024);
			char *readBuffer = (char*)memalign(32,chunkSize);
			sprintf(txtbuffer, "Copying to: %s",getRelativeName(destFile->name));
			uiDrawObj_t* progBar = DrawProgressBar(false, 0, txtbuffer);
			DrawPublish(progBar);
			
			u64 startTime = gettime();
			u64 lastTime = gettime();
			u32 lastOffset = 0;
			int speed = 0;
			int timeremain = 0;
			print_gecko("Copying %i byte file from %s to %s\r\n", curFile.size, &curFile.name[0], destFile->name);
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
				devices[DEVICE_CUR]->seekFile(&curFile, curOffset, DEVICE_HANDLER_SEEK_SET);
				ret = devices[DEVICE_CUR]->readFile(&curFile, readBuffer, amountToCopy);
				if(ret != amountToCopy) {	// Retry the read.
					devices[DEVICE_CUR]->seekFile(&curFile, curOffset, DEVICE_HANDLER_SEEK_SET);
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
						DrawDispose(msgBox);
						setGCIInfo(NULL);
						setCopyGCIMode(FALSE);
						return true;
					}
				}
				ret = devices[DEVICE_DEST]->writeFile(destFile, readBuffer, amountToCopy);
				if(ret != amountToCopy) {
					DrawDispose(progBar);
					free(readBuffer);
					devices[DEVICE_CUR]->closeFile(&curFile);
					devices[DEVICE_DEST]->closeFile(destFile);
					sprintf(txtbuffer, "Failed to Write! (%d %d)\n%s",amountToCopy,ret,destFile->name);
					uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
					DrawPublish(msgBox);
					wait_press_A();
					DrawDispose(msgBox);
					setGCIInfo(NULL);
					setCopyGCIMode(FALSE);
					return true;
				}
				curOffset+=amountToCopy;
			}
			DrawDispose(progBar);
			free(readBuffer);
			devices[DEVICE_CUR]->closeFile(&curFile);

			ret = devices[DEVICE_DEST]->writeFile(destFile, NULL, 0);
			if(ret == 0)
				ret = devices[DEVICE_DEST]->closeFile(destFile);
			if(ret != 0) {
				sprintf(txtbuffer, "Failed to Write! (%d)\n%s",ret,destFile->name);
				uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
				DrawPublish(msgBox);
				wait_press_A();
				DrawDispose(msgBox);
				setGCIInfo(NULL);
				setCopyGCIMode(FALSE);
				return true;
			}
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
					msgBox = DrawMessageBox(D_INFO,"Copy Complete.\nPress A to continue");
				}
			} 
			else {
				sprintf(txtbuffer, "%s cancelled.\nPress A to continue", (option == MOVE_OPTION) ? "Move" : "Copy");
				msgBox = DrawMessageBox(D_INFO,txtbuffer);
			}
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
		}
	}

	return true;
}

void verify_game()
{
	u32 crc = 0;
	u32 curOffset = 0, cancelled = 0, chunkSize = (32*1024);
	unsigned char *readBuffer = (unsigned char*)memalign(32,chunkSize);
	uiDrawObj_t* progBar = DrawProgressBar(false, 0, "Verifying ...");
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
		u32 amountToRead = curOffset + chunkSize > curFile.size ? curFile.size - curOffset : chunkSize;
		devices[DEVICE_CUR]->seekFile(&curFile, curOffset, DEVICE_HANDLER_SEEK_SET);
		u32 ret = devices[DEVICE_CUR]->readFile(&curFile, readBuffer, amountToRead);
		if(ret != amountToRead) {
			DrawDispose(progBar);
			free(readBuffer);
			sprintf(txtbuffer, "Failed to Read! (%d %d)\n%s",amountToRead,ret, &curFile.name[0]);
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
			return;
		}
		crc = crc32(crc,readBuffer,amountToRead);
		curOffset+=amountToRead;
	}
	DrawDispose(progBar);
	free(readBuffer);
	if(!cancelled) {
		uiDrawObj_t *msgBox = NULL;
		if(valid_gcm_crc32(&GCMDisk, crc)) {
			msgBox = DrawMessageBox(D_PASS,"Passed integrity verification!\nPress A to continue.");
		}
		else {
			msgBox = DrawMessageBox(D_FAIL,"Failed integrity verification!\nPress A to continue.");
		}
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
	}
}

void load_game() {
	file_handle *disc2File = meta_find_disc2(&curFile);
	
	if(devices[DEVICE_CUR] == &__device_wode) {
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Setup base offset please Wait .."));
		devices[DEVICE_CUR]->setupFile(&curFile, disc2File, NULL, -1);
		DrawDispose(msgBox);
	}
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading ..."));
	
	// boot the GCM/ISO file, gamecube disc or multigame selected entry
	memset(&tgcFile, 0, sizeof(TGCHeader));
	if(endsWith(curFile.name,".tgc")) {
		devices[DEVICE_CUR]->seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,&tgcFile,sizeof(TGCHeader)) != sizeof(TGCHeader) || tgcFile.magic != TGC_MAGIC) {
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File!"));
			sleep(2);
			DrawDispose(msgBox);
			return;
		}
		
		devices[DEVICE_CUR]->seekFile(&curFile,tgcFile.headerStart,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,&GCMDisk,sizeof(DiskHeader)) != sizeof(DiskHeader) || !valid_gcm_magic(&GCMDisk)) {
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File!"));
			sleep(2);
			DrawDispose(msgBox);
			return;
		}
		
		swissSettings.audioStreaming = is_streaming_disc(&GCMDisk);
	}
	else {
		devices[DEVICE_CUR]->seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,&GCMDisk,sizeof(DiskHeader)) != sizeof(DiskHeader) || !valid_gcm_magic(&GCMDisk)) {
			if(!GCMDisk.ConsoleID && !memcmp(&GCMDisk.ConsoleID, &GCMDisk.GamecodeA, sizeof(DiskHeader) - 1)) {
				devices[DEVICE_CUR]->seekFile(&curFile,0x8000,DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(&curFile,&GCMDisk,sizeof(DiskHeader)) == sizeof(DiskHeader) &&
					!GCMDisk.ConsoleID && !memcmp(&GCMDisk.ConsoleID, &GCMDisk.GamecodeA, sizeof(DiskHeader) - 1)) {
					DrawDispose(msgBox);
					msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File! (Fake SD Card?)"));
					sleep(2);
					DrawDispose(msgBox);
					return;
				}
			}
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File!"));
			sleep(2);
			DrawDispose(msgBox);
			return;
		}
		
		swissSettings.audioStreaming = is_streaming_disc(&GCMDisk);
		if(is_redump_disc(curFile.meta) && !valid_gcm_size(&GCMDisk, curFile.size)) {
			if(swissSettings.audioStreaming) {
				DrawDispose(msgBox);
				msgBox = DrawPublish(DrawMessageBox(D_WARN, "File is a bad dump and is not playable.\nPlease attempt recovery using NKit."));
				sleep(5);
				DrawDispose(msgBox);
				return;
			}
			else {
				DrawDispose(msgBox);
				msgBox = DrawPublish(DrawMessageBox(D_WARN, "File is a bad dump, but may be playable.\nPlease attempt recovery using NKit."));
				sleep(5);
			}
		}
	}
	
	DrawDispose(msgBox);
	// Find the config for this game, or default if we don't know about it
	ConfigEntry *config = calloc(1, sizeof(ConfigEntry));
	memcpy(config->game_id, &GCMDisk.ConsoleID, 4);
	memcpy(config->game_name, GCMDisk.GameName, 64);
	config->cleanBoot = !valid_gcm_boot(&GCMDisk);
	config_find(config);
	
	// Show game info or return to the menu
	if(!info_game(config)) {
		free(config);
		return;
	}
	
	if(devices[DEVICE_CONFIG] != NULL) {
		// Update the recent list.
		if(update_recent()) {
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving recent list ..."));
			config_update_recent(true);
			DrawDispose(msgBox);
		}
	}
	
	// Load config for this game into our current settings
	config_load_current(config);
	gameID_early_set(&GCMDisk);
	
	if(config->cleanBoot) {
		gameID_set(&GCMDisk, get_gcm_boot_hash(&GCMDisk));
		
		if(devices[DEVICE_CUR]->location != LOC_DVD_CONNECTOR) {
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Device does not support clean boot."));
			sleep(2);
			DrawDispose(msgBox);
			goto fail;
		}
		if(!devices[DEVICE_CUR]->setupFile(&curFile, disc2File, NULL, -2)) {
			msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Failed to setup the file (too fragmented?)"));
			wait_press_A();
			DrawDispose(msgBox);
			goto fail;
		}
		if(devices[DEVICE_CUR] != &__device_dvd) {
			devices[DEVICE_CUR]->deinit(&curFile);
		}
		
		DrawShutdown();
		SYS_ResetSystem(SYS_HOTRESET, 0, FALSE);
		__builtin_unreachable();
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
	if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
		setTopAddr(WIIRD_ENGINE);
	}
	else {
		setTopAddr(0x81800000);
	}
	
	int numToPatch = 0;
	ExecutableFile *filesToPatch = memalign(32, sizeof(ExecutableFile)*512);
	memset(filesToPatch, 0, sizeof(ExecutableFile)*512);

	// Report to the user the patch status of this GCM/ISO file
	numToPatch = check_game(&curFile, disc2File, filesToPatch);
	
	// Prompt for DOL selection if multi-dol
	ExecutableFile *fileToPatch = NULL;
	if(devices[DEVICE_PATCHES] == NULL) {
		fileToPatch = select_alt_dol(filesToPatch, numToPatch);
	}
	if(fileToPatch != NULL) {
		print_gecko("Alt DOL selected: %s\r\n", fileToPatch->name);
		gameID_set(&GCMDisk, fileToPatch->hash);
	}
	else if(tgcFile.magic == TGC_MAGIC) {
		for(int i = 0; i < numToPatch; i++) {
			if(filesToPatch[i].file == &curFile && filesToPatch[i].offset == tgcFile.dolStart) {
				fileToPatch = &filesToPatch[i];
				gameID_set(&GCMDisk, filesToPatch[i].hash);
				break;
			}
		}
	}
	else {
		for(int i = 0; i < numToPatch; i++) {
			if(filesToPatch[i].file == &curFile && filesToPatch[i].offset == GCMDisk.DOLOffset) {
				if(!swissSettings.bs2Boot)
					fileToPatch = &filesToPatch[i];
				gameID_set(&GCMDisk, filesToPatch[i].hash);
				break;
			}
		}
	}
	
	*(vu8*)VAR_CURRENT_DISC = fileToPatch && fileToPatch->file == disc2File;
	*(vu8*)VAR_SECOND_DISC = !!disc2File;
	*(vu8*)VAR_DRIVE_PATCHED = 0;
	*(vu8*)VAR_EMU_READ_SPEED = swissSettings.emulateReadSpeed;
	*(vu32**)VAR_EXI_REGS = NULL;
	*(vu8*)VAR_EXI_SLOT = EXI_CHANNEL_MAX;
	*(vu8*)VAR_EXI_FREQ = EXI_SPEED1MHZ;
	*(vu8*)VAR_SD_SHIFT = 0;
	*(vu8*)VAR_IGR_TYPE = swissSettings.igrType | (tgcFile.magic == TGC_MAGIC ? 0x80:0x00);
	*(vu32**)VAR_FRAG_LIST = NULL;
	*(vu8*)VAR_TRIGGER_LEVEL = swissSettings.triggerLevel;
	*(vu8*)VAR_CARD_A_ID = 0x00;
	*(vu8*)VAR_CARD_B_ID = 0x00;
	
	// Call the special setup for each device (e.g. SD will set the sector(s))
	if(!devices[DEVICE_CUR]->setupFile(&curFile, disc2File, filesToPatch, numToPatch)) {
		msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Failed to setup the file (too fragmented?)"));
		wait_press_A();
		DrawDispose(msgBox);
		goto fail_patched;
	}

	load_app(fileToPatch);

fail_patched:
	if(devices[DEVICE_PATCHES] != NULL) {
		for(int i = 0; i < numToPatch; i++) {
			devices[DEVICE_PATCHES]->closeFile(filesToPatch[i].patchFile);
			free(filesToPatch[i].patchFile);
		}
		if(devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
			devices[DEVICE_PATCHES]->deinit(devices[DEVICE_PATCHES]->initial);
		}
		devices[DEVICE_PATCHES] = NULL;
	}
	free(filesToPatch);
fail:
	gameID_unset();
	config_unload_current();
	free(config);
}

/* Execute/Load/Parse the currently selected file */
void load_file()
{
	char *fileName = &curFile.name[0];
		
	//if it's a DOL, boot it
	if(strlen(fileName)>4) {
		if(endsWith(fileName,".dol") || endsWith(fileName,".dol+cli") || endsWith(fileName,".elf")) {
			boot_dol();
			// if it was invalid (overlaps sections, too large, etc..) it'll return
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid DOL"));
			sleep(2);
			DrawDispose(msgBox);
			return;
		}
		else if(endsWith(fileName,".fzn")) {
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
			snprintf(&fwFile.name[0], PATHNAME_MAX, "%s.fw", &curFile.name[0]);
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
		else if(endsWith(fileName,".gcm") || endsWith(fileName,".iso") || endsWith(fileName,".tgc")) {
			if(devices[DEVICE_CUR]->features & FEAT_BOOT_GCM) {
				load_game();
				memset(&GCMDisk, 0, sizeof(DiskHeader));
			}
			else {
				uiDrawObj_t *msgBox = NULL;
				if(devices[DEVICE_CUR] == &__device_sd_a || devices[DEVICE_CUR] == &__device_sd_b || devices[DEVICE_CUR] == &__device_sd_c) {
					msgBox = DrawPublish(DrawMessageBox(D_WARN, "Device does not support disc images.\nSet EXI Speed to 16 MHz to bypass."));
				}
				else {
					msgBox = DrawPublish(DrawMessageBox(D_WARN, "Device does not support disc images."));
				}
				sleep(5);
				DrawDispose(msgBox);
			}
			return;
		}
		else if(endsWith(fileName,".gcz") || endsWith(fileName,".rvz")) {
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "Compressed disc images must be\ndecompressed using NKit or Dolphin."));
			sleep(5);
			DrawDispose(msgBox);
			return;
		}
		else if(endsWith(fileName,".mp3")) {
			mp3_player(getCurrentDirEntries(), getCurrentDirEntryCount(), &curFile);
			return;
		}
		// This should be unreachable now anyway.
		uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "Unknown File Type!"));
		sleep(1);
		DrawDispose(msgBox);
		return;			
	}

}

int check_game(file_handle *file, file_handle *file2, ExecutableFile *filesToPatch)
{ 	
	char* gameID = (char*)&GCMDisk;
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Checking Game .."));
	
	int numToPatch;
	if(tgcFile.magic == TGC_MAGIC) {
		numToPatch = parse_tgc(file, filesToPatch, 0, getRelativeName(file->name));
	}
	else {
		numToPatch = parse_gcm(file, file2, filesToPatch);
		
		if(!strncmp(gameID, "GCCE01", 6) || !strncmp(gameID, "GCCJGC", 6) || !strncmp(gameID, "GCCP01", 6)) {
			parse_gcm_add(file, filesToPatch, &numToPatch, "ffcc_cli.bin");
		}
		else if(!strncmp(gameID, "GHAE08", 6) || !strncmp(gameID, "GHAJ08", 6)) {
			parse_gcm_add(file, filesToPatch, &numToPatch, "claire.rel");
			parse_gcm_add(file, filesToPatch, &numToPatch, "leon.rel");
		}
		else if(!strncmp(gameID, "GHAP08", 6)) {
			switch(swissSettings.sramLanguage) {
				case SYS_LANG_ENGLISH:
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon.rel");
					break;
				case SYS_LANG_GERMAN:
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_g.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_g.rel");
					break;
				case SYS_LANG_FRENCH:
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_f.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_f.rel");
					break;
				case SYS_LANG_SPANISH:
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_s.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_s.rel");
					break;
				case SYS_LANG_ITALIAN:
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_i.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_i.rel");
					break;
				default:
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_f.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_g.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_i.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "claire_s.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_f.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_g.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_i.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "leon_s.rel");
					break;
			}
		}
		else if(!strncmp(gameID, "GLEP08", 6)) {
			switch(swissSettings.sramLanguage) {
				case SYS_LANG_ENGLISH:
					parse_gcm_add(file, filesToPatch, &numToPatch, "eng.rel");
					break;
				case SYS_LANG_GERMAN:
					parse_gcm_add(file, filesToPatch, &numToPatch, "ger.rel");
					break;
				case SYS_LANG_FRENCH:
					parse_gcm_add(file, filesToPatch, &numToPatch, "fra.rel");
					break;
				case SYS_LANG_SPANISH:
					parse_gcm_add(file, filesToPatch, &numToPatch, "spa.rel");
					break;
				case SYS_LANG_ITALIAN:
					parse_gcm_add(file, filesToPatch, &numToPatch, "ita.rel");
					break;
				default:
					parse_gcm_add(file, filesToPatch, &numToPatch, "eng.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "fra.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "ger.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "ita.rel");
					parse_gcm_add(file, filesToPatch, &numToPatch, "spa.rel");
					break;
			}
		}
		if(swissSettings.disableVideoPatches < 1) {
			if(!strncmp(gameID, "GS8P7D", 6)) {
				parse_gcm_add(file, filesToPatch, &numToPatch, "SPYROCFG_NGC.CFG");
			}
		}
	}
	DrawDispose(msgBox);
	
	if(devices[DEVICE_CUR]->features & FEAT_HYPERVISOR) {
		patch_gcm(filesToPatch, numToPatch);
	}
	return numToPatch;
}

uiDrawObj_t* draw_game_info() {
	uiDrawObj_t *container = DrawEmptyBox(75,120, getVideoMode()->fbWidth-78, 400);

	sprintf(txtbuffer, "%s", curFile.meta && curFile.meta->displayName ? curFile.meta->displayName : getRelativeName(curFile.name));
	float scale = GetTextScaleToFitInWidth(txtbuffer,(getVideoMode()->fbWidth-78)-75);
	DrawAddChild(container, DrawStyledLabel(640/2, 130, txtbuffer, scale, true, defaultColor));

	if(devices[DEVICE_CUR] == &__device_qoob) {
		formatBytes(stpcpy(txtbuffer, "Size: "), curFile.size, 65536, false);
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
		formatBytes(stpcpy(txtbuffer, "Size: "), curFile.size, 8192, false);
		DrawAddChild(container, DrawStyledLabel(640/2, 160, txtbuffer, 0.8f, true, defaultColor));
		sprintf(txtbuffer,"Position on Card: %08X",curFile.offset);
		DrawAddChild(container, DrawStyledLabel(640/2, 180, txtbuffer, 0.8f, true, defaultColor));
	}
	else {
		formatBytes(stpcpy(txtbuffer, "Size: "), curFile.size, 0, true);
		DrawAddChild(container, DrawStyledLabel(640/2, 160, txtbuffer, 0.8f, true, defaultColor));
		if(curFile.meta) {
			if(curFile.meta->banner)
				DrawAddChild(container, DrawTexObj(&curFile.meta->bannerTexObj, 215, 240, 192, 64, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
			if(curFile.meta->regionTexObj)
				DrawAddChild(container, DrawTexObj(curFile.meta->regionTexObj, 449, 262, 32, 20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));

			sprintf(txtbuffer, "%s", curFile.meta->bannerDesc.description);
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
		if(is_verifiable_disc(&GCMDisk)) {
			DrawAddChild(container, DrawStyledLabel(640/2, 180, "Verify (R)", 0.6f, true, defaultColor));
		}
		sprintf(txtbuffer, "Game ID: [%.6s] Audio Streaming: [%s]", (char*)&GCMDisk, (swissSettings.audioStreaming ? "Yes":"No"));
		DrawAddChild(container, DrawStyledLabel(640/2, 200, txtbuffer, 0.8f, true, defaultColor));

		if(GCMDisk.TotalDisc > 1) {
			sprintf(txtbuffer, "Disc %i/%i [Found: %s]", GCMDisk.DiscID+1, GCMDisk.TotalDisc, meta_find_disc2(&curFile) ? "Yes":"No");
			DrawAddChild(container, DrawStyledLabel(640/2, 220, txtbuffer, 0.6f, true, defaultColor));
		}
	}
	if(devices[DEVICE_CUR] == &__device_wode) {
		DrawAddChild(container, DrawStyledLabel(640/2, 390, "Settings (X) - Cheats (Y) - Boot (A)", 0.75f, true, defaultColor));
	}
	else {
		if(devices[DEVICE_CONFIG] != NULL) {
			bool isAutoLoadEntry = !strcmp(&swissSettings.autoload[0], &curFile.name[0]);
			sprintf(txtbuffer, "Load at startup (Z) [Current: %s]", isAutoLoadEntry ? "Yes":"No");
			DrawAddChild(container, DrawStyledLabel(640/2, 370, txtbuffer, 0.6f, true, defaultColor));
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 390, "Settings (X) - Cheats (Y) - Exit (B) - Boot (A)", 0.75f, true, defaultColor));
	}
	return container;
}

/* Show info about the game - and also load the config for it */
int info_game(ConfigEntry *config)
{
	if(swissSettings.autoBoot) {
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
			swissSettings.autoBoot = 0;
		} else {
			return swissSettings.autoBoot;
		}
	}
	int ret = 0, num_cheats = -1;
	uiDrawObj_t *infoPanel = DrawPublish(draw_game_info());
	while(1) {
		while(PAD_ButtonsHeld(0) & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y | PAD_TRIGGER_Z | PAD_TRIGGER_R)){ VIDEO_WaitVSync (); }
		while(!(PAD_ButtonsHeld(0) & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y | PAD_TRIGGER_Z | PAD_TRIGGER_R))){ VIDEO_WaitVSync (); }
		u32 buttons = PAD_ButtonsHeld(0);
		if(buttons & PAD_BUTTON_A) {
			if(buttons & PAD_TRIGGER_L) {
				config->cleanBoot = 1;
			}
			ret = 1;
			break;
		}
		// WODE can't return from here.
		if((buttons & PAD_BUTTON_B) && devices[DEVICE_CUR] != &__device_wode) {
			ret = 0;
			break;
		}
		if((buttons & PAD_TRIGGER_R) && is_verifiable_disc(&GCMDisk)) {
			verify_game();
		}
		if(buttons & PAD_BUTTON_X) {
			show_settings(PAGE_GAME, 0, config);
		}
		if((buttons & PAD_TRIGGER_Z) && devices[DEVICE_CONFIG] != NULL) {
			// Toggle autoload
			if(!strcmp(&swissSettings.autoload[0], &curFile.name[0])) {
				memset(&swissSettings.autoload[0], 0, PATHNAME_MAX);
			}
			else {
				strcpy(&swissSettings.autoload[0], &curFile.name[0]);
			}
			// Save config
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving autoload ..."));
			config_update_global(true);
			DrawDispose(infoPanel);
			infoPanel = DrawPublish(draw_game_info());
			DrawDispose(msgBox);
		}
		// Look for a cheats file based on the GameID
		if(buttons & PAD_BUTTON_Y) {
			// don't find cheats again if we've just found some for this game since it'll wipe selections.
			if(num_cheats == -1) {
				num_cheats = findCheats(false);
			}
			if(num_cheats != 0) {
				DrawCheatsSelector(getRelativeName(getCurrentDirEntries()[curSelection].name));
			}
		}
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	}
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	DrawDispose(infoPanel);
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
		uiDrawObj_t *deviceImage = DrawImage(devImage->textureId, 640/2, 270-(devImage->realHeight/2), devImage->realWidth, devImage->realHeight, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
		uiDrawObj_t *deviceNameLabel = DrawStyledLabel(640/2, 330, (char*)allDevices[curDevice]->deviceName, 0.85f, true, defaultColor);
		uiDrawObj_t *deviceDescLabel = DrawStyledLabel(640/2, 350, (char*)allDevices[curDevice]->deviceDescription, 0.65f, true, defaultColor);
		DrawAddChild(deviceSelectBox, deviceImage);
		DrawAddChild(deviceSelectBox, deviceNameLabel);
		DrawAddChild(deviceSelectBox, deviceDescLabel);
		if(allDevices[curDevice]->features & FEAT_BOOT_GCM) {
			uiDrawObj_t *gameBootLabel = DrawStyledLabel(640/2, 365, (allDevices[curDevice]->features & FEAT_AUDIO_STREAMING) ? "Supports Game Boot & Audio Streaming":"Supports Game Boot", 0.65f, true, defaultColor);
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
	if(devices[type] != NULL) {
		// Don't deinit our current device when selecting a destination device
		if(!(type == DEVICE_DEST && devices[type] == devices[DEVICE_CUR])) {
			devices[type]->deinit( devices[type]->initial );
		}
	}
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
		DrawUpdateMenuButtons((curMenuLocation == ON_OPTIONS) ? curMenuSelection : MENU_NOSELECT);
		select_device(DEVICE_CUR);
		if(devices[DEVICE_CUR] != NULL) {
			memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Setting up device"));
			// If the user selected a device, make sure it's ready before we browse the filesystem
			s32 ret = devices[DEVICE_CUR]->init( devices[DEVICE_CUR]->initial );
			if(ret) {
				needsDeviceChange = 1;
				if(ret == ENODEV) {	// for completely removed devices vs something like the disc drive without a disc.
					deviceHandler_setDeviceAvailable(devices[DEVICE_CUR], false);
				}
				DrawDispose(msgBox);
				char* statusMsg = devices[DEVICE_CUR]->status(devices[DEVICE_CUR]->initial);
				msgBox = DrawPublish(DrawMessageBox(D_FAIL, statusMsg ? statusMsg : strerror(ret)));
				sleep(2);
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
		DrawUpdateMenuButtons((curMenuLocation == ON_OPTIONS) ? curMenuSelection : MENU_NOSELECT);
		if(devices[DEVICE_CUR] != NULL && needsRefresh) {
			curMenuLocation=ON_OPTIONS;
			curSelection=0; curMenuSelection=0;
			scanFiles();
			if(getCurrentDirEntryCount()<1) { devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial); needsDeviceChange=1; break;}
			needsRefresh = 0;
			curMenuLocation = ON_FILLIST;
			DrawUpdateMenuButtons((curMenuLocation == ON_OPTIONS) ? curMenuSelection : MENU_NOSELECT);
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
			while (!((btns=PAD_ButtonsHeld(0)) & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT | PAD_BUTTON_START))) {
				VIDEO_WaitVSync();
			}
			
			if(btns & PAD_BUTTON_LEFT){	curMenuSelection = (--curMenuSelection < 0) ? (MENU_MAX-1) : curMenuSelection;}
			else if(btns & PAD_BUTTON_RIGHT){curMenuSelection = (curMenuSelection + 1) % MENU_MAX;	}

			if(btns & PAD_BUTTON_A) {
				//handle menu event
				switch(curMenuSelection) {
					case MENU_DEVICE:
						needsDeviceChange = 1;  //Change from SD->DVD or vice versa
						break;
					case MENU_SETTINGS:
						show_settings(PAGE_GLOBAL, 0, NULL);
						break;
					case MENU_INFO:
						show_info();
						break;
					case MENU_REFRESH:
						if(devices[DEVICE_CUR] != NULL) {
							memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
							if(devices[DEVICE_CUR] == &__device_wkf) { 
								wkfReinit(); devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
							}
						}
						needsRefresh=1;
						break;
					case MENU_EXIT:
						DrawShutdown();
						SYS_ResetSystem(SYS_HOTRESET, 0, TRUE);
						__builtin_unreachable();
						break;
				}
			}
			if((btns & PAD_BUTTON_B) && devices[DEVICE_CUR] != NULL) {
				curMenuLocation = ON_FILLIST;
			}
			if((swissSettings.recentListLevel != 2) && (PAD_ButtonsHeld(0) & PAD_BUTTON_START)) {
				select_recent_entry();
			}
			while(PAD_ButtonsHeld(0) & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT | PAD_BUTTON_START)) {
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
}
