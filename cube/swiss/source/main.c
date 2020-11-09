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
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <sdcard/gcsd.h>
#include "main.h"
#include "settings.h"
#include "swiss.h"
#include "bba.h"
#include "dvd.h"
#include "wkf.h"
#include "exi.h"
#include "httpd.h"
#include "config.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "devices/fat/ata.h"
#include "aram/sidestep.h"
#include "devices/filemeta.h"

u8 driveVersion[8];
SwissSettings swissSettings;


/* Initialise Video, PAD, DVD, Font */
void Initialise (void)
{
	VIDEO_Init ();
	PAD_Init ();  
	DVD_Init(); 
	*(volatile unsigned long*)0xcc00643c = 0x00000000; //allow 32mhz exi bus
	
	// Disable IPL modchips to allow access to IPL ROM fonts
	ipl_set_config(6); 
	usleep(1000); //wait for modchip to disable (overkill)
	
	
	__SYS_ReadROM(IPLInfo,256,0);	// Read IPL tag

	// By default, let libOGC figure out the video mode
	GXRModeObj *vmode = VIDEO_GetPreferredMode(NULL); //Last mode used
	if(is_gamecube()) {	// Gamecube, determine based on IPL
		int retPAD = 0, retCnt = 10000;
		while(retPAD <= 0 && retCnt >= 0) { retPAD = PAD_ScanPads(); usleep(100); retCnt--; }
		// L Trigger held down ignores the fact that there's a component cable plugged in.
		if(VIDEO_HaveComponentCable() && !(PAD_ButtonsDown(0) & PAD_TRIGGER_L)) {
			if(strstr(IPLInfo,"MPAL")!=NULL) {
				swissSettings.sramVideo = 2;
				vmode = &TVNtsc480Prog; //Progressive 480p
			}
			else if((strstr(IPLInfo,"PAL")!=NULL)) {
				swissSettings.sramVideo = 1;
				vmode = &TVPal576ProgScale; //Progressive 576p
			}
			else {
				swissSettings.sramVideo = 0;
				vmode = &TVNtsc480Prog; //Progressive 480p
			}
		}
		else {
			//try to use the IPL region
			if(strstr(IPLInfo,"MPAL")!=NULL) {
				swissSettings.sramVideo = 2;
				vmode = &TVMpal480IntDf;        //PAL-M
			}
			else if(strstr(IPLInfo,"PAL")!=NULL) {
				swissSettings.sramVideo = 1;
				vmode = &TVPal576IntDfScale;         //PAL
			}
			else {
				swissSettings.sramVideo = 0;
				vmode = &TVNtsc480IntDf;        //NTSC
			}
		}
	}
	setVideoMode(vmode);

	init_font();
	DrawInit();
	
	drive_version(&driveVersion[0]);
	swissSettings.hasDVDDrive = *(u32*)&driveVersion[0] ? 1 : 0;
	
	if(!driveVersion[0]) {
		// Reset DVD if there was a IPL replacement that hasn't done that for us yet
		uiDrawObj_t *progBox = DrawPublish(DrawProgressBar(true, 0, "Initialise DVD .. (HOLD B if NO DVD Drive)"));
		dvd_reset();	// low-level, basic
		dvd_read_id();
		if(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B)) {
			dvd_set_streaming(*(char*)0x80000008);
		}
		drive_version(&driveVersion[0]);
		swissSettings.hasDVDDrive = *(u32*)&driveVersion[0] ? 2 : 0;
		if(!swissSettings.hasDVDDrive) {
			DrawDispose(progBox);
			progBox = DrawPublish(DrawMessageBox(D_INFO, "No DVD Drive Detected !!"));
			sleep(2);
		}
		DrawDispose(progBox);
	}
}

/****************************************************************************
* Main
****************************************************************************/
int main () 
{
	// Setup defaults (if no config is found)
	memset(&swissSettings, 0 , sizeof(SwissSettings));

	// Register all devices supported (order matters for boot devices)
	int i = 0;
	for(i = 0; i < MAX_DEVICES; i++)
		allDevices[i] = NULL;
	i = 0;
	allDevices[i++] = &__device_wkf;
	allDevices[i++] = &__device_gcloader;
	allDevices[i++] = &__device_wode;
	allDevices[i++] = &__device_sd_c;
	allDevices[i++] = &__device_sd_a;
	allDevices[i++] = &__device_sd_b;
	allDevices[i++] = &__device_card_a;
	allDevices[i++] = &__device_card_b;
	allDevices[i++] = &__device_dvd;
	allDevices[i++] = &__device_ide_a;
	allDevices[i++] = &__device_ide_b;
	allDevices[i++] = &__device_qoob;
	allDevices[i++] = &__device_smb;
	allDevices[i++] = &__device_sys;
	allDevices[i++] = &__device_usbgecko;
	allDevices[i++] = &__device_ftp;
	allDevices[i++] = &__device_fsp;
	allDevices[i++] = NULL;
	
	// Set current devices
	devices[DEVICE_CUR] = NULL;
	devices[DEVICE_DEST] = NULL;
	devices[DEVICE_TEMP] = NULL;
	devices[DEVICE_CONFIG] = NULL;
	devices[DEVICE_PATCHES] = NULL;
	
	Initialise();
	
	// Sane defaults
	refreshSRAM(&swissSettings);
	swissSettings.debugUSB = 0;
	swissSettings.gameVMode = 0;	// Auto video mode
	swissSettings.exiSpeed = 1;		// 32MHz
	swissSettings.uiVMode = 0; 		// Auto UI mode
	swissSettings.aveCompat = 1;
	swissSettings.enableFileManagement = 0;

	needsDeviceChange = 1;
	needsRefresh = 1;
	
	//debugging stuff
	if(swissSettings.debugUSB) {
		if(usb_isgeckoalive(1)) {
			usb_flush(1);
		}
		print_gecko("Arena Size: %iKb\r\n",(SYS_GetArena1Hi()-SYS_GetArena1Lo())/1024);
		print_gecko("DVD Drive Present? %s\r\n",swissSettings.hasDVDDrive?"Yes":"No");
		print_gecko("GIT Commit: %s\r\n", GITREVISION);
		print_gecko("GIT Revision: %s\r\n", GITVERSION);
	}
	
	// Go through all devices with FEAT_BOOT_DEVICE feature and set it as current if one is available
	for(i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && (allDevices[i]->features & FEAT_BOOT_DEVICE)) {
			print_gecko("Testing device %s\r\n", allDevices[i]->deviceName);
			if(allDevices[i]->test()) {
				deviceHandler_setDeviceAvailable(allDevices[i], true);
				devices[DEVICE_CUR] = allDevices[i];
				break;
			}
		}
	}
	if(devices[DEVICE_CUR] != NULL) {
		print_gecko("Detected %s\r\n", devices[DEVICE_CUR]->deviceName);
		if(devices[DEVICE_CUR]->init(devices[DEVICE_CUR]->initial)) {
			if(devices[DEVICE_CUR]->features & FEAT_AUTOLOAD_DOL) {
				load_auto_dol();
			}
			memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
			needsDeviceChange = 0;
		}
	}

	// Scan here since some devices would already be initialised (faster)
	populateDeviceAvailability();

	// If there's no default config device, set it to the first writable device available
	if(swissSettings.configDeviceId == DEVICE_ID_UNK) {
		for(int i = 0; i < MAX_DEVICES; i++) {
			if(allDevices[i] != NULL && (allDevices[i]->features & FEAT_CONFIG_DEVICE) && deviceHandler_getDeviceAvailable(allDevices[i])) {
				swissSettings.configDeviceId = allDevices[i]->deviceUniqueId;
				print_gecko("No default config device found, using [%s]\r\n", allDevices[i]->deviceName);
				syssramex* sramex = __SYS_LockSramEx();
				sramex->__padding0 = swissSettings.configDeviceId;
				__SYS_UnlockSramEx(1);
				while(!__SYS_SyncSram());
				break;
			}
		}
	}
	
	// Try to open up the config .ini in case it hasn't been opened already
	if(config_init()) {
		// TODO notification area this
		print_gecko("Loaded %i entries from the config file\r\n",config_get_count());
	}
	
	// Swiss video mode force
	GXRModeObj *forcedMode = getVideoModeFromSwissSetting(swissSettings.uiVMode);
	
	if((forcedMode != NULL) && (forcedMode != getVideoMode())) {
		setVideoMode(forcedMode);
	}
	
	if(swissSettings.initNetworkAtStart) {
		// Start up the BBA if it exists
		uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Initialising Network"));
		init_network();
		init_httpd_thread();
		DrawDispose(msgBox);
	}
	
	// DVD Motor off setting; Always stop the drive if we only started it to read the ID out
	if((swissSettings.stopMotor && swissSettings.hasDVDDrive) || (swissSettings.hasDVDDrive == 2)) {
		dvd_motor_off();
	}
	
	// Check for autoload entry
	if(swissSettings.autoload[0]) {
		// Check that the path in the autoload entry points at a device that has been detected
		print_gecko("Autoload entry found [%s]\r\n", swissSettings.autoload);
		
		// get the device handler for it
		DEVICEHANDLER_INTERFACE *autoloadDevice = getDeviceFromPath(&swissSettings.autoload[0]);
		if(autoloadDevice) {
			print_gecko("Autoload entry lives on found device [%s]\r\n", autoloadDevice->deviceName);
			
			DEVICEHANDLER_INTERFACE *oldDevice = devices[DEVICE_CUR];
			devices[DEVICE_CUR] = autoloadDevice;
			// Init the device if it isn't one we were about to browse anyway
			if(devices[DEVICE_CUR] == oldDevice || devices[DEVICE_CUR]->init(devices[DEVICE_CUR]->initial)) {
				file_handle *oldPath = calloc(1, sizeof(file_handle));
				memcpy(oldPath, &curFile, sizeof(file_handle));
				memset(&curFile, 0, sizeof(file_handle));
				strcpy(&curFile.name[0], &swissSettings.autoload[0]);
				if(devices[DEVICE_CUR] == &__device_dvd) curFile.size = DISC_SIZE;
				if(devices[DEVICE_CUR]->readFile(&curFile, NULL, 0) == 0) {
					populate_meta(&curFile);
					load_file();
				}
				// User cancelled, clean things up
				memcpy(&curFile, oldPath, sizeof(file_handle));
				free(oldPath);
			}
			devices[DEVICE_CUR] = oldDevice;
		}
		else {
			print_gecko("Autoload entry device was not found\r\n");
		}
	}

	while(1) {
		menu_loop();
	}
	return 0;
}


// Checks if devices are available, prints name of device being detected for slow init devices
void populateDeviceAvailability() {
	if(PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
		deviceHandler_setAllDevicesAvailable();
		return;
	}
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Detecting devices ...\nThis can be skipped by holding B next time"));
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && !deviceHandler_getDeviceAvailable(allDevices[i])) {
			print_gecko("Checking device availability for device %s\r\n", allDevices[i]->deviceName);
			deviceHandler_setDeviceAvailable(allDevices[i], allDevices[i]->test());
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
			deviceHandler_setAllDevicesAvailable();
			break;
		}
	}
	DrawDispose(msgBox);
}
