#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <ogc/color.h>
#include <ogc/dvdlow.h>
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
#include "flippy.h"
#include "gcloader.h"
#include "wkf.h"
#include "exi.h"
#include "wiiload.h"
#include "config.h"
#include "sram.h"
#include "stub_bin.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "devices/fat/ata.h"
#include "aram/sidestep.h"
#include "devices/filemeta.h"

dvdcmdblk DVDCommandBlock;
dvdcmdblk DVDInquiryBlock;
dvdcmdblk DVDStopMotorBlock;
dvddrvinfo DVDDriveInfo[2] __attribute__((aligned(32)));
u16* const DVDDeviceCode = (u16*)0x800030E6;
dvddiskid* DVDDiskID = (dvddiskid*)0x80000000;
SwissSettings swissSettings;

static void driveInfoCallback(s32 result, dvdcmdblk *block) {
	dvddrvinfo* DVDDriveInfo = DVD_GetUserData(block);
	if(result == DVD_ERROR_CANCELED) {
		DVD_StopMotorAsync(block, NULL);
		return;
	}
	else if(result == DVD_ERROR_FATAL) {
		swissSettings.hasDVDDrive = 0;
		*DVDDeviceCode = 0x0001;
	}
	else if(result >= 0) {
		swissSettings.hasDVDDrive = 1;
		switch(DVDDriveInfo->rel_date) {
			case 0:
				*DVDDeviceCode = 0x0001;
				break;
			case 0x20220420:
			case 0x20220426:
				syssramex* sramex = __SYS_LockSramEx();
				*DVDDeviceCode = sramex->dvddev_code;
				__SYS_UnlockSramEx(FALSE);
				return;
			default:
				*DVDDeviceCode = 0x8000 | DVDDriveInfo->dev_code;
				break;
		}
	}
	syssramex* sramex = __SYS_LockSramEx();
	sramex->dvddev_code = *DVDDeviceCode;
	__SYS_UnlockSramEx(TRUE);
}

static void resetCoverCallback(s32 result) {
	syssramex* sramex = __SYS_LockSramEx();
	if(sramex->dvddev_code & 0x8000) {
		swissSettings.hasDVDDrive = 2;
		*DVDDeviceCode = sramex->dvddev_code;
		DVD_Pause();
		DVD_Reset(DVD_RESETSOFT);
	}
	__SYS_UnlockSramEx(FALSE);
}

/* Initialise Video, PAD, DVD, Font */
void Initialise(void)
{
	PAD_Init ();  
	DVD_Init();
	DVD_LowSetResetCoverCallback(resetCoverCallback);
	DVD_Reset(DVD_RESETNONE);
	while(DVD_LowGetCoverStatus() == DVD_COVER_RESET);
	DVD_LowSetResetCoverCallback(NULL);
	refreshDeviceCode(false);

	// Disable IPL modchips to allow access to IPL ROM fonts
	ipl_set_config(6); 
	usleep(1000); //wait for modchip to disable (overkill)
	__SYS_ReadROM(IPLInfo,256,0);	// Read IPL tag

	if(!strncmp(&IPLInfo[0x55], "NTSC", 4))
		swissSettings.sramVideo = SYS_VIDEO_NTSC;
	else if(!strncmp(&IPLInfo[0x55], "PAL ", 4))
		swissSettings.sramVideo = SYS_VIDEO_PAL;
	else if(!strncmp(&IPLInfo[0x55], "MPAL", 4))
		swissSettings.sramVideo = SYS_VIDEO_MPAL;
	else if(!strncmp(&IPLInfo[0x55], "TDEV", 4) && (SYS_GetConsoleType() & SYS_CONSOLE_MASK) == SYS_CONSOLE_RETAIL) {
		*(u32*)0x8000002C += SYS_CONSOLE_TDEV_HW1 - SYS_CONSOLE_RETAIL_HW1;
		swissSettings.sramVideo = SYS_VIDEO_NTSC;
	}

	*(u32*)0x800000CC = swissSettings.sramVideo;
	SYS_SetVideoMode(swissSettings.sramVideo);
	swissSettings.fontEncode = SYS_GetFontEncoding();

	GXRModeObj *vmode = getVideoMode();
	setVideoMode(vmode);

	init_font();
	DrawInit(swissSettings.cubebootInvoked);
}

void __SYS_PreInit(void)
{
	u32 start = 0x80000000, end = 0x80003100;

	if (DVDDiskID->magic == DVD_MAGIC)
		start += sizeof(dvddiskid);

	DCZeroRange((void *)start, end - start);

	switch (((vu16 *)0xCC004000)[20] & 7) {
		case 0:
			*(u32 *)0x80000028 = 0x1000000;
			break;
		case 1: case 4:
			*(u32 *)0x80000028 = 0x2000000;
			break;
		case 2: case 6:
			*(u32 *)0x80000028 = 0x1800000;
			break;
		case 3: case 7:
			*(u32 *)0x80000028 = 0x3000000;
			break;
		case 5:
			*(u32 *)0x80000028 = 0x4000000;
			break;
	}

	if (((vu32 *)0xCC006000)[9] == 0xFF)
		*(u32 *)0x8000002C = SYS_CONSOLE_RETAIL_HW1;
	else
		*(u32 *)0x8000002C = SYS_CONSOLE_DEVELOPMENT_HW1;

	*(u32 *)0x8000002C += SYS_GetFlipperRevision();

	*(u32 *)0x800000F8 = SYS_GetBusFrequency();
	*(u32 *)0x800000FC = SYS_GetCoreFrequency();

	memcpy((void *)0x80001800, stub_bin, stub_bin_size);
	DCFlushRangeNoSync((void *)0x80001800, stub_bin_size);
	ICInvalidateRange((void *)0x80001800, stub_bin_size);
	_sync();

	*(u64 *)0x800030D8 = -__builtin_ppc_get_timebase();
}

uiDrawObj_t *configProgBar = NULL;
void config_migration(char* text, int state, int percent) {
	if(state) {
		if(percent < 0) {
			configProgBar = DrawPublish(DrawProgressBar(percent == -2, 0, text));
		}
		else {
			DrawUpdateProgressBar(configProgBar, percent);
		}
	}
	else {
		if(configProgBar) {
			DrawDispose(configProgBar);
		}
	}
}

/****************************************************************************
* Main
****************************************************************************/
int main(int argc, char *argv[])
{
	// Setup defaults (if no config is found)
	memset(&swissSettings, 0 , sizeof(SwissSettings));
	strcpy(swissSettings.flattenDir, "*/games");

	// Register all devices supported (order matters for boot devices)
	int i = 0;
	for(i = 0; i < MAX_DEVICES; i++)
		allDevices[i] = NULL;
	i = 0;
	allDevices[i++] = &__device_flippy;
	allDevices[i++] = &__device_flippyflash;
	allDevices[i++] = &__device_wkf;
	allDevices[i++] = &__device_wode;
	allDevices[i++] = &__device_gcloader;
	allDevices[i++] = &__device_ata_c;
	allDevices[i++] = &__device_sd_c;
	allDevices[i++] = &__device_sd_a;
	allDevices[i++] = &__device_sd_b;
	allDevices[i++] = &__device_dvd;
	allDevices[i++] = &__device_card_a;
	allDevices[i++] = &__device_card_b;
	allDevices[i++] = &__device_qoob;
	allDevices[i++] = &__device_aram;
	allDevices[i++] = &__device_sys;
	allDevices[i++] = &__device_ata_a;
	allDevices[i++] = &__device_ata_b;
	allDevices[i++] = &__device_usbgecko;
	allDevices[i++] = &__device_smb;
	allDevices[i++] = &__device_ftp;
	allDevices[i++] = &__device_fsp;
	allDevices[i++] = NULL;
	
	// Set current devices
	devices[DEVICE_CUR] = NULL;
	devices[DEVICE_PREV] = NULL;
	devices[DEVICE_DEST] = NULL;
	devices[DEVICE_CHEATS] = NULL;
	devices[DEVICE_CONFIG] = NULL;
	devices[DEVICE_PATCHES] = NULL;
	
	// Sane defaults
	refreshSRAM(&swissSettings);
	swissSettings.enableUSBGecko = USBGECKO_OFF;
	swissSettings.exiSpeed = 1;		// 32MHz
	swissSettings.uiVMode = 0; 		// Auto UI mode
	swissSettings.gameVMode = 0;	// Auto video mode
	swissSettings.lastDTVStatus = getRawDTVStatus();
	swissSettings.emulateAudioStream = 1;
	swissSettings.ftpPort = 21;
	swissSettings.fspPort = 21;
	swissSettings.fspPathMtu = 1500;
	swissSettings.bbaUseDhcp = 1;
	swissSettings.aveCompat = GCVIDEO_COMPAT;
	swissSettings.enableFileManagement = 0;
	swissSettings.recentListLevel = 2;
	memset(&swissSettings.recent[0][0], 0, PATHNAME_MAX);
	config_init_environ();

	Initialise();
	needsDeviceChange = 1;
	needsRefresh = 1;
	
	//debugging stuff
	for(i = 0; i < EXI_CHANNEL_MAX; i++) {
		if(usb_isgeckoalive(i)) {
			swissSettings.enableUSBGecko = USBGECKO_MEMCARD_SLOT_A + i;
			init_wiiload_usb_thread(&swissSettings.enableUSBGecko);
			break;
		}
	}
	print_debug("Arena Size: %iKB\n",SYS_GetArenaSize()/1024);
	print_debug("DVD Drive Present? %s\n",swissSettings.hasDVDDrive?"Yes":"No");
	print_debug("GIT Commit: %s\n", GIT_COMMIT);
	print_debug("GIT Revision: %s\n", GIT_REVISION);
	
	// Go through all devices with FEAT_BOOT_DEVICE feature and set it as current if one is available
	for(i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && (allDevices[i]->features & FEAT_BOOT_DEVICE)) {
			print_debug("Testing device %s\n", allDevices[i]->deviceName);
			if(allDevices[i]->test()) {
				deviceHandler_setDeviceAvailable(allDevices[i], true);
				devices[DEVICE_CUR] = allDevices[i];
				break;
			}
		}
	}
	if(devices[DEVICE_CUR] != NULL) {
		print_debug("Detected %s\n", devices[DEVICE_CUR]->deviceName);
		if(!devices[DEVICE_CUR]->init(devices[DEVICE_CUR]->initial)) {
			if(devices[DEVICE_CUR]->features & FEAT_AUTOLOAD_DOL) {
				load_auto_dol(argc, argv);
			}
			memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
			needsDeviceChange = 0;
			DrawLoadBackdrop();
		}
	}

	// Scan here since some devices would already be initialised (faster)
	populateDeviceAvailability();

	// Read Swiss settings
	if(!config_init(&config_migration)) {
		swissSettings.configDeviceId = DEVICE_ID_UNK;
	}
	else if(swissSettings.aveCompat != AVE_RVL_COMPAT && swissSettings.lastDTVStatus != getRawDTVStatus()) {
		show_settings(PAGE_GLOBAL, SET_AVE_COMPAT, NULL);
	}
	// If there's no default config device, set it to the first writable device available
	if(swissSettings.configDeviceId == DEVICE_ID_UNK) {
		for(i = 0; i < MAX_DEVICES; i++) {
			if(allDevices[i] != NULL && (allDevices[i]->features & FEAT_CONFIG_DEVICE) && deviceHandler_getDeviceAvailable(allDevices[i])) {
				swissSettings.configDeviceId = allDevices[i]->deviceUniqueId;
				print_debug("No default config device found, using [%s]\n", allDevices[i]->deviceName);
				if(!config_init(&config_migration)) {
					show_settings(PAGE_GLOBAL, SET_CONFIG_DEV, NULL);
				}
				else if(swissSettings.aveCompat != AVE_RVL_COMPAT && swissSettings.lastDTVStatus != getRawDTVStatus()) {
					show_settings(PAGE_GLOBAL, SET_AVE_COMPAT, NULL);
				}
				break;
			}
		}
	}
	config_parse_args(argc, argv);
	
	// Swiss video mode force
	GXRModeObj *forcedMode = getVideoModeFromSwissSetting(swissSettings.uiVMode);
	DrawVideoMode(forcedMode);
	
	// Save settings to SRAM
	swissSettings.sram60Hz = getTVFormat() == VI_EURGB60;
	swissSettings.sramProgressive = getScanMode() == VI_PROGRESSIVE;
	if(in_range(swissSettings.aveCompat, GCDIGITAL_COMPAT, GCVIDEO_COMPAT)) {
		swissSettings.sramHOffset &= ~1;
	}
	VIDEO_SetAdjustingValues(swissSettings.sramHOffset, 0);
	updateSRAM(&swissSettings, true);
	
	swissSettings.initNetworkAtStart |= !!bba_exists(LOC_MEMCARD_SLOT_A | LOC_MEMCARD_SLOT_B | LOC_SERIAL_PORT_2);
	if(swissSettings.initNetworkAtStart) {
		// Start up the BBA if it exists
		init_network_async();
	}
	
	DEVICEHANDLER_INTERFACE *device = getDeviceByLocation(LOC_DVD_CONNECTOR);
	if(device == &__device_dvd) {
		if(swissSettings.initDVDDriveAtStart) {
			DVD_Reset(DVD_RESETNONE);
			DVD_Resume();
		}
		// DVD Motor off setting
		if(swissSettings.stopMotor && DVD_GetDriveStatus() != DVD_STATE_PAUSING) {
			DVD_StopMotorAsync(&DVDStopMotorBlock, NULL);
		}
	}
	else if(device == &__device_flippy) {
		flippyversion *version = (flippyversion*)DVDDriveInfo->pad;
		u32 flippy_version = FLIPPY_VERSION(version->major, version->minor, version->build);
		if(flippy_version < FLIPPY_VERSION(FLIPPY_MINVER_MAJOR, FLIPPY_MINVER_MINOR, FLIPPY_MINVER_BUILD)) {
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_WARN, "A firmware update is required.\nflippydrive.com/updates"));
			wait_press_A();
			DrawDispose(msgBox);
		}
		else if(flippy_version < FLIPPY_VERSION(1,4,2) && (SYS_GetConsoleType() & SYS_CONSOLE_MASK) == SYS_CONSOLE_RETAIL) {
			uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_INFO, "A firmware update is available.\nflippydrive.com/updates"));
			wait_press_A();
			DrawDispose(msgBox);
		}
	}
	else if(device == &__device_gcloader) {
		if(gcloaderVersionStr != NULL) {
			if(strverscmp(swissSettings.gcloaderTopVersion, gcloaderVersionStr) < 0 || swissSettings.gcloaderHwVersion != gcloaderHwVersion) {
				strlcpy(swissSettings.gcloaderTopVersion, gcloaderVersionStr, sizeof(swissSettings.gcloaderTopVersion));
				swissSettings.gcloaderHwVersion = gcloaderHwVersion;
			}
			switch(gcloaderHwVersion) {
				case 1:
					if(strverscmp(swissSettings.gcloaderTopVersion, "2.0.1") < 0) {
						find_existing_entry("gcldr:/GCLoader_Updater_2.0.1*.dol", true);
						uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_INFO, "A firmware update is available.\ngc-loader.com/firmware-updates"));
						wait_press_A();
						DrawDispose(msgBox);
					}
					break;
				case 2:
					if(strverscmp(swissSettings.gcloaderTopVersion, "1.0.1") < 0) {
						find_existing_entry("gcldr:/GC_LOADER_HW2_UPDATER_1.0.1.dol", true);
						find_existing_entry("gcldr:/GC_LOADER_HW2_UPDATER_1.1.0*.dol", true);
						uiDrawObj_t *msgBox = DrawPublish(DrawMessageBox(D_INFO, "A firmware update is available.\ngc-loader.com/firmware-updates-hw2"));
						wait_press_A();
						DrawDispose(msgBox);
					}
					break;
			}
		}
	}
	
	// Check for autoload entry
	if(swissSettings.autoload[0]) {
		// Check that the path in the autoload entry points at a device that has been detected
		print_debug("Autoload entry detected [%s]\n", swissSettings.autoload);
		find_existing_entry(&swissSettings.autoload[0], true);
	}
	else if(swissSettings.recent[0][0] && swissSettings.recentListLevel > 1) {
		find_existing_entry(&swissSettings.recent[0][0], false);
	}

	if(swissSettings.cubebootInvoked) {
		return 0;
	}
	while(1) {
		menu_loop();
	}
	return 0;
}


// Checks if devices are available, prints name of device being detected for slow init devices
void populateDeviceAvailability() {
	uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Detecting devices\205\nThis can be skipped by holding B next time"));
	while(DVD_GetCmdBlockStatus(&DVDInquiryBlock) == DVD_STATE_BUSY) {
		if(DVD_LowGetCoverStatus() == DVD_COVER_OPEN) {
			break;
		}
		if(padsButtonsHeld() & PAD_BUTTON_B) {
			DVD_CancelAsync(&DVDInquiryBlock, NULL);
			break;
		}
	}
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && !deviceHandler_getDeviceAvailable(allDevices[i])) {
			print_debug("Checking device availability for device %s\n", allDevices[i]->deviceName);
			deviceHandler_setDeviceAvailable(allDevices[i], allDevices[i]->test());
		}
	}
	DrawDispose(msgBox);
}

void refreshDeviceCode(bool subdevice) {
	switch(DVD_GetCmdBlockStatus(&DVDInquiryBlock)) {
		case DVD_STATE_END:
		case DVD_STATE_FATAL_ERROR:
		case DVD_STATE_CANCELED:
			DVD_SetUserData(&DVDInquiryBlock, &DVDDriveInfo[subdevice]);
			DVD_InquiryAsync(&DVDInquiryBlock, &DVDDriveInfo[subdevice], driveInfoCallback);
			break;
	}
}
