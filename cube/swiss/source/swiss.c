/*
*
*   Swiss - The Gamecube IPL replacement
*
*/

#include <argz.h>
#include <fnmatch.h>
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
#include <psoarchive/PRS.h>
#include <xxhash.h>
#include <zlib.h>

#include "swiss.h"
#include "main.h"
#include "util.h"
#include "info.h"
#include "httpd.h"
#include "exi.h"
#include "bba.h"
#include "patcher.h"
#include "dvd.h"
#include "elf.h"
#include "flippy.h"
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
#include "devices/filelock.h"
#include "devices/filemeta.h"
#include "dolparameters.h"
#include "reservedarea.h"

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

/* re-init video for a given game */
void ogc_video__reset()
{
	/* set TV mode for current game */
	switch(swissSettings.gameVMode) {
		case -2:
			sprintf(txtbuffer, "Video Mode: %s", "PAL 576p");
			newmode = &TVPal576ProgScale;
			break;
		case -1:
			sprintf(txtbuffer, "Video Mode: %s", "NTSC 480p");
			newmode = &TVNtsc480Prog;
			break;
		case 0:
			switch(swissSettings.sramVideo) {
				case SYS_VIDEO_PAL:
					sprintf(txtbuffer, "Video Mode: %s", "PAL 576i");
					newmode = &TVPal576IntDfScale;
					break;
				case SYS_VIDEO_MPAL:
					sprintf(txtbuffer, "Video Mode: %s", "PAL-M 480i");
					newmode = &TVMpal480IntDf;
					break;
				default:
					sprintf(txtbuffer, "Video Mode: %s", "NTSC 480i");
					newmode = &TVNtsc480IntDf;
					break;
			}
			break;
		case 1 ... 3:
			sprintf(txtbuffer, "Video Mode: %s %s", "NTSC", gameVModeStr[swissSettings.gameVMode]);
			newmode = &TVNtsc480IntDf;
			break;
		case 4 ... 7:
			sprintf(txtbuffer, "Video Mode: %s %s", "NTSC", gameVModeStr[swissSettings.gameVMode]);
			newmode = &TVNtsc480Prog;
			break;
		case 8 ... 10:
			sprintf(txtbuffer, "Video Mode: %s %s\n%s Mode selected.", "PAL", gameVModeStr[swissSettings.gameVMode], swissSettings.sram60Hz ? "60Hz":"50Hz");
			newmode = &TVPal576IntDfScale;
			break;
		case 11 ... 14:
			sprintf(txtbuffer, "Video Mode: %s %s\n%s Mode selected.", "PAL", gameVModeStr[swissSettings.gameVMode], swissSettings.sram60Hz ? "60Hz":"50Hz");
			newmode = &TVPal576ProgScale;
			break;
		default:
			newmode = NULL;
			break;
	}
	if((newmode != NULL) && (newmode != getVideoMode())) {
		DrawVideoMode(newmode);
		uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, txtbuffer);
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
	if(devices[DEVICE_CUR]->location & LOC_SYSTEM)
		sprintf(txtbuffer, "%s", "System");
	else if(devices[DEVICE_CUR]->location == LOC_MEMCARD_SLOT_A)
		sprintf(txtbuffer, "%s", "Slot A");
	else if(devices[DEVICE_CUR]->location == LOC_MEMCARD_SLOT_B)
		sprintf(txtbuffer, "%s", "Slot B");
	else if(devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR)
		sprintf(txtbuffer, "%s", "DVD Device");
	else if(devices[DEVICE_CUR]->location == LOC_SERIAL_PORT_1)
		sprintf(txtbuffer, "%s", "Serial Port 1");
	else if(devices[DEVICE_CUR]->location == LOC_SERIAL_PORT_2)
		sprintf(txtbuffer, "%s", "Serial Port 2");
	else if(devices[DEVICE_CUR]->location == LOC_HSP)
		sprintf(txtbuffer, "%s", "Hi Speed Port");
	else
		sprintf(txtbuffer, "%s", "Unknown");
	uiDrawObj_t *devLocationLabel = DrawStyledLabel(30 + ((135-30) / 2), 195, txtbuffer, 0.65f, true, defaultColor);
	DrawAddChild(containerPanel, devLocationLabel);
	
	device_info *info = devices[DEVICE_CUR]->info(devices[DEVICE_CUR]->initial);
	if (info == NULL) {
		uiDrawObj_t *devInfoBox = DrawTransparentBox(30, 225, 135, 260);	// Device size/extra info box
		DrawAddChild(containerPanel, devInfoBox);
		
		// Used space
		uiDrawObj_t *devUsedLabel = DrawStyledLabel(83, 233, "Used:", 0.6f, true, defaultColor);
		DrawAddChild(containerPanel, devUsedLabel);
		formatBytes(txtbuffer, getCurrentDirSize(), 0, !(devices[DEVICE_CUR]->location & LOC_SYSTEM));
		uiDrawObj_t *devUsedSizeLabel = DrawStyledLabel(83, 248, txtbuffer, 0.6f, true, defaultColor);
		DrawAddChild(containerPanel, devUsedSizeLabel);
	} else {
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
		while ((padsStickY() > -16 && padsStickY() < 16) && !(padsButtonsHeld() & PAD_BUTTON_B) && !(padsButtonsHeld() & PAD_BUTTON_A) && !(padsButtonsHeld() & PAD_BUTTON_UP) && !(padsButtonsHeld() & PAD_BUTTON_DOWN))
			{ VIDEO_WaitVSync (); }
		if((padsButtonsHeld() & PAD_BUTTON_UP) || padsStickY() > 16){	idx = (--idx < 0) ? max-1 : idx;}
		if((padsButtonsHeld() & PAD_BUTTON_DOWN) || padsStickY() < -16) {idx = (idx + 1) % max;	}
		if((padsButtonsHeld() & PAD_BUTTON_A))	break;
		if((padsButtonsHeld() & PAD_BUTTON_B))	{ idx = -1; break; }
		if(padsStickY() < -16 || padsStickY() > 16) {
			usleep(50000 - abs(padsStickY()*16));
		}
		while (!(!(padsButtonsHeld() & PAD_BUTTON_B) && !(padsButtonsHeld() & PAD_BUTTON_A) && !(padsButtonsHeld() & PAD_BUTTON_UP) && !(padsButtonsHeld() & PAD_BUTTON_DOWN)))
			{ VIDEO_WaitVSync (); }
	}
	do {VIDEO_WaitVSync();} while (padsButtonsHeld() & PAD_BUTTON_B);
	DrawDispose(container);
	if(idx >= 0) {
		int res = find_existing_entry(&swissSettings.recent[idx][0], true);
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
	int i = 0, j = 0;
	current_view_start = MIN(MAX(0,curSelection-FILES_PER_PAGE/2),MAX(0,num_files-FILES_PER_PAGE));
	current_view_end = MIN(num_files, MAX(curSelection+(FILES_PER_PAGE+1)/2,FILES_PER_PAGE));
	drawCurrentDevice(containerPanel);
	int fileListBase = 105;
	int scrollBarHeight = (FILES_PER_PAGE*40);
	int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
	if(num_files > 0) {
		// Draw which directory we're in
		sprintf(txtbuffer, "%s", getDevicePath(&curDir.name[0]));
		float scale = GetTextScaleToFitInWidthWithMax(txtbuffer, ((getVideoMode()->fbWidth-150)-20), .85);
		DrawAddChild(containerPanel, DrawStyledLabel(150, 80, txtbuffer, scale, false, defaultColor));
		if(!strcmp(&swissSettings.autoload[0], &curDir.name[0])
		|| !fnmatch(&swissSettings.autoload[0], &curDir.name[0], FNM_PATHNAME)) {
			DrawAddChild(containerPanel, DrawImage(TEX_STAR, ((getVideoMode()->fbWidth-30)-16), 80, 16, 16, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		}
		if(num_files > FILES_PER_PAGE) {
			uiDrawObj_t *scrollBar = DrawVertScrollBar(getVideoMode()->fbWidth-25, fileListBase, 16, scrollBarHeight, (float)((float)curSelection/(float)(num_files-1)),scrollBarTabHeight);
			DrawAddChild(containerPanel, scrollBar);
		}
		for(i = current_view_start,j = 0; i<current_view_end; ++i,++j) {
			lockFile(directory[i]);
			populate_meta(directory[i]);
			uiDrawObj_t *browserButton = DrawFileBrowserButton(150, fileListBase+(j*40),
									getVideoMode()->fbWidth-30, fileListBase+(j*40)+40,
									getRelativePath(directory[i]->name, curDir.name),
									directory[i],
									(i == curSelection) ? B_SELECTED:B_NOSELECT);
			directory[i]->uiObj = browserButton;
			unlockFile(directory[i]);
			DrawAddChild(containerPanel, browserButton);
		}
	}
}

// Modify entry to go up a directory.
bool upToParent(file_handle* entry)
{
	// If we're a file, go up to the parent of the file
	if(entry->fileType == IS_FILE)
		getParentPath(entry->name, entry->name);
	
	// Go up a folder
	return getParentPath(entry->name, entry->name);
}

uiDrawObj_t* renderFileBrowser(file_handle** directory, int num_files, uiDrawObj_t* filePanel)
{
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) {
		memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
		needsRefresh=1;
		return filePanel;
	}
	uiDrawObj_t *loadingBox = DrawProgressLoading(PROGRESS_BOX_BOTTOMLEFT);
	DrawPublish(loadingBox);
	meta_thread_start(loadingBox);
	while(1) {
		DrawUpdateProgressLoading(loadingBox, +1);
		uiDrawObj_t *newPanel = DrawContainer();
		drawFiles(directory, num_files, newPanel);
		if(filePanel != NULL) {
			DrawDispose(filePanel);
		}
		filePanel = newPanel;
		DrawPublish(filePanel);
		DrawUpdateProgressLoading(loadingBox, -1);
		
		u32 waitButtons = PAD_BUTTON_X|PAD_BUTTON_START|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_TRIGGER_Z;
		while ((padsStickY() > -16 && padsStickY() < 16) && !(padsButtonsHeld() & waitButtons))
			{ VIDEO_WaitVSync (); }
		if((padsButtonsHeld() & PAD_BUTTON_UP) || padsStickY() >= 16){	curSelection = (--curSelection < 0) ? num_files-1 : curSelection;}
		if((padsButtonsHeld() & PAD_BUTTON_DOWN) || padsStickY() <= -16) {curSelection = (curSelection + 1) % num_files;	}
		if(padsButtonsHeld() & (PAD_BUTTON_LEFT|PAD_TRIGGER_L)) {
			if(curSelection == 0) {
				curSelection = num_files-1;
			}
			else {
				curSelection = (curSelection - FILES_PER_PAGE < 0) ? 0 : curSelection - FILES_PER_PAGE;
			}
		}
		if(padsButtonsHeld() & (PAD_BUTTON_RIGHT|PAD_TRIGGER_R)) {
			if(curSelection == num_files-1) {
				curSelection = 0;
			}
			else {
				curSelection = (curSelection + FILES_PER_PAGE > num_files-1) ? num_files-1 : (curSelection + FILES_PER_PAGE) % num_files;
			}
		}
		
		if(padsButtonsHeld() & PAD_BUTTON_A) {
			lockFile(directory[curSelection]);
			//go into a folder or select a file
			if(directory[curSelection]->fileType==IS_DIR) {
				memcpy(&curDir, directory[curSelection], sizeof(file_handle));
				needsRefresh=1;
			}
			else if(directory[curSelection]->fileType==IS_SPECIAL) {
				memcpy(&curFile, &curDir, sizeof(file_handle));
				curDir.fileBase = directory[curSelection]->fileBase;
				needsDeviceChange = upToParent(&curDir);
				needsRefresh=1;
			}
			else if(directory[curSelection]->fileType==IS_FILE) {
				memcpy(&curFile, directory[curSelection], sizeof(file_handle));
				if(canLoadFileType(curFile.name, devices[DEVICE_CUR]->extraExtensions)) {
					meta_thread_stop();
					load_file();
				}
				else if(swissSettings.enableFileManagement) {
					meta_thread_stop();
					needsRefresh = manage_file() ? 1:0;
				}
				memcpy(directory[curSelection], &curFile, sizeof(file_handle));
			}
			unlockFile(directory[curSelection]);
			break;
		}
		if(padsButtonsHeld() & PAD_BUTTON_X) {
			memcpy(&curFile, &curDir, sizeof(file_handle));
			curDir.fileBase = directory[0]->fileBase;
			needsDeviceChange = upToParent(&curDir);
			needsRefresh=1;
			while(padsButtonsHeld() & PAD_BUTTON_X) VIDEO_WaitVSync();
			break;
		}
		if((padsButtonsHeld() & PAD_TRIGGER_Z) && swissSettings.enableFileManagement) {
			lockFile(directory[curSelection]);
			if(directory[curSelection]->fileType == IS_FILE || directory[curSelection]->fileType == IS_DIR) {
				memcpy(&curFile, directory[curSelection], sizeof(file_handle));
				meta_thread_stop();
				needsRefresh = manage_file() ? 1:0;
				memcpy(directory[curSelection], &curFile, sizeof(file_handle));
				while(padsButtonsHeld() & PAD_BUTTON_B) VIDEO_WaitVSync();
				if(needsRefresh) {
					// If we return from doing something with a file, refresh the device in the same dir we were at
					unlockFile(directory[curSelection]);
					break;
				}
			}
			else if(directory[curSelection]->fileType == IS_SPECIAL) {
				// Toggle autoload
				if(!strcmp(&swissSettings.autoload[0], &curDir.name[0])
				|| !fnmatch(&swissSettings.autoload[0], &curDir.name[0], FNM_PATHNAME)) {
					memset(&swissSettings.autoload[0], 0, PATHNAME_MAX);
				}
				else {
					strcpy(&swissSettings.autoload[0], &curDir.name[0]);
				}
				// Save config
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving autoload\205"));
				config_update_global(true);
				DrawDispose(msgBox);
			}
			unlockFile(directory[curSelection]);
		}
		
		if((padsButtonsHeld() & PAD_BUTTON_START) && swissSettings.recentListLevel > 0) {
			meta_thread_stop();
			select_recent_entry();
			break;
		}
		if(padsButtonsHeld() & PAD_BUTTON_B) {
			curMenuLocation = ON_OPTIONS;
			DrawUpdateFileBrowserButton(directory[curSelection]->uiObj, (curMenuLocation == ON_FILLIST) ? B_SELECTED:B_NOSELECT);
			break;
		}
		if(padsStickY() <= -16 || padsStickY() >= 16) {
			usleep((abs(padsStickY()) > 64 ? 50000:100000) - abs(padsStickY()*64));
		}
		else {
			while (padsButtonsHeld() & waitButtons)
				{ VIDEO_WaitVSync (); }
		}
	}
	meta_thread_stop();
	DrawDispose(loadingBox);
	return filePanel;
}

void drawCurrentDeviceCarousel(uiDrawObj_t *containerPanel) {
	uiDrawObj_t *bgBox = DrawTransparentBox(30, 395, getVideoMode()->fbWidth-30, 420);
	DrawAddChild(containerPanel, bgBox);
	// Device name
	uiDrawObj_t *devNameLabel = DrawStyledLabel(50, 400, devices[DEVICE_CUR]->deviceName, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devNameLabel);
	
	device_info *info = devices[DEVICE_CUR]->info(devices[DEVICE_CUR]->initial);
	if (info == NULL) {
		char *textPtr = txtbuffer;
		textPtr = stpcpy(textPtr, "Used: ");
		textPtr += formatBytes(textPtr, getCurrentDirSize(), 0, !(devices[DEVICE_CUR]->location & LOC_SYSTEM));
	} else {
		// Info labels
		char *textPtr = txtbuffer;
		textPtr = stpcpy(textPtr, "Total: ");
		textPtr += formatBytes(textPtr, info->totalSpace, 0, info->metric);
		textPtr = stpcpy(textPtr, " | Free: ");
		textPtr += formatBytes(textPtr, info->freeSpace, 0, info->metric);
		textPtr = stpcpy(textPtr, " | Used: ");
		textPtr += formatBytes(textPtr, info->totalSpace - info->freeSpace, 0, info->metric);
	}
	
	int starting_pos = getVideoMode()->fbWidth-50-GetTextSizeInPixels(txtbuffer)*0.5;
	uiDrawObj_t *devInfoLabel = DrawStyledLabel(starting_pos, 400, txtbuffer, 0.5f, false, defaultColor);
	DrawAddChild(containerPanel, devInfoLabel);
}

// Draws all the files in the current dir.
void drawFilesCarousel(file_handle** directory, int num_files, uiDrawObj_t *containerPanel) {
	int i = 0;
	current_view_start = MAX(0,curSelection-FILES_PER_PAGE_CAROUSEL/2);
	current_view_end = MIN(num_files,curSelection+(FILES_PER_PAGE_CAROUSEL+1)/2);
	drawCurrentDeviceCarousel(containerPanel);
	if(num_files > 0) {
		// Draw which directory we're in
		sprintf(txtbuffer, "%s", getDevicePath(&curDir.name[0]));
		float scale = GetTextScaleToFitInWidthWithMax(txtbuffer, (getVideoMode()->fbWidth-60), .85);
		DrawAddChild(containerPanel, DrawStyledLabel(30, 80, txtbuffer, scale, false, defaultColor));
		if(!strcmp(&swissSettings.autoload[0], &curDir.name[0])
		|| !fnmatch(&swissSettings.autoload[0], &curDir.name[0], FNM_PATHNAME)) {
			DrawAddChild(containerPanel, DrawImage(TEX_STAR, ((getVideoMode()->fbWidth-30)-16), 80, 16, 16, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		}
		//int left_num = curSelection - current_view_start; // Number of entries to the left
		//int right_num = (current_view_end - curSelection)-1;
		//print_gecko("%i entries to the left, %i to the right in this view\r\n", left_num, right_num);
		//print_gecko("%i cur sel, %i start, %i end\r\n", curSelection, current_view_start, current_view_end);
		
		bool parentLink = (directory[curSelection]->fileType==IS_SPECIAL);
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
		for(i = current_view_start; i < curSelection; i++) {
			lockFile(directory[i]);
			populate_meta(directory[i]);
			browserObject = DrawFileCarouselEntry(left_x_base + ((sub_entry_width*(i-curSelection))), y_base + 10,
									left_x_base + ((sub_entry_width*(i-curSelection))+sub_entry_width), y_base + 10 + sub_entry_height,
									getRelativePath(directory[i]->name, curDir.name),
									directory[i], i - curSelection);
			directory[i]->uiObj = browserObject;
			unlockFile(directory[i]);
			DrawAddChild(containerPanel, browserObject);
		}
		
		// Main entry
		lockFile(directory[curSelection]);
		populate_meta(directory[curSelection]);
		browserObject = DrawFileCarouselEntry(((getVideoMode()->fbWidth / 2) - (main_entry_width / 2)), y_base,
								((getVideoMode()->fbWidth / 2) + (main_entry_width / 2)), y_base + main_entry_height,
								getRelativePath(directory[curSelection]->name, curDir.name),
								directory[curSelection], 0);
		directory[curSelection]->uiObj = browserObject;
		unlockFile(directory[curSelection]);
		DrawAddChild(containerPanel, browserObject);
		
		// Right spineart entries
		for(i = curSelection+1; i < current_view_end; i++) {
			lockFile(directory[i]);
			populate_meta(directory[i]);
			browserObject = DrawFileCarouselEntry(right_x_base + ((sub_entry_width*(i-curSelection-1))), y_base + 10,
									right_x_base + ((sub_entry_width*(i-curSelection-1))+sub_entry_width), y_base + 10 + sub_entry_height,
									getRelativePath(directory[i]->name, curDir.name),
									directory[i], i - curSelection);
			directory[i]->uiObj = browserObject;
			unlockFile(directory[i]);
			DrawAddChild(containerPanel, browserObject);
		}
	}
}

// Carousel (one main file in the middle, entries to either side)
uiDrawObj_t* renderFileCarousel(file_handle** directory, int num_files, uiDrawObj_t* filePanel)
{
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) {
		memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
		needsRefresh=1;
		return filePanel;
	}
	if(curSelection == 0 && num_files > 1 && directory[0]->fileType==IS_SPECIAL) {
		curSelection = 1; // skip the ".." by default
	}
	uiDrawObj_t *loadingBox = DrawProgressLoading(PROGRESS_BOX_TOPRIGHT);
	DrawPublish(loadingBox);
	meta_thread_start(loadingBox);
	while(1) {
		DrawUpdateProgressLoading(loadingBox, +1);
		uiDrawObj_t *newPanel = DrawContainer();
		drawFilesCarousel(directory, num_files, newPanel);
		if(filePanel != NULL) {
			DrawDispose(filePanel);
		}
		filePanel = newPanel;
		DrawPublish(filePanel);
		DrawUpdateProgressLoading(loadingBox, -1);
		
		u32 waitButtons = PAD_BUTTON_X|PAD_BUTTON_START|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_TRIGGER_Z;
		while ((padsStickX() > -16 && padsStickX() < 16) && !(padsButtonsHeld() & waitButtons))
			{ VIDEO_WaitVSync (); }
		if((padsButtonsHeld() & PAD_BUTTON_LEFT) || padsStickX() <= -16){	curSelection = (--curSelection < 0) ? num_files-1 : curSelection;}
		if((padsButtonsHeld() & PAD_BUTTON_RIGHT) || padsStickX() >= 16) {curSelection = (curSelection + 1) % num_files;	}
		if(padsButtonsHeld() & (PAD_BUTTON_UP|PAD_TRIGGER_L)) {
			if(curSelection == 0) {
				curSelection = num_files-1;
			}
			else {
				curSelection = (curSelection - FILES_PER_PAGE_CAROUSEL < 0) ? 0 : curSelection - FILES_PER_PAGE_CAROUSEL;
			}
		}
		if(padsButtonsHeld() & (PAD_BUTTON_DOWN|PAD_TRIGGER_R)) {
			if(curSelection == num_files-1) {
				curSelection = 0;
			}
			else {
				curSelection = (curSelection + FILES_PER_PAGE_CAROUSEL > num_files-1) ? num_files-1 : (curSelection + FILES_PER_PAGE_CAROUSEL) % num_files;
			}
		}
		
		if((padsButtonsHeld() & PAD_BUTTON_A)) {
			lockFile(directory[curSelection]);
			//go into a folder or select a file
			if(directory[curSelection]->fileType==IS_DIR) {
				memcpy(&curDir, directory[curSelection], sizeof(file_handle));
				needsRefresh=1;
			}
			else if(directory[curSelection]->fileType==IS_SPECIAL){
				memcpy(&curFile, &curDir, sizeof(file_handle));
				curDir.fileBase = directory[curSelection]->fileBase;
				needsDeviceChange = upToParent(&curDir);
				needsRefresh=1;
			}
			else if(directory[curSelection]->fileType==IS_FILE){
				memcpy(&curFile, directory[curSelection], sizeof(file_handle));
				if(canLoadFileType(curFile.name, devices[DEVICE_CUR]->extraExtensions)) {
					meta_thread_stop();
					load_file();
				}
				else if(swissSettings.enableFileManagement) {
					meta_thread_stop();
					needsRefresh = manage_file() ? 1:0;
				}
				memcpy(directory[curSelection], &curFile, sizeof(file_handle));
			}
			unlockFile(directory[curSelection]);
			break;
		}
		if(padsButtonsHeld() & PAD_BUTTON_X) {
			memcpy(&curFile, &curDir, sizeof(file_handle));
			curDir.fileBase = directory[0]->fileBase;
			needsDeviceChange = upToParent(&curDir);
			needsRefresh=1;
			while(padsButtonsHeld() & PAD_BUTTON_X) VIDEO_WaitVSync();
			break;
		}
		if((padsButtonsHeld() & PAD_TRIGGER_Z) && swissSettings.enableFileManagement) {
			lockFile(directory[curSelection]);
			if(directory[curSelection]->fileType == IS_FILE || directory[curSelection]->fileType == IS_DIR) {
				memcpy(&curFile, directory[curSelection], sizeof(file_handle));
				meta_thread_stop();
				needsRefresh = manage_file() ? 1:0;
				memcpy(directory[curSelection], &curFile, sizeof(file_handle));
				while(padsButtonsHeld() & PAD_BUTTON_B) VIDEO_WaitVSync();
				if(needsRefresh) {
					// If we return from doing something with a file, refresh the device in the same dir we were at
					unlockFile(directory[curSelection]);
					break;
				}
			}
			else if(directory[curSelection]->fileType == IS_SPECIAL) {
				// Toggle autoload
				if(!strcmp(&swissSettings.autoload[0], &curDir.name[0])
				|| !fnmatch(&swissSettings.autoload[0], &curDir.name[0], FNM_PATHNAME)) {
					memset(&swissSettings.autoload[0], 0, PATHNAME_MAX);
				}
				else {
					strcpy(&swissSettings.autoload[0], &curDir.name[0]);
				}
				// Save config
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving autoload\205"));
				config_update_global(true);
				DrawDispose(msgBox);
			}
			unlockFile(directory[curSelection]);
		}
		
		if(padsButtonsHeld() & PAD_BUTTON_B) {
			curMenuLocation = ON_OPTIONS;
			DrawUpdateFileBrowserButton(directory[curSelection]->uiObj, (curMenuLocation == ON_FILLIST) ? B_SELECTED:B_NOSELECT);
			break;
		}
		if((padsButtonsHeld() & PAD_BUTTON_START) && swissSettings.recentListLevel > 0) {
			meta_thread_stop();
			select_recent_entry();
			break;
		}
		if(padsStickX() <= -16 || padsStickX() >= 16) {
			usleep((abs(padsStickX()) > 64 ? 50000:100000) - abs(padsStickX()*64));
		}
		else {
			while (padsButtonsHeld() & waitButtons)
				{ VIDEO_WaitVSync (); }
		}
	}
	meta_thread_stop();
	DrawDispose(loadingBox);
	return filePanel;
}

// Draws all the files in the current dir.
void drawFilesFullwidth(file_handle** directory, int num_files, uiDrawObj_t *containerPanel) {
	int i = 0, j = 0;
	current_view_start = MIN(MAX(0,curSelection-FILES_PER_PAGE_FULLWIDTH/2),MAX(0,num_files-FILES_PER_PAGE_FULLWIDTH));
	current_view_end = MIN(num_files, MAX(curSelection+(FILES_PER_PAGE_FULLWIDTH+1)/2,FILES_PER_PAGE_FULLWIDTH));
	drawCurrentDeviceCarousel(containerPanel);
	int fileListBase = 105;
	int scrollBarHeight = (FILES_PER_PAGE_FULLWIDTH*40);
	int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
	if(num_files > 0) {
		// Draw which directory we're in
		sprintf(txtbuffer, "%s", getDevicePath(&curDir.name[0]));
		float scale = GetTextScaleToFitInWidthWithMax(txtbuffer, (getVideoMode()->fbWidth-60), .85);
		DrawAddChild(containerPanel, DrawStyledLabel(30, 80, txtbuffer, scale, false, defaultColor));
		if(!strcmp(&swissSettings.autoload[0], &curDir.name[0])
		|| !fnmatch(&swissSettings.autoload[0], &curDir.name[0], FNM_PATHNAME)) {
			DrawAddChild(containerPanel, DrawImage(TEX_STAR, ((getVideoMode()->fbWidth-30)-16), 80, 16, 16, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		}
		if(num_files > FILES_PER_PAGE_FULLWIDTH) {
			uiDrawObj_t *scrollBar = DrawVertScrollBar(getVideoMode()->fbWidth-25, fileListBase, 16, scrollBarHeight, (float)((float)curSelection/(float)(num_files-1)),scrollBarTabHeight);
			DrawAddChild(containerPanel, scrollBar);
		}
		for(i = current_view_start,j = 0; i<current_view_end; ++i,++j) {
			lockFile(directory[i]);
			populate_meta(directory[i]);
			uiDrawObj_t *browserButton = DrawFileBrowserButtonMeta(30, fileListBase+(j*40),
									getVideoMode()->fbWidth-30, fileListBase+(j*40)+40,
									getRelativePath(directory[i]->name, curDir.name),
									directory[i],
									(i == curSelection) ? B_SELECTED:B_NOSELECT);
			directory[i]->uiObj = browserButton;
			unlockFile(directory[i]);
			DrawAddChild(containerPanel, browserButton);
		}
	}
}

uiDrawObj_t* renderFileFullwidth(file_handle** directory, int num_files, uiDrawObj_t* filePanel)
{
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) {
		memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
		needsRefresh=1;
		return filePanel;
	}
	if(curSelection == 0 && num_files > 1 && directory[0]->fileType==IS_SPECIAL) {
		curSelection = 1; // skip the ".." by default
	}
	uiDrawObj_t *loadingBox = DrawProgressLoading(PROGRESS_BOX_TOPRIGHT);
	DrawPublish(loadingBox);
	meta_thread_start(loadingBox);
	while(1) {
		DrawUpdateProgressLoading(loadingBox, +1);
		uiDrawObj_t *newPanel = DrawContainer();
		drawFilesFullwidth(directory, num_files, newPanel);
		if(filePanel != NULL) {
			DrawDispose(filePanel);
		}
		filePanel = newPanel;
		DrawPublish(filePanel);
		DrawUpdateProgressLoading(loadingBox, -1);
		
		u32 waitButtons = PAD_BUTTON_X|PAD_BUTTON_START|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_TRIGGER_Z;
		while ((padsStickY() > -16 && padsStickY() < 16) && !(padsButtonsHeld() & waitButtons))
			{ VIDEO_WaitVSync (); }
		if((padsButtonsHeld() & PAD_BUTTON_UP) || padsStickY() >= 16){	curSelection = (--curSelection < 0) ? num_files-1 : curSelection;}
		if((padsButtonsHeld() & PAD_BUTTON_DOWN) || padsStickY() <= -16) {curSelection = (curSelection + 1) % num_files;	}
		if(padsButtonsHeld() & (PAD_BUTTON_LEFT|PAD_TRIGGER_L)) {
			if(curSelection == 0) {
				curSelection = num_files-1;
			}
			else {
				curSelection = (curSelection - FILES_PER_PAGE_FULLWIDTH < 0) ? 0 : curSelection - FILES_PER_PAGE_FULLWIDTH;
			}
		}
		if(padsButtonsHeld() & (PAD_BUTTON_RIGHT|PAD_TRIGGER_R)) {
			if(curSelection == num_files-1) {
				curSelection = 0;
			}
			else {
				curSelection = (curSelection + FILES_PER_PAGE_FULLWIDTH > num_files-1) ? num_files-1 : (curSelection + FILES_PER_PAGE_FULLWIDTH) % num_files;
			}
		}
		
		if(padsButtonsHeld() & PAD_BUTTON_A) {
			lockFile(directory[curSelection]);
			//go into a folder or select a file
			if(directory[curSelection]->fileType==IS_DIR) {
				memcpy(&curDir, directory[curSelection], sizeof(file_handle));
				needsRefresh=1;
			}
			else if(directory[curSelection]->fileType==IS_SPECIAL) {
				memcpy(&curFile, &curDir, sizeof(file_handle));
				curDir.fileBase = directory[curSelection]->fileBase;
				needsDeviceChange = upToParent(&curDir);
				needsRefresh=1;
			}
			else if(directory[curSelection]->fileType==IS_FILE) {
				memcpy(&curFile, directory[curSelection], sizeof(file_handle));
				if(canLoadFileType(curFile.name, devices[DEVICE_CUR]->extraExtensions)) {
					meta_thread_stop();
					load_file();
				}
				else if(swissSettings.enableFileManagement) {
					meta_thread_stop();
					needsRefresh = manage_file() ? 1:0;
				}
				memcpy(directory[curSelection], &curFile, sizeof(file_handle));
			}
			unlockFile(directory[curSelection]);
			break;
		}
		if(padsButtonsHeld() & PAD_BUTTON_X) {
			memcpy(&curFile, &curDir, sizeof(file_handle));
			curDir.fileBase = directory[0]->fileBase;
			needsDeviceChange = upToParent(&curDir);
			needsRefresh=1;
			while(padsButtonsHeld() & PAD_BUTTON_X) VIDEO_WaitVSync();
			break;
		}
		if((padsButtonsHeld() & PAD_TRIGGER_Z) && swissSettings.enableFileManagement) {
			lockFile(directory[curSelection]);
			if(directory[curSelection]->fileType == IS_FILE || directory[curSelection]->fileType == IS_DIR) {
				memcpy(&curFile, directory[curSelection], sizeof(file_handle));
				meta_thread_stop();
				needsRefresh = manage_file() ? 1:0;
				memcpy(directory[curSelection], &curFile, sizeof(file_handle));
				while(padsButtonsHeld() & PAD_BUTTON_B) VIDEO_WaitVSync();
				if(needsRefresh) {
					// If we return from doing something with a file, refresh the device in the same dir we were at
					unlockFile(directory[curSelection]);
					break;
				}
			}
			else if(directory[curSelection]->fileType == IS_SPECIAL) {
				// Toggle autoload
				if(!strcmp(&swissSettings.autoload[0], &curDir.name[0])
				|| !fnmatch(&swissSettings.autoload[0], &curDir.name[0], FNM_PATHNAME)) {
					memset(&swissSettings.autoload[0], 0, PATHNAME_MAX);
				}
				else {
					strcpy(&swissSettings.autoload[0], &curDir.name[0]);
				}
				// Save config
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving autoload\205"));
				config_update_global(true);
				DrawDispose(msgBox);
			}
			unlockFile(directory[curSelection]);
		}
		
		if((padsButtonsHeld() & PAD_BUTTON_START) && swissSettings.recentListLevel > 0) {
			meta_thread_stop();
			select_recent_entry();
			break;
		}
		if(padsButtonsHeld() & PAD_BUTTON_B) {
			curMenuLocation = ON_OPTIONS;
			DrawUpdateFileBrowserButton(directory[curSelection]->uiObj, (curMenuLocation == ON_FILLIST) ? B_SELECTED:B_NOSELECT);
			break;
		}
		if(padsStickY() <= -16 || padsStickY() >= 16) {
			usleep((abs(padsStickY()) > 64 ? 50000:100000) - abs(padsStickY()*64));
		}
		else {
			while (padsButtonsHeld() & waitButtons)
				{ VIDEO_WaitVSync (); }
		}
	}
	meta_thread_stop();
	DrawDispose(loadingBox);
	return filePanel;
}

bool select_dest_dir(file_handle* initial, file_handle* selection)
{
	file_handle **directory = NULL;
	file_handle *curDirEntries = NULL;
	file_handle curDir;
	memcpy(&curDir, initial, sizeof(file_entry));
	int i = 0, j = 0, max = 0, refresh = 1, num_files =0, idx = 0;
	
	bool cancelled = false;
	int fileListBase = 90;
	int scrollBarHeight = (FILES_PER_PAGE*40);
	int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
	uiDrawObj_t* destDirBox = NULL;
	while(1){
		// Read the directory
		if(refresh) {
			free(directory);
			free(curDirEntries);
			curDirEntries = NULL;
			num_files = devices[DEVICE_DEST]->readDir(&curDir, &curDirEntries, IS_DIR);
			num_files = sortFiles(curDirEntries, num_files, &directory);
			if(num_files <= 1 && destDirBox == NULL) {
				memcpy(selection, &curDir, sizeof(file_handle));
				break;
			}
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
			DrawAddChild(tempBox, DrawSelectableButton(50,fileListBase+(j*40), getVideoMode()->fbWidth-35, fileListBase+(j*40)+40, getRelativeName(directory[i]->name), (i == idx) ? B_SELECTED:B_NOSELECT));
		}
		if(destDirBox != NULL) {
			DrawDispose(destDirBox);
		}
		destDirBox = tempBox;
		DrawPublish(destDirBox);
		while ((padsStickY() > -16 && padsStickY() < 16) && !(padsButtonsHeld() & (PAD_BUTTON_X|PAD_BUTTON_A|PAD_BUTTON_B|PAD_BUTTON_UP|PAD_BUTTON_DOWN)))
			{ VIDEO_WaitVSync (); }
		if((padsButtonsHeld() & PAD_BUTTON_UP) || padsStickY() > 16){	idx = (--idx < 0) ? num_files-1 : idx;}
		if((padsButtonsHeld() & PAD_BUTTON_DOWN) || padsStickY() < -16) {idx = (idx + 1) % num_files;	}
		if((padsButtonsHeld() & PAD_BUTTON_A))	{
			//go into a folder or select a file
			if(directory[idx]->fileType==IS_DIR) {
				memcpy(&curDir, directory[idx], sizeof(file_handle));
				refresh=1;
			}
			else if(directory[idx]->fileType==IS_SPECIAL){
				curDir.fileBase = directory[idx]->fileBase;
				upToParent(&curDir);
				refresh=1;
			}
		}
		if(padsStickY() < -16 || padsStickY() > 16) {
			usleep(50000 - abs(padsStickY()*256));
		}
		if(padsButtonsHeld() & PAD_BUTTON_X)	{
			memcpy(selection, &curDir, sizeof(file_handle));
			break;
		}
		if(padsButtonsHeld() & PAD_BUTTON_B)	{
			cancelled = true;
			break;
		}
		while (!(!(padsButtonsHeld() & PAD_BUTTON_X) && !(padsButtonsHeld() & (PAD_BUTTON_X|PAD_BUTTON_A|PAD_BUTTON_B|PAD_BUTTON_UP|PAD_BUTTON_DOWN))))
			{ VIDEO_WaitVSync (); }
	}
	if(destDirBox != NULL) {
		DrawDispose(destDirBox);
	}
	free(curDirEntries);
	free(directory);
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
	
	if(in_range(a->type, PATCH_BIN, PATCH_ELF) && !in_range(b->type, PATCH_BIN, PATCH_ELF))
		return -1;
	if(!in_range(a->type, PATCH_BIN, PATCH_ELF) && in_range(b->type, PATCH_BIN, PATCH_ELF))
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
		if(!in_range(filesToPatch[i].type, PATCH_BIN, PATCH_ELF)) {
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
		while ((padsStickY() > -16 && padsStickY() < 16) && !(padsButtonsHeld() & PAD_BUTTON_B) && !(padsButtonsHeld() & PAD_BUTTON_A) && !(padsButtonsHeld() & PAD_BUTTON_UP) && !(padsButtonsHeld() & PAD_BUTTON_DOWN))
			{ VIDEO_WaitVSync (); }
		if((padsButtonsHeld() & PAD_BUTTON_UP) || padsStickY() > 16){	idx = (--idx < 0) ? num_files-1 : idx;}
		if((padsButtonsHeld() & PAD_BUTTON_DOWN) || padsStickY() < -16) {idx = (idx + 1) % num_files;	}
		if((padsButtonsHeld() & PAD_BUTTON_A))	break;
		if((padsButtonsHeld() & PAD_BUTTON_B))	{ idx = -1; break; }
		if(padsStickY() < -16 || padsStickY() > 16) {
			usleep(50000 - abs(padsStickY()*256));
		}
		while (!(!(padsButtonsHeld() & PAD_BUTTON_B) && !(padsButtonsHeld() & PAD_BUTTON_A) && !(padsButtonsHeld() & PAD_BUTTON_UP) && !(padsButtonsHeld() & PAD_BUTTON_DOWN)))
			{ VIDEO_WaitVSync (); }
	}
	DrawDispose(container);
	return idx >= 0 ? &filesToPatch[idx] : NULL;
	
}

void load_app(ExecutableFile *fileToPatch)
{
	uiDrawObj_t* progBox = NULL;
	const char* message = NULL;
	char* gameID = VAR_AREA;
	void* buffer;
	u32 sizeToRead;
	int type;
	char* argz = NULL;
	size_t argz_len = 0;
	
	// Get top of memory
	u32 topAddr = getTopAddr();
	if (topAddr < 0x81700000) {
		topAddr = 0x81800000;
	}
	print_gecko("Top of RAM simulated as: 0x%08X\r\n", topAddr);
	
	*(vu32*)(VAR_AREA+0x0028) = 0x01800000;
	*(vu32*)(VAR_AREA+0x002C) = (swissSettings.debugUSB ? 0x10000004:0x00000001) + (*(vu32*)0xCC00302C >> 28);
	*(vu32*)(VAR_AREA+0x00CC) = swissSettings.sramVideo;
	*(vu32*)(VAR_AREA+0x00D0) = 0x01000000;
	*(vu32*)(VAR_AREA+0x00E8) = 0x81800000 - topAddr;
	*(vu32*)(VAR_AREA+0x00EC) = topAddr;
	*(vu32*)(VAR_AREA+0x00F0) = 0x01800000;
	*(vu32*)(VAR_AREA+0x00F8) = TB_BUS_CLOCK;
	*(vu32*)(VAR_AREA+0x00FC) = TB_CORE_CLOCK;
	
	// Copy the game header to 0x80000000
	memcpy(VAR_AREA,(void*)&GCMDisk,0x20);
	
	if(fileToPatch != NULL && fileToPatch->file != NULL) {
		argz = getExternalPath(fileToPatch->file->name);
		argz_len = strlen(argz) + 1;
		
		// For a DOL from a TGC, redirect the FST to the TGC FST.
		if(fileToPatch->tgcBase + fileToPatch->tgcFileStartArea != 0) {
			// Read FST to top of Main Memory (round to 32 byte boundary)
			u32 fstAddr = (topAddr-fileToPatch->fstSize)&~31;
			u32 fstSize = (fileToPatch->fstSize+31)&~31;
			devices[DEVICE_CUR]->seekFile(fileToPatch->file,fileToPatch->fstOffset,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_CUR]->readFile(fileToPatch->file,(void*)fstAddr,fstSize) != fstSize) {
				message = "Failed to read FST!";
				goto fail_early;
			}
			adjust_tgc_fst((void*)fstAddr, fileToPatch->tgcBase, fileToPatch->tgcFileStartArea, fileToPatch->tgcFakeOffset);
			
			// Copy bi2.bin (Disk Header Information) to just under the FST
			u32 bi2Addr = (fstAddr-0x2000)&~31;
			memcpy((void*)bi2Addr,(void*)&GCMDisk+0x440,0x2000);
			
			// Patch bi2.bin
			Patch_GameSpecificFile((void*)bi2Addr, 0x2000, gameID, "bi2.bin");
			
			*(vu32*)(VAR_AREA+0x0020) = 0x0D15EA5E;
			*(vu32*)(VAR_AREA+0x0024) = 1;
			*(vu32*)(VAR_AREA+0x0034) = fstAddr;								// Arena Hi
			*(vu32*)(VAR_AREA+0x0038) = fstAddr;								// FST Location in ram
			*(vu32*)(VAR_AREA+0x003C) = fileToPatch->fstSize;					// FST Max Length
			*(vu32*)(VAR_AREA+0x00F4) = bi2Addr;								// bi2.bin location
			*(vu32*)(VAR_AREA+0x30F4) = fileToPatch->tgcBase;
		}
		else {
			// Read FST to top of Main Memory (round to 32 byte boundary)
			u32 fstAddr = (topAddr-GCMDisk.MaxFSTSize)&~31;
			u32 fstSize = (fileToPatch->fstSize+31)&~31;
			devices[DEVICE_CUR]->seekFile(fileToPatch->file,fileToPatch->fstOffset,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_CUR]->readFile(fileToPatch->file,(void*)fstAddr,fstSize) != fstSize) {
				message = "Failed to read FST!";
				goto fail_early;
			}
			
			// Copy bi2.bin (Disk Header Information) to just under the FST
			u32 bi2Addr = (fstAddr-0x2000)&~31;
			memcpy((void*)bi2Addr,(void*)&GCMDisk+0x440,0x2000);
			
			// Patch bi2.bin
			Patch_GameSpecificFile((void*)bi2Addr, 0x2000, gameID, "bi2.bin");
			
			*(vu32*)(VAR_AREA+0x0020) = 0x0D15EA5E;
			*(vu32*)(VAR_AREA+0x0024) = 1;
			*(vu32*)(VAR_AREA+0x0034) = fstAddr;								// Arena Hi
			*(vu32*)(VAR_AREA+0x0038) = fstAddr;								// FST Location in ram
			*(vu32*)(VAR_AREA+0x003C) = GCMDisk.MaxFSTSize;						// FST Max Length
			*(vu32*)(VAR_AREA+0x00F4) = bi2Addr;								// bi2.bin location
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
		if(buffer == NULL) goto fail;
		
		if(fileToPatch->patchFile != NULL) {
			devices[DEVICE_PATCHES]->seekFile(fileToPatch->patchFile,0,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_PATCHES]->readFile(fileToPatch->patchFile,buffer,sizeToRead) != sizeToRead) {
				message = "Failed to read DOL!";
				goto fail;
			}
			
			XXH128_hash_t old_hash, new_hash = XXH3_128bits(buffer, sizeToRead);
			if(devices[DEVICE_PATCHES]->readFile(fileToPatch->patchFile, &old_hash, sizeof(old_hash)) != sizeof(old_hash) ||
				!XXH128_isEqual(old_hash, new_hash)) {
				devices[DEVICE_PATCHES]->deleteFile(fileToPatch->patchFile);
				sprintf(txtbuffer, "Failed integrity check in patched file!\nPlease test %s for defects.", devices[DEVICE_PATCHES]->deviceName);
				message = txtbuffer;
				goto fail;
			}
		}
		else {
			devices[DEVICE_CUR]->seekFile(fileToPatch->file,fileToPatch->offset,DEVICE_HANDLER_SEEK_SET);
			if(devices[DEVICE_CUR]->readFile(fileToPatch->file,buffer,sizeToRead) != sizeToRead) {
				message = "Failed to read DOL!";
				goto fail;
			}
			
			fileToPatch->hash = XXH3_64bits(buffer, sizeToRead);
			if(!valid_file_xxh3(&GCMDisk, fileToPatch)) {
				message = "Failed integrity check!";
				goto fail;
			}
			gameID_set(&GCMDisk, fileToPatch->hash);
		}
		
		u8 *oldBuffer = buffer, *newBuffer = NULL;
		if(type == PATCH_DOL_PRS || type == PATCH_OTHER_PRS) {
			int ret = pso_prs_decompress_buf(buffer, &newBuffer, fileToPatch->size);
			if(ret < 0) {
				message = "Failed to decompress DOL!";
				goto fail;
			}
			sizeToRead = ret;
			buffer = newBuffer;
			newBuffer = NULL;
			free(oldBuffer);
			oldBuffer = NULL;
		}
	}
	else {
		if(devices[DEVICE_PATCHES] && devices[DEVICE_PATCHES] != devices[DEVICE_CUR]) {
			sprintf(txtbuffer, "Loading BS2\nDo not remove %s", devices[DEVICE_PATCHES]->deviceName);
			progBox = DrawPublish(DrawProgressBar(true, 0, txtbuffer));

			if(!load_rom_ipl(devices[DEVICE_PATCHES], &buffer, &sizeToRead) &&
				!load_rom_ipl(devices[DEVICE_CUR], &buffer, &sizeToRead) &&
				!load_rom_ipl(&__device_sys, &buffer, &sizeToRead)) {
				message = "Failed to read BS2!";
				goto fail;
			}
		}
		else {
			progBox = DrawPublish(DrawProgressBar(true, 0, "Loading BS2"));

			if(!load_rom_ipl(devices[DEVICE_CUR], &buffer, &sizeToRead) &&
				!load_rom_ipl(&__device_sys, &buffer, &sizeToRead)) {
				message = "Failed to read BS2!";
				goto fail;
			}
		}
		type = PATCH_BS2;

		if(fileToPatch != NULL) {
			fileToPatch->size = sizeToRead;
			fileToPatch->hash = XXH3_64bits(buffer, sizeToRead);
			if(!valid_file_xxh3(&GCMDisk, fileToPatch)) {
				message = "Unknown BS2";
				goto fail;
			}
		}
	}
	
	if(getTopAddr() == topAddr) {
		if(type == PATCH_BS2) setTopAddr(0);
		else setTopAddr(HI_RESERVE);
	}
	if(fileToPatch == NULL || fileToPatch->patchFile == NULL) {
		Patch_ExecutableFile(&buffer, &sizeToRead, gameID, type);
	}
	if(getTopAddr() == 0) {
		setTopAddr(HI_RESERVE);
		*(vu32*)(VAR_AREA+0x0034) = (u32)SYS_GetArenaHi() & ~31;
	}
	
	// See if the combination of our patches has exhausted our play area.
	if(!install_code(0)) {
		message = "Exhausted reserved memory.\nAn SD Card Adapter is necessary in order\nfor patches to reserve additional memory.";
		goto fail;
	}
	setTopAddr(topAddr);
	
	if(swissSettings.wiirdEngine) {
		kenobi_install_engine();
	}
	
	if(devices[DEVICE_PATCHES] && !(devices[DEVICE_PATCHES]->quirks & QUIRK_NO_DEINIT)) {
		devices[DEVICE_PATCHES]->deinit(devices[DEVICE_PATCHES]->initial);
	}
	// Don't spin down the drive when running something from it...
	if(!(devices[DEVICE_CUR]->quirks & QUIRK_NO_DEINIT)) {
		devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
	}
	if(devices[DEVICE_CUR]->location & LOC_DVD_CONNECTOR) {
		// Check DVD Status, make sure it's error code 0
		print_gecko("DVD: %08X\r\n",dvd_get_error());
	}
	else if(swissSettings.hasFlippyDrive) {
		flippy_bypass(false);
		flippy_reset();
	}
	
	DrawDispose(progBox);
	DrawShutdown();
	
	VIDEO_SetPostRetraceCallback(NULL);
	VIDEO_SetBlack(true);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	
	print_gecko("libogc shutdown and boot game!\r\n");
	if(devices[DEVICE_CUR] == &__device_sd_a || devices[DEVICE_CUR] == &__device_sd_b || devices[DEVICE_CUR] == &__device_sd_c) {
		s32 exi_channel;
		if(getExiDeviceByLocation(devices[DEVICE_CUR]->location, &exi_channel, NULL)) {
			sdgecko_setPageSize(exi_channel, 512);
			sdgecko_enableCRC(exi_channel, false);
			print_gecko("set size\r\n");
		}
	}
	else if(devices[DEVICE_PATCHES] == &__device_sd_a || devices[DEVICE_PATCHES] == &__device_sd_b || devices[DEVICE_PATCHES] == &__device_sd_c) {
		s32 exi_channel;
		if(getExiDeviceByLocation(devices[DEVICE_PATCHES]->location, &exi_channel, NULL)) {
			sdgecko_setPageSize(exi_channel, 512);
			sdgecko_enableCRC(exi_channel, false);
			print_gecko("set size\r\n");
		}
	}
	if(type == PATCH_BS2) {
		BINtoARAM(buffer, sizeToRead, 0x81300000, 0x812FFFE0);
	}
	else if(type == PATCH_BIN) {
		BINtoARAM(buffer, sizeToRead, 0x80003100, 0x80003100);
	}
	else if(type == PATCH_DOL || type == PATCH_DOL_PRS) {
		DOLtoARAM(buffer, argz, argz_len);
	}
	else if(type == PATCH_ELF) {
		ELFtoARAM(buffer, argz, argz_len);
	}
	SYS_ResetSystem(SYS_HOTRESET, 0, FALSE);
	__builtin_unreachable();

fail:
	DrawDispose(progBox);
	free(buffer);
fail_early:
	free(argz);
	if(message) {
		uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_FAIL, message));
		wait_press_A();
		DrawDispose(msgBox);
	}
}

void boot_dol(file_handle* file, int argc, char *argv[])
{
	void *buffer = memalign(32, file->size);
	if(!buffer) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"DOL is too big. Press A.");
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return;
	}
		
	int i=0;
	void *ptr = buffer;
	uiDrawObj_t* progBar = DrawProgressBar(false, 0, "Loading DOL");
	DrawPublish(progBar);
	for(i = 0; i < file->size; i+= 131072) {
		DrawUpdateProgressBar(progBar, (int)((float)((float)i/(float)file->size)*100));
		
		devices[DEVICE_CUR]->seekFile(file,i,DEVICE_HANDLER_SEEK_SET);
		int size = i+131072 > file->size ? file->size-i : 131072; 
		if(devices[DEVICE_CUR]->readFile(file,ptr,size)!=size) {
			DrawDispose(progBar);
			free(buffer);
			devices[DEVICE_CUR]->closeFile(file);
			uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"Failed to read DOL. Press A.");
			DrawPublish(msgBox);
			wait_press_A();
			DrawDispose(msgBox);
			return;
		}
  		ptr+=size;
	}
	
	XXH64_hash_t hash = XXH3_64bits(buffer, file->size);
	if(!valid_dol_xxh3(file, hash)) {
		DrawDispose(progBar);
		free(buffer);
		devices[DEVICE_CUR]->closeFile(file);
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,"DOL is corrupted. Press A.");
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return;
	}
	gameID_set(NULL, hash);
	DrawDispose(progBar);
	
	if(devices[DEVICE_CONFIG] != NULL) {
		// Update the recent list.
		if(update_recent()) {
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving recent list\205"));
			config_update_recent(true);
			DrawDispose(msgBox);
		}
	}

	char fileName[PATHNAME_MAX];
	memset(fileName, 0, PATHNAME_MAX);
	strncpy(fileName, file->name, strrchr(file->name, '.') - file->name);
	print_gecko("DOL file name without extension [%s]\r\n", fileName);

	// .iso disc image file
	file_handle *imageFile = calloc(1, sizeof(file_handle));
	snprintf(imageFile->name, PATHNAME_MAX, "%s.iso", fileName);

	if(!fnmatch("*/apps/*/*", file->name, FNM_PATHNAME | FNM_CASEFOLD | FNM_LEADING_DIR)) {
		getParentPath(imageFile->name, imageFile->name);
		concat_path(imageFile->name, imageFile->name, "data.iso");
	}
	if(devices[DEVICE_CUR]->statFile) {
		devices[DEVICE_CUR]->statFile(imageFile);
	}

	file_handle *bootFile = NULL;
	if(devices[DEVICE_CUR] == &__device_gcloader) {
		bootFile = calloc(1, sizeof(file_handle));

		if(!gcloaderGetBootFile(bootFile)) {
			free(bootFile);
			bootFile = NULL;
		}
	}

	// Build a command line to pass to the DOL
	char *argz = getExternalPath(imageFile->fileType == IS_FILE ? imageFile->name : file->name);
	size_t argz_len = strlen(argz) + 1;

	// .cli argument file
	file_handle *cliArgFile = calloc(1, sizeof(file_handle));
	snprintf(cliArgFile->name, PATHNAME_MAX, "%s.cli", fileName);
	
	// we found something, use parameters (.cli)
	if(devices[DEVICE_CUR]->readFile(cliArgFile, NULL, 0) == 0 && cliArgFile->size) {
		print_gecko("Argument file found [%s]\r\n", cliArgFile->name);
		char *cli_buffer = calloc(1, cliArgFile->size + 1);
		if(cli_buffer) {
			devices[DEVICE_CUR]->seekFile(cliArgFile, 0, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CUR]->readFile(cliArgFile, cli_buffer, cliArgFile->size);

			// Parse CLI
			char *line, *linectx = NULL;
			line = strtok_r(cli_buffer, "\r\n", &linectx);
			while(line != NULL) {
				argz_add(&argz, &argz_len, line);
				line = strtok_r(NULL, "\r\n", &linectx);
			}

			free(cli_buffer);
		}
	}
	devices[DEVICE_CUR]->closeFile(cliArgFile);
	free(cliArgFile);

	// .dcp parameter file
	file_handle *dcpArgFile = calloc(1, sizeof(file_handle));
	snprintf(dcpArgFile->name, PATHNAME_MAX, "%s.dcp", fileName);
	
	// we found something, parse and display parameters for selection (.dcp)
	if(devices[DEVICE_CUR]->readFile(dcpArgFile, NULL, 0) == 0 && dcpArgFile->size) {
		print_gecko("Argument file found [%s]\r\n", dcpArgFile->name);
		char *dcp_buffer = calloc(1, dcpArgFile->size + 1);
		if(dcp_buffer) {
			devices[DEVICE_CUR]->seekFile(dcpArgFile, 0, DEVICE_HANDLER_SEEK_SET);
			devices[DEVICE_CUR]->readFile(dcpArgFile, dcp_buffer, dcpArgFile->size);

			// Parse DCP
			parseParameters(dcp_buffer);
			free(dcp_buffer);

			Parameters *params = (Parameters*)getParameters();
			if(params->num_params > 0) {
				DrawArgsSelector(getRelativeName(file->name));
				// Get an argv back or none.
				populateArgz(&argz, &argz_len);
			}
		}
	}
	devices[DEVICE_CUR]->closeFile(dcpArgFile);
	free(dcpArgFile);

	for(i = 1; i < argc; i++) {
		argz_add(&argz, &argz_len, argv[i]);
	}

	// Boot
	if(devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR) {
		devices[DEVICE_CUR]->setupFile(imageFile, bootFile, NULL, -1);
	}

	if(!memcmp(buffer, ELFMAG, SELFMAG)) {
		ELFtoARAM(buffer, argz, argz_len);
	}
	else if(endsWith(file->name, "/SDLOADER.BIN")) {
		BINtoARAM(buffer, file->size, 0x81700000, 0x81700000);
	}
	else if(branchResolve(buffer, PATCH_BIN, 0)) {
		BINtoARAM(buffer, file->size, 0x80003100, 0x80003100);
	}
	else {
		DOLtoARAM(buffer, argz, argz_len);
	}
	free(argz);
	free(buffer);

	devices[DEVICE_CUR]->closeFile(bootFile);
	devices[DEVICE_CUR]->closeFile(imageFile);
	free(bootFile);
	free(imageFile);
}

bool copy_file(file_handle *srcFile, file_handle *destFile, int option, bool silent) {
    // Read from one file and write to the new directory
	u32 isDestCard = devices[DEVICE_DEST] == &__device_card_a || devices[DEVICE_DEST] == &__device_card_b;
	u32 isSrcCard = devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b;
	u32 bulkWrite = isSrcCard || isDestCard || devices[DEVICE_DEST] == &__device_qoob;
	u32 curOffset = srcFile->offset, cancelled = 0, chunkSize = bulkWrite ? srcFile->size - curOffset : (256*1024);
	char *readBuffer = (char*)memalign(32,chunkSize);
	sprintf(txtbuffer, "Copying to: %s",getRelativeName(destFile->name));
	uiDrawObj_t* progBar = DrawProgressBar(false, 0, txtbuffer);
	DrawPublish(progBar);

	u64 startTime = gettime();
	u64 lastTime = gettime();
	u32 lastOffset = 0;
	int speed = 0;
	int timeremain = 0;
	print_gecko("Copying %i byte file from %s to %s\r\n", srcFile->size, srcFile->name, destFile->name);

	while(curOffset < srcFile->size) {
		u32 buttons = padsButtonsHeld();
		if(buttons & PAD_BUTTON_B) {
			cancelled = 1;
			break;
		}
		u32 timeDiff = diff_msec(lastTime, gettime());
		u32 timeStart = diff_msec(startTime, gettime());
		if(timeDiff >= 1000) {
			speed = (int)((float)(curOffset-lastOffset) / (float)(timeDiff/1000.0f));
			timeremain = (srcFile->size - curOffset) / speed;
			lastTime = gettime();
			lastOffset = curOffset;
		}
		DrawUpdateProgressBarDetail(progBar, (int)((float)((float)curOffset/(float)srcFile->size)*100), speed, timeStart/1000, timeremain);
		u32 amountToCopy = curOffset + chunkSize > srcFile->size ? srcFile->size - curOffset : chunkSize;
		devices[DEVICE_CUR]->seekFile(srcFile, curOffset, DEVICE_HANDLER_SEEK_SET);
		
		u32 ret = devices[DEVICE_CUR]->readFile(srcFile, readBuffer, amountToCopy);
		if(ret != amountToCopy) {	// Retry the read.
			devices[DEVICE_CUR]->seekFile(srcFile, curOffset, DEVICE_HANDLER_SEEK_SET);
			ret = devices[DEVICE_CUR]->readFile(srcFile, readBuffer, amountToCopy);
			if(ret != amountToCopy) {
				DrawDispose(progBar);
				free(readBuffer);
				devices[DEVICE_CUR]->closeFile(srcFile);
				devices[DEVICE_DEST]->closeFile(destFile);
				sprintf(txtbuffer, "Failed to Read! (%d %d)\n%s",amountToCopy,ret, srcFile->name);
				msgBox = DrawMessageBox(D_FAIL,txtbuffer);
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
			devices[DEVICE_CUR]->closeFile(srcFile);
			devices[DEVICE_DEST]->closeFile(destFile);
			sprintf(txtbuffer, "Failed to Write! (%d %d)\n%s",amountToCopy,ret,destFile->name);
			msgBox = DrawMessageBox(D_FAIL,txtbuffer);
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
	devices[DEVICE_CUR]->closeFile(srcFile);

	u32 ret = devices[DEVICE_DEST]->writeFile(destFile, NULL, 0);
	if(ret == 0) {
		ret = devices[DEVICE_DEST]->closeFile(destFile);
	}
	if(ret != 0) {
		sprintf(txtbuffer, "Failed to Write! (%d)\n%s",ret,destFile->name);
		msgBox = DrawMessageBox(D_FAIL,txtbuffer);
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		setGCIInfo(NULL);
		setCopyGCIMode(FALSE);
		return true;
	}

	setGCIInfo(NULL);
	setCopyGCIMode(FALSE);
	
	msgBox = NULL;
	if(!cancelled) {
		// If cut, delete from source device
		bool canDelete = (devices[DEVICE_CUR]->features & FEAT_WRITE) && devices[DEVICE_CUR]->deleteFile;
		if(canDelete && option == MOVE_OPTION) { 
			devices[DEVICE_CUR]->deleteFile(srcFile);
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

	if (!silent) {
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
	}
    return false;
}

bool copy_folder_recursive(file_handle *srcFolder, file_handle *destFolder, int option) {
	// Check that destination directory doesn't contain the entire source directory
	if (strstr(destFolder->name, srcFolder->name) != NULL) {
		sprintf(txtbuffer, "Source folder cannot\ncontain destination folder!");
		uiDrawObj_t *msgBox = DrawMessageBox(D_WARN,txtbuffer);
		DrawPublish(msgBox);
		wait_press_A();
		DrawDispose(msgBox);
		return true;
	}

    // Create destination folder
	u32 ret = devices[DEVICE_DEST]->makeDir(destFolder);
    if (ret != 0) {
        sprintf(txtbuffer, "Failed to create folder:\n%s\nSrc: %s", destFolder->name, srcFolder->name);
        uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, txtbuffer);
        DrawPublish(msgBox);
        wait_press_A();
        DrawDispose(msgBox);
        return true;
    }

    // Read each item in source folder
    file_handle **directory = NULL;
    file_handle *curDirEntries = NULL;
    int num_files = devices[DEVICE_CUR]->readDir(srcFolder, &curDirEntries, -1);
    num_files = sortFiles(curDirEntries, num_files, &directory);

    for (int i = 0; i < num_files; i++) {
        file_handle *srcEntry = directory[i];

        // Create handle to new file or folder
        file_handle newDestFile;
        memset(&newDestFile, 0, sizeof(file_handle));
        snprintf(newDestFile.name, PATHNAME_MAX, "%s/%s", destFolder->name, getRelativeName(srcEntry->name));

        if (srcEntry->fileType == IS_DIR) {
            // Recurse into subdirectory
            if (copy_folder_recursive(srcEntry, &newDestFile, option)) {
				sprintf(txtbuffer, "Recursive folder copy error");
				uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL,txtbuffer);
				DrawPublish(msgBox);
				wait_press_A();
				DrawDispose(msgBox);

                free(directory);
                free(curDirEntries);
                return true;
            }
        } else if (srcEntry->fileType == IS_FILE) {
            // Copy file
            copy_file(srcEntry, &newDestFile, option, true);
        }
    }

    free(directory);
    free(curDirEntries);
    return false;
}

/* Manage file  - The user will be asked what they want to do with the currently selected file - copy/move/delete*/
bool manage_file() {
	bool isFile    = curFile.fileType == IS_FILE;
	bool isHidden  = curFile.fileAttrib & ATTRIB_HIDDEN;
	bool canWrite  = devices[DEVICE_CUR]->features & FEAT_WRITE;
	bool canCopy   = canWrite;
	bool canMove   = canWrite && devices[DEVICE_CUR]->deleteFile;
	bool canDelete = canWrite && devices[DEVICE_CUR]->deleteFile;
	bool canRename = canWrite && devices[DEVICE_CUR]->renameFile;
	bool canHide   = canWrite && devices[DEVICE_CUR]->hideFile;
	
	// Ask the user what they want to do with the selected entry
	uiDrawObj_t* manageFileBox = DrawEmptyBox(10,150, getVideoMode()->fbWidth-10, 320);
	sprintf(txtbuffer, "Manage %s:", isFile ? "File" : "Directory");
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 160, txtbuffer, 1.0f, true, defaultColor));
	float scale = GetTextScaleToFitInWidth(getRelativeName(curFile.name), getVideoMode()->fbWidth-10-10);
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 190, getRelativeName(curFile.name), scale, true, defaultColor));
	sprintf(txtbuffer, "%s%s%s%s%s",
			canCopy ? " (X) Copy " : "",
			canMove ? " (Y) Move " : "",
			canDelete ? " (Z) Delete " : "",
			canRename ? " (R) Rename " : "",
			canHide ? isHidden ? " (L) Unhide " : " (L) Hide " : "");
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 250, txtbuffer, GetTextScaleToFitInWidth(txtbuffer, getVideoMode()->fbWidth-10-10), true, defaultColor));
	DrawAddChild(manageFileBox, DrawStyledLabel(640/2, 310, "Press an option to continue, or B to return", 1.0f, true, defaultColor));
	DrawPublish(manageFileBox);
	u32 waitButtons = PAD_BUTTON_X|PAD_BUTTON_Y|PAD_BUTTON_B|PAD_TRIGGER_Z|PAD_TRIGGER_R|PAD_TRIGGER_L;
	do {VIDEO_WaitVSync();} while (padsButtonsHeld() & waitButtons);
	int option = 0;
	while(1) {
		u32 buttons = padsButtonsHeld();
		if(canCopy && (buttons & PAD_BUTTON_X)) {
			option = COPY_OPTION;
			while(padsButtonsHeld() & PAD_BUTTON_X){ VIDEO_WaitVSync (); }
			break;
		}
		if(canMove && (buttons & PAD_BUTTON_Y)) {
			option = MOVE_OPTION;
			while(padsButtonsHeld() & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
			break;
		}
		if(canDelete && (buttons & PAD_TRIGGER_Z)) {
			option = DELETE_OPTION;
			while(padsButtonsHeld() & PAD_TRIGGER_Z){ VIDEO_WaitVSync (); }
			break;
		}
		if(canRename && (buttons & PAD_TRIGGER_R)) {
			option = RENAME_OPTION;
			while(padsButtonsHeld() & PAD_TRIGGER_R){ VIDEO_WaitVSync (); }
			break;
		}
		if(canRename && (buttons & PAD_TRIGGER_L)) {
			option = HIDE_OPTION;
			while(padsButtonsHeld() & PAD_TRIGGER_L){ VIDEO_WaitVSync (); }
			break;
		}
		if(buttons & PAD_BUTTON_B) {
			DrawDispose(manageFileBox);
			return false;
		}
	}
	do {VIDEO_WaitVSync();} while (padsButtonsHeld() & waitButtons);
	DrawDispose(manageFileBox);
	
	// "Are you sure?" option for deletes.
	if(option == DELETE_OPTION) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_WARN, "Delete confirmation required.\n \nPress L + A to continue, or B to cancel.");
		DrawPublish(msgBox);
		bool cancel = false;
		while(1) {
			u16 btns = padsButtonsHeld();
			if ((btns & (PAD_BUTTON_A|PAD_TRIGGER_L)) == (PAD_BUTTON_A|PAD_TRIGGER_L)) {
				break;
			}
			else if (btns & PAD_BUTTON_B) {
				cancel = true;
				break;
			}
			VIDEO_WaitVSync();
		}
		do {VIDEO_WaitVSync();} while (padsButtonsHeld() & (PAD_BUTTON_A|PAD_TRIGGER_L|PAD_BUTTON_B));
		DrawDispose(msgBox);
		if(cancel) {
			return false;
		}
	}

	// Handles (un)hiding directories or files on FAT FS devices.
	if (canHide && option == HIDE_OPTION) {
		devices[DEVICE_CUR]->hideFile(&curFile, !isHidden);
	}
	// Handles renaming directories or files on FAT FS devices.
	else if(canRename && option == RENAME_OPTION) {
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
		do {VIDEO_WaitVSync();} while (padsButtonsHeld() & (PAD_BUTTON_B|PAD_BUTTON_START));
		return modified;
	}
	// Handles deletes (dir or file).
	else if(canDelete && option == DELETE_OPTION) {
		uiDrawObj_t *progBar = DrawPublish(DrawProgressBar(true, 0, "Deleting\205"));
		bool deleted = deleteFileOrDir(&curFile);
		DrawDispose(progBar);
		sprintf(txtbuffer, "%s %s\nPress A to continue.", isFile ? "File" : "Directory", deleted ? "deleted successfully" : "failed to delete!");
		uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(deleted ? D_INFO : D_FAIL, txtbuffer));
		wait_press_A();
		DrawDispose(msgBox);
		return deleted;
	}

	// Handles copies and moves (dir or file).
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
		file_handle *destDir = memalign(32,sizeof(file_handle));
		memset(destDir, 0, sizeof(file_handle));
		
		// Show a directory only browser and get the destination file location
		ret = select_dest_dir(devices[DEVICE_DEST]->initial, destDir);
		if(ret) {
			if(devices[DEVICE_DEST] != devices[DEVICE_CUR]) {
				devices[DEVICE_DEST]->deinit( devices[DEVICE_DEST]->initial );
			}
			return false;
		}
		
		u32 isDestCard = devices[DEVICE_DEST] == &__device_card_a || devices[DEVICE_DEST] == &__device_card_b;
		u32 isSrcCard = devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b;
		file_handle *destFile = memalign(32,sizeof(file_handle));
		concat_path(destFile->name, destDir->name, stripInvalidChars(getRelativeName(curFile.name)));
		free(destDir);
		destFile->fp = 0;
		destFile->ffsFp = 0;
		destFile->fileBase = 0;
		destFile->offset = 0;
		destFile->size = 0;
		destFile->fileType = IS_FILE;

		// Create a GCI if something is coming out from CARD to another device
		if(isSrcCard && !isDestCard) {
			strlcat(destFile->name, ".gci", PATHNAME_MAX);
		}

		// If the destination file already exists, ask the user what to do
		if(devices[DEVICE_DEST]->readFile(destFile, NULL, 0) == 0) {
			devices[DEVICE_DEST]->closeFile(destFile);
			uiDrawObj_t* dupeBox = DrawEmptyBox(10,150, getVideoMode()->fbWidth-10, 350);
			DrawAddChild(dupeBox, DrawStyledLabel(640/2, 160, "File exists:", 1.0f, true, defaultColor));
			float scale = GetTextScaleToFitInWidth(getRelativeName(curFile.name), getVideoMode()->fbWidth-10-10);
			DrawAddChild(dupeBox, DrawStyledLabel(640/2, 200, getRelativeName(curFile.name), scale, true, defaultColor));
			DrawAddChild(dupeBox, DrawStyledLabel(640/2, 230, "(A) Rename (Z) Overwrite", 1.0f, true, defaultColor));
			DrawAddChild(dupeBox, DrawStyledLabel(640/2, 300, "Press an option to continue, or B to return", 1.0f, true, defaultColor));
			DrawPublish(dupeBox);
			while(padsButtonsHeld() & (PAD_BUTTON_A | PAD_TRIGGER_Z)) { VIDEO_WaitVSync (); }
			while(1) {
				u32 buttons = padsButtonsHeld();
				if(buttons & PAD_TRIGGER_Z) {
					if(!strcmp(curFile.name, destFile->name)) {
						DrawDispose(dupeBox);
						uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Can't overwrite a file with itself!");
						DrawPublish(msgBox);
						wait_press_A();
						DrawDispose(msgBox);
						return false; 
					}
					else if(devices[DEVICE_DEST]->deleteFile) {
						devices[DEVICE_DEST]->deleteFile(destFile);
					}

					while(padsButtonsHeld() & PAD_TRIGGER_Z){ VIDEO_WaitVSync (); }
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

					while(padsButtonsHeld() & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
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
		if(devices[DEVICE_CUR] == devices[DEVICE_DEST] && canRename && option == MOVE_OPTION) {
			// Check that destination directory doesn't contain the entire source directory
			if (strstr(destFile->name, curFile.name) != NULL) {
				sprintf(txtbuffer, "Source folder cannot\ncontain destination folder!");
				uiDrawObj_t *msgBox = DrawMessageBox(D_WARN,txtbuffer);
				DrawPublish(msgBox);
				wait_press_A();
				DrawDispose(msgBox);
				return false;
			}

			ret = devices[DEVICE_CUR]->renameFile(&curFile, destFile->name);
			needsRefresh=1;
			uiDrawObj_t *msgBox = DrawMessageBox(D_INFO,ret ? "Move Failed!\nPress A to continue":"Move Successful!\nPress A to continue");
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
			else if(isDestCard && (endsWith(curFile.name,".gci") || endsWith(curFile.name,".gcs") || endsWith(curFile.name,".sav"))) {
				GCI gci;
				devices[DEVICE_CUR]->seekFile(&curFile, 0, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(&curFile, &gci, sizeof(GCI)) == sizeof(GCI)) {
					if(!memcmp(&gci, "DATELGC_SAVE", 12)) {
						devices[DEVICE_CUR]->seekFile(&curFile, 0x80, DEVICE_HANDLER_SEEK_SET);
						devices[DEVICE_CUR]->readFile(&curFile, &gci, sizeof(GCI));
						#pragma GCC diagnostic push
						#pragma GCC diagnostic ignored "-Wrestrict"
						swab(&gci.reserved01, &gci.reserved01, 2);
						swab(&gci.icon_addr,  &gci.icon_addr, 20);
						#pragma GCC diagnostic pop
					}
					else if(!memcmp(&gci, "GCSAVE", 6)) {
						devices[DEVICE_CUR]->seekFile(&curFile, 0x110, DEVICE_HANDLER_SEEK_SET);
						devices[DEVICE_CUR]->readFile(&curFile, &gci, sizeof(GCI));
					}
					if(curFile.size - curFile.offset == gci.filesize8 * 8192) setGCIInfo(&gci);
					else devices[DEVICE_CUR]->seekFile(&curFile, -sizeof(GCI), DEVICE_HANDLER_SEEK_CUR);
				}
			}

			if (isFile) {
				// Copy file
				copy_file(&curFile, destFile, option, false);
			}
			else {
				// Copy folder and subfolders
				if (copy_folder_recursive(&curFile, destFile, option) != 0) {
					// Delete source folder structure if move was successful
					deleteFileOrDir(&curFile);
				}
			}

			free(destFile);
		}
	}

	return true;
}

void verify_game()
{
	u32 crc = 0;
	u32 curOffset = 0, cancelled = 0, chunkSize = (512*1024);
	
	if(devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR) {
		devices[DEVICE_CUR]->setupFile(&curFile, NULL, NULL, -1);
		if(swissSettings.audioStreaming) {
			AUDIO_SetStreamVolLeft(0);
			AUDIO_SetStreamVolRight(0);
			AUDIO_SetStreamPlayState(AI_STREAM_START);
			DVD_PrepareStreamAbs(&commandBlock, curFile.size & ~(32*1024-1), 0);
			DVD_StopStreamAtEnd(&commandBlock);
		}
	}
	
	unsigned char *readBuffer = (unsigned char*)memalign(32,chunkSize);
	uiDrawObj_t* progBar = DrawProgressBar(false, 0, "Verifying\205");
	DrawPublish(progBar);
	
	u64 startTime = gettime();
	u64 lastTime = gettime();
	u32 lastOffset = 0;
	int speed = 0;
	int timeremain = 0;
	while(curOffset < curFile.size) {
		u32 buttons = padsButtonsHeld();
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
			goto fail;
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

fail:
	if(devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR) {
		if(swissSettings.audioStreaming) {
			DVD_CancelStream(&commandBlock);
			AUDIO_SetStreamPlayState(AI_STREAM_STOP);
		}
		devices[DEVICE_CUR]->closeFile(&curFile);
	}
}

void load_game() {
	file_handle *disc2File = meta_find_disc2(&curFile);
	
	if(devices[DEVICE_CUR] == &__device_wode) {
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Setup base offset please Wait\205"));
		devices[DEVICE_CUR]->setupFile(&curFile, disc2File, NULL, -1);
		DrawDispose(msgBox);
		repopulate_meta(&curFile);
	}
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading\205"));
	
	// boot the GCM/ISO file, gamecube disc or multigame selected entry
	memset(&tgcFile, 0, sizeof(TGCHeader));
	if(endsWith(curFile.name,".tgc")) {
		devices[DEVICE_CUR]->seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,&tgcFile,sizeof(TGCHeader)) != sizeof(TGCHeader) || tgcFile.magic != TGC_MAGIC) {
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File!"));
			sleep(2);
			DrawDispose(msgBox);
			goto exit;
		}
		
		devices[DEVICE_CUR]->seekFile(&curFile,tgcFile.headerStart,DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(&curFile,&GCMDisk,sizeof(DiskHeader)) != sizeof(DiskHeader) || !valid_gcm_magic(&GCMDisk)) {
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File!"));
			sleep(2);
			DrawDispose(msgBox);
			goto exit;
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
					goto exit;
				}
			}
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid or Corrupt File!"));
			sleep(2);
			DrawDispose(msgBox);
			goto exit;
		}
		
		swissSettings.audioStreaming = is_streaming_disc(&GCMDisk);
		
		if(needs_nkit_reencode(&GCMDisk, curFile.size)) {
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "Please reconvert to NKit.iso using\nNKit bundled with this Swiss release."));
			sleep(5);
			DrawDispose(msgBox);
			goto exit;
		}
		else if(is_nkit_format(&GCMDisk) && !valid_gcm_boot(&GCMDisk)) {
			DrawDispose(msgBox);
			msgBox = DrawPublish(DrawMessageBox(D_WARN, "File is not playable in NKit.iso format.\nPlease convert back to ISO using NKit."));
			sleep(5);
			DrawDispose(msgBox);
			goto exit;
		}
		else if(is_redump_disc(curFile.meta) && !valid_gcm_size(&GCMDisk, curFile.size)) {
			if(swissSettings.audioStreaming && !valid_gcm_size2(&GCMDisk, curFile.size)) {
				DrawDispose(msgBox);
				msgBox = DrawPublish(DrawMessageBox(D_WARN, "File is a bad dump and is not playable.\nPlease attempt recovery using NKit."));
				sleep(5);
				DrawDispose(msgBox);
				goto exit;
			}
			else {
				DrawDispose(msgBox);
				msgBox = DrawPublish(DrawMessageBox(D_WARN, "File is a bad dump, but may be playable.\nPlease attempt recovery using NKit."));
				sleep(5);
			}
		}
	}
	if(swissSettings.audioStreaming && !(devices[DEVICE_CUR]->features & FEAT_AUDIO_STREAMING)) {
		DrawDispose(msgBox);
		msgBox = DrawPublish(DrawMessageBox(D_WARN, "Device does not support audio streaming.\nThis may impact playability."));
		sleep(5);
	}
	if(swissSettings.exiSpeed && (devices[DEVICE_CUR]->quirks & QUIRK_EXI_SPEED)) {
		DrawDispose(msgBox);
		msgBox = DrawPublish(DrawMessageBox(D_WARN, "Device is operating in a degraded state.\nThis may impact playability."));
		sleep(5);
	}
	DrawDispose(msgBox);
	
	// Find the config for this game, or default if we don't know about it
	ConfigEntry *config = calloc(1, sizeof(ConfigEntry));
	memcpy(config->game_id, &GCMDisk.ConsoleID, 4);
	memcpy(config->game_name, GCMDisk.GameName, 64);
	config->region = wodeRegionToChar(GCMDisk.RegionCode);
	config->forceCleanBoot = is_diag_disc(&GCMDisk);
	config_find(config);
	
	// Show game info or return to the menu
	if(!info_game(config)) {
		free(config);
		goto exit;
	}
	
	if(devices[DEVICE_CONFIG] != NULL) {
		// Update the recent list.
		if(update_recent()) {
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving recent list\205"));
			config_update_recent(true);
			DrawDispose(msgBox);
		}
	}
	
	// Load config for this game into our current settings
	config_load_current(config);
	gameID_early_set(&GCMDisk);
	
	if(config->forceCleanBoot || (config->preferCleanBoot && (devices[DEVICE_CUR]->location & LOC_DVD_CONNECTOR))) {
		gameID_set(&GCMDisk, get_gcm_boot_hash(&GCMDisk, curFile.meta));
		
		if(!(devices[DEVICE_CUR]->location & LOC_DVD_CONNECTOR)) {
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
		if(!(devices[DEVICE_CUR]->quirks & QUIRK_NO_DEINIT)) {
			devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
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
			loadCheatsSelection();
			int appliedCount = getEnabledCheatsCount();
			sprintf(txtbuffer, "Applied %i cheats", appliedCount);
			msgBox = DrawPublish(DrawMessageBox(D_INFO, txtbuffer));
			sleep(1);
			DrawDispose(msgBox);
		}
	}
	if(swissSettings.wiirdDebug && !usb_isgeckoalive(1)) {
		swissSettings.wiirdDebug = 0;
	}
	if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
		swissSettings.wiirdEngine = 1;
		setTopAddr(WIIRD_ENGINE);
	}
	else {
		swissSettings.wiirdEngine = 0;
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
				gameID_set(&GCMDisk, fileToPatch->hash);
				break;
			}
		}
	}
	else if(valid_gcm_boot(&GCMDisk)) {
		for(int i = 0; i < numToPatch; i++) {
			if(filesToPatch[i].file == &curFile && filesToPatch[i].offset == GCMDisk.DOLOffset) {
				fileToPatch = &filesToPatch[i];
				gameID_set(&GCMDisk, fileToPatch->hash);
				break;
			}
		}
		if(swissSettings.bs2Boot) {
			fileToPatch = &filesToPatch[numToPatch++];
			strcpy(fileToPatch->name, "BS2.img");
			fileToPatch->type = PATCH_BS2;
		}
	}
	else {
		gameID_set(&GCMDisk, get_gcm_boot_hash(&GCMDisk, curFile.meta));
		
		fileToPatch = &filesToPatch[numToPatch++];
		strcpy(fileToPatch->name, "BS2.img");
		fileToPatch->type = PATCH_BS2;
	}
	
	s32 exi_channel = EXI_CHANNEL_MAX;
	s32 exi_device = EXI_DEVICE_MAX;
	switch(swissSettings.disableMemoryCard) {
		case 1:
			if(EXI_Probe(EXI_CHANNEL_0)) {
				exi_channel = EXI_CHANNEL_0;
				exi_device = EXI_DEVICE_0;
			}
			break;
		case 2:
			if(EXI_Probe(EXI_CHANNEL_1)) {
				exi_channel = EXI_CHANNEL_1;
				exi_device = EXI_DEVICE_0;
			}
			break;
	}
	
	*(vu8*)VAR_CURRENT_DISC = disc2File && disc2File == fileToPatch->file;
	*(vu8*)VAR_DRIVE_FLAGS = (!!swissSettings.hasFlippyDrive << 3) | ((drive_status == DEBUG_MODE) << 2) | ((tgcFile.magic == TGC_MAGIC) << 1) | !!disc2File;
	*(vu8*)VAR_EMU_READ_SPEED = swissSettings.emulateReadSpeed;
	*(vu8*)VAR_IGR_TYPE = swissSettings.igrType;
	*(vu32**)VAR_FRAG_LIST = NULL;
	*(vu8*)VAR_SD_SHIFT = 0;
	*(vu8*)VAR_EXI_SLOT = (exi_device << 6) | (exi_channel << 4) | (exi_device << 2) | exi_channel;
	*(vu8*)VAR_EXI_CPR = (EXI_CHANNEL_MAX << 6) | EXI_SPEED1MHZ;
	*(vu8*)VAR_EXI2_CPR = (EXI_CHANNEL_MAX << 6) | EXI_SPEED1MHZ;
	*(vu32**)VAR_EXI_REGS = NULL;
	net_get_mac_address((u8*)VAR_CLIENT_MAC);
	*(vu32**)VAR_EXI2_REGS = NULL;
	*(vu8*)VAR_TRIGGER_LEVEL = swissSettings.triggerLevel;
	*(vu8*)VAR_CARD_A_ID = 0x00;
	*(vu8*)VAR_CARD_B_ID = 0x00;
	
	if(getTopAddr() == 0x81800000) {
		if(fileToPatch->type == PATCH_BS2) setTopAddr(0);
		else setTopAddr(HI_RESERVE);
	}
	// Call the special setup for each device (e.g. SD will set the sector(s))
	if(!devices[DEVICE_CUR]->setupFile(&curFile, disc2File, filesToPatch, numToPatch)) {
		msgBox = DrawPublish(DrawMessageBox(D_FAIL, "Failed to setup the file (too fragmented?)"));
		wait_press_A();
		DrawDispose(msgBox);
		goto fail_patched;
	}

	if(devices[DEVICE_CUR]->emulated() & EMU_ETHERNET) {
		s32 exi_channel, exi_device, exi_interrupt;
		if(getExiDeviceByLocation(bba_location, &exi_channel, &exi_device) &&
			getExiInterruptByLocation(bba_location, &exi_interrupt)) {
			*(vu8*)VAR_EXI_SLOT = (*(vu8*)VAR_EXI_SLOT & 0x0F) | (((exi_device << 6) | (exi_channel << 4)) & 0xF0);
			*(vu8*)VAR_EXI2_CPR = (exi_interrupt << 6) | ((1 << exi_device) << 3) | (exi_device == EXI_DEVICE_0 ? EXI_SPEED32MHZ : EXI_SPEED16MHZ);
			*(vu32**)VAR_EXI2_REGS = ((vu32(*)[5])0xCC006800)[exi_channel];
		}
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
	setTopAddr(0x81800000);
	setTopAddr(0);
fail:
	gameID_unset();
	config_unload_current();
	free(config);
exit:
	devices[DEVICE_CUR]->closeFile(&curFile);
	devices[DEVICE_CUR]->closeFile(disc2File);
}

/* Execute/Load/Parse the currently selected file */
void load_file()
{
	char *fileName = &curFile.name[0];
		
	//if it's a DOL, boot it
	if(strlen(fileName)>4) {
		if(endsWith(fileName,".bin") || endsWith(fileName,".dol") || endsWith(fileName,".dol+cli") || endsWith(fileName,".elf")) {
			boot_dol(&curFile, 0, NULL);
			// if it was invalid (overlaps sections, too large, etc..) it'll return
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "Invalid DOL"));
			sleep(2);
			DrawDispose(msgBox);
			return;
		}
		else if(endsWith(fileName,".fpkg")) {
			if(devices[DEVICE_CUR] == &__device_flippy || devices[DEVICE_CUR] == &__device_flippyflash) {
				uiDrawObj_t *progBar = DrawPublish(DrawProgressBar(true, 0, "Resetting RP2040"));
				flippy_closefrom(1);
				flippy_reset();
				DVD_Inquiry(&commandBlock, &driveInfo);
				flippy_boot(FLIPPY_MODE_UPDATE);
				flippybootstatus *status;
				while((status = flippy_getbootstatus()) && status->current_progress != 0xFFFF) {
					sprintf(txtbuffer, "%.64s\n%.64s", status->text, status->subtext);
					uiDrawObj_t *newBar = DrawProgressBar(!status->show_progress_bar, (int)(((float)status->current_progress/(float)1000)*100), txtbuffer);
					if(progBar != NULL) {
						DrawDispose(progBar);
					}
					progBar = newBar;
					DrawPublish(progBar);
				}
				DrawDispose(progBar);
				deviceHandler_setDeviceAvailable(&__device_flippy, deviceHandler_Flippy_test());
				deviceHandler_setDeviceAvailable(&__device_flippyflash, deviceHandler_FlippyFlash_test());
				needsDeviceChange = !deviceHandler_getDeviceAvailable(devices[DEVICE_CUR]);
				needsRefresh = 1;
				return;
			}
			needsRefresh = manage_file() ? 1:0;
			return;
		}
		else if(endsWith(fileName,".fzn")) {
			if(curFile.size != 0x1D0000) {
				uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "File Size must be 0x1D0000 bytes!"));
				sleep(2);
				DrawDispose(msgBox);
				return;
			}
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Reading Flash File\205"));
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
		else if(endsWith(fileName,".fdi") || endsWith(fileName,".gcm") || endsWith(fileName,".iso") || endsWith(fileName,".tgc")) {
			if(devices[DEVICE_CUR]->features & FEAT_BOOT_GCM) {
				load_game();
				memset(&GCMDisk, 0, sizeof(DiskHeader));
			}
			else {
				uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "Device does not support disc images."));
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
			mp3_player(getSortedDirEntries(), getSortedDirEntryCount(), &curFile);
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
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Checking Game\205"));
	
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
	
	if(devices[DEVICE_CUR]->emulated()) {
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
			float minScale = 1.0f;
			while ((tok = strtok_r (rest,"\r\n", &rest))) {
				minScale = MIN(GetTextScaleToFitInWidth(tok,(getVideoMode()->fbWidth-78)-75), minScale);
			}
			sprintf(txtbuffer, "%s", curFile.meta->bannerDesc.description);
			rest = &txtbuffer[0]; 
			while ((tok = strtok_r (rest,"\r\n", &rest))) {
				DrawAddChild(container, DrawStyledLabel(640/2, 315+(line*minScale*24), tok, minScale, true, defaultColor));
				line++;
			}
		}
	}
	if(GCMDisk.DVDMagicWord == DVD_MAGIC) {
		if(is_verifiable_disc(&GCMDisk)) {
			DrawAddChild(container, DrawStyledLabel(640/2, 180, "(R) Verify", 0.6f, true, defaultColor));
		}
		sprintf(txtbuffer, "Game ID: [%.6s] Audio Streaming: [%s]", (char*)&GCMDisk, (swissSettings.audioStreaming ? "Yes":"No"));
		DrawAddChild(container, DrawStyledLabel(640/2, 200, txtbuffer, 0.8f, true, defaultColor));

		if(GCMDisk.TotalDisc > 1) {
			if(devices[DEVICE_CUR]->quirks & QUIRK_GCLOADER_NO_DISC_2) {
				DrawAddChild(container, DrawStyledLabel(640/2, 220, "A firmware update is required.", 0.6f, true, defaultColor));
			}
			else {
				sprintf(txtbuffer, "Disc %i/%i [Found: %s]", GCMDisk.DiscID+1, GCMDisk.TotalDisc, meta_find_disc2(&curFile) ? "Yes":"No");
				DrawAddChild(container, DrawStyledLabel(640/2, 220, txtbuffer, 0.6f, true, defaultColor));
			}
		}
		else if(GCMDisk.CountryCode == 'E'
			&& GCMDisk.RegionCode == 0) {
			if(GCMDisk.Version > 0x30) {
				sprintf(txtbuffer, "Revision %X", GCMDisk.Version - 0x30);
				DrawAddChild(container, DrawStyledLabel(640/2, 220, txtbuffer, 0.6f, true, defaultColor));
			}
		}
		else if(GCMDisk.Version) {
			sprintf(txtbuffer, "Revision %X", GCMDisk.Version);
			DrawAddChild(container, DrawStyledLabel(640/2, 220, txtbuffer, 0.6f, true, defaultColor));
		}
	}

	char *textPtr = txtbuffer;
	if(devices[DEVICE_CONFIG] != NULL) {
		bool isAutoLoadEntry = !strcmp(swissSettings.autoload, curFile.name) || !fnmatch(swissSettings.autoload, curFile.name, FNM_PATHNAME);
		textPtr += sprintf(textPtr, "(Z) Load at startup [Current: %s]", isAutoLoadEntry ? "Yes":"No");
	}
	if(devices[DEVICE_CUR]->location & LOC_DVD_CONNECTOR) {
		textPtr = stpcpy(textPtr, textPtr == txtbuffer ? "(L+A) Clean Boot":" \267 (L+A) Clean Boot");
	}
	if(textPtr != txtbuffer) {
		DrawAddChild(container, DrawStyledLabel(640/2, 370, txtbuffer, 0.6f, true, defaultColor));
	}
	if(devices[DEVICE_CUR] == &__device_wode) {
		DrawAddChild(container, DrawStyledLabel(640/2, 390, "(X) Settings \267 (Y) Cheats \267 (A) Boot", 0.75f, true, defaultColor));
	}
	else {
		DrawAddChild(container, DrawStyledLabel(640/2, 390, "(X) Settings \267 (Y) Cheats \267 (B) Exit \267 (A) Boot", 0.75f, true, defaultColor));
	}
	return container;
}

/* Show info about the game - and also load the config for it */
int info_game(ConfigEntry *config)
{
	if(swissSettings.autoBoot) {
		if(padsButtonsHeld() & PAD_BUTTON_B) {
			swissSettings.autoBoot = 0;
		} else {
			return swissSettings.autoBoot;
		}
	}
	int ret = 0, num_cheats = -1;
	uiDrawObj_t *infoPanel = DrawPublish(draw_game_info());
	while(1) {
		while(padsButtonsHeld() & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y | PAD_TRIGGER_Z | PAD_TRIGGER_R)){ VIDEO_WaitVSync (); }
		while(!(padsButtonsHeld() & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y | PAD_TRIGGER_Z | PAD_TRIGGER_R))){ VIDEO_WaitVSync (); }
		u32 buttons = padsButtonsHeld();
		if(buttons & PAD_BUTTON_A) {
			if(buttons & PAD_TRIGGER_L) {
				config->forceCleanBoot = 1;
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
			needsRefresh = show_settings(PAGE_GAME, 0, config);
		}
		if((buttons & PAD_TRIGGER_Z) && devices[DEVICE_CONFIG] != NULL) {
			// Toggle autoload
			if(!strcmp(&swissSettings.autoload[0], &curFile.name[0])
			|| !fnmatch(&swissSettings.autoload[0], &curFile.name[0], FNM_PATHNAME)) {
				memset(&swissSettings.autoload[0], 0, PATHNAME_MAX);
			}
			else if(!fnmatch("dvd:/*.gcm", &curFile.name[0], FNM_PATHNAME)) {
				strcpy(&swissSettings.autoload[0], "dvd:/*.gcm");
			}
			else {
				strcpy(&swissSettings.autoload[0], &curFile.name[0]);
			}
			// Save config
			uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving autoload\205"));
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
				loadCheatsSelection();
				DrawCheatsSelector(getRelativeName(&curFile.name[0]));
				saveCheatsSelection();
			}
		}
		while(padsButtonsHeld() & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	}
	while(padsButtonsHeld() & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
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

	while((allDevices[curDevice] != devices[DEVICE_PREV])) {
		curDevice = (curDevice + 1) % MAX_DEVICES;
	}
	// Find the first device that meets the requiredFeatures and is available
	while((allDevices[curDevice] == NULL) || !(deviceHandler_getDeviceAvailable(allDevices[curDevice])||showAllDevices) || !(allDevices[curDevice]->features & requiredFeatures)) {
		curDevice = (curDevice + 1) % MAX_DEVICES;
	}

	uiDrawObj_t *deviceSelectBox = NULL;
	while(1) {
		// Device selector
		deviceSelectBox = DrawEmptyBox(20,190, getVideoMode()->fbWidth-20, 410);
		uiDrawObj_t *selectLabel = DrawStyledLabel(640/2, 195
													, type == DEVICE_DEST ? "Destination Device" : "Device Selection"
													, 1.0f, true, defaultColor);
		uiDrawObj_t *fwdLabel = DrawLabel(530, 270, "\233");
		uiDrawObj_t *backLabel = DrawLabel(100, 270, "\213");
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
				curDevice = (curDevice + MAX_DEVICES) % MAX_DEVICES;
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
		if(allDevices[curDevice]->features & FEAT_EXI_SPEED) {
			uiDrawObj_t *exiOptionsLabel = DrawStyledLabel(getVideoMode()->fbWidth-190, 400, "(X) EXI Options", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			DrawAddChild(deviceSelectBox, exiOptionsLabel);
			if(inAdvanced) {
				// Draw speed selection if advanced menu is showing.
				uiDrawObj_t *exiSpeedLabel = DrawStyledLabel(getVideoMode()->fbWidth-160, 385, swissSettings.exiSpeed ? "Speed: 32 MHz":"Speed: 16 MHz", 0.65f, false, defaultColor);
				DrawAddChild(deviceSelectBox, exiSpeedLabel);
			}
		}
		if(allDevices[curDevice]->details) {
			uiDrawObj_t *deviceDetailLabel = DrawStyledLabel(20, 385, "(Y) Show device details", 0.65f, false, deSelectedColor);
			DrawAddChild(deviceSelectBox, deviceDetailLabel);
		}
		DrawPublish(deviceSelectBox);
		while (!(padsButtonsHeld() & 
			(PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_B|PAD_BUTTON_A|PAD_BUTTON_X|PAD_BUTTON_Y|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_TRIGGER_Z) ))
			{ VIDEO_WaitVSync (); }
		u16 btns = padsButtonsHeld();
		if((btns & PAD_BUTTON_Y) && allDevices[curDevice]->details) {
			char *deviceDetails = allDevices[curDevice]->details(allDevices[curDevice]->initial);
			if(deviceDetails) {
				uiDrawObj_t *deviceDetailBox = DrawPublish(DrawTooltip(deviceDetails));
				while (padsButtonsHeld() & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
				while (!((padsButtonsHeld() & PAD_BUTTON_Y) || (padsButtonsHeld() & PAD_BUTTON_B))){ VIDEO_WaitVSync (); }
				DrawDispose(deviceDetailBox);
				free(deviceDetails);
			}
		}
		if((btns & PAD_BUTTON_X) && (allDevices[curDevice]->features & FEAT_EXI_SPEED))
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
			if(btns & (PAD_BUTTON_RIGHT|PAD_TRIGGER_R)) {
				direction = 1;
			}
			if(btns & (PAD_BUTTON_LEFT|PAD_TRIGGER_L)) {
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
		while ((padsButtonsHeld() & 
			(PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_B|PAD_BUTTON_A
			|PAD_BUTTON_X|PAD_BUTTON_Y|PAD_TRIGGER_L|PAD_TRIGGER_R|PAD_TRIGGER_Z) ))
			{ VIDEO_WaitVSync (); }
		DrawDispose(deviceSelectBox);
	}
	while ((padsButtonsHeld() & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	// Deinit any existing device
	if(devices[type] != NULL) {
		// Don't deinit our current device when selecting a destination device
		if(!(type == DEVICE_DEST && devices[type] == devices[DEVICE_CUR])) {
			devices[type]->deinit( devices[type]->initial );
		}
	}
	if(showAllDevices && (allDevices[curDevice]->location & (LOC_MEMCARD_SLOT_A | LOC_MEMCARD_SLOT_B | LOC_SERIAL_PORT_2))) {
		EXI_ProbeReset();
	}
	devices[type] = allDevices[curDevice];
	DrawDispose(deviceSelectBox);
}

void menu_loop()
{ 
	while(padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
	// We don't care if a subsequent device is "default"
	if(needsDeviceChange) {
		freeFiles();
		if(devices[DEVICE_CUR]) {
			devices[DEVICE_PREV] = devices[DEVICE_CUR];
			devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
		}
		devices[DEVICE_CUR] = NULL;
		needsDeviceChange = 0;
		needsRefresh = 1;
		curMenuLocation = ON_FILLIST;
		DrawUpdateMenuButtons((curMenuLocation == ON_OPTIONS) ? curMenuSelection : MENU_NOSELECT);
		select_device(DEVICE_CUR);
		if(devices[DEVICE_CUR] != NULL) {
			memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
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
			if(swissSettings.fileBrowserType == BROWSER_CAROUSEL) {
				filePanel = renderFileCarousel(getSortedDirEntries(), getSortedDirEntryCount(), filePanel);
			}
			else if(swissSettings.fileBrowserType == BROWSER_FULLWIDTH) {
				filePanel = renderFileFullwidth(getSortedDirEntries(), getSortedDirEntryCount(), filePanel);
			}
			else {
				filePanel = renderFileBrowser(getSortedDirEntries(), getSortedDirEntryCount(), filePanel);
			}
			while(padsButtonsHeld() & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT | PAD_BUTTON_START)) {
				VIDEO_WaitVSync (); 
			}
		}
		else if (curMenuLocation==ON_OPTIONS) {
			u16 btns = padsButtonsHeld();
			while (!((btns=padsButtonsHeld()) & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT | PAD_BUTTON_START))) {
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
						needsRefresh = show_settings(PAGE_GLOBAL, 0, NULL);
						break;
					case MENU_INFO:
						show_info();
						break;
					case MENU_REFRESH:
						if(devices[DEVICE_CUR] != NULL) {
							memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
							if(devices[DEVICE_CUR] == &__device_wkf) { 
								wkfReinit(); devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
							}
						}
						needsRefresh=1;
						break;
					case MENU_EXIT:
						if(devices[DEVICE_CUR] != NULL) {
							devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
						}
						if(swissSettings.hasFlippyDrive) {
							flippy_bypass(false);
							flippy_reset();
						}
						DrawShutdown();
						SYS_ResetSystem(SYS_HOTRESET, 0, !swissSettings.hasFlippyDrive);
						__builtin_unreachable();
						break;
				}
			}
			if((btns & PAD_BUTTON_B) && devices[DEVICE_CUR] != NULL) {
				curMenuLocation = ON_FILLIST;
			}
			if((btns & PAD_BUTTON_START) && swissSettings.recentListLevel > 0) {
				select_recent_entry();
			}
			while(padsButtonsHeld() & (PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_RIGHT | PAD_BUTTON_LEFT | PAD_BUTTON_START)) {
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
