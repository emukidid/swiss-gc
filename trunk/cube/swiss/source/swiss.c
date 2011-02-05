/*
*
*   Swiss - The Gamecube IPL replacement
*
*/

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

#include "swiss.h"
#include "main.h"
#include "font.h"
#include "exi.h"
#include "patcher.h"
#include "bnr2yuv.h"
#include "ff.h"
#include "diskio.h"
#include "TDPATCH.h"
#include "qchparse.h"
#include "dvd.h"
#include "gcm.h"
#include "gcdvdicon.h"
#include "sdicon.h"
#include "hddicon.h"
#include "redcross.h"
#include "greentick.h"
#include "aram/sidestep.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "devices/dvd/deviceHandler-DVD.h"
#include "devices/ide-exi/deviceHandler-ide-exi.h"
#include "devices/ide-exi/ata.h"
#include "devices/sd/deviceHandler-SD.h"

static DiskHeader GCMDisk;      //Gamecube Disc Header struct
char IPLInfo[256] __attribute__((aligned(32)));
u32 driveVersion = 0;
GXRModeObj *newmode = NULL;
char txtbuffer[2048];           //temporary text buffer
file_handle curFile;     //filedescriptor for current file
int curVideoSelection = AUTO;	  //video forcing selection (default == auto)
int GC_SD_CHANNEL = 0;          //SD slot
u32 GC_SD_SPEED   = EXI_SPEED16MHZ;
int SDHCCard = 0; //0 == SDHC, 1 == normal SD
int curDevice = 0;  //SD_CARD or DVD_DISC or IDEEXI
int noVideoPatch = 0;
char *videoStr = NULL;

/* The default boot up MEM1 lowmem values (from ipl when booting a game) */
static const u32 GC_DefaultConfig[56] =
{
	0x0D15EA5E, 0x00000001, 0x01800000, 0x00000003, //  0.. 3 80000020
	0x00000000, 0x816FFFF0, 0x817FE8C0, 0x00000024, //  4.. 7 80000030
	0x00000000, 0x00000000, 0x00000000, 0x00000000, //  8..11 80000040
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 12..15 80000050
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 16..19 80000060
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 20..23 80000070
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 24..27 80000080
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 28..31 80000090
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 32..35 800000A0
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 36..39 800000B0
	0x015D47F8, 0xF8248360, 0x00000000, 0x00000001, // 40..43 800000C0
	0x00000000, 0x00000000, 0x00000000, 0x00000000, // 44..47 800000D0
	0x814B7F50, 0x815D47F8, 0x00000000, 0x81800000, // 48..51 800000E0
	0x01800000, 0x817FC8C0, 0x09A7EC80, 0x1CF7C580  // 52..55 800000F0
};

char* _menu_array[] =
{
	"Boot"		,
	"Device"	,
	"Setup"		,
	"Install"	,
	"Info"		,
	"Credits"	,
	"Refresh"
};

char *getVideoString()
{
	switch(curVideoSelection)
	{
		case 0:
			return "NTSC";
		case 1:
			return "PAL50";
		case 2:
			return "PAL60";
		case 3:
			return "Auto";
		case 4:
			return "480p";
	}
	return "Autodetect";
}
 
static void ProperScanPADS()	{ 
	PAD_ScanPads(); 
}

void print_gecko(char *string)
{
	if(debugUSB) {
		int i = 0;
		for(i=0;i<2;i++) {
			usb_sendbuffer_safe(1,string,strlen(string));
		}
	}
}

/* re-init video for a given game */
void ogc_video__reset()
{
    DrawFrameStart();
    if(curVideoSelection==AUTO) {
		noVideoPatch = 1;
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
				curVideoSelection = PAL50;
				break;
			case 'E':
			case 'J':
				curVideoSelection = NTSC;
				break;
			case 'U':
				curVideoSelection = PAL60;
				break;
			default:
				break;
		}
    }

    /* set TV mode for current game*/
	if(curVideoSelection!=AUTO)	{		//if not autodetect
		switch(curVideoSelection) {
			case PAL60:
				*(volatile unsigned long*)0x80002F40 = VI_TVMODE_EURGB60_INT;
				newmode = &TVEurgb60Hz480IntDf;
				DrawMessageBox(D_INFO,"Video Mode: PAL 60Hz");
				break;
			case PAL50:
				*(volatile unsigned long*)0x80002F40 = VI_TVMODE_PAL_INT;
				newmode = &TVPal528IntDf;
				DrawMessageBox(D_INFO,"Video Mode: PAL 50Hz");
				break;
			case NTSC:
				*(volatile unsigned long*)0x80002F40 = VI_TVMODE_NTSC_INT;
				newmode = &TVNtsc480IntDf;
				DrawMessageBox(D_INFO,"Video Mode: NTSC 60Hz");
				break;
			case P480:
				*(volatile unsigned long*)0x80002F40 = VI_TVMODE_NTSC_PROG;
				newmode = &TVNtsc480Prog;
				DrawMessageBox(D_INFO,"Video Mode: NTSC 480p");
				break;
			default:
				*(volatile unsigned long*)0x80002F40 = VI_TVMODE_NTSC_INT;
				newmode = &TVNtsc480IntDf;
				DrawMessageBox(D_INFO,"Video Mode: NTSC 60Hz");
		}
		DrawFrameFinish();
	}
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
	
	if(VIDEO_HaveComponentCable()) {
		vmode = &TVNtsc480Prog; //Progressive 480p
		videoStr = ProgStr;
	}
	else {
		//try to use the IPL region
		if(strstr(IPLInfo,"PAL")!=NULL) {
			vmode = &TVPal528IntDf;         //PAL
			videoStr = PALStr;
		}
		else if(strstr(IPLInfo,"NTSC")!=NULL) {
			vmode = &TVNtsc480IntDf;        //NTSC
			videoStr = NTSCStr;
		}
		else {
			vmode = VIDEO_GetPreferredMode(NULL); //Last mode used
			videoStr = UnkStr;
		}
	}

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
	init_font();
	whichfb = 0;
	
	driveVersion = drive_version();
	
	if(driveVersion == -1) {
		// Reset DVD if there was a modchip
		whichfb ^= 1;
		WriteFont(55,250, "Initialise DVD from cold boot.. (Qoob user?)");
		VIDEO_SetNextFramebuffer (xfb[whichfb]);
		VIDEO_Flush ();
		VIDEO_WaitVSync ();
		if(vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
		DVD_Reset(DVD_RESETHARD);
		dvd_read_id();
		driveVersion = drive_version();
	}
	
	return xfb[0];
}

void doBackdrop()
{
	DrawFrameStart();
	int i;
	for(i = 0; i<MENU_MAX; i++)
		DrawSelectableButton(458,130+(i*40),-1,140+(i*40)+24,_menu_array[i],B_NOSELECT);
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

// textFileBrowser lives on :)
void textFileBrowser(file_handle** directory, int num_files)
{
	int i = 0,j=0,max;
	
	memset(txtbuffer,0,sizeof(txtbuffer));
	if(num_files<=0) return;
  
	while(1){
		doBackdrop();
		WriteFont(50, 120, deviceHandler_initial->name);
		i = MIN(MAX(0,curSelection-4),MAX(0,num_files-8));
		max = MIN(num_files, MAX(curSelection+4,8));
		for(j = 0; i<max; ++i,++j) {
			DrawSelectableButton(50,160+(j*30), 430, 160+(j*30)+30, getRelativeName((*directory)[i].name), (i == curSelection) ? B_SELECTED:B_NOSELECT);
		}
		DrawFrameFinish();
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP){	curSelection = (--curSelection < 0) ? num_files-1 : curSelection;}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN){curSelection = (curSelection + 1) % num_files;	}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A))	{
			//go into a folder or select a file
			if((*directory)[curSelection].fileAttrib==IS_DIR) {
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
				needsRefresh=1;
			}
			else if((*directory)[curSelection].fileAttrib==IS_SPECIAL){
				//go up a folder
				int len = strlen(&curFile.name[0]);
				while(len && curFile.name[len-1]!='/') {
      				len--;
				}
				if(len != strlen(&curFile.name[0])) {
					curFile.name[len-1] = '\0';
					needsRefresh=1;
				}
			}
			else if((*directory)[curSelection].fileAttrib==IS_FILE){
				memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
			}
			return;
		}
		
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT)	{
			memcpy(&curFile, &(*directory)[curSelection], sizeof(file_handle));
			curMenuLocation=ON_OPTIONS;
			return;
		}
		
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)));
	}
}

static void nothing(const char* txt, ...){}
unsigned int load_app(int mode)
{
	// Apploader internal function defines
	void  (*app_init) (void (*report)(const char* fmt, ...));
	int   (*app_main) (void** dst, int* size, int* offset);
	void* (*app_final)();
	void  (*app_entry)(void(**init)(void (*report)(const char* fmt, ...)),
	int (**main)(), void *(**final)());
	// Apploader related variables
	void* app_dst = 0;
	int app_len = 0,app_offset = 0, apploader_info[3], res;
	
	VIDEO_SetPostRetraceCallback (NULL);

	if(mode == NO_CHEATS) {
		memcpy((void*)0x80000020,GC_DefaultConfig,0xE0);
	}
  
	// Read the game header to 0x80000000 & apploader header
	deviceHandler_seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(unsigned char*)0x80000000,32) != 32) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Apploader Header Failed to read");
		DrawFrameFinish();
		while(1);
	}
	deviceHandler_seekFile(&curFile,0x2450,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(unsigned char*)apploader_info,sizeof(int)*3) != sizeof(int)*3) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Apploader Header Failed to read");
		DrawFrameFinish();
		while(1);
	}
	 	
	sprintf(txtbuffer,"Reading Apploader (%d bytes)",apploader_info[1]+apploader_info[2]);
	DrawFrameStart();
	DrawProgressBar(33, txtbuffer);
	DrawFrameFinish();

	// Read the apploader itself
	deviceHandler_seekFile(&curFile,0x2460,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(void*)0x81200000,apploader_info[1]+apploader_info[2]) != apploader_info[1]+apploader_info[2]) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Apploader Data Failed to read");
		DrawFrameFinish();
		while(1);
	}
	DCFlushRange((void*)0x81200000,apploader_info[1]+apploader_info[2]);
	ICInvalidateRange((void*)0x81200000,apploader_info[1]+apploader_info[2]);
  
	DrawFrameStart();
	DrawProgressBar(66, "Executing Apploader...");
	DrawFrameFinish();
	
	app_entry = (void (*)(void(**)(void (*)(const char*, ...)), int (**)(), void *(**)()))(apploader_info[0]);
	app_entry(&app_init, &app_main, &app_final);
	app_init((void (*)(const char*, ...))nothing);
	
	while(1) {
		res = app_main(&app_dst, &app_len, &app_offset);
		if(!res) {
			break;
		}
		if(debugUSB) {
			sprintf(txtbuffer,"Apploader Reading: %dbytes from %08X to %08x\n",app_len,app_offset,(u32)app_dst);
			print_gecko(txtbuffer);
		}
		// Read app_len amount to app_dst specified by the apploader from offset app_offset
		deviceHandler_seekFile(&curFile,app_offset,DEVICE_HANDLER_SEEK_SET);
		if(deviceHandler_readFile(&curFile,app_dst,app_len) != app_len) {
			DrawFrameStart();
			DrawMessageBox(D_FAIL, "DOL Failed to read");
			DrawFrameFinish();
			while(1);
		}
		// Patch to read from SD/HDD
		if((curDevice == SD_CARD)||((curDevice == IDEEXI))) {
			dvd_patchDVDRead(app_dst, app_len);
			dvd_patchDVDReadID(app_dst, app_len);
			dvd_patchAISCount(app_dst, app_len);
			dvd_patchDVDLowSeek(app_dst, app_len);
		}
		// 2 Disc support with no modchip
		if((curDevice == DVD_DISC) && (mfpvr() == GC_CPU_VERSION) && (drive_status == DEBUG_MODE)) {
			dvd_patchreset(app_dst,app_len);  
		}
		// Patch OSReport to print out over USBGecko
		if(debugUSB) {
			dvd_patchfwrite(app_dst, app_len);
		}
		dvd_patchVideoMode(app_dst, app_len, noVideoPatch ? AUTO:curVideoSelection);
		DCFlushRange(app_dst, app_len);
		ICInvalidateRange(app_dst, app_len);
	}
	deviceHandler_deinit(&curFile);
 
	DrawFrameStart();
	DrawProgressBar(100, "Executing Game!");
	DrawFrameFinish();
/*
	do_videomode_swap();
	
	if(noVideoPatch) {*/
		switch(*(char*)0x80000003) {
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
				*(volatile unsigned long*)0x800000CC = 1;
				break;
			case 'E':
			case 'J':
				*(volatile unsigned long*)0x800000CC = 0;
				break;
			default:
				*(volatile unsigned long*)0x800000CC = 0;
		}
	/*}*/
      
	DCFlushRange((void*)0x80001800,0x1800);
	ICInvalidateRange((void*)0x80001800,0x1800);
	
	// copy sd/hdd read or 2 disc code to 0x80001800
	if((curDevice == SD_CARD)||((curDevice == IDEEXI))) {
		install_code();
	}
	else if((curDevice == DVD_DISC) && (drive_status == DEBUG_MODE)) {
		memcpy((void*)0x80001800,TDPatch,TDPATCH_LEN);
	} 
  
	DCFlushRange((void*)0x80000000, 0x3100);
	ICInvalidateRange((void*)0x80000000, 0x3100);
	
	if(debugUSB) {
		sprintf(txtbuffer,"Sector: %08X%08X Speed: %08x\n",*(volatile unsigned int*)0x80002F00,
		*(volatile unsigned int*)0x80002F04,*(volatile unsigned int*)0x80002F30);
		print_gecko(txtbuffer);
	}
	
	// Disable interrupts and exceptions
	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	mtmsr(mfmsr() & ~0x8000);
	mtmsr(mfmsr() | 0x2002);
	
	if(mode == CHEATS) {
		return (u32)app_final();
	}
 
	void (*entrypoint)() = app_final();
	entrypoint();
	
	return 0;
}

void boot_dol()
{ 
	unsigned char *dol_buffer;
	unsigned char *ptr;
  
	sprintf(txtbuffer, "Loading DOL [%d/%d Kb] ..",curFile.size/1024,SYS_GetArena1Size()/1024);
	DrawFrameStart();
	DrawMessageBox(D_INFO,txtbuffer);
	DrawFrameFinish();
  
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

/* Boot currently selected game or DOL */
void boot_file()
{
	if((curDevice != DVD_DISC) || (dvdDiscTypeInt==ISO9660_DISC)) {
		char *fileName = &curFile.name[0];
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
		
			if(!((strstr(fileName,".iso")!=NULL) || (strstr(fileName,".gcm")!=NULL) 
				|| (strstr(fileName,".ISO")!=NULL) || (strstr(fileName,".GCM")!=NULL))) {
				DrawFrameStart();
				DrawMessageBox(D_WARN, "Unknown File Type!");
				DrawFrameFinish();
				sleep(2);
				return;
			}
		}
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
	
	// Report to the user the patch status of this GCM/ISO file
	if((curDevice == SD_CARD) || ((curDevice == IDEEXI))) {
		check_game();
	}
	
	// Start up the DVD Drive
	if(curDevice != DVD_DISC) {
		if(initialize_disc(GCMDisk.AudioStreaming) == DRV_ERROR) {
			return; //fail
		}
	}
  	
	file_handle *secondDisc = NULL;
	
	// If we're booting from SD card or IDE hdd
	if((curDevice == SD_CARD) || ((curDevice == IDEEXI))) {
		// look to see if it's a two disc game
		// set things up properly to allow disc swapping
		// the files must be setup as so: game-disc1.xxx game-disc2.xxx
		secondDisc = memalign(32,sizeof(file_handle));
		memcpy(secondDisc,&curFile,sizeof(file_handle));
		
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
		if(secondDisc) {
			FIL tempFile;
			if(f_open(&tempFile,secondDisc->name,FA_READ)==FR_OK) {
				secondDisc->fileBase = mclust2sect(tempFile.org_clust);
				DrawFrameStart();
				DrawMessageBox(D_INFO, "Setting up two disc game");
				DrawFrameFinish();
				sleep(2);
				f_close(&tempFile);
			}
		}
	}
  
	// Call the special setup for each device (e.g. SD will set the sector(s))
	deviceHandler_setupFile(&curFile, secondDisc);
		
	if(secondDisc) {
		free(secondDisc);
	}

	// setup the video mode before we kill libOGC kernel
	ogc_video__reset();
	
	if(getCodeBasePtr()[0]) {
		GCARSStartGame(getCodeBasePtr());
	}
	else {
		load_app(NO_CHEATS);
	}
}

void check_game()
{ 	
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Checking Game ..");
	DrawFrameFinish();
  
	u32 isPatched[3];
	deviceHandler_seekFile(&curFile,0x100,DEVICE_HANDLER_SEEK_SET);
	deviceHandler_readFile(&curFile,&isPatched,12);
	
	// Check to see if it's already patched
	if((isPatched[0] == 0x50617463) && (isPatched[1] == 0x68656421)) { //'Patched!'
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Game is already patched!");
		DrawFrameFinish();
		sleep(1);
		return;
	}
	
	if(((isPatched[0] == 0x50617463) && (isPatched[1] == 0x68656421)) && (isPatched[2] != 0x00000002)) {
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Game is using outdated pre-patcher!");
		DrawFrameFinish();
		sleep(5);
		return;
	}
	
	DrawFrameStart();
	DrawEmptyBox(75,120, vmode->fbWidth-78, 460, COLOR_BLACK);  
	WriteCentre(130, "Checking ISO/GCM");
	DrawFrameFinish();	// We want to draw these as they are worked out
	
	int read_patchable = check_dol(&curFile, (u32*)_Read_original, sizeof(_Read_original));
	if(read_patchable != 1) {
		read_patchable = check_dol(&curFile, (u32*)_Read_original_2, sizeof(_Read_original_2));
	}
	if(read_patchable == -1)
		WriteCentre(170,"Read patchable: Error checking");
	else {
		WriteCentre(170,"Read patchable:");
		if(read_patchable == 1)
			drawBitmap(greentick_Bitmap, 240, 170, 32,24);
		else
			drawBitmap(redcross_Bitmap, 240, 170, 30,25);
	}
		
	int seek_patchable = check_dol(&curFile, _DVDLowSeek_original, sizeof(_DVDLowSeek_original));
	if(seek_patchable == -1)
		WriteCentre(195,"Seek patchable: Error checking");
	else {
		WriteCentre(195,"Seek patchable:");
		if(seek_patchable == 1)
			drawBitmap(greentick_Bitmap, 240, 195, 32,24);
		else
			drawBitmap(redcross_Bitmap, 240, 195, 30,25);
	}
		
	int cover_patchable = check_dol(&curFile, _DVDLowReadDiskID_original, sizeof(_DVDLowReadDiskID_original));
	if(cover_patchable == -1)
		WriteCentre(220,"Disc Swap patchable: Error checking");
	else {
		WriteCentre(220,"Disc Swap patchable:");
		if(cover_patchable == 1)
			drawBitmap(greentick_Bitmap, 240, 220, 32,24);
		else
			drawBitmap(redcross_Bitmap, 240, 220, 30,25);
	}
		
	parse_gcm(&curFile);
	sprintf(txtbuffer,"Found %d extra executables",numExecutables);
	WriteCentre(280,txtbuffer);
	sprintf(txtbuffer,"Found %d embedded GCMs",numEmbeddedGCMs);
	WriteCentre(305,txtbuffer);
	
	if(!read_patchable || read_patchable == -1)
		WriteCentre(365,"ISO is unlikely to boot");
	else {
		WriteCentre(365,"ISO is likely to boot");  
		drawBitmap(greentick_Bitmap, 240, 365, 32,24);
	}
	if(numExecutables||numEmbeddedGCMs)
		WriteCentre(390,"Pre-Patching might be required");
	else if(numExecutables+numEmbeddedGCMs==0)
		WriteCentre(390,"Pre-Patching is not required");

	WriteCentre(430,"Press A to continue");
	DrawFrameFinish();	
	wait_press_A();
}

void cheats_game()
{ 
	int ret;
  
	// lets just quickly check if this is a valid GCM (contains magic)
	memset(&GCMDisk,0,sizeof(DiskHeader));
 	
	deviceHandler_seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
	if((deviceHandler_readFile(&curFile,(unsigned char*)&GCMDisk,sizeof(DiskHeader))!= sizeof(DiskHeader)) || (GCMDisk.DVDMagicWord != DVD_MAGIC)) {
		return; 
	}  
  
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Loading Cheat DB");
	DrawFrameFinish();
	ret = QCH_Init();
	if(!ret) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"Failed to open cheats.qch. Press A.");
		DrawFrameFinish();
		wait_press_A();
		return;
	}
  
	ret = QCH_Parse(NULL);
	if(ret <= 0) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"Failed to parse cheat DB. Press A.");
		DrawFrameFinish();
		wait_press_A();
		return;
	}
	sprintf(txtbuffer,"Found %d Games",ret);
	DrawFrameStart();
	DrawMessageBox(D_INFO,txtbuffer);
	DrawFrameFinish();
	
	curGameCheats *selected_cheats = memalign(32,sizeof(curGameCheats));
	QCH_Find(&GCMDisk.GameName[0],selected_cheats);
	free(selected_cheats);
  
	QCH_DeInit();  
}

void install_game()
{
	char dumpName[32];
	FIL dumpFile;
	FRESULT res;
	FATFS *fs;
	DWORD clust;
  
	if((curDevice!=SD_CARD) && (curDevice!=IDEEXI)) {
		DrawFrameStart();
		DrawMessageBox(D_INFO, "Only available in SD Card mode");
		DrawFrameFinish();
		sleep(2);
		return;
	}
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
  
	DrawFrameStart();
	DrawEmptyBox(10,120, vmode->fbWidth-10, 470, COLOR_BLACK);
	WriteCentre(130,"*** WARNING ***");
	WriteCentre(200,"This program is not responsible");
	WriteCentre(230,"for any loss of data or card corruption");
	WriteCentre(300, "Press A to Continue, or B to return");
	DrawFrameFinish();
	while(1) {
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_A) {
			while((PAD_ButtonsHeld(0) & PAD_BUTTON_A));
			break;
		}
		if(btns & PAD_BUTTON_B) {
			while((PAD_ButtonsHeld(0) & PAD_BUTTON_B));
			return;
		}
	}
  
	res = f_getfree("", &clust, &fs);
	if(res!=FR_OK) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL, "Failed to get free space amount!");
		DrawFrameFinish();
		sleep(2);
		return;
	}
   
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Insert a DVD and press the A button");
	DrawFrameFinish();
	wait_press_A();
    
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Mounting Disc");
	DrawFrameFinish();

	dvddiskid *disk_id = memalign(32,sizeof(dvddiskid));
	if(mfpvr() == GC_CPU_VERSION)
		DVD_Mount();
	else
		DVD_Reset(DVD_RESETHARD);
	DVD_ReadID(disk_id);
	char *gameID = memalign(32,2048);
		
	memset(gameID,0,2048);
	DVD_Read(gameID, 0, 32);
	xeno_disable();
	if(!gameID[0]) {
		free(gameID);
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"Disc is not GC Game. Press A");
		DrawFrameFinish();
		wait_press_A();
		return;
	}

	strcpy(dumpName,GC_SD_CHANNEL ? "1:/DISC-":"0:/DISC-");
	strncat(dumpName,gameID,6);
	strcat(dumpName,".iso");
	free(gameID);
  
	sprintf(txtbuffer,"Creating: %s",dumpName);
	DrawFrameStart();
	DrawMessageBox(D_INFO,txtbuffer);
	DrawFrameFinish();
  
	res = f_open(&dumpFile,(const char*)&dumpName[0], FA_CREATE_ALWAYS|FA_WRITE|FA_READ );
	if(res!=FR_OK) {
		DrawFrameStart();
		DrawMessageBox(D_FAIL,"Unable to create file. Press A.");
		DrawFrameFinish();
		wait_press_A();
		return;
	}
    
	unsigned char *dvdDumpBuffer = (unsigned char*)memalign(32,CHUNK_SIZE);
    
	long long copyTime = gettime();
	long long startTime = gettime();
	unsigned int writtenOffset=0;
	UINT bytesWritten;
	
	for(writtenOffset = 0; (writtenOffset+CHUNK_SIZE) < DISC_SIZE; writtenOffset+=CHUNK_SIZE)
	{
		DVD_LowRead64(dvdDumpBuffer, CHUNK_SIZE, writtenOffset);
		f_write(&dumpFile,&dvdDumpBuffer[0],CHUNK_SIZE,&bytesWritten);
		long long timeNow = gettime();
		int etaTime = (((DISC_SIZE-writtenOffset)/1024)/(CHUNK_SIZE/diff_msec(copyTime,timeNow))); //in secs
		sprintf(txtbuffer,(bytesWritten!=CHUNK_SIZE) ? 
                      "Write Error!!!"  :
                      "%dMb dumped @ %.2fmb/s - ETA %02d:%02d",
                      (int)(writtenOffset/(1024*1024)),(float)((float)((float)CHUNK_SIZE/(float)diff_msec(copyTime,timeNow))/1024),
                      (etaTime/60),(etaTime%60));
		DrawFrameStart();
		DrawProgressBar((int)((float)((float)writtenOffset/(float)DISC_SIZE)*100), txtbuffer);
		DrawFrameFinish();
		copyTime = gettime();       
	}
  
	if(writtenOffset<DISC_SIZE) {
		DVD_LowRead64(dvdDumpBuffer, DISC_SIZE-writtenOffset, writtenOffset);
		f_write(&dumpFile,&dvdDumpBuffer[0],(DISC_SIZE-writtenOffset),&bytesWritten);
		if((bytesWritten!=(DISC_SIZE-writtenOffset))) {
			DrawFrameStart();
			DrawProgressBar(100.0f, "Write Error!!!");
			DrawFrameFinish();
		}
		copyTime = gettime();
	}
	f_close(&dumpFile);
	free(dvdDumpBuffer);
  
	sprintf(txtbuffer,"Copy completed in %d mins. Press A",diff_sec(startTime, gettime())/60);
	DrawFrameStart();
	DrawMessageBox(D_INFO, txtbuffer);
	DrawFrameFinish();
	wait_press_A();
	needsRefresh=1;
}

/* Show info about the game */
void info_game()
{  
	
	memset(&GCMDisk,0,sizeof(DiskHeader));
	
	deviceHandler_seekFile(&curFile,0,DEVICE_HANDLER_SEEK_SET);
	if(deviceHandler_readFile(&curFile,(unsigned char*)&GCMDisk,sizeof(DiskHeader)) != sizeof(DiskHeader)){
		return; 
	}
		
	DrawFrameStart();
	DrawEmptyBox(75,120, vmode->fbWidth-78, 400, COLOR_BLACK);
	if(GCMDisk.DVDMagicWord == DVD_MAGIC) {
		if(GetTextSizeInPixels(GCMDisk.GameName)>((vmode->fbWidth-80)-65)) {
			GCMDisk.GameName[32] = '\0';	//Cut what we can't fit onscreen
		}
		showBanner(230, 270, 2);  //Convert & display game banner
	}
	sprintf(txtbuffer,"%s",(GCMDisk.DVDMagicWord != DVD_MAGIC)?curFile.name:GCMDisk.GameName);
	WriteCentre(130,txtbuffer);
	if((curDevice==SD_CARD)||(curDevice == IDEEXI) ||((curDevice == DVD_DISC) && (dvdDiscTypeInt==ISO9660_DISC))) {
		sprintf(txtbuffer,"Size: %.2fMB", (float)curFile.size/1024/1024);
		WriteCentre(160,txtbuffer);
		sprintf(txtbuffer,"Position on Disk: %d",(u32)(curFile.fileBase&0xFFFFFFFF));
		WriteCentre(190,txtbuffer);
	}
	else if(curDevice == DVD_DISC)  {
		sprintf(txtbuffer,"%s DVD disc", dvdDiscTypeStr);
		WriteCentre(160,txtbuffer);
	}
	if(GCMDisk.DVDMagicWord == DVD_MAGIC) {
		sprintf(txtbuffer,"Region [%s] Audio Streaming [%s]",(GCMDisk.CountryCode=='P') ? "PAL":"NTSC",(GCMDisk.AudioStreaming=='\1') ? "YES":"NO");
		WriteCentre(220,txtbuffer);
	}

	WriteCentre(370,"Cheats(Y) - Exit(B)");
	DrawFrameFinish();
	while((PAD_ButtonsHeld(0) & PAD_BUTTON_B) || (PAD_ButtonsHeld(0) & PAD_BUTTON_Y));
	while(1){
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_Y) {
			cheats_game();
			break;
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B){
			while(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B));	
			break;
		}
	}
}

void setup_game()
{ 
	int currentSettingPos = 0/*, maxSettingPos = (curDevice==SD_CARD)?MAX_SETTING_POS:0*/;
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
	
	while(1)
	{
		DrawFrameStart();
		DrawEmptyBox (75,120, vmode->fbWidth-78, 400, COLOR_BLACK);
		WriteCentre(130,"Game Setup");
		
		//write out all the settings (dodgy)
		WriteFont(80, 160+(32*1), "Game Video Mode");
		sprintf(txtbuffer,"%s",getVideoString());
		DrawSelectableButton(vmode->fbWidth-170, 160+(32*1), -1, 160+(32*1)+30, txtbuffer, (!currentSettingPos) ? B_SELECTED:B_NOSELECT);
		WriteCentre(370,"Press B to return");
		DrawFrameFinish();
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_RIGHT)
		{
			if(currentSettingPos==0)
				curVideoSelection = (curVideoSelection<MAX_VIDEO_MODES) ? (curVideoSelection+1):0;
		}
		if(btns & PAD_BUTTON_LEFT)
		{
			if(currentSettingPos==0)
				curVideoSelection = (curVideoSelection>0) ? (curVideoSelection-1):MAX_VIDEO_MODES;
		}
		if(btns & PAD_BUTTON_B)
			break;
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) &&!(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)));
	}
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_B);
}

void credits()
{ 
	DrawFrameStart();
	DrawEmptyBox(55,120, vmode->fbWidth-58, 400, COLOR_BLACK);
	WriteCentre(130,"Swiss ver 0.1");
	WriteCentre(160,"by emu_kidid 2010");
	if(mfpvr() != GC_CPU_VERSION)
		WriteCentre(220,"Running on a Wii");
	else
		WriteCentre(220,"Running on a Gamecube");
	sprintf(txtbuffer,"ECID: %08X:%08X:%08X",mfspr(0x39C),mfspr(0x39D),mfspr(0x39E));
	WriteCentre(245,txtbuffer);
	WriteCentre(295,"Thanks to");
	WriteCentre(320,"Testers & libOGC/dkPPC authors");
	WriteCentre(370,"Press A to return");
	DrawFrameFinish();	
	wait_press_A();
}

void select_speed()
{ 	
	GC_SD_SPEED=EXI_SPEED32MHZ;
	return;/*
	while(1)
	{
		doBackdrop();
		DrawEmptyBox (75,190, vmode->fbWidth-78, 330, COLOR_BLACK);
		WriteCentre(215,"Select Speed and press A");
		DrawSelectableButton(100, 280, -1, 310, "Compatible", (GC_SD_SPEED==EXI_SPEED16MHZ) ? B_SELECTED:B_NOSELECT);
		DrawSelectableButton(380, 280, -1, 310, "Fast", (GC_SD_SPEED==EXI_SPEED32MHZ) ? B_SELECTED:B_NOSELECT);
		DrawFrameFinish();
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
		u16 btns = PAD_ButtonsHeld(0);
		if((btns & PAD_BUTTON_RIGHT) || (btns & PAD_BUTTON_LEFT)){
			if(GC_SD_SPEED==EXI_SPEED16MHZ) GC_SD_SPEED=EXI_SPEED32MHZ;
			else GC_SD_SPEED=EXI_SPEED16MHZ;
		 }
		if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_B))
			break;
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)));
	}
	sdgecko_setSpeed(GC_SD_SPEED);
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A));*/
}

void select_slot()
{  
	GC_SD_CHANNEL = 0;
	while(1) {
		doBackdrop();
		DrawEmptyBox(75,190, vmode->fbWidth-78, 330, COLOR_BLACK);
		WriteCentre(215,"Select Slot and press A");
		DrawSelectableButton(100, 280, -1, 310, "Slot A", (GC_SD_CHANNEL==0) ? B_SELECTED:B_NOSELECT);
		DrawSelectableButton(380, 280, -1, 310, "Slot B", (GC_SD_CHANNEL==1) ? B_SELECTED:B_NOSELECT);
		DrawFrameFinish();
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
		u16 btns = PAD_ButtonsHeld(0);
		if((btns & PAD_BUTTON_RIGHT) || (btns & PAD_BUTTON_LEFT)) {
			GC_SD_CHANNEL^=1;
		}
		if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_B))
			break;
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)));
	}
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A));
}

void select_device()
{  
	dvdDiscTypeStr = NotInitStr;
	while(1) {
		doBackdrop();
		DrawEmptyBox(75,190, vmode->fbWidth-78, 355, COLOR_BLACK);
		WriteCentre(195,"Select device and press A");
		drawBitmap(gcdvdsmall_Bitmap, 60, 230, 80,79);
		drawBitmap(sdsmall_Bitmap, 145, 225, 60,80);
		drawBitmap(hdd_Bitmap, 225, 225, 80,80);
		DrawSelectableButton(90, 315, -1, 345, "DVD Disc", (curDevice==DVD_DISC) ? B_SELECTED:B_NOSELECT);
		DrawSelectableButton(260, 315, -1, 345, "SDGecko", (curDevice==SD_CARD) ? B_SELECTED:B_NOSELECT);
		DrawSelectableButton(430, 315, -1, 345, "Ide-Exi", (curDevice==IDEEXI) ? B_SELECTED:B_NOSELECT);
		DrawFrameFinish();
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_RIGHT)
			curDevice = curDevice == 2 ? 0 : curDevice+1;
		if(btns & PAD_BUTTON_LEFT)
			curDevice = curDevice == 0 ? 2 : curDevice-1;
		if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_B))
			break;
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)));
	}
	while ((PAD_ButtonsHeld(0) & PAD_BUTTON_A));
	// Deinit any existing deviceHandler state
	if(deviceHandler_deinit) deviceHandler_deinit( deviceHandler_initial );
	switch(curDevice) {
		case SD_CARD:
			select_speed();
			select_slot();
			// Change all the deviceHandler pointers
			deviceHandler_initial = GC_SD_CHANNEL==0 ? &initial_SD0 : &initial_SD1;
			deviceHandler_readDir  =  deviceHandler_SD_readDir;
			deviceHandler_readFile =  deviceHandler_SD_readFile;
			deviceHandler_seekFile =  deviceHandler_SD_seekFile;
			deviceHandler_setupFile=  deviceHandler_SD_setupFile;
			deviceHandler_init     =  deviceHandler_SD_init;
			deviceHandler_deinit   =  deviceHandler_SD_deinit;
		break;
		case DVD_DISC:
			// Change all the deviceHandler pointers
			deviceHandler_initial = &initial_DVD;
			deviceHandler_readDir  =  deviceHandler_DVD_readDir;
			deviceHandler_readFile =  deviceHandler_DVD_readFile;
			deviceHandler_seekFile =  deviceHandler_DVD_seekFile;
			deviceHandler_setupFile=  deviceHandler_DVD_setupFile;
			deviceHandler_init     =  deviceHandler_DVD_init;
			deviceHandler_deinit   =  deviceHandler_DVD_deinit;
 		break;
		case IDEEXI:
			select_speed();
			select_slot();
			// Change all the deviceHandler pointers
			deviceHandler_initial = GC_SD_CHANNEL==0 ? &initial_IDEEXI0 : &initial_IDEEXI1;
			deviceHandler_readDir  =  deviceHandler_IDEEXI_readDir;
			deviceHandler_readFile =  deviceHandler_IDEEXI_readFile;
			deviceHandler_seekFile =  deviceHandler_IDEEXI_seekFile;
			deviceHandler_setupFile=  deviceHandler_IDEEXI_setupFile;
			deviceHandler_init     =  deviceHandler_IDEEXI_init;
			deviceHandler_deinit   =  deviceHandler_IDEEXI_deinit;
		break;
	}
	memcpy(&curFile, deviceHandler_initial, sizeof(file_handle));
}

