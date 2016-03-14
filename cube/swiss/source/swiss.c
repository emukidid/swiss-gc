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

#include "frag.h"
#include "swiss.h"
#include "main.h"
#include "httpd.h"
#include "exi.h"
#include "patcher.h"
#include "banner.h"
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

DiskHeader GCMDisk;      //Gamecube Disc Header struct
char IPLInfo[256] __attribute__((aligned(32)));
GXRModeObj *newmode = NULL;
char txtbuffer[2048];           //temporary text buffer
file_handle curFile;    //filedescriptor for current file
file_handle curDir;     //filedescriptor for current directory
int SDHCCard = 0; //0 == SDHC, 1 == normal SD
int curDevice = 0;  //SD_CARD or DVD_DISC or IDEEXI or WODE
int curCopyDevice = 0;  //SD_CARD or DVD_DISC or IDEEXI or WODE
char *videoStr = NULL;
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

void print_gecko(const char* fmt, ...)
{
	if(swissSettings.debugUSB && usb_isgeckoalive(1)) {
		char tempstr[2048];
		va_list arglist;
		va_start(arglist, fmt);
		vsprintf(tempstr, fmt, arglist);
		va_end(arglist);
		// write out over usb gecko ;)
		usb_sendbuffer_safe(1,tempstr,strlen(tempstr));
	}
}


/* re-init video for a given game */
void ogc_video__reset()
{
	if(swissSettings.forceEncoding == 0) {
		if(GCMDisk.CountryCode == 'J')
			swissSettings.forceEncoding = 2;
		else
			swissSettings.forceEncoding = 1;
	}
	
	if(swissSettings.gameVMode == 0) {
		switch(GCMDisk.CountryCode) {
			case 'P': // PAL
			case 'D': // German
			case 'F': // French
			case 'S': // Spanish
			case 'I': // Italian
			case 'L': // Japanese Import to PAL
			case 'M': // American Import to PAL
			case 'X': // PAL other languages?
			case 'Y': // PAL other languages?
			case 'U':
				swissSettings.gameVMode = -2;
				break;
			case 'E':
			case 'J':
				swissSettings.gameVMode = -1;
				break;
		}
	} else {
		syssram* sram = __SYS_LockSram();
		sram->ntd = (swissSettings.gameVMode >= 1) && (swissSettings.gameVMode <= 4) ? (sram->ntd|0x40):(sram->ntd&~0x40);
		sram->flags = (swissSettings.gameVMode == 4) || (swissSettings.gameVMode == 8) ? (sram->flags|0x80):(sram->flags&~0x80);
		__SYS_UnlockSram(1);
		while(!__SYS_SyncSram());
	}
	
	/* set TV mode for current game */
	switch(swissSettings.gameVMode) {
		case -1:
			DrawFrameStart();
			DrawMessageBox(D_INFO, "Video Mode: NTSC 60Hz");
			DrawFrameFinish();
			newmode = &TVNtsc480IntDf;
			break;
		case -2:
			DrawFrameStart();
			DrawMessageBox(D_INFO, "Video Mode: PAL 50Hz");
			DrawFrameFinish();
			newmode = &TVPal576IntDfScale;
			break;
		case 1:
			DrawFrameStart();
			DrawMessageBox(D_INFO, "Video Mode: NTSC 480i");
			DrawFrameFinish();
			newmode = &TVNtsc480IntDf;
			break;
		case 2:
			DrawFrameStart();
			DrawMessageBox(D_INFO, "Video Mode: NTSC 240p");
			DrawFrameFinish();
			newmode = &TVNtsc480IntDf;
			break;
		case 3:
			if(VIDEO_HaveComponentCable()) {
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Video Mode: NTSC 960i");
				DrawFrameFinish();
				newmode = &TVNtsc480Prog;
			} else {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Video Mode: NTSC 480i");
				DrawFrameFinish();
				swissSettings.gameVMode = 1;
				newmode = &TVNtsc480IntDf;
				sleep(5);
			}
			break;
		case 4:
			if(VIDEO_HaveComponentCable()) {
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Video Mode: NTSC 480p");
				DrawFrameFinish();
				newmode = &TVNtsc480Prog;
			} else {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Video Mode: NTSC 480i");
				DrawFrameFinish();
				swissSettings.gameVMode = 1;
				newmode = &TVNtsc480IntDf;
				sleep(5);
			}
			break;
		case 5:
			DrawFrameStart();
			DrawMessageBox(D_INFO, "Video Mode: PAL 576i");
			DrawFrameFinish();
			newmode = &TVPal576IntDfScale;
			break;
		case 6:
			DrawFrameStart();
			DrawMessageBox(D_INFO, "Video Mode: PAL 288p");
			DrawFrameFinish();
			newmode = &TVPal576IntDfScale;
			break;
		case 7:
			if(VIDEO_HaveComponentCable()) {
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Video Mode: PAL 1152i");
				DrawFrameFinish();
				newmode = &TVPal576ProgScale;
			} else {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Video Mode: PAL 576i");
				DrawFrameFinish();
				swissSettings.gameVMode = 5;
				newmode = &TVPal576IntDfScale;
				sleep(5);
			}
			break;
		case 8:
			if(VIDEO_HaveComponentCable()) {
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Video Mode: PAL 576p");
				DrawFrameFinish();
				newmode = &TVPal576ProgScale;
			} else {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Video Mode: PAL 576i");
				DrawFrameFinish();
				swissSettings.gameVMode = 5;
				newmode = &TVPal576IntDfScale;
				sleep(5);
			}
			break;
	}
}

void do_videomode_swap() {
	if((newmode != NULL) && (newmode != vmode)) {
		initialise_video(newmode);
		vmode = newmode;
	}
}

void doBackdrop()
{
	DrawFrameStart();
	DrawMenuButtons((curMenuLocation==ON_OPTIONS)?curMenuSelection:-1);
}

char *getRelativeName(char *str) {
	int i;
	for(i=strlen(str);i>0;i--){
		if(str[i]=='/') {
			return str+i+1;
		}
	}
	return str;
}

char stripbuffer[1024];
char *stripInvalidChars(char *str) {
	strcpy(stripbuffer, str);
	int i = 0;
	for(i = 0; i < strlen(stripbuffer); i++) {
		if(str[i] == '\\' || str[i] == '/' || str[i] == ':'|| str[i] == '*'
		|| str[i] == '?'|| str[i] == '"'|| str[i] == '<'|| str[i] == '>'|| str[i] == '|') {
			stripbuffer[i] = '_';
		}
	}
	return &stripbuffer[0];
}

void drawCurrentDevice() {
	if(deviceHandler_initial == NULL)
		return;
	device_info* info = deviceHandler_info();
	int slot = -1;
	
	DrawTransparentBox(30, 100, 135, 180);	// Device icon + slot box
	DrawImage(info->textureId, 50, 95, 64, 64, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);	
	if(deviceHandler_initial == &initial_SD0 || deviceHandler_initial == &initial_SD1)
		slot = (deviceHandler_initial->name[2] == 'b');
	else if(deviceHandler_initial == &initial_IDE0 || deviceHandler_initial == &initial_IDE1)
		slot = (deviceHandler_initial->name[3] == 'b');
	else if(deviceHandler_initial == &initial_CARDA || deviceHandler_initial == &initial_CARDB)
		slot = (int)deviceHandler_initial->fileBase;
	if(slot != -1)
		WriteFontStyled(50, 160, slot?"SLOT B":"SLOT A", 0.65f, false, defaultColor);

	DrawTransparentBox(30, 200, 135, 305);	// Device size/extra info box
	WriteFontStyled(30, 200, "Total:", 0.6f, false, defaultColor);
	if(info->totalSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%iKB", info->totalSpaceInKB);
	if(info->totalSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)info->totalSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)info->totalSpaceInKB/(1024*1024));
	WriteFontStyled(60, 215, txtbuffer, 0.6f, false, defaultColor);
	
	WriteFontStyled(30, 235, "Free:", 0.6f, false, defaultColor);
	if(info->freeSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%iKB", info->freeSpaceInKB);
	if(info->freeSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)info->freeSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)info->freeSpaceInKB/(1024*1024));
	WriteFontStyled(60, 250, txtbuffer, 0.6f, false, defaultColor);
	
	WriteFontStyled(30, 270, "Used:", 0.6f, false, defaultColor);
	u32 usedSpaceInKB = (info->totalSpaceInKB)-(info->freeSpaceInKB);
	if(usedSpaceInKB < 1024)	// < 1 MB
		sprintf(txtbuffer,"%iKB", usedSpaceInKB);
	if(usedSpaceInKB < 1024*1024)	// < 1 GB
		sprintf(txtbuffer,"%.2fMB", (float)usedSpaceInKB/1024);
	else
		sprintf(txtbuffer,"%.2fGB", (float)usedSpaceInKB/(1024*1024));
	WriteFontStyled(60, 285, txtbuffer, 0.6f, false, defaultColor);
}

// Draws all the files in the current dir.
void drawFiles(file_handle** directory, int num_files) {
	int j = 0;
	current_view_start = MIN(MAX(0,curSelection-FILES_PER_PAGE/2),MAX(0,num_files-FILES_PER_PAGE));
	current_view_end = MIN(num_files, MAX(curSelection+FILES_PER_PAGE/2,FILES_PER_PAGE));
	doBackdrop();
	drawCurrentDevice();
	int fileListBase = 105;
	int scrollBarHeight = fileListBase+(FILES_PER_PAGE*20)+50;
	int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)num_files);
	if(num_files > 0) {
		// Draw which directory we're in
		sprintf(txtbuffer, "%s", &curFile.name[0]);
		float scale = GetTextScaleToFitInWidthWithMax(txtbuffer, ((vmode->fbWidth-150)-20), .85);
		WriteFontStyled(150, 85, txtbuffer, scale, false, defaultColor);
		DrawVertScrollBar(vmode->fbWidth-25, fileListBase, 16, scrollBarHeight, (float)((float)curSelection/(float)(num_files-1)),scrollBarTabHeight);
		for(j = 0; current_view_start<current_view_end; ++current_view_start,++j) {
			populate_meta(&((*directory)[current_view_start]));
			DrawFileBrowserButton(150, fileListBase+(j*40), vmode->fbWidth-30, fileListBase+(j*40)+40, 
									getRelativeName((*directory)[current_view_start].name),
									&((*directory)[current_view_start]), 
									(current_view_start == curSelection) ? B_SELECTED:B_NOSELECT,-1);
		}
	}
	DrawFrameFinish();
}

void renderFileBrowser(file_handle** directory, int num_files)
{
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) return;
  
	while(1){
		drawFiles(directory, num_files);
		while ((PAD_StickY(0) > -16 && PAD_StickY(0) < 16) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN))
			{ VIDEO_WaitVSync (); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_UP) || PAD_StickY(0) > 16){	curSelection = (--curSelection < 0) ? num_files-1 : curSelection;}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || PAD_StickY(0) < -16) {curSelection = (curSelection + 1) % num_files;	}
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
				manage_file();
				// If we return from doing something with a file, refresh the device in the same dir we were at
				memcpy(&curFile, &curDir, sizeof(file_handle));
				needsRefresh=1;
			}
			return;
		}
		
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B)	{
			curMenuLocation=ON_OPTIONS;
			return;
		}
		if(PAD_StickY(0) < -16 || PAD_StickY(0) > 16) {
			usleep((abs(PAD_StickY(0)) > 64 ? 50000:100000) - abs(PAD_StickY(0)*64));
		}
		else {
			while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)))
				{ VIDEO_WaitVSync (); }
		}
	}
}

void select_dest_dir(file_handle* directory, file_handle* selection)
{
	file_handle *directories = NULL;
	file_handle curDir;
	memcpy(&curDir, directory, sizeof(file_entry));
	int i = 0, j = 0, max = 0, refresh = 1, num_files =0, idx = 0;
	
	while(1){
		// Read the directory
		if(refresh) {
			num_files = deviceHandler_dest_readDir(&curDir, &directories, IS_DIR);
			sortFiles(directories, num_files);
			refresh = idx = 0;
		}
		doBackdrop();
		DrawEmptyBox(20,40, vmode->fbWidth-20, 450, COLOR_BLACK);
		WriteFont(50, 55, "Enter directory and press X");
		i = MIN(MAX(0,idx-FILES_PER_PAGE/2),MAX(0,num_files-FILES_PER_PAGE));
		max = MIN(num_files, MAX(idx+FILES_PER_PAGE/2,FILES_PER_PAGE));
		for(j = 0; i<max; ++i,++j) {
			DrawSelectableButton(50,90+(j*50), 550, 90+(j*50)+50, getRelativeName((directories)[i].name), (i == idx) ? B_SELECTED:B_NOSELECT,-1);
		}
		DrawFrameFinish();
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
	free(directories);
}

unsigned int load_app(int multiDol)
{
	char* gameID = (char*)0x80000000;
	int i = 0;
	u32 main_dol_size = 0;
	u8 *main_dol_buffer = 0;
	DOLHEADER dolhdr;
	
	// If there's no drive, we need to use blocking patches.
	if(!swissSettings.hasDVDDrive) multiDol = 1;
	
	memcpy((void*)0x80000020,GC_DefaultConfig,0xE0);
  
	// Read the game header to 0x80000000 & apploader header
	deviceHandler_seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(unsigned char*)0x80000000,32) != 32) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Apploader Header Failed to read");
		DrawFrameFinish();
		while(1);
	}

	DrawFrameStart();
	DrawProgressBar(33, "Reading Main DOL");
	DrawFrameFinish();
	
	// Don't needlessly apply audio streaming if the game doesn't want it
	if(!GCMDisk.AudioStreaming || curDevice == WKF || curDevice == DVD_DISC) {
		swissSettings.muteAudioStreaming = 1;
	}
	
	// Adjust top of memory
	u32 top_of_main_ram = swissSettings.muteAudioStreaming ? 0x81800000 : DECODED_BUFFER_0;
	// Steal even more if there's cheats!
	if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
		top_of_main_ram = WIIRD_ENGINE;
	}

	print_gecko("Top of RAM simulated as: 0x%08X\r\n", top_of_main_ram);
	
	// Read FST to top of Main Memory (round to 32 byte boundary)
	u32 fstSizeAligned = GCMDisk.MaxFSTSize + (32-(GCMDisk.MaxFSTSize%32));
	deviceHandler_seekFile(&curFile,GCMDisk.FSTOffset,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(void*)(top_of_main_ram-fstSizeAligned),GCMDisk.MaxFSTSize) != GCMDisk.MaxFSTSize) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Failed to read fst.bin");
		DrawFrameFinish();
		while(1);
	}
	
	// Read bi2.bin (Disk Header Information) to just under the FST
	deviceHandler_seekFile(&curFile,0x440,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(void*)(top_of_main_ram-fstSizeAligned-0x2000),0x2000) != 0x2000) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Failed to read bi2.bin");
		DrawFrameFinish();
		while(1);
	}

	*(volatile u32*)0x800000F4 = top_of_main_ram-fstSizeAligned-0x2000;	// bi2.bin location
	*(volatile u32*)0x80000038 = top_of_main_ram-fstSizeAligned;		// FST Location in ram
	*(volatile u32*)0x8000003C = GCMDisk.MaxFSTSize;					// FST Max Length
	*(volatile u32*)0x80000034 = *(volatile u32*)0x80000038;			// Arena Hi
	*(volatile u32*)0x80000028 = top_of_main_ram & 0x01FFFFFF;			// Physical Memory Size
    *(volatile u32*)0x800000F0 = top_of_main_ram & 0x01FFFFFF;			// Console Simulated Mem size
	u32* osctxblock = (u32*)memalign(32, 1024);
	*(volatile u32*)0x800000C0 = (u32)osctxblock | 0x7FFFFFFF;
	*(volatile u32*)0x800000D4 = (u32)osctxblock;
	memset(osctxblock, 0, 1024);
		
	print_gecko("Main DOL Lives at %08X\r\n", GCMDisk.DOLOffset);
	
	// Read the Main DOL header
	deviceHandler_seekFile(&curFile,GCMDisk.DOLOffset,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,&dolhdr,DOLHDRLENGTH) != DOLHDRLENGTH) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Failed to read Main DOL Header");
		DrawFrameFinish();
		while(1);
	}
	
	// Figure out the size of the Main DOL so that we can read it all
	for (i = 0; i < MAXTEXTSECTION; i++) {
		if (dolhdr.textLength[i] + dolhdr.textOffset[i] > main_dol_size)
			main_dol_size = dolhdr.textLength[i] + dolhdr.textOffset[i];
	}
	for (i = 0; i < MAXDATASECTION; i++) {
		if (dolhdr.dataLength[i] + dolhdr.dataOffset[i] > main_dol_size)
			main_dol_size = dolhdr.dataLength[i] + dolhdr.dataOffset[i];
	}
	print_gecko("Main DOL size %i\r\n", main_dol_size);

	// Read the entire Main DOL
	main_dol_buffer = (u8*)memalign(32,main_dol_size+DOLHDRLENGTH);
	print_gecko("Main DOL buffer %08X\r\n", (u32)main_dol_buffer);
	deviceHandler_seekFile(&curFile,GCMDisk.DOLOffset,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(void*)main_dol_buffer,main_dol_size+DOLHDRLENGTH) != main_dol_size+DOLHDRLENGTH) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Failed to read Main DOL");
		DrawFrameFinish();
		while(1);
	}

	// Patch to read from SD/HDD/USBGecko/WKF (if frag req or audio streaming)
	if((curDevice == SD_CARD)
		||(curDevice == IDEEXI)
		||(curDevice == USBGECKO)) {
		u32 ret = Patch_DVDLowLevelRead(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
		if(READ_PATCHED_ALL != ret)	{
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Failed to find necessary functions for patching!");
			DrawFrameFinish();
			sleep(5);
		}
	}
	
	// Only set up the WKF fragmentation patch if we have to.
	if(curDevice == WKF && wkfFragSetupReq) {
		u32 ret = Patch_DVDLowLevelReadForWKF(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
		if(1 != ret) {
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Fragmentation patch failed to apply!");
			DrawFrameFinish();
			sleep(5);
			return 0;
		}
	}
		
	// Patch specific game hacks
	Patch_GameSpecific(main_dol_buffer, main_dol_size+DOLHDRLENGTH, gameID);

	// 2 Disc support with no modchip
	if((curDevice == DVD_DISC) && (is_gamecube()) && (drive_status == DEBUG_MODE)) {
		Patch_DVDReset(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	}
	// Support patches for multi-dol discs that need to read patched data from SD
	if(multiDol && curDevice == DVD_DISC && is_gamecube()) {
		Patch_DVDLowLevelReadForDVD(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
	}
	
	// Patch OSReport to print out over USBGecko
	if(swissSettings.debugUSB && usb_isgeckoalive(1) && !swissSettings.wiirdDebug) {
		Patch_Fwrite(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	}
	// Force Video Mode
	if(swissSettings.gameVMode > 0) {
		Patch_VidMode(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
	}
	// Force Widescreen
	if(swissSettings.forceWidescreen) {
		Patch_WideAspect(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
	}
	// Force Anisotropy
	if(swissSettings.forceAnisotropy) {
		Patch_TexFilt(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
	}
	// Force Encoding
	Patch_FontEnc(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	
	// Cheats
	if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
		Patch_CheatsHook(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
	}

	DCFlushRange(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	ICInvalidateRange(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	
	// See if the combination of our patches has exhausted our play area.
	if(!install_code()) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Too many patches enabled memory limit reached!");
		DrawFrameFinish();
		wait_press_A();
		return 0;
	}
	
	deviceHandler_deinit(&curFile);
	
	DrawFrameStart();
	DrawProgressBar(100, "Executing Game!");
	DrawFrameFinish();

	do_videomode_swap();
	VIDEO_SetPostRetraceCallback (NULL);
	*(volatile u32*)0x800000CC = VIDEO_GetCurrentTvMode();
	DCFlushRange((void*)0x80000000, 0x3100);
	ICInvalidateRange((void*)0x80000000, 0x3100);
	
	// Try a device speed test using the actual in-game read code
	if((curDevice == SD_CARD)||(curDevice == IDEEXI)||(curDevice == USBGECKO)) {
		print_gecko("Attempting speed test\r\n");
		char *buffer = memalign(32,1024*1024);
		typedef u32 (*_calc_speed) (void* dst, u32 len, u32 *speed);
		_calc_speed calculate_speed = (_calc_speed) (void*)(CALC_SPEED);
		u32 speed = 0;
		if(curDevice == IDEEXI) {
			calculate_speed(buffer, 1024*1024, &speed);	//Once more for HDD seek
			speed = 0;
		}
		calculate_speed(buffer, 1024*1024, &speed);
		float timeTakenInSec = (float)speed/1000000;
		print_gecko("Speed is %i usec %.2f sec for 1MB\r\n",speed,timeTakenInSec);
		float bytePerSec = (1024*1024) / timeTakenInSec;
		print_gecko("Speed is %.2f KB/s\r\n",bytePerSec/1024);
		print_gecko("Speed for 1024 bytes is: %i usec\r\n",speed/1024);
		*(unsigned int*)VAR_DEVICE_SPEED = speed/1024;
		free(buffer);
	}
	
	if(swissSettings.hasDVDDrive) {
		// Check DVD Status, make sure it's error code 0
		print_gecko("DVD: %08X\r\n",dvd_get_error());
	}
	// Clear out some patch variables
	*(volatile unsigned int*)VAR_FAKE_IRQ_SET = 0;
	*(volatile unsigned int*)VAR_INTERRUPT_TIMES = 0;
	*(volatile unsigned int*)VAR_READS_IN_AS = 0;
	*(volatile unsigned int*)VAR_DISC_CHANGING = 0;
	*(volatile unsigned int*)VAR_LAST_OFFSET = 0xBEEFCAFE;
	*(volatile unsigned int*)VAR_AS_ENABLED = !swissSettings.muteAudioStreaming;
	memset((void*)VAR_DI_REGS, 0, 0x24);
	memset((void*)VAR_STREAM_START, 0, 0xA0);
	print_gecko("Audio Streaming is %s\r\n",*(volatile unsigned int*)VAR_AS_ENABLED?"Enabled":"Disabled");

	if(swissSettings.wiirdDebug || getEnabledCheatsSize() > 0) {
		kenobi_install_engine();
	}
		
	// Set WKF base offset if not using the frag or audio streaming patch
	if(curDevice == WKF /*&& !wkfFragSetupReq && swissSettings.muteAudioStreaming*/) {
		wkfWriteOffset(*(volatile unsigned int*)VAR_DISC_1_LBA);
	}
	print_gecko("libogc shutdown and boot game!\r\n");
	DOLtoARAM(main_dol_buffer, 0, NULL);
	return 0;
}

void boot_dol()
{ 
	unsigned char *dol_buffer;
	unsigned char *ptr;
  
	dol_buffer = (unsigned char*)memalign(32,curFile.size);
	if(!dol_buffer) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"DOL is too big. Press A.");
		DrawFrameFinish();
		wait_press_A();
		return;
	}
		
	int i=0;
	ptr = dol_buffer;
	for(i = 0; i < curFile.size; i+= 131072) {
		sprintf(txtbuffer, "Loading DOL [%d/%d Kb] ..",curFile.size/1024,(SYS_GetArena1Size()+curFile.size)/1024);
		DrawFrameStart();
		DrawProgressBar((int)((float)((float)i/(float)curFile.size)*100), txtbuffer);
		DrawFrameFinish();
    
		deviceHandler_seekFile(&curFile,i,DEVICE_HANDLER_SEEK_SET);
		int size = i+131072 > curFile.size ? curFile.size-i : 131072; 
		if(deviceHandler_readFile(&curFile,ptr,size)!=size) {
			DrawFrameStart();
			DrawMessageBox(D_FAIL,"Failed to read DOL. Press A.");
			DrawFrameFinish();
			wait_press_A();
			return;
		}
  		ptr+=size;
	}
	
	// Build a command line to pass to the DOL
	int argc = 0;
	char *argv[1024];

	// If there's a .cli file next to the DOL, use that as a source for arguments
	for(i = 0; i < files; i++) {
		if (i == curSelection) continue;
		int eq = !strncmp(allFiles[i].name, allFiles[curSelection].name, strlen(allFiles[curSelection].name)-3);
		if(eq && (endsWith(allFiles[i].name,".cli") || endsWith(allFiles[i].name,".dcp"))) {
			
			file_handle *argFile = &allFiles[i];
			char *cli_buffer = memalign(32, argFile->size);
			if(cli_buffer) {
				deviceHandler_seekFile(argFile, 0, DEVICE_HANDLER_SEEK_SET);
				deviceHandler_readFile(argFile, cli_buffer, argFile->size);

				// CLI support
				if(endsWith(allFiles[i].name,".cli")) {
					argv[argc] = (char*)&curFile.name;
					argc++;
					// First argument is at the beginning of the file
					if(cli_buffer[0] != '\r' && cli_buffer[0] != '\n') {
						argv[argc] = cli_buffer;
						argc++;
					}

					// Search for the others after each newline
					for(i = 0; i < argFile->size; i++) {
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
				// DCP support
				if(endsWith(allFiles[i].name,".dcp")) {
					parseParameters(cli_buffer);
					Parameters *params = (Parameters*)getParameters();
					if(params->num_params > 0) {
						DrawArgsSelector(getRelativeName(allFiles[curSelection].name));
						// Get an argv back or none.
						populateArgv(&argc, argv, (char*)&curFile.name);
					}
				}
			}
		}
	}

	// Boot
	DOLtoARAM(dol_buffer, argc, argc == 0 ? NULL : argv);
}

/* Manage file  - The user will be asked what they want to do with the currently selected file - copy/move/delete*/
void manage_file() {
	// If it's a file
	if(curFile.fileAttrib == IS_FILE) {
		if(!swissSettings.enableFileManagement) {
			load_file();
			return;
		}
		// Ask the user what they want to do with it
		DrawFrameStart();
		DrawEmptyBox(10,150, vmode->fbWidth-10, 350, COLOR_BLACK);
		WriteFontStyled(640/2, 160, "Manage File:", 1.0f, true, defaultColor);
		float scale = GetTextScaleToFitInWidth(getRelativeName(curFile.name), vmode->fbWidth-10-10);
		WriteFontStyled(640/2, 200, getRelativeName(curFile.name), scale, true, defaultColor);
		if(deviceHandler_deleteFile) {
			WriteFontStyled(640/2, 230, "(A) Load (X) Copy (Y) Move (Z) Delete", 1.0f, true, defaultColor);
		}
		else {
			WriteFontStyled(640/2, 230, "(A) Load (X) Copy", 1.0f, true, defaultColor);
		}
		WriteFontStyled(640/2, 300, "Press an option to Continue, or B to return", 1.0f, true, defaultColor);
		DrawFrameFinish();
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
		int option = 0;
		while(1) {
			u32 buttons = PAD_ButtonsHeld(0);
			if(buttons & PAD_BUTTON_X) {
				option = COPY_OPTION;
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_X){ VIDEO_WaitVSync (); }
				break;
			}
			if(deviceHandler_deleteFile && (buttons & PAD_BUTTON_Y)) {
				option = MOVE_OPTION;
				while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
				break;
			}
			if(deviceHandler_deleteFile && (buttons & PAD_TRIGGER_Z)) {
				option = DELETE_OPTION;
				while(PAD_ButtonsHeld(0) & PAD_TRIGGER_Z){ VIDEO_WaitVSync (); }
				break;
			}
			if(buttons & PAD_BUTTON_A) {
				load_file();
				return;
			}
			if(buttons & PAD_BUTTON_B) {
				return;
			}
		}
	
		// If delete, delete it + refresh the device
		if(option == DELETE_OPTION) {
			if(!deviceHandler_deleteFile(&curFile)) {
				//go up a folder
				int len = strlen(&curFile.name[0]);
				while(len && curFile.name[len-1]!='/') {
      				len--;
				}
				curFile.name[len-1] = '\0';
				DrawFrameStart();
				DrawMessageBox(D_INFO,"File deleted! Press A to continue");
				DrawFrameFinish();
				wait_press_A();
				needsRefresh=1;
			}
			else {
				DrawFrameStart();
				DrawMessageBox(D_INFO,"Delete Failed! Press A to continue");
				DrawFrameFinish();
			}
		}
		// If copy, ask which device is the destination device and copy
		else if((option == COPY_OPTION) || (option == MOVE_OPTION)) {
			int ret = 0;
			// Show a list of destination devices (the same device is also a possibility)
			select_copy_device();
			// If the devices are not the same, init the second, fail on non-existing device/etc
			if(deviceHandler_initial != deviceHandler_dest_initial) {
				deviceHandler_dest_deinit( deviceHandler_dest_initial );
				if(!deviceHandler_dest_init( deviceHandler_dest_initial )) {
					// Try the alternate slot for SDGecko or IDE-EXI
					if(deviceHandler_dest_initial->name[0] == 's')
						deviceHandler_dest_initial = (deviceHandler_dest_initial == &initial_SD0) ?
												&initial_SD1:&initial_SD0;
					else
						deviceHandler_dest_initial = (deviceHandler_dest_initial == &initial_IDE0) ?
												&initial_IDE1:&initial_IDE0;

					print_gecko("Trying alternate slot...\r\n");
					if(!deviceHandler_dest_init( deviceHandler_dest_initial )) {
						DrawFrameStart();
						sprintf(txtbuffer, "Failed to init destination device! (%i)",ret);
						DrawMessageBox(D_FAIL,txtbuffer);
						DrawFrameFinish();
						wait_press_A();
						return;
					}
				}
			}
			// Traverse this destination device and let the user select a directory to dump the file in
			file_handle *destFile = memalign(32,sizeof(file_handle));
			
			// Show a directory only browser and get the destination file location
			select_dest_dir(deviceHandler_dest_initial, destFile);
			sprintf(destFile->name, "%s/%s",destFile->name,getRelativeName(&curFile.name[0]));
			destFile->fp = 0;
			destFile->fileBase = 0;
			destFile->size = 0;
			destFile->fileAttrib = IS_FILE;
			destFile->status = 0;
			destFile->offset = 0;

			// If the destination file already exists, ask the user what to do
			u8 nothing[1];
			if(deviceHandler_dest_readFile(destFile, nothing, 1) >= 0) {
				DrawFrameStart();
				DrawEmptyBox(10,150, vmode->fbWidth-10, 350, COLOR_BLACK);
				WriteFontStyled(640/2, 160, "File exists:", 1.0f, true, defaultColor);
				float scale = GetTextScaleToFitInWidth(getRelativeName(curFile.name), vmode->fbWidth-10-10);
				WriteFontStyled(640/2, 200, getRelativeName(curFile.name), scale, true, defaultColor);
				WriteFontStyled(640/2, 230, "(A) Rename (Z) Overwrite", 1.0f, true, defaultColor);
				WriteFontStyled(640/2, 300, "Press an option to Continue, or B to return", 1.0f, true, defaultColor);
				DrawFrameFinish();

				while(PAD_ButtonsHeld(0) & (PAD_BUTTON_A | PAD_TRIGGER_Z)) { VIDEO_WaitVSync (); }
				while(1) {
					u32 buttons = PAD_ButtonsHeld(0);
					if(buttons & PAD_TRIGGER_Z) {
						if(!strcmp(curFile.name, destFile->name)) {
							DrawFrameStart();
							DrawMessageBox(D_INFO, "Can't overwrite a file with itself!");
							DrawFrameFinish();
							wait_press_A();
							return; 
						}
						else {
							deviceHandler_dest_deleteFile(destFile);
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

						deviceHandler_dest_closeFile(destFile);

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
								DrawFrameStart();
								DrawMessageBox(D_INFO, "File name too long!");
								DrawFrameFinish();
								wait_press_A();
								return;
							}
							destFile->name[cursor - 3] = '_';
							sprintf(destFile->name + cursor - 2, "%02i", copy_num);
						}

						if(extension_start >= 0) {
							strcpy(destFile->name + cursor, name_backup + extension_start);
						}

						while(deviceHandler_dest_readFile(destFile, nothing, 1) >= 0) {
							deviceHandler_dest_closeFile(destFile);
							copy_num++;
							if(copy_num > 99) {
								DrawFrameStart();
								DrawMessageBox(D_INFO, "Too many copies!");
								DrawFrameFinish();
								wait_press_A();
								return;
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
						return;
					}
				}
			}

			// Seek back to 0 after all these reads
			deviceHandler_dest_seekFile(destFile, 0, DEVICE_HANDLER_SEEK_SET);
			
			// Same (fat based) device and user wants to move the file, just rename ;)
			if(deviceHandler_initial == deviceHandler_dest_initial 
				&& option == MOVE_OPTION 
				&& (deviceHandler_initial->name[0] =='s' || deviceHandler_initial->name[0] =='i')) {
				ret = rename(&curFile.name[0], &destFile->name[0]);
				//go up a folder
				int len = strlen(&curFile.name[0]);
				while(len && curFile.name[len-1]!='/') {
					len--;
				}
				curFile.name[len-1] = '\0';
				needsRefresh=1;
				DrawFrameStart();
				DrawMessageBox(D_INFO,ret ? "Move Failed! Press A to continue":"File moved! Press A to continue");
				DrawFrameFinish();
				wait_press_A();
			}
			else {
				u32 isDestCard = deviceHandler_dest_writeFile == deviceHandler_CARD_writeFile;
				if(isDestCard && (!endsWith(destFile->name,".gci"))) {
					// Only .GCI files can go to the memcard
					DrawFrameStart();
					DrawMessageBox(D_INFO,"Only GCI files allowed on memcard. Press A to continue");
					DrawFrameFinish();
					wait_press_A();
					return;
				}
			
				// Read from one file and write to the new directory
				u32 isCard = deviceHandler_readFile == deviceHandler_CARD_readFile;
				u32 curOffset = 0, cancelled = 0, chunkSize = (isCard||isDestCard) ? curFile.size : (256*1024);
				char *readBuffer = (char*)memalign(32,chunkSize);
				
				while(curOffset < curFile.size) {
					u32 buttons = PAD_ButtonsHeld(0);
					if(buttons & PAD_BUTTON_B) {
						cancelled = 1;
						break;
					}
					sprintf(txtbuffer, "Copying to: %s",getRelativeName(destFile->name));
					DrawFrameStart();
					DrawProgressBar((int)((float)((float)curOffset/(float)curFile.size)*100), txtbuffer);
					DrawFrameFinish();
					u32 amountToCopy = curOffset + chunkSize > curFile.size ? curFile.size - curOffset : chunkSize;
					ret = deviceHandler_readFile(&curFile, readBuffer, amountToCopy);
					if(ret != amountToCopy) {	// Retry the read.
						deviceHandler_seekFile(&curFile, curFile.offset-ret, DEVICE_HANDLER_SEEK_SET);
						ret = deviceHandler_readFile(&curFile, readBuffer, amountToCopy);
						if(ret != amountToCopy) {
							DrawFrameStart();
							sprintf(txtbuffer, "Failed to Read! (%i %i)\n%s",amountToCopy,ret, &curFile.name[0]);
							DrawMessageBox(D_FAIL,txtbuffer);
							DrawFrameFinish();
							wait_press_A();
							return;
						}
					}
					ret = deviceHandler_dest_writeFile(destFile, readBuffer, amountToCopy);
					if(ret != amountToCopy) {
						DrawFrameStart();
						sprintf(txtbuffer, "Failed to Write! (%i %i)\n%s",amountToCopy,ret,destFile->name);
						DrawMessageBox(D_FAIL,txtbuffer);
						DrawFrameFinish();
						wait_press_A();
						return;
					}
					curOffset+=amountToCopy;
				}

				// Handle empty files as a special case
				if(curFile.size == 0) {
					ret = deviceHandler_dest_writeFile(destFile, readBuffer, 0);
					if(ret != 0) {
						DrawFrameStart();
						sprintf(txtbuffer, "Failed to Write! (%i %i)\n%s",0,ret,destFile->name);
						DrawMessageBox(D_FAIL,txtbuffer);
						DrawFrameFinish();
						wait_press_A();
						return;
					}
				}
					
				if(deviceHandler_dest_initial->name[0] == 'i' || deviceHandler_dest_initial->name[0] == 's') {
					fclose(destFile->fp);
				}
				free(destFile);
				DrawFrameStart();
				if(!cancelled) {
					// If cut, delete from source device
					if(option == MOVE_OPTION) {
						deviceHandler_deleteFile(&curFile);
						needsRefresh=1;
						DrawMessageBox(D_INFO,"Move Complete!");
					}
					else {
						DrawMessageBox(D_INFO,"Copy Complete! Press A to continue");
					}
				} 
				else {
					DrawMessageBox(D_INFO,"Operation Cancelled! Press A to continue");
				}
				DrawFrameFinish();
				wait_press_A();
			}
		}
	}
	// Else if directory, mention support not yet implemented.
	else {
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Directory support not implemented");
		DrawFrameFinish();
	}
}

/* Execute/Load/Parse the currently selected file */
void load_file()
{
	char *fileName = &curFile.name[0];
		
	// If it's not a DVD Disc, or it's a DVD disc with some file structure, browse by file type
	if((curDevice != DVD_DISC) || (curDevice == DVD_DISC && dvdDiscTypeInt==ISO9660_DISC)) {
		//if it's a DOL, boot it
		if(strlen(fileName)>4) {
			if(endsWith(fileName,".dol")) {
				boot_dol();
				// if it was invalid (overlaps sections, too large, etc..) it'll return
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Invalid DOL");
				DrawFrameFinish();
				sleep(2);
				return;
			}
			if(endsWith(fileName,".mp3")) {
				mp3_player(allFiles, files, &curFile);
				return;
			}
			if(endsWith(fileName,".fzn")) {
				if(curFile.size != 0x1D0000) {
					DrawFrameStart();
					DrawMessageBox(D_WARN, "File Size must be 0x1D0000 bytes!");
					DrawFrameFinish();
					sleep(2);
					return;
				}
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Reading Flash File ...");
				DrawFrameFinish();
				u8 *flash = (u8*)memalign(32,0x1D0000);
				deviceHandler_seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
				deviceHandler_readFile(&curFile,flash,0x1D0000);
				// Try to read a .fw file too.
				file_handle fwFile;
				memset(&fwFile, 0, sizeof(file_handle));
				sprintf(&fwFile.name[0],"%s.fw", &curFile.name[0]);
				u8 *firmware = (u8*)memalign(32, 0x3000);
				DrawFrameStart();
				if(curDevice == DVD_DISC || deviceHandler_readFile(&fwFile,firmware,0x3000) != 0x3000) {
					free(firmware); firmware = NULL;
					DrawMessageBox(D_WARN, "Didn't find a firmware file, flashing menu only.");
				}
				else {
					DrawMessageBox(D_INFO, "Found firmware file, this will be flashed too.");
				}
				DrawFrameFinish();
				sleep(1);
				wkfWriteFlash(flash, firmware);
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Flashing Complete !!");
				DrawFrameFinish();
				sleep(2);
				return;
			}
			if(!(endsWith(fileName,".iso") || endsWith(fileName,".gcm"))) {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Unknown File Type\nEnable file management to manage this file.");
				DrawFrameFinish();
				sleep(1);
				return;
			}
			if((endsWith(fileName,".iso") || endsWith(fileName,".gcm")) && (curDevice == SAMBA)) {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Not Supported");
				DrawFrameFinish();
				sleep(1);
				return;
			}
			
		}
	}
	DrawFrameStart();
	DrawMessageBox(D_INFO, "Reading ...");
	DrawFrameFinish();
	
	if((curDevice==WODE)) {
		DrawFrameStart();
		DrawMessageBox(D_INFO, "Setup base offset please Wait ..");
		DrawFrameFinish();
		deviceHandler_setupFile(&curFile, 0);
	}
	
	// boot the GCM/ISO file, gamecube disc or multigame selected entry
	deviceHandler_seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,&GCMDisk,sizeof(DiskHeader)) != sizeof(DiskHeader)) {
		DrawFrameStart();
		DrawMessageBox(D_WARN, "Invalid or Corrupt File!");
		DrawFrameFinish();
		sleep(2);
		return;
	}

	if(GCMDisk.DVDMagicWord != DVD_MAGIC) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Disc does not contain valid word at 0x1C");
		DrawFrameFinish();
		sleep(2);
		return;
	}
	
	// Show game info or return to the menu
	if(!info_game()) {
		return;
	}
	
	int multiDol = 0;
	// Report to the user the patch status of this GCM/ISO file
	if((curDevice == SD_CARD) || (curDevice == IDEEXI) || (curDevice == DVD_DISC)) {
		multiDol = check_game();
	}
	
  	if(curDevice!=WODE) {
		file_handle *secondDisc = NULL;
		
		// If we're booting from SD card or IDE hdd
		if((curDevice == SD_CARD) || (curDevice == IDEEXI) || (curDevice == WKF)) {
			// look to see if it's a two disc game
			// set things up properly to allow disc swapping
			// the files must be setup as so: game-disc1.xxx game-disc2.xxx
			secondDisc = memalign(32,sizeof(file_handle));
			memcpy(secondDisc,&curFile,sizeof(file_handle));
			secondDisc->fp = 0;
			
			// you're trying to load a disc1 of something
			if(curFile.name[strlen(secondDisc->name)-5] == '1') {
				secondDisc->name[strlen(secondDisc->name)-5] = '2';
			} else if(curFile.name[strlen(secondDisc->name)-5] == '2') {
				secondDisc->name[strlen(secondDisc->name)-5] = '1';
			}
			else {
				free(secondDisc);
				secondDisc = NULL;
			}
		}
	  
		// Call the special setup for each device (e.g. SD will set the sector(s))
		if(!deviceHandler_setupFile(&curFile, secondDisc)) {
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Failed to setup the file (too fragmented?)");
			DrawFrameFinish();
			wait_press_A();
			return;
		}

		
		if(secondDisc) {
			free(secondDisc);
		}
	}

	// setup the video mode before we kill libOGC kernel
	ogc_video__reset();
	load_app(multiDol);
}

int check_game()
{ 	
	int multiDol = 0;
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Checking Game ..");
	DrawFrameFinish();
	
	ExecutableFile *filesToPatch = memalign(32, sizeof(ExecutableFile)*512);
	int numToPatch = parse_gcm(&curFile, filesToPatch);
	
	if(numToPatch>0) {
		// Game requires patch files, lets do it.	
		multiDol = patch_gcm(&curFile, filesToPatch, numToPatch, 0);
	}
	free(filesToPatch);
	return multiDol;
}

void save_config(ConfigEntry *config) {
	// Save settings to current device
	if(curDevice == SD_CARD || curDevice == IDEEXI) {
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Saving Config ...");
		DrawFrameFinish();
		if(config_update(config)) {
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Config Saved Successfully!");
			DrawFrameFinish();
		}
		else {
			DrawFrameStart();
			DrawMessageBox(D_INFO,"Config Failed to Save!");
			DrawFrameFinish();
		}
	}
}

void draw_game_info() {
	DrawFrameStart();
	DrawEmptyBox(75,120, vmode->fbWidth-78, 400, COLOR_BLACK);

	sprintf(txtbuffer,"%s",(GCMDisk.DVDMagicWord != DVD_MAGIC)?getRelativeName(&curFile.name[0]):GCMDisk.GameName);
	float scale = GetTextScaleToFitInWidth(txtbuffer,(vmode->fbWidth-78)-75);
	WriteFontStyled(640/2, 130, txtbuffer, scale, true, defaultColor);

	if((curDevice==SD_CARD)||(curDevice == IDEEXI) ||(curDevice == WKF) ||(curDevice == DVD_DISC)) {
		sprintf(txtbuffer,"Size: %.2fMB", (float)curFile.size/1024/1024);
		WriteFontStyled(640/2, 160, txtbuffer, 0.8f, true, defaultColor);
		if((curDevice==SD_CARD)||(curDevice == IDEEXI) ||(curDevice == WKF)) {
			get_frag_list(curFile.name);
			if(frag_list->num > 1)
				sprintf(txtbuffer,"File is in %i fragments.", frag_list->num);
			else
				sprintf(txtbuffer,"File base sector 0x%08X", frag_list->frag[0].sector);
			WriteFontStyled(640/2, 180, txtbuffer, 0.8f, true, defaultColor);
		}
		if(curFile.meta) {
			if(curFile.meta->banner)
				DrawTexObj(&curFile.meta->bannerTexObj, 215, 240, 192, 64, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
			if(curFile.meta->regionTexId != -1 && curFile.meta->regionTexId != 0)
				DrawImage(curFile.meta->regionTexId, 450, 262, 30,20, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);

			sprintf(txtbuffer, "%s", &curFile.meta->description[0]);
			char * tok = strtok (txtbuffer,"\n");
			int line = 0;
			while (tok != NULL)	{
				float scale = GetTextScaleToFitInWidth(tok,(vmode->fbWidth-78)-75);
				WriteFontStyled(640/2, 315+(line*scale*24), tok, scale, true, defaultColor);
				tok = strtok (NULL, "\n");
				line++;
			}
		}
	}
	else if(curDevice == QOOB_FLASH) {
		sprintf(txtbuffer,"Size: %.2fKb (%i blocks)", (float)curFile.size/1024, curFile.size/0x10000);
		WriteFontStyled(640/2, 160, txtbuffer, 0.8f, true, defaultColor);
		sprintf(txtbuffer,"Position on Flash: %08X",(u32)(curFile.fileBase&0xFFFFFFFF));
		WriteFontStyled(640/2, 180, txtbuffer, 0.8f, true, defaultColor);
	}
	else if(curDevice == WODE) {
		ISOInfo_t* isoInfo = (ISOInfo_t*)&curFile.other;
		sprintf(txtbuffer,"Partition: %i, ISO: %i", isoInfo->iso_partition,isoInfo->iso_number);
		WriteFontStyled(640/2, 160, txtbuffer, 0.8f, true, defaultColor);
	}
	else if(curDevice == MEMCARD) {
		sprintf(txtbuffer,"Size: %.2fKb (%i blocks)", (float)curFile.size/1024, curFile.size/8192);
		WriteFontStyled(640/2, 160, txtbuffer, 0.8f, true, defaultColor);
		sprintf(txtbuffer,"Position on Card: %08X",curFile.offset);
		WriteFontStyled(640/2, 180, txtbuffer, 0.8f, true, defaultColor);
	}
	if(GCMDisk.DVDMagicWord == DVD_MAGIC) {
		sprintf(txtbuffer,"Region [%s] Audio Streaming [%s]",(GCMDisk.CountryCode=='P') ? "PAL":"NTSC",(GCMDisk.AudioStreaming=='\1') ? "YES":"NO");
		WriteFontStyled(640/2, 200, txtbuffer, 0.8f, true, defaultColor);
		WriteFontStyled(640/2, 220, (GCMDisk.DiscID ? "Disc 2":""), 0.8f, true, defaultColor);
	}

	WriteFontStyled(640/2, 370, "Settings (X) - Cheats (Y) - Exit (B) - Continue (A)", 0.75f, true, defaultColor);
	DrawFrameFinish();
}

/* Show info about the game - and also load the config for it */
int info_game()
{
	int ret = -1;
	ConfigEntry *config = NULL;
	// Find the config for this game, or default if we don't know about it
	config = memalign(32, sizeof(ConfigEntry));
	*(u32*)&config->game_id[0] = *(u32*)&GCMDisk.ConsoleID;	// Lazy
	strncpy(&config->game_name[0],&GCMDisk.GameName[0],32);
	config_find(config);	// populate
	// load settings
	swissSettings.gameVMode = config->gameVMode;
	swissSettings.softProgressive = config->softProgressive;
	swissSettings.muteAudioStreaming = config->muteAudioStreaming;
	swissSettings.forceWidescreen = config->forceWidescreen;
	swissSettings.forceAnisotropy = config->forceAnisotropy;
	swissSettings.forceEncoding = config->forceEncoding;
	while(1) {
		draw_game_info();
		while(PAD_ButtonsHeld(0) & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y)){ VIDEO_WaitVSync (); }
		while(!(PAD_ButtonsHeld(0) & (PAD_BUTTON_X | PAD_BUTTON_B | PAD_BUTTON_A | PAD_BUTTON_Y))){ VIDEO_WaitVSync (); }
		if(PAD_ButtonsHeld(0) & (PAD_BUTTON_B|PAD_BUTTON_A)){
			ret = (PAD_ButtonsHeld(0) & PAD_BUTTON_A) ? 1:0;
			break;
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) {
			if(show_settings((GCMDisk.DVDMagicWord == DVD_MAGIC) ? &curFile : NULL, config)) {
				save_config(config);
			}
		}
		// Look for a cheats file based on the GameID
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_Y) {
			char trimmedGameId[8];
			memset(trimmedGameId, 0, 8);
			memcpy(trimmedGameId, (char*)&GCMDisk, 6);
			file_handle *cheatsFile = memalign(32,sizeof(file_handle));
			memcpy(cheatsFile, deviceHandler_initial, sizeof(file_handle));
			sprintf(cheatsFile->name, "%s/cheats/%s.txt", deviceHandler_initial->name, trimmedGameId);
			print_gecko("Looking for cheats file @ %s\r\n", cheatsFile->name);
			cheatsFile->size = -1;
			
			deviceHandler_temp_readFile =  deviceHandler_readFile;
			deviceHandler_temp_seekFile =  deviceHandler_seekFile;
			
			// Check SD in both slots if we're not already running from SD, or if we fail from SD
			if((curDevice != SD_CARD && curDevice != IDEEXI && curDevice != WKF) || deviceHandler_temp_readFile(cheatsFile, &trimmedGameId, 8) != 8) {
				if(deviceHandler_initial != &initial_SD0 && deviceHandler_initial != &initial_SD1) {
					int slot = 0;
					deviceHandler_temp_init     =  deviceHandler_FAT_init;
					deviceHandler_temp_readFile =  deviceHandler_FAT_readFile;
					deviceHandler_temp_seekFile =  deviceHandler_FAT_seekFile;
					deviceHandler_temp_deinit   =  deviceHandler_FAT_deinit;
					while(slot < 2) {
						file_handle *slotFile = slot ? &initial_SD1:&initial_SD0;
						// Try SD slots now
						memcpy(cheatsFile, slotFile, sizeof(file_handle));
						sprintf(cheatsFile->name, "%s/cheats/%s.txt", slotFile->name, trimmedGameId);
						print_gecko("Looking for cheats file @ %s\r\n", cheatsFile->name);
						cheatsFile->size = -1;
						deviceHandler_temp_init(cheatsFile);
						if(deviceHandler_temp_readFile(cheatsFile, &trimmedGameId, 8) == 8) {
							break;
						}
						slot++;
					}
				}
				// Still fail?
				if(deviceHandler_temp_readFile(cheatsFile, &trimmedGameId, 8) != 8) {
					while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y);
					DrawFrameStart();
					DrawMessageBox(D_INFO,"No cheats file found.\nPress A to continue.");
					DrawFrameFinish();
					while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
					while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
					return 0;
				}
			}
			print_gecko("Cheats file found with size %i\r\n", cheatsFile->size);
			char *cheats_buffer = memalign(32, cheatsFile->size);
			if(cheats_buffer) {
				deviceHandler_temp_seekFile(cheatsFile, 0, DEVICE_HANDLER_SEEK_SET);
				deviceHandler_temp_readFile(cheatsFile, cheats_buffer, cheatsFile->size);
				parseCheats(cheats_buffer);
				free(cheats_buffer);
				DrawCheatsSelector(getRelativeName(allFiles[curSelection].name));
			}
		}
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	}
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	free(config);
	return ret;
}


void select_copy_device()
{  
	int inAdvanced = 0;
	int inAdvancedPos = 0;
	int slot = 1;
	while(1) {
		doBackdrop();
		DrawEmptyBox(20,190, vmode->fbWidth-20, 355, COLOR_BLACK);
		WriteFontStyled(640/2, 195, "Destination device selection", 1.0f, true, defaultColor);
		if(inAdvanced) {
			// Draw slot / speed selection if advanced menu is showing.
			DrawEmptyBox(vmode->fbWidth-170, 370, vmode->fbWidth-20, 405, COLOR_BLACK);
			WriteFontStyled(vmode->fbWidth-165, 370, slot ? "Slot: B":"Slot: A", 0.65f, false, !inAdvancedPos ? defaultColor:deSelectedColor);
			WriteFontStyled(vmode->fbWidth-165, 385, swissSettings.exiSpeed ? "Speed: Fast":"Speed: Compatible", 0.65f, false, inAdvancedPos ? defaultColor:deSelectedColor);
		}
		WriteFontStyled(vmode->fbWidth-120, 345, "(X) Advanced", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
		if(curCopyDevice==DEST_SD_CARD) {
			DrawImage(TEX_SDSMALL, 640/2, 230, 60, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
			WriteFontStyled(640/2, 330, "SD Card via SD Gecko", 0.85f, true, defaultColor);
		}
		else if(curCopyDevice==DEST_IDEEXI) {
			DrawImage(TEX_HDD, 640/2, 230, 80, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
			WriteFontStyled(640/2, 330, "IDE HDD via IDE-EXI", 0.85f, true, defaultColor);
		}
		else if(curCopyDevice==DEST_MEMCARD) {
			DrawImage(TEX_MEMCARD, 640/2, 230, 107, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
			WriteFontStyled(640/2, 330, "Memory Card", 0.85f, true, defaultColor);
		}
		if(curCopyDevice != 2) {
			WriteFont(520, 270, "->");
		}
		if(curCopyDevice != 0) {
			WriteFont(100, 270, "<-");
		}
		DrawFrameFinish();
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_X)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) )
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_X)
			inAdvanced ^= 1;
		if(inAdvanced) {
			if((btns & PAD_BUTTON_DOWN) || (btns & PAD_BUTTON_UP)) {
				inAdvancedPos = inAdvancedPos + ((btns & PAD_BUTTON_DOWN) ? 1:-1);
				if(inAdvancedPos > 1) inAdvancedPos = 0;
				if(inAdvancedPos < 0) inAdvancedPos = 1;
			}
			if((btns & PAD_BUTTON_RIGHT) || (btns & PAD_BUTTON_LEFT)) {
				if(!inAdvancedPos) {
					slot ^= 1;
				}
				else {
					swissSettings.exiSpeed^=1;
				}
			}
		}
		else {
			if((btns & PAD_BUTTON_RIGHT) && curCopyDevice < 2)
				curCopyDevice++;
			if((btns & PAD_BUTTON_LEFT) && curCopyDevice > 0)
				curCopyDevice--;
			if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_B))
				break;
		}
		if(btns & PAD_BUTTON_A) {
			if(!inAdvanced)
				break;
			else 
				inAdvanced = 0;
		}
		if(btns & PAD_BUTTON_B) break;
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) ))
			{ VIDEO_WaitVSync (); }
	}
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
	// Deinit any existing deviceHandler state
	if((deviceHandler_dest_deinit && deviceHandler_dest_initial) && (deviceHandler_dest_initial != deviceHandler_initial)) deviceHandler_dest_deinit( deviceHandler_dest_initial );
	// Change all the deviceHandler pointers based on the current device
	switch(curCopyDevice) {
		case DEST_SD_CARD:
		case DEST_IDEEXI:
			if(curCopyDevice==DEST_IDEEXI)
				deviceHandler_dest_initial = !slot ? &initial_IDE0 : &initial_IDE1;
			else
				deviceHandler_dest_initial = !slot ? &initial_SD0 : &initial_SD1;
			deviceHandler_dest_readDir  =  deviceHandler_FAT_readDir;
			deviceHandler_dest_readFile =  deviceHandler_FAT_readFile;
			deviceHandler_dest_writeFile=  deviceHandler_FAT_writeFile;
			deviceHandler_dest_deleteFile=  deviceHandler_FAT_deleteFile;
			deviceHandler_dest_seekFile =  deviceHandler_FAT_seekFile;
			deviceHandler_dest_setupFile=  deviceHandler_FAT_setupFile;
			deviceHandler_dest_init     =  deviceHandler_FAT_init;
			deviceHandler_dest_deinit   =  deviceHandler_FAT_deinit;
			deviceHandler_dest_closeFile=  deviceHandler_FAT_closeFile;
		break;
		case DEST_MEMCARD:
			deviceHandler_dest_initial = !slot ? &initial_CARDA : &initial_CARDB;
			deviceHandler_dest_readDir  =  deviceHandler_CARD_readDir;
			deviceHandler_dest_readFile =  deviceHandler_CARD_readFile;
			deviceHandler_dest_writeFile=  deviceHandler_CARD_writeFile;
			deviceHandler_dest_deleteFile=  deviceHandler_CARD_deleteFile;
			deviceHandler_dest_seekFile =  deviceHandler_CARD_seekFile;
			deviceHandler_dest_setupFile=  deviceHandler_CARD_setupFile;
			deviceHandler_dest_init     =  deviceHandler_CARD_init;
			deviceHandler_dest_deinit   =  deviceHandler_CARD_deinit;
			deviceHandler_dest_closeFile=  deviceHandler_CARD_closeFile;
		break;
	}
}

void select_device(int skipPrompts)
{
	if(is_httpd_in_use()) {
		doBackdrop();
		DrawMessageBox(D_INFO,"Can't load device while HTTP is processing!");
		DrawFrameFinish();
		sleep(5);
		return;
	}

	int slot = 1;
	if(curDevice < 0) {
		curDevice = SD_CARD;
		// Go to the first device after SD Gecko if it's not avail
		if(!deviceHandler_getDeviceAvailable(curDevice)) {
			int i;
			for(i=curDevice+1;i<9;i++) {
				if(deviceHandler_getDeviceAvailable(i)) {
					curDevice = i;
					break;
				}
			}
		}
		dvdDiscTypeStr = NotInitStr;
		int inAdvanced = 0, showAllDevices = 0, inAdvancedPos = 0;
		while(1) {
			doBackdrop();
			DrawEmptyBox(20,190, vmode->fbWidth-20, 410, COLOR_BLACK);
			WriteFontStyled(640/2, 195, "Device Selection", 1.0f, true, defaultColor);		
			if(inAdvanced) {
				// Draw slot / speed selection if advanced menu is showing.
				WriteFontStyled(vmode->fbWidth-160, 370, slot ? "Slot: B":"Slot: A", 0.65f, false, !inAdvancedPos ? defaultColor:deSelectedColor);
				WriteFontStyled(vmode->fbWidth-160, 385, swissSettings.exiSpeed ? "Speed: Fast":"Speed: Compatible", 0.65f, false, inAdvancedPos ? defaultColor:deSelectedColor);
			}
			if(curDevice==DVD_DISC) {
				DrawImage(TEX_GCDVDSMALL, 640/2, 230, 80, 79, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "DVD", 0.85f, true, defaultColor);
				WriteFontStyled(640/2, 350, "Supported File System(s): GCM, ISO 9660, Multi-Game", 0.65f, true, defaultColor);
			}
			else if(curDevice==SD_CARD) {
				DrawImage(TEX_SDSMALL, 640/2, 230, 60, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "SD Card via SD Gecko", 0.85f, true, defaultColor);
				WriteFontStyled(640/2, 350, "Supported File System(s): FAT16, FAT32", 0.65f, true, defaultColor);
				WriteFontStyled(vmode->fbWidth-190, 400, "(X) Advanced Options", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			}
			else if(curDevice==IDEEXI) {
				DrawImage(TEX_HDD, 640/2, 230, 80, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "IDE HDD via IDE-EXI", 0.85f, true, defaultColor);
				WriteFontStyled(640/2, 350, "Supported File System(s): FAT16, FAT32", 0.65f, true, defaultColor);
				WriteFontStyled(vmode->fbWidth-190, 400, "(X) Advanced Options", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			}
			else if(curDevice==QOOB_FLASH) {
				DrawImage(TEX_QOOB, 640/2, 230, 70, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "Qoob PRO", 0.85f, true, defaultColor);
				WriteFontStyled(640/2, 350, "Supported File System(s): QOOB Flash", 0.65f, true, defaultColor);
			}
			else if(curDevice==WODE) {
				DrawImage(TEX_WODEIMG, 640/2, 230, 146, 72, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "WODE Jukebox", 0.85f, true, defaultColor);
				WriteFontStyled(640/2, 350, "Supported File System(s): FAT32, NTFS, EXT2/3, HPFS", 0.65f, true, defaultColor);
			}
			else if(curDevice==MEMCARD) {
				DrawImage(TEX_MEMCARD, 640/2, 230, 107, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "Memory Card", 0.85f, true, defaultColor);
				WriteFontStyled(vmode->fbWidth-190, 400, "(X) Advanced Options", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			}
			else if(curDevice==WKF) {
				DrawImage(TEX_WIIKEY, 640/2, 230, 102, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "Wiikey / Wasp Fusion", 0.85f, true, defaultColor);
				WriteFontStyled(640/2, 350, "Supported File System(s): FAT16, FAT32", 0.65f, true, defaultColor);
			}
			else if(curDevice==USBGECKO) {
				DrawImage(TEX_USBGECKO, 640/2, 230, 129, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "USB Gecko (req. PC app)", 0.85f, true, defaultColor);
				WriteFontStyled(640/2, 350, "Slot B only", 0.65f, true, defaultColor);
			}
			else if(curDevice==SAMBA) {
				DrawImage(TEX_SAMBA, 640/2, 230, 160, 85, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "Samba via BBA", 0.85f, true, defaultColor);
			}
			else if(curDevice==SYS) {
				DrawImage(TEX_SYSTEM, 640/2, 230, 160, 85, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "System", 0.85f, true, defaultColor);
			}
			WriteFont(520, 270, "->");
			WriteFont(100, 270, "<-");
			WriteFontStyled(20, 400, "(Z) Show all devices", 0.65f, false, showAllDevices ? defaultColor:deSelectedColor);
			DrawFrameFinish();
			while (!(PAD_ButtonsHeld(0) & 
				(PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_B|PAD_BUTTON_A
				|PAD_BUTTON_X|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_TRIGGER_Z) ))
				{ VIDEO_WaitVSync (); }
			u16 btns = PAD_ButtonsHeld(0);
			if(btns & PAD_BUTTON_X && ((curDevice == SD_CARD) || (curDevice == IDEEXI) ||(curDevice == MEMCARD)))
				inAdvanced ^= 1;
			if(btns & PAD_TRIGGER_Z)
				showAllDevices ^= 1;
			if(inAdvanced) {
				if((btns & PAD_BUTTON_DOWN) || (btns & PAD_BUTTON_UP)) {
					inAdvancedPos = inAdvancedPos + ((btns & PAD_BUTTON_DOWN) ? 1:-1);
					if(inAdvancedPos > 1) inAdvancedPos = 0;
					if(inAdvancedPos < 0) inAdvancedPos = 1;
				}
				if((btns & PAD_BUTTON_RIGHT) || (btns & PAD_BUTTON_LEFT)) {
					if(!inAdvancedPos) {
						slot ^= 1;
					}
					else {
						swissSettings.exiSpeed^=1;
					}
				}
			}
			else {
				if(btns & PAD_BUTTON_RIGHT) {
					curDevice = curDevice < 9 ? curDevice+1 : 0;
					while(!showAllDevices && !deviceHandler_getDeviceAvailable(curDevice)) {
						if(curDevice == 9)
							curDevice = 0;
						else
							curDevice ++;
					}
				}
				if(btns & PAD_BUTTON_LEFT) {
					curDevice = curDevice > 0 ? curDevice-1 : 9;
					while(!showAllDevices && !deviceHandler_getDeviceAvailable(curDevice)) {
						if(curDevice == 0)
							curDevice = 9;
						else
							curDevice --;
					}
				}
			}
			if(btns & PAD_BUTTON_A) {
				if(!inAdvanced)
					break;
				else 
					inAdvanced = 0;
			}
			if(btns & PAD_BUTTON_B) {
				return;
			}
			while ((PAD_ButtonsHeld(0) & 
				(PAD_BUTTON_RIGHT|PAD_BUTTON_LEFT|PAD_BUTTON_B|PAD_BUTTON_A
				|PAD_BUTTON_X|PAD_BUTTON_UP|PAD_BUTTON_DOWN|PAD_TRIGGER_Z) ))
				{ VIDEO_WaitVSync (); }
		}
		while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
		// Deinit any existing deviceHandler state
		if(deviceHandler_deinit && deviceHandler_initial) deviceHandler_deinit( deviceHandler_initial );
	}
	
	// Change all the deviceHandler pointers based on the current device
	switch(curDevice) {
		case SD_CARD:
		case IDEEXI:
			if(skipPrompts) {
				slot = (deviceHandler_initial->name[2] == 'b');
			}
			if(curDevice==IDEEXI)
				deviceHandler_initial = !slot ? &initial_IDE0 : &initial_IDE1;
			else
				deviceHandler_initial = !slot ? &initial_SD0 : &initial_SD1;
			deviceHandler_readDir  =  deviceHandler_FAT_readDir;
			deviceHandler_readFile =  deviceHandler_FAT_readFile;
			deviceHandler_writeFile=  deviceHandler_FAT_writeFile;
			deviceHandler_deleteFile=  deviceHandler_FAT_deleteFile;
			deviceHandler_seekFile =  deviceHandler_FAT_seekFile;
			deviceHandler_setupFile=  deviceHandler_FAT_setupFile;
			deviceHandler_init     =  deviceHandler_FAT_init;
			deviceHandler_deinit   =  deviceHandler_FAT_deinit;
			deviceHandler_closeFile=  deviceHandler_FAT_closeFile;
			deviceHandler_info 	   =  deviceHandler_FAT_info;
		break;
		case DVD_DISC:
			deviceHandler_initial = &initial_DVD;
			deviceHandler_readDir  =  deviceHandler_DVD_readDir;
			deviceHandler_readFile =  deviceHandler_DVD_readFile;
			deviceHandler_seekFile =  deviceHandler_DVD_seekFile;
			deviceHandler_setupFile=  deviceHandler_DVD_setupFile;
			deviceHandler_init     =  deviceHandler_DVD_init;
			deviceHandler_deinit   =  deviceHandler_DVD_deinit;
			deviceHandler_closeFile=  deviceHandler_DVD_closeFile;
			deviceHandler_info 	   =  deviceHandler_DVD_info;
			deviceHandler_deleteFile = NULL;
 		break;
 		case QOOB_FLASH:
			deviceHandler_initial = &initial_Qoob;
			deviceHandler_readDir  =  deviceHandler_Qoob_readDir;
			deviceHandler_readFile =  deviceHandler_Qoob_readFile;
			deviceHandler_seekFile =  deviceHandler_Qoob_seekFile;
			deviceHandler_setupFile=  deviceHandler_Qoob_setupFile;
			deviceHandler_init     =  deviceHandler_Qoob_init;
			deviceHandler_deinit   =  deviceHandler_Qoob_deinit;
			deviceHandler_closeFile=  deviceHandler_Qoob_closeFile;
			deviceHandler_info 	   =  deviceHandler_Qoob_info;
			deviceHandler_deleteFile = NULL;
 		break;
 		case WODE:
			deviceHandler_initial = &initial_WODE;
			deviceHandler_readDir  =  deviceHandler_WODE_readDir;
			deviceHandler_readFile =  deviceHandler_WODE_readFile;
			deviceHandler_seekFile =  deviceHandler_WODE_seekFile;
			deviceHandler_setupFile=  deviceHandler_WODE_setupFile;
			deviceHandler_init     =  deviceHandler_WODE_init;
			deviceHandler_deinit   =  deviceHandler_WODE_deinit;
			deviceHandler_closeFile=  deviceHandler_WODE_closeFile;
			deviceHandler_info 	   =  deviceHandler_WODE_info;
			deviceHandler_deleteFile = NULL;
 		break;
		case MEMCARD:
			deviceHandler_initial = !slot ? &initial_CARDA : &initial_CARDB;
			deviceHandler_readDir  =  deviceHandler_CARD_readDir;
			deviceHandler_readFile =  deviceHandler_CARD_readFile;
			deviceHandler_writeFile=  deviceHandler_CARD_writeFile;
			deviceHandler_deleteFile=  deviceHandler_CARD_deleteFile;
			deviceHandler_seekFile =  deviceHandler_CARD_seekFile;
			deviceHandler_setupFile=  deviceHandler_CARD_setupFile;
			deviceHandler_init     =  deviceHandler_CARD_init;
			deviceHandler_deinit   =  deviceHandler_CARD_deinit;
			deviceHandler_closeFile=  deviceHandler_CARD_closeFile;
			deviceHandler_info 	   =  deviceHandler_CARD_info;
		break;
		case WKF:
			deviceHandler_initial = &initial_WKF;
			deviceHandler_readDir  =  deviceHandler_WKF_readDir;
			deviceHandler_readFile =  deviceHandler_WKF_readFile;
			deviceHandler_seekFile =  deviceHandler_WKF_seekFile;
			deviceHandler_setupFile=  deviceHandler_WKF_setupFile;
			deviceHandler_init     =  deviceHandler_WKF_init;
			deviceHandler_deinit   =  deviceHandler_WKF_deinit;
			deviceHandler_closeFile=  deviceHandler_WKF_closeFile;
			deviceHandler_info 	   =  deviceHandler_WKF_info;
			deviceHandler_deleteFile = NULL;
		break;
		case USBGECKO:
			deviceHandler_initial = &initial_USBGecko;
			deviceHandler_readDir  =  deviceHandler_USBGecko_readDir;
			deviceHandler_readFile =  deviceHandler_USBGecko_readFile;
			deviceHandler_writeFile = deviceHandler_USBGecko_writeFile;
			deviceHandler_seekFile =  deviceHandler_USBGecko_seekFile;
			deviceHandler_setupFile=  deviceHandler_USBGecko_setupFile;
			deviceHandler_init     =  deviceHandler_USBGecko_init;
			deviceHandler_deinit   =  deviceHandler_USBGecko_deinit;
			deviceHandler_closeFile=  deviceHandler_USBGecko_closeFile;
			deviceHandler_info 	   =  deviceHandler_USBGecko_info;
			deviceHandler_deleteFile = NULL;
		break;
		case SAMBA:
			deviceHandler_initial = &initial_SMB;
			deviceHandler_readDir  =  deviceHandler_SMB_readDir;
			deviceHandler_readFile =  deviceHandler_SMB_readFile;
			deviceHandler_seekFile =  deviceHandler_SMB_seekFile;
			deviceHandler_init     =  deviceHandler_SMB_init;
			deviceHandler_deinit   =  deviceHandler_SMB_deinit;
			deviceHandler_closeFile=  deviceHandler_SMB_closeFile;
			deviceHandler_info 	   =  deviceHandler_SMB_info;
			deviceHandler_deleteFile = NULL;
		break;
		case SYS:
			deviceHandler_initial = &initial_SYS;
			deviceHandler_readDir  =  deviceHandler_SYS_readDir;
			deviceHandler_readFile =  deviceHandler_SYS_readFile;
			deviceHandler_seekFile =  deviceHandler_SYS_seekFile;
			deviceHandler_init     =  deviceHandler_SYS_init;
			deviceHandler_deinit   =  deviceHandler_SYS_deinit;
			deviceHandler_closeFile=  deviceHandler_SYS_closeFile;
			deviceHandler_info 	   =  deviceHandler_SYS_info;
			deviceHandler_deleteFile = NULL;
		break;
	}
	memcpy(&curFile, deviceHandler_initial, sizeof(file_handle));
}

