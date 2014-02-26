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

#include "cheats.h"
#include "frag.h"
#include "swiss.h"
#include "main.h"
#include "httpd.h"
#include "exi.h"
#include "patcher.h"
#include "banner.h"
#include "qchparse.h"
#include "dvd.h"
#include "gcm.h"
#include "mp3.h"
#include "wkf.h"
#include "settings.h"
#include "aram/sidestep.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"

static DiskHeader GCMDisk;      //Gamecube Disc Header struct
char IPLInfo[256] __attribute__((aligned(32)));
GXRModeObj *newmode = NULL;
char txtbuffer[2048];           //temporary text buffer
file_handle curFile;     //filedescriptor for current file
int SDHCCard = 0; //0 == SDHC, 1 == normal SD
int curDevice = 0;  //SD_CARD or DVD_DISC or IDEEXI or WODE
int curCopyDevice = 0;  //SD_CARD or DVD_DISC or IDEEXI or WODE
char *videoStr = NULL;
extern file_handle* allFiles;
extern int files;

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
	DrawFrameStart();
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
				swissSettings.gameVMode = 4;
				break;
			case 'E':
			case 'J':
				swissSettings.gameVMode = 1;
				break;
		}
	} else {
		syssram* sram = __SYS_LockSram();
		sram->ntd = (swissSettings.gameVMode >= 1) && (swissSettings.gameVMode <= 3) ? (sram->ntd|0x40):(sram->ntd&~0x40);
		sram->flags = (swissSettings.gameVMode == 3) || (swissSettings.gameVMode == 6) ? (sram->flags|0x80):(sram->flags&~0x80);
		__SYS_UnlockSram(1);
		while(!__SYS_SyncSram());
	}
	
	/* set TV mode for current game */
	switch(swissSettings.gameVMode) {
		case 1:
			newmode = &TVNtsc480IntDf;
			DrawMessageBox(D_INFO,"Video Mode: NTSC 60Hz");
			break;
		case 2:
			newmode = &TVNtsc480IntDf;
			DrawMessageBox(D_INFO,"Video Mode: NTSC 240p");
			break;
		case 3:
			if(VIDEO_HaveComponentCable()) {
				newmode = &TVNtsc480Prog;
				DrawMessageBox(D_INFO,"Video Mode: NTSC 480p");
			} else {
				swissSettings.gameVMode = 1;	// Can't force with no cable
				newmode = &TVNtsc480IntDf;
				DrawMessageBox(D_INFO,"Video Mode: NTSC 60Hz");
			}
			break;
		case 4:
			newmode = &TVPal576IntDfScale;
			DrawMessageBox(D_INFO,"Video Mode: PAL 50Hz");
			break;
		case 5:
			newmode = &TVPal576IntDfScale;
			DrawMessageBox(D_INFO,"Video Mode: PAL 288p");
			break;
		case 6:
			if(VIDEO_HaveComponentCable()) {
				newmode = &TVPal576ProgScale;
				DrawMessageBox(D_INFO,"Video Mode: PAL 576p");
			} else {
				swissSettings.gameVMode = 4;	// Can't force with no cable
				newmode = &TVPal576IntDfScale;
				DrawMessageBox(D_INFO,"Video Mode: PAL 50Hz");
			}
			break;
		default:
			newmode = &TVNtsc480IntDf;
			DrawMessageBox(D_INFO,"Video Mode: NTSC 60Hz");
	}
	DrawFrameFinish();
}

void do_videomode_swap() {
	if(vmode!=newmode) {
		vmode = newmode;
		VIDEO_Configure (vmode);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		if (vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
		else while (VIDEO_GetNextField())  VIDEO_WaitVSync();
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

	if((curDevice == SD_CARD)||(curDevice == IDEEXI)||(curDevice == WKF)||(curDevice == QOOB_FLASH)||(curDevice == MEMCARD)) {
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
}

// Draws all the files in the current dir.
void drawFiles(file_handle** directory, int num_files) {
	int j = 0;
	int i = MIN(MAX(0,curSelection-FILES_PER_PAGE/2),MAX(0,num_files-FILES_PER_PAGE));
	int max = MIN(num_files, MAX(curSelection+FILES_PER_PAGE/2,FILES_PER_PAGE));
	doBackdrop();
	drawCurrentDevice();
	for(j = 0; i<max; ++i,++j) {
		DrawFileBrowserButton(150,90+(j*40), vmode->fbWidth-30, 90+(j*40)+40, getRelativeName((*directory)[i].name),&((*directory)[i]), (i == curSelection) ? B_SELECTED:B_NOSELECT,-1);
	}
	DrawFrameFinish();
}

// textFileBrowser lives on :)
void textFileBrowser(file_handle** directory, int num_files)
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
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				manage_file();
			}
			return;
		}
		
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B)	{
			memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
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
	int i = 0, j = 0, max = 0, refresh = 1, num_files =0, idx = 0;
	
	while(1){
		// Read the directory
		if(refresh) {
			num_files = deviceHandler_dest_readDir(directory, &directories, IS_DIR);
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
				memcpy(directory, &(directories)[idx], sizeof(file_handle));
				refresh=1;
			}
			else if(directories[idx].fileAttrib==IS_SPECIAL){
				//go up a folder
				int len = strlen(directory->name);
				while(len && directory->name[len-1]!='/') {
      				len--;
				}
				if(len != strlen(directory->name)) {
					directory->name[len-1] = '\0';
					refresh=1;
				}
			}
		}
		if(PAD_StickY(0) < -16 || PAD_StickY(0) > 16) {
			usleep(50000 - abs(PAD_StickY(0)*256));
		}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_X))	{
			memcpy(selection, directory, sizeof(file_handle));
			break;
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)))
			{ VIDEO_WaitVSync (); }
	}
	free(directories);
}

int setup_memcard_emulation() {
	// If memcard emulation is enabled, create the %game%.memcard.sav file
	if(swissSettings.emulatemc) {
		if(deviceHandler_initial != &initial_SD0 && deviceHandler_initial != &initial_DVD &&
			deviceHandler_initial != &initial_WKF && deviceHandler_initial != &initial_WODE) {
			// Memory card emulation will only work from SD Slot A when running a game in SD Slot A or from DVD/WKF/WODE
			DrawFrameStart();
			DrawMessageBox(D_INFO, "For MC emulation, loading device must be:\nDVD, Wode, WKF or SD in Slot A\nPress A to Return ...");
			DrawFrameFinish();
			swissSettings.emulatemc = 0;
			wait_press_A();
			return 0;
		}
		// Check if game device isn't SDGecko in slot A, if so, we need to set up the SDGecko and the patchcode here
		if(deviceHandler_initial != &initial_SD0) {
			DrawFrameStart();
			DrawMessageBox(D_INFO, "Insert a SDGecko inserted into slot A\nPress A when ready ...");
			DrawFrameFinish();
			wait_press_A();
			// Try to init SDGecko in Slot A
			if(deviceHandler_FAT_init(&initial_SD0)) {
				// Setup patch code
				memcpy((void*)0x80001800,sd_bin,sd_bin_size);
			}
			else {
				DrawFrameStart();
				DrawMessageBox(D_FAIL, "SDGecko in Slot A failed to init!\nMemcard emulation disabled, returning to menu.");
				DrawFrameFinish();
				swissSettings.emulatemc = 0;
				sleep(2);
				return 0;
			}
		}
		// Obtain the offset of it on SD Card and stash it somewhere
		sprintf(txtbuffer, "sda:/%s.memcard.sav", stripInvalidChars(getRelativeName(curFile.name)));
		print_gecko("Looking for %s\r\n",txtbuffer);
		FILE *fp = fopen(txtbuffer, "r+");
		if(!fp) {
			print_gecko("Creating %s\r\n",txtbuffer);
			fp = fopen( txtbuffer, "wb" );
			if(fp) {
				print_gecko("Writing empty buffer to %s\r\n",txtbuffer);
				char *empty = (char*)memalign(32,512*1024);
				memset(empty,0, 512*1024);
				fwrite(empty, 1, 512*1024, fp);
				free(empty);
			}
		}
		if(fp) {
			print_gecko("Closing %s\r\n",txtbuffer);
			fclose(fp);
			print_gecko("Getting file base %s\r\n",txtbuffer);
			get_frag_list(txtbuffer);
			u32 file_base = frag_list->num > 1 ? -1 : frag_list->frag[0].sector;
			*(u32*)VAR_MEMCARD_LBA = file_base;
			print_gecko("File base %08X\r\n",*(u32*)VAR_MEMCARD_LBA);
			if (file_base == -1) {
				// fatal
				DrawFrameStart();
				DrawMessageBox(D_FAIL, "Memory Card file is fragmented!\nMemcard emulation disabled, returning to menu.");
				DrawFrameFinish();
				print_gecko("File base fragmented in %i pieces!\r\n",frag_list->num);
				swissSettings.emulatemc = 0;
				sleep(2);
				return 0;
			}
		}
		else {
			// fatal
			print_gecko("Could not create or open file %s\r\n",txtbuffer);
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Memory Card file failed to create!\nMemcard emulation disabled, returning to menu.");
			DrawFrameFinish();
			swissSettings.emulatemc = 0;
			sleep(2);
			return 0;
		}
	}
	return 1;
}

unsigned int load_app(int mode, int multiDol)
{
	char* gameID = (char*)0x80000000;
	int zeldaVAT = 0, i = 0;
	u32 main_dol_size = 0;
	u8 *main_dol_buffer = 0;
	DOLHEADER dolhdr;
	
	memcpy((void*)0x80000020,GC_DefaultConfig,0xE0);
  
	// Read the game header to 0x80000000 & apploader header
	deviceHandler_seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(unsigned char*)0x80000000,32) != 32) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Apploader Header Failed to read");
		DrawFrameFinish();
		while(1);
	}
	
	// Fix Zelda WW on Wii (__GXSetVAT? patch)
	if (!is_gamecube() && (!strncmp(gameID, "GZLP01", 6) || !strncmp(gameID, "GZLE01", 6) || !strncmp(gameID, "GZLJ01", 6))) {
		if(!strncmp(gameID, "GZLP01", 6))
			zeldaVAT = 1;	//PAL
		else 
			zeldaVAT = 2;	//NTSC-U,NTSC-J
	}
	
	// Game in the ID list below wants to write over 0x80001800
	if (!strncmp(gameID, "GPXE01", 6) || !strncmp(gameID, "GPXP01", 6) || !strncmp(gameID, "GPXJ01", 6)) {
		// TODO Fix this game with a individual patch
	}

	DrawFrameStart();
	DrawProgressBar(33, "Reading Main DOL");
	DrawFrameFinish();
	
	// Adjust top of memory (we reserve some for high memory location patching and cheats)
	u32 top_of_main_ram = 0x81800000;

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

	// Patch to read from SD/HDD
	if((curDevice == SD_CARD)||(curDevice == IDEEXI)||(curDevice == USBGECKO)) {
		u32 ret = Patch_DVDLowLevelRead(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL, multiDol);
		if(READ_PATCHED_ALL != ret)	{
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "Failed to find necessary functions for patching!");
			DrawFrameFinish();
			sleep(5);
		}
		Patch_DVDCompareDiskId(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
		Patch_DVDStatusFunctions(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
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
	if(swissSettings.muteAudioStreaming || curDevice != DVD_DISC)
			Patch_DVDAudioStreaming(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
		
	// Fix Zelda WW on Wii
	if(zeldaVAT) {
		Patch_GXSetVATZelda(main_dol_buffer, main_dol_size+DOLHDRLENGTH, zeldaVAT);
	}
	// 2 Disc support with no modchip
	if((curDevice == DVD_DISC) && (is_gamecube()) && (drive_status == DEBUG_MODE)) {
		Patch_DVDReset(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	}
	// Patch OSReport to print out over USBGecko
	if(swissSettings.debugUSB && usb_isgeckoalive(1)) {
		Patch_Fwrite(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	}
	// Force 480p/576p
	Patch_ProgVideo(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);

	// Force Widescreen
	if(swissSettings.forceWidescreen) {
		Patch_WideAspect(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
	}
	// Force Anisotropy
	if(swissSettings.forceAnisotropy) {
		Patch_TexFilt(main_dol_buffer, main_dol_size+DOLHDRLENGTH, PATCH_DOL);
	}
	// Emulate memory card via SDGecko
	if(swissSettings.emulatemc) {
		Patch_CARDFunctions(main_dol_buffer, main_dol_size+DOLHDRLENGTH);
	}
	// Cheats hook
	if(mode == CHEATS || swissSettings.wiirdDebug) {
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

	VIDEO_SetPostRetraceCallback (NULL);
	do_videomode_swap();
	*(volatile u32*)0x800000CC = VIDEO_GetCurrentTvMode();
	if(mode == CHEATS || swissSettings.wiirdDebug) {
		kenobi_set_debug(swissSettings.wiirdDebug);
		kenobi_install_engine();
	}
	DCFlushRange((void*)0x80000000, 0x3100);
	ICInvalidateRange((void*)0x80000000, 0x3100);
	
	// Try a device speed test using the actual in-game read code
	if((curDevice == SD_CARD)||(curDevice == IDEEXI)||(curDevice == USBGECKO)) {
		char *buffer = memalign(32,1024*1024);
		typedef u32 (*_calc_speed) (void* dst, u32 len, u32 *speed);
		_calc_speed calculate_speed = (_calc_speed) (void*)(CALC_SPEED);
		u32 speed = 0;
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
	*(volatile unsigned int*)VAR_MEMCARD_RESULT = 0;
	*(volatile unsigned int*)VAR_MC_CB_ADDR = 0;
	*(volatile unsigned int*)VAR_MC_CB_ARG1 = 0;
	*(volatile unsigned int*)VAR_MC_CB_ARG2 = 0;
	*(volatile unsigned int*)VAR_FAKE_IRQ_SET = 0;
	*(volatile unsigned int*)VAR_INTERRUPT_TIMES = 0;
	memset((void*)VAR_READ_DVDSTRUCT, 0, 0x18);

	// Memcard emulation with read device not being SDGecko in Slot A, setup the SD variables
	if(swissSettings.emulatemc && deviceHandler_initial != &initial_SD0) {
		*(volatile unsigned int*)VAR_EXI_BUS_SPD = 208;
		*(volatile unsigned int*)VAR_SD_TYPE = sdgecko_getAddressingType(0);
		*(volatile unsigned int*)VAR_EXI_FREQ = EXI_SPEED32MHZ;
		*(volatile unsigned int*)VAR_EXI_SLOT = 0;
	}
	// Set WKF base offset if not using the frag patch
	if(curDevice == WKF && !wkfFragSetupReq) {
		wkfWriteOffset(*(volatile unsigned int*)VAR_DISC_1_LBA);
	}
	print_gecko("libogc shutdown and boot game!\r\n");
	DOLtoARAM(main_dol_buffer);
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
	
	DOLtoARAM(dol_buffer);
}

/* Manage file  - The user will be asked what they want to do with the currently selected file - copy/move/delete*/
void manage_file() {
	// If it's a file
	if(curFile.fileAttrib == IS_FILE) {
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
				ret = deviceHandler_dest_init( deviceHandler_dest_initial );
				if(!ret) {
					DrawFrameStart();
					sprintf(txtbuffer, "Failed to init destination device! (%i)",ret);
					DrawMessageBox(D_FAIL,txtbuffer);
					DrawFrameFinish();
					wait_press_A();
					return;
				}
			}
			// Traverse this destination device and let the user select a directory to dump the file in
			file_handle *destFile = memalign(32,sizeof(file_handle));
			
			// Show a directory only browser and get the destination file location
			select_dest_dir(deviceHandler_dest_initial, destFile);
			destFile->fp = 0;
			destFile->fileBase = 0;
			destFile->size = 0;
			destFile->fileAttrib = IS_FILE;
			destFile->status = 0;
			destFile->offset = 0;
			
			// Same (fat based) device and user wants to move the file, just rename ;)
			if(deviceHandler_initial == deviceHandler_dest_initial 
				&& option == MOVE_OPTION 
				&& (deviceHandler_initial->name[0] =='s' || deviceHandler_initial->name[0] =='i')) {
				sprintf(destFile->name, "%s/%s",destFile->name,getRelativeName(&curFile.name[0]));
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
				sprintf(destFile->name, "%s/%s",destFile->name,stripInvalidChars(getRelativeName(&curFile.name[0])));
				
				u32 isDestCard = deviceHandler_dest_writeFile == deviceHandler_CARD_writeFile;
				if(isDestCard && strstr(destFile->name,".gci")==NULL && strstr(destFile->name,".GCI")==NULL) {
					// Only .GCI files can go to the memcard
					DrawFrameStart();
					DrawMessageBox(D_INFO,"Only GCI files allowed on memcard. Press A to continue");
					DrawFrameFinish();
					wait_press_A();
					return;
				}
			
				// Read from one file and write to the new directory
				u32 isCard = deviceHandler_readFile == deviceHandler_CARD_readFile;
				u32 curOffset = 0, cancelled = 0, chunkSize = (isCard||isDestCard) ? curFile.size : 0x8000;
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
					if(ret != amountToCopy) {
						DrawFrameStart();
						sprintf(txtbuffer, "Failed to Read! (%i %i)",amountToCopy,ret);
						DrawMessageBox(D_FAIL,txtbuffer);
						DrawFrameFinish();
						wait_press_A();
						return;
					}
					ret = deviceHandler_dest_writeFile(destFile, readBuffer, amountToCopy);
					if(ret != amountToCopy) {
						DrawFrameStart();
						sprintf(txtbuffer, "Failed to Write! (%i %i)",amountToCopy,ret);
						DrawMessageBox(D_FAIL,txtbuffer);
						DrawFrameFinish();
						wait_press_A();
						return;
					}
					curOffset+=amountToCopy;
				}
				deviceHandler_dest_deinit( destFile );
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
	int isPrePatched = 0, hasCheatsFile = 0;
		
	// Cheats file?
	if(strlen(fileName)>4) {
		if((strstr(fileName,".QCH")!=NULL) || (strstr(fileName,".qch")!=NULL)) {
			if(DrawYesNoDialog("Load this cheats file?")) {
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Loading Cheats File ..");
				DrawFrameFinish();
				QCH_SetCheatsFile(&curFile);
				return;
			}
			else {
				return;
			}
		}
	}
	// If it's not a DVD Disc, or it's a DVD disc with some file structure, browse by file type
	if((curDevice != DVD_DISC) || (curDevice == DVD_DISC && dvdDiscTypeInt==ISO9660_DISC)) {
		//if it's a DOL, boot it
		if(strlen(fileName)>4) {
			if((strstr(fileName,".DOL")!=NULL) || (strstr(fileName,".dol")!=NULL)) {
				boot_dol();
				// if it was invalid (overlaps sections, too large, etc..) it'll return
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Invalid DOL");
				DrawFrameFinish();
				sleep(2);
				return;
			}
			if((strstr(fileName,".MP3")!=NULL) || (strstr(fileName,".mp3")!=NULL)) {
				mp3_player(allFiles, files, &curFile);
				return;
			}
			if((strstr(fileName,".FZN")!=NULL) || (strstr(fileName,".fzn")!=NULL)) {
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
			if(!((strstr(fileName,".iso")!=NULL) || (strstr(fileName,".gcm")!=NULL) 
				|| (strstr(fileName,".ISO")!=NULL) || (strstr(fileName,".GCM")!=NULL))) {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Unknown File Type!");
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
	
	// User may have selected cheats via the cheats database
	hasCheatsFile = getARToWiirdCheats()[0] != 0;
	if(getARToWiirdCheats()[0] != 0) {
		kenobi_set_cheats((u8*)getARToWiirdCheats(), getARToWiirdCheatsSize());
	}
	int multiDol = 0;
	// Report to the user the patch status of this GCM/ISO file and look for a cheats file too
	if((curDevice == SD_CARD) || (curDevice == IDEEXI)) {
		multiDol = check_game();
		
		// Look for cheats file if the user hasn't loaded one up
		if(!hasCheatsFile) {
			int i = 0;
			char *expectedFileName = (char*)memalign(32,1024);
			strcpy(expectedFileName, getRelativeName(&curFile.name[0]));
			strcat(expectedFileName, ".gct");
			for(i = 0; i < files; i++) {
				char *fileName = getRelativeName(&allFiles[i].name[0]);
				if(!strcmp(fileName, expectedFileName)) {
					// Check cheats size
					if(allFiles[i].size > kenobi_get_maxsize()) {
						sprintf(txtbuffer, "Cheats file is too big! (%i byte limit)", kenobi_get_maxsize());
						DrawFrameStart();
						DrawMessageBox(D_FAIL, txtbuffer);
						DrawFrameFinish();
						wait_press_A();
						free(expectedFileName);
						return;
					}
					
					u8* cheatsbuffer = (u8*)memalign(32,0x8000);
					// Found the cheats file, read it
					deviceHandler_seekFile(&allFiles[i],0,DEVICE_HANDLER_SEEK_SET);
					if(deviceHandler_readFile(&allFiles[i],cheatsbuffer,allFiles[i].size) == allFiles[i].size) {
						// Try to set it
						kenobi_set_cheats(cheatsbuffer, allFiles[i].size);
						hasCheatsFile = 1;
					}
					free(cheatsbuffer);
				}
			}
			free(expectedFileName);
		}

	}
	
	// We can't relocate the cheats stub so we must ensure our patch code is located elsewhere
	// TODO: FIX
	
	// Setup memory card emulation
	if(!setup_memcard_emulation())
		return;
	
	// Start up the DVD Drive
	if((curDevice != DVD_DISC) && (curDevice != WODE) && (curDevice != WKF) 
		&& (swissSettings.hasDVDDrive)
		&& DrawYesNoDialog("Use a DVD Disc for higher compatibility?")) {
		if(initialize_disc(GCMDisk.AudioStreaming) == DRV_ERROR) {
			return; //fail
		}
		if(!isPrePatched && DrawYesNoDialog("Stop DVD Motor?")) {
			dvd_motor_off();
		}
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
	load_app(hasCheatsFile ? CHEATS:NO_CHEATS, multiDol);
}

int cheats_game()
{ 
	int ret;

	DrawFrameStart();
	DrawMessageBox(D_INFO,"Loading Cheat DB");
	DrawFrameFinish();
	ret = QCH_Init();
	if(!ret) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"Failed to open cheats.qch. Press A.");
		DrawFrameFinish();
		wait_press_A();
		return 0;
	}

	ret = QCH_Parse(NULL);
	if(ret <= 0) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"Failed to parse cheat DB. Press A.");
		DrawFrameFinish();
		wait_press_A();
		return 0;
	}
	sprintf(txtbuffer,"Found %d Games",ret);
	DrawFrameStart();
	DrawMessageBox(D_INFO,txtbuffer);
	DrawFrameFinish();
	
	curGameCheats *selected_cheats = memalign(32,sizeof(curGameCheats));
	QCH_Find(&GCMDisk.GameName[0],selected_cheats);
	free(selected_cheats);

	QCH_DeInit();  
	return 1;
}


int check_game()
{ 	
	int multiDol = 0;
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Checking Game ..");
	DrawFrameFinish();
	
	ExecutableFile *filesToPatch = memalign(32, sizeof(ExecutableFile)*64);
	int numToPatch = parse_gcm(&curFile, filesToPatch), i =0;
	for(i=0; i<numToPatch; i++) {
		if(strstr(filesToPatch[i].name,"execD.img") && 
			(!strncmp((char*)&GCMDisk, "D43U01", 6) || !strncmp((char*)&GCMDisk, "D43E01", 6))) {
			print_gecko("OOT Multi Disc Detected\r\n");
			multiDol = 1;
			break;
		}
	}
	
	if(numToPatch>0) {
		// Game requires patch files, lets do it.	
		int res = patch_gcm(&curFile, filesToPatch, numToPatch, multiDol);
		DrawFrameStart();
		DrawMessageBox(D_INFO,res ? "Game Patched Successfully":"Game could not be patched or not required");
		DrawFrameFinish();
	}
	free(filesToPatch);
	return multiDol;
}

void save_config(ConfigEntry *config) {
	// Save settings to current device
	if(curDevice != SD_CARD) {
		// If the device is Read-Only, warn/etc
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Cannot save config on read-only device!");
		DrawFrameFinish();
	}
	else {
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
	if(GCMDisk.DVDMagicWord == DVD_MAGIC) {
		u32 bannerOffset = showBanner(215, 240, 2);  //Convert & display game banner
		if(bannerOffset) {
			char description[128];
			char bnrType[8];
			// If this is a BNR2 banner, show the proper description for the language the console is set to
			deviceHandler_seekFile(&curFile,bannerOffset,DEVICE_HANDLER_SEEK_SET);
			deviceHandler_readFile(&curFile,&bnrType[0],4);
			if(!strncmp(bnrType, "BNR2", 4)) {
				deviceHandler_seekFile(&curFile,bannerOffset+(0x18e0+(swissSettings.sramLanguage*0x0140)),DEVICE_HANDLER_SEEK_SET);
			}
			else {
				deviceHandler_seekFile(&curFile,bannerOffset+0x18e0,DEVICE_HANDLER_SEEK_SET);
			}
			if(deviceHandler_readFile(&curFile,&description[0],0x80)==0x80) {
				char * tok = strtok (&description[0],"\n");
				int line = 0;
				while (tok != NULL)	{
					float scale = GetTextScaleToFitInWidth(tok,(vmode->fbWidth-78)-75);
					WriteFontStyled(640/2, 315+(line*scale*24), tok, scale, true, defaultColor);
					tok = strtok (NULL, "\n");
					line++;
				}
			}
		}
	}
	sprintf(txtbuffer,"%s",(GCMDisk.DVDMagicWord != DVD_MAGIC)?getRelativeName(&curFile.name[0]):GCMDisk.GameName);
	float scale = GetTextScaleToFitInWidth(txtbuffer,(vmode->fbWidth-78)-75);
	WriteFontStyled(640/2, 130, txtbuffer, scale, true, defaultColor);

	if((curDevice==SD_CARD)||(curDevice == IDEEXI) ||(curDevice == WKF) ||((curDevice == DVD_DISC) && (dvdDiscTypeInt==ISO9660_DISC))) {
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
	}
	else if(curDevice == DVD_DISC)  {
		sprintf(txtbuffer,"%s DVD disc", dvdDiscTypeStr);
		WriteFontStyled(640/2, 160, txtbuffer, 0.8f, true, defaultColor);
	}
	else if(curDevice == QOOB_FLASH) {
		sprintf(txtbuffer,"Size: %.2fKb (%i blocks)", (float)curFile.size/1024, curFile.size/0x10000);
		WriteFontStyled(640/2, 160, txtbuffer, 0.8f, true, defaultColor);
		sprintf(txtbuffer,"Position on Flash: %08X",(u32)(curFile.fileBase&0xFFFFFFFF));
		WriteFontStyled(640/2, 180, txtbuffer, 0.8f, true, defaultColor);
	}
	else if(curDevice == WODE) {
		sprintf(txtbuffer,"Partition: %i, ISO: %i", (int)(curFile.fileBase>>24)&0xFF,(int)(curFile.fileBase&0xFFFFFF));
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
	}

	WriteFontStyled(640/2, 370, "Cheats (Y) - Settings (X) - Exit (B) - Continue (A)", 0.75f, true, defaultColor);
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
	swissSettings.emulatemc = config->emulatemc;
	swissSettings.forceWidescreen = config->forceWidescreen;
	swissSettings.forceAnisotropy = config->forceAnisotropy;
	while(ret == -1) {
		draw_game_info();
		while((PAD_ButtonsHeld(0) & PAD_BUTTON_X) || (PAD_ButtonsHeld(0) & PAD_BUTTON_B) || (PAD_ButtonsHeld(0) & PAD_BUTTON_Y) || (PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
		while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_Y) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)){ VIDEO_WaitVSync (); }
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_Y) {
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
			cheats_game();
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_X) {
			show_settings((GCMDisk.DVDMagicWord == DVD_MAGIC) ? &curFile : NULL, config);
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
		}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_B) || (PAD_ButtonsHeld(0) & PAD_BUTTON_A)){
			ret = (PAD_ButtonsHeld(0) & PAD_BUTTON_A) ? 1:0;
		}
	}
	save_config(config);
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
	if(deviceHandler_dest_deinit && deviceHandler_dest_initial) deviceHandler_dest_deinit( deviceHandler_dest_initial );
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
		break;
	}
}

void select_device()
{  
	if(is_httpd_in_use()) {
		doBackdrop();
		DrawMessageBox(D_INFO,"Can't load device while HTTP is processing!");
		DrawFrameFinish();
		sleep(5);
		return;
	}

	int slot = 1;
	if(!swissSettings.defaultDevice) {
		dvdDiscTypeStr = NotInitStr;
		int inAdvanced = 0;
		int inAdvancedPos = 0;
		while(1) {
			doBackdrop();
			DrawEmptyBox(20,190, vmode->fbWidth-20, 355, COLOR_BLACK);
			WriteFontStyled(640/2, 195, "Device Selection", 1.0f, true, defaultColor);		
			if(inAdvanced) {
				// Draw slot / speed selection if advanced menu is showing.
				DrawEmptyBox(vmode->fbWidth-170, 370, vmode->fbWidth-20, 405, COLOR_BLACK);
				WriteFontStyled(vmode->fbWidth-165, 370, slot ? "Slot: B":"Slot: A", 0.65f, false, !inAdvancedPos ? defaultColor:deSelectedColor);
				WriteFontStyled(vmode->fbWidth-165, 385, swissSettings.exiSpeed ? "Speed: Fast":"Speed: Compatible", 0.65f, false, inAdvancedPos ? defaultColor:deSelectedColor);
			}
			if(curDevice==DVD_DISC) {
				DrawImage(TEX_GCDVDSMALL, 640/2, 230, 80, 79, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "DVD Disc (GCM, ISO9660, MultiGame)", 0.85f, true, defaultColor);
			}
			else if(curDevice==SD_CARD) {
				DrawImage(TEX_SDSMALL, 640/2, 230, 60, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "SD Card via SD Gecko", 0.85f, true, defaultColor);
				WriteFontStyled(vmode->fbWidth-120, 345, "(X) Advanced", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			}
			else if(curDevice==IDEEXI) {
				DrawImage(TEX_HDD, 640/2, 230, 80, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "IDE HDD via IDE-EXI", 0.85f, true, defaultColor);
				WriteFontStyled(vmode->fbWidth-120, 345, "(X) Advanced", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			}
			else if(curDevice==QOOB_FLASH) {
				DrawImage(TEX_QOOB, 640/2, 230, 70, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "Qoob PRO Flash", 0.85f, true, defaultColor);
			}
			else if(curDevice==WODE) {
				DrawImage(TEX_WODEIMG, 640/2, 230, 146, 72, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "WODE Jukebox", 0.85f, true, defaultColor);
			}
			else if(curDevice==MEMCARD) {
				DrawImage(TEX_MEMCARD, 640/2, 230, 107, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "Memory Card", 0.85f, true, defaultColor);
				WriteFontStyled(vmode->fbWidth-120, 345, "(X) Advanced", 0.65f, false, inAdvanced ? defaultColor:deSelectedColor);
			}
			else if(curDevice==WKF) {
				DrawImage(TEX_WIIKEY, 640/2, 230, 200, 75, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "Wiikey / Wasp Fusion", 0.85f, true, defaultColor);
			}
			else if(curDevice==USBGECKO) {
				DrawImage(TEX_USBGECKO, 640/2, 230, 129, 80, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
				WriteFontStyled(640/2, 330, "USB Gecko (req. PC app)", 0.85f, true, defaultColor);
			}
			if(curDevice != 7) {
				WriteFont(520, 270, "->");
			}
			if(curDevice != 0) {
				WriteFont(100, 270, "<-");
			}
			DrawFrameFinish();
			while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
					&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)
					&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_X)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) )
				{ VIDEO_WaitVSync (); }
			u16 btns = PAD_ButtonsHeld(0);
			if(btns & PAD_BUTTON_X && ((curDevice == SD_CARD) || (curDevice == IDEEXI) ||(curDevice == MEMCARD)))
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
				if((btns & PAD_BUTTON_RIGHT) && curDevice < 7)
					curDevice++;
				if((btns & PAD_BUTTON_LEFT) && curDevice > 0)
					curDevice--;
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
			while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
					&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) 
					&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_X) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) ))
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
			if(swissSettings.defaultDevice) {
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
			deviceHandler_info 	   =  deviceHandler_USBGecko_info;
			deviceHandler_deleteFile = NULL;
		break;
	}
	memcpy(&curFile, deviceHandler_initial, sizeof(file_handle));
}

