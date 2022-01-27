#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <ogc/exi.h>
#include <ogc/machine/processor.h>
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "info.h"
#include "config.h"
#include "settings.h"
#include "exi.h"

#define page_x_ofs_key (30)
#define page_x_ofs_val (410)
#define page_y_line (25)
#define page_nav_y (390)
#define page_saveexit_y (425)
#define label_size (0.75f)
	
SwissSettings tempSettings;
char *uiVModeStr[] = {"Auto", "480i", "480p", "576i", "576p"};
char *gameVModeStr[] = {"No", "480i", "480sf", "240p", "960i", "480p", "1080i60", "540p60", "576i", "576sf", "288p", "1152i", "576p", "1080i50", "540p50"};
char *forceHScaleStr[] = {"Auto", "1:1", "11:10", "9:8", "640px", "656px", "672px", "704px", "720px"};
char *forceVFilterStr[] = {"Auto", "0", "1", "2"};
char *forceWidescreenStr[] = {"No", "3D", "2D+3D"};
char *invertCStickStr[] = {"No", "X", "Y", "X&Y"};
char *disableVideoPatchesStr[] = {"None", "Game", "All"};
char *emulateReadSpeedStr[] = {"No", "Yes", "Wii"};
char *igrTypeStr[] = {"Disabled", "Reboot", "igr.dol"};
char *aveCompatStr[] = {"CMPV-DOL", "GCVideo", "AVE-RVL", "AVE N-DOL"};
char *fileBrowserStr[] = {"Standard", "Carousel"};
char *bs2BootStr[] = {"No", "Yes", "Sound 1", "Sound 2"};
char *sramLang[] = {"English", "German", "French", "Spanish", "Italian", "Dutch", "Japanese"};
char *recentListLevelStr[] = {"On", "Lazy", "Off"};

static char *tooltips_global[PAGE_GLOBAL_MAX+1] = {
	"System Sound:\n\nSets the default audio output type used by most games",
	"Screen Position:\n\nAdjusts the horizontal screen position in games",
	"System Language:\n\nSystem language used in games, primarily multi-5 PAL games",
	"SD/IDE Speed:\n\nThe speed to try and use on the EXI bus for SD Card Adapters or IDE-EXI devices.\n32 MHz may not work on some SD cards.",
	 NULL,
	"In-Game Reset: (A + Z + Start)\n\nReboot: Soft-Reset the GameCube\nigr.dol: Low mem (< 0x81300000) igr.dol at the root of SD Card",
	"Configuration Device:\n\nThe device that Swiss will use to load and save swiss.ini from.\nThis setting is stored in SRAM and will remain on reboot.",
	"AVE Compatibility:\n\nSets the compatibility mode for the used audio/video encoder.\n\nAVE N-DOL - Output PAL as NTSC 50\nCMPV-DOL - Enable 1080i & 540p\nGCVideo - Apply firmware workarounds for GCVideo (default)\nAVE-RVL - Support 960i & 1152i without WiiVideo",
	"File Browser Type:\n\nStandard - Displays files with minimal detail (default)\n\nCarousel - Suited towards Game/DOL only use, consider combining\nthis option with the File Management setting turned off\nand Hide Unknown File Types turned on for a better experience.",
	NULL,
	"Recent List:\n\n(On) - Press Start while browsing to show a recent list.\n(Lazy) - Same as On but list updates only for new entries.\n(Off) - Recent list is completely disabled.\n\nThe lazy/off options exist to minimise SD card writes."
};

static char *tooltips_advanced[PAGE_ADVANCED_MAX+1] = {
	"Enable USB Gecko Debug via Slot B:\n\nIf a USB Gecko is present in slot B, debug output from\nSwiss & in game (if the game supported output over OSReport)\nwill be output. If nothing is reading the data out from the\ndevice it may cause Swiss/games to hang.",
	"Hide Unknown file types:\n\nDisabled - Show all files (default)\nEnabled - Swiss will hide unknown file types from being displayed\n\nKnown file types are:\n GameCube Executables (.dol)\n Disc backups (.iso/.gcm/.tgc)\n MP3 Music (.mp3)\n WASP/WKF Flash files (.fzn)\n GameCube Memory Card Files (.gci)\n GameCube Executables with parameters appended (.dol+cli)\n GameCube ELF files (.elf)",
	"Stop DVD Motor on startup\n\nDisabled - Leave it as-is (default)\nEnabled - Stop the DVD drive from spinning when Swiss starts\n\nThis option is mostly for users booting from game\nexploits where the disc will already be spinning.",
	"WiiRD debugging:\n\nDisabled - Boot as normal (default)\nEnabled - This will start a game with the WiiRD debugger enabled & paused\n\nThe WiiRD debugger takes up more memory and can cause issues.",
	"File Management:\n\nWhen enabled, pressing Z on an entry in the file browser will allow it to be managed.",
	"Auto-load all cheats:\n\nIf enabled, and a cheats file for a particular game is found\ne.g. sd:/cheats/GPOP8D.txt (on a compatible device)\nthen all cheats in the file will be enabled",
	NULL,
	NULL,
	"Force DTV Status:\n\nDisabled - Use signal from the video interface (default)\nEnabled - Force on in case of hardware fault"
};

static char *tooltips_network[PAGE_NETWORK_MAX+1] = {
	"Init network at startup:\n\nDisabled - Do not initialise the BBA even if present (default)\nEnabled - If a BBA is present, it will be initialised at startup\n\nIf initialised, navigate to the IP in a web browser to backup various data"
};

static char *tooltips_game[PAGE_GAME_MAX+1] = {
	NULL,
	NULL,
	"Force Vertical Offset:\n\n+0 - Standard value\n-2 - GCVideo-DVI compatible (480i)\n-3 - GCVideo-DVI compatible (default)\n-4 - GCVideo-DVI compatible (240p)\n-12 - Datapath VisionRGB (480p)",
	NULL,
	NULL,
	NULL,
	NULL,
	"Invert Camera Stick:\n\nNo - Leave C Stick as-is (default)\nX - Invert X-axis of the C Stick\nY - Invert Y-axis of the C Stick\nX&Y - Invert both axes of the C Stick",
	"Digital Trigger Level:\n\nSets the level where the L/R Button is fully pressed.",
	"Emulate Read Speed:\n\nNo - Start transfer immediately (default)\nYes - Delay transfer to simulate the GameCube disc drive\nWii - Delay transfer to simulate the Wii disc drive\n\nThis is necessary to avoid programming mistakes obfuscated\nby the original medium, or for speedrunning."
};

syssram* sram;
syssramex* sramex;

// Number of settings (including Back, Next, Save, Exit buttons) per page
int settings_count_pp[5] = {PAGE_GLOBAL_MAX, PAGE_NETWORK_MAX, PAGE_ADVANCED_MAX, PAGE_GAME_DEFAULTS_MAX, PAGE_GAME_MAX};

void refreshSRAM(SwissSettings *settings) {
	bool writeSram = false;
	sram = __SYS_LockSram();
	if(!__SYS_CheckSram()) {
		memset(sram, 0, sizeof(syssram));
		sram->flags |= 0x10;
		sram->flags |= 0x04;
		sram->flags |= swissSettings.sramVideo & 0x03;
		writeSram = true;
	}
	settings->sramHOffset = sram->display_offsetH;
	settings->sram60Hz = (sram->ntd >> 6) & 1;
	settings->sramLanguage = sram->lang;
	settings->sramProgressive = (sram->flags >> 7) & 1;
	settings->sramStereo = (sram->flags >> 2) & 1;
	__SYS_UnlockSram(writeSram);
	if(writeSram)
		while(!__SYS_SyncSram());
	sramex = __SYS_LockSramEx();
	settings->configDeviceId = sramex->__padding0;
	if(settings->configDeviceId > DEVICE_ID_MAX || !(getDeviceByUniqueId(settings->configDeviceId)->features & FEAT_CONFIG_DEVICE)) {
		settings->configDeviceId = DEVICE_ID_UNK;
	}
	__SYS_UnlockSramEx(0);
}

char* getConfigDeviceName(SwissSettings *settings) {
	DEVICEHANDLER_INTERFACE *configDevice = getDeviceByUniqueId(settings->configDeviceId);
	return configDevice != NULL ? (char*)(configDevice->deviceName) : "None";
}

char* get_tooltip(int page_num, int option) {
	char *textPtr = NULL;
	if(page_num == PAGE_GLOBAL) {
		textPtr = tooltips_global[option];
	}
	else if(page_num == PAGE_NETWORK) {
		textPtr = tooltips_network[option];
	}
	else if(page_num == PAGE_ADVANCED) {
		textPtr = tooltips_advanced[option];
	}
	else if(page_num == PAGE_GAME_DEFAULTS) {
		textPtr = tooltips_game[option];
	}
	else if(page_num == PAGE_GAME) {
		textPtr = tooltips_game[option];
	}
	return textPtr;
}

void add_tooltip_label(uiDrawObj_t* page, int page_num, int option) {
	if(get_tooltip(page_num, option)) {
		DrawAddChild(page, DrawFadingLabel(484, 54, "Press (Y) for help", 0.65f));
	}
}

void drawSelectIndicator(uiDrawObj_t* page, int y) {
	DrawAddChild(page, DrawStyledLabel(20, y + (page_y_line / 4), "*", 0.35f, false, disabledColor));	
}

void drawSettingEntryString(uiDrawObj_t* page, int *y, char *label, char *key, bool selected, bool enabled) {
	if(selected) {
		drawSelectIndicator(page, *y);
	}
	DrawAddChild(page, DrawStyledLabel(page_x_ofs_key, *y, label, label_size, false, enabled ? defaultColor:deSelectedColor));
	DrawAddChild(page, DrawStyledLabel(page_x_ofs_val, *y, key, label_size, false, enabled && selected ? defaultColor:deSelectedColor));
	*y += page_y_line; 
}

void drawSettingEntryBoolean(uiDrawObj_t* page, int *y, char *label, bool boolval, bool selected, bool enabled) {
	if(selected) {
		drawSelectIndicator(page, *y);
	}
	drawSettingEntryString(page, y, label, boolval ? "Yes" : "No", selected, enabled);
}

void drawSettingEntryNumeric(uiDrawObj_t* page, int *y, char *label, int num, bool selected, bool enabled) {
	if(selected) {
		drawSelectIndicator(page, *y);
	}
	sprintf(txtbuffer, "%i", num);
	drawSettingEntryString(page, y, label, txtbuffer, selected, enabled);
}

uiDrawObj_t* settings_draw_page(int page_num, int option, file_handle *file, ConfigEntry *gameConfig) {
	uiDrawObj_t* page = DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 460);
	char sramHOffsetStr[8];
	char forceVOffsetStr[8];
	char triggerLevelStr[8];
	
	// Save Settings to current device (**Shown on all tabs**)
	/** Global Settings (Page 1/) */
	// System Sound [Mono/Stereo]
	// Screen Position [+/-0]
	// System Language [English/German/French/Spanish/Italian/Dutch]
	// SD/IDE Speed [16/32 MHz]
	// Swiss Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), etc]
	// In-Game Reset [Yes/No]
	// Configuration Device [Writable device name]
	// AVE Compatibility
	// Filebrowser Type [Standard / Carousel]
	// ShowHiddenFiles [Yes/No]
	// Recent List: [On/Lazy/Off]

	/** Advanced Settings (Page 2/) */
	// Enable USB Gecko Debug via Slot B [Yes/No]
	// Hide Unknown file types [Yes/No]	// TODO Implement
	// Stop DVD Motor on startup [Yes/No]
	// Enable WiiRD debugging in Games [Yes/No]
	// Enable File Management [Yes/No]
	// Auto-load all cheats [Yes/No]
	// Init network at startup [Yes/No]
	
	/** Current Game Settings - only if a valid GCM file is highlighted (Page 3/) */
	// Force Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), Auto, etc]
	// Force Horizontal Scale [Auto/1:1/11:10/9:8/640px/656px/672px/704px/720px]
	// Force Vertical Offset [+/-0]
	// Force Vertical Filter [Auto/0/1/2]
	// Force Anisotropic Filter [Yes/No]
	// Force Widescreen [No/3D/2D+3D]
	// Force Text Encoding [Auto/ANSI/SJIS]
	// Disable Audio Streaming [Yes/No]

	bool isNavOption = false;
	// Add paging and save/cancel buttons
	if(page_num != PAGE_MIN) {
		isNavOption = option == settings_count_pp[page_num]-(page_num != PAGE_MAX ? 3:2);
		DrawAddChild(page, DrawSelectableButton(50, page_nav_y, -1, 420, "Back", isNavOption));
	}
	if(page_num != PAGE_MAX) {
		isNavOption = isNavOption || option == settings_count_pp[page_num]-2;
		DrawAddChild(page, DrawSelectableButton(510, page_nav_y, -1, 420, "Next", option == settings_count_pp[page_num]-2 ? B_SELECTED:B_NOSELECT));
	}
	DrawAddChild(page, DrawSelectableButton(120, page_saveexit_y, -1, 455, "Save & Exit", option == settings_count_pp[page_num]-1 ? B_SELECTED:B_NOSELECT));
	DrawAddChild(page, DrawSelectableButton(320, page_saveexit_y, -1, 455, "Discard & Exit", option ==  settings_count_pp[page_num] ? B_SELECTED:B_NOSELECT));
	isNavOption = isNavOption || (option >= settings_count_pp[page_num]-1);
	
	int page_y_ofs = 110;
	// Page specific buttons
	if(page_num == PAGE_GLOBAL) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Global Settings (1/5):"));
		drawSettingEntryString(page, &page_y_ofs, "System Sound:", swissSettings.sramStereo ? "Stereo":"Mono", option == SET_SYS_SOUND, true);
		sprintf(sramHOffsetStr, "%+hi", swissSettings.sramHOffset);
		drawSettingEntryString(page, &page_y_ofs, "Screen Position:", sramHOffsetStr, option == SET_SCREEN_POS, true);
		drawSettingEntryString(page, &page_y_ofs, "System Language:", swissSettings.sramLanguage > SRAM_LANG_MAX ? "Unknown" : sramLang[swissSettings.sramLanguage], option == SET_SYS_LANG, true);
		drawSettingEntryString(page, &page_y_ofs, "SD/IDE Speed:", swissSettings.exiSpeed ? "32 MHz":"16 MHz", option == SET_EXI_SPEED, true);	
		drawSettingEntryString(page, &page_y_ofs, "Swiss Video Mode:", uiVModeStr[swissSettings.uiVMode], option == SET_SWISS_VIDEOMODE, true);
		drawSettingEntryString(page, &page_y_ofs, "In-Game Reset:", igrTypeStr[swissSettings.igrType], option == SET_IGR, true);
		drawSettingEntryString(page, &page_y_ofs, "Configuration Device:", getConfigDeviceName(&swissSettings), option == SET_CONFIG_DEV, true);
		drawSettingEntryString(page, &page_y_ofs, "AVE Compatibility:", aveCompatStr[swissSettings.aveCompat], option == SET_AVE_COMPAT, true);
		drawSettingEntryString(page, &page_y_ofs, "File Browser Type:", fileBrowserStr[swissSettings.fileBrowserType], option == SET_FILEBROWSER_TYPE, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Show hidden files:", swissSettings.showHiddenFiles, option == SET_SHOW_HIDDEN, true);
		drawSettingEntryString(page, &page_y_ofs, "Recent List:", recentListLevelStr[swissSettings.recentListLevel], option == SET_RECENT_LIST, true);
		
	}
	else if(page_num == PAGE_NETWORK) {
		int settings_per_page = 10;
		int scrollBarHeight = 90+(settings_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)SET_SMB_PASS);
		DrawAddChild(page, DrawVertScrollBar(getVideoMode()->fbWidth-45, page_y_ofs, 25, scrollBarHeight, (float)((float)option/(float)(SET_PAGE_2_BACK-1)),scrollBarTabHeight));
		bool netEnable = exi_bba_exists();
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Network Settings (2/5):"));	
		// TODO settings to a new typedef that ties type etc all together, then draw a "page" of these rather than this at some point.
		if(option < SET_SMB_HOSTIP) {
			drawSettingEntryBoolean(page, &page_y_ofs, "Init network at startup:", swissSettings.initNetworkAtStart, option == SET_INIT_NET, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FTP Host IP:", swissSettings.ftpHostIp, option == SET_FTP_HOSTIP, netEnable);
			drawSettingEntryNumeric(page, &page_y_ofs, "FTP Port:", swissSettings.ftpPort, option == SET_FTP_PORT, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FTP Username:", swissSettings.ftpUserName, option == SET_FTP_USER, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FTP Password:", "*****", option == SET_FTP_PASS, netEnable);
			drawSettingEntryBoolean(page, &page_y_ofs, "FTP PASV Mode:", swissSettings.ftpUsePasv, option == SET_FTP_PASV, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FSP Host IP:", swissSettings.fspHostIp, option == SET_FSP_HOSTIP, netEnable);
			drawSettingEntryNumeric(page, &page_y_ofs, "FSP Port:", swissSettings.fspPort, option == SET_FSP_PORT, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FSP Password:", "*****", option == SET_FSP_PASS, netEnable);
		} else {
			drawSettingEntryString(page, &page_y_ofs, "SMB Host IP:", swissSettings.smbServerIp, option == SET_SMB_HOSTIP, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "SMB Share:", swissSettings.smbShare, option == SET_SMB_SHARE, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "SMB Username:", swissSettings.smbUser, option == SET_SMB_USER, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "SMB Password:", "*****", option == SET_SMB_PASS, netEnable);
		}
	}
	else if(page_num == PAGE_ADVANCED) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Advanced Settings (3/5):"));
		bool enabledVideoPatches = swissSettings.disableVideoPatches < 2;
		bool emulatedMemoryCard = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_MEMCARD);
		drawSettingEntryBoolean(page, &page_y_ofs, "USB Gecko Debug via Slot B:", swissSettings.debugUSB, option == SET_ENABLE_USBGECKODBG, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Hide Unknown file types:", swissSettings.hideUnknownFileTypes, option == SET_HIDE_UNK, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Stop DVD Motor on startup:", swissSettings.stopMotor, option == SET_STOP_MOTOR, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "WiiRD debugging:", swissSettings.wiirdDebug, option == SET_WIIRDDBG, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "File Management:", swissSettings.enableFileManagement, option == SET_FILE_MGMT, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Auto-load all cheats:", swissSettings.autoCheats, option == SET_ALL_CHEATS, true);
		drawSettingEntryString(page, &page_y_ofs, "Disable Video Patches:", disableVideoPatchesStr[swissSettings.disableVideoPatches], option == SET_ENABLE_VIDPATCH, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Force Video Active:", swissSettings.forceVideoActive, option == SET_FORCE_VIDACTIVE, enabledVideoPatches);
		drawSettingEntryBoolean(page, &page_y_ofs, "Force DTV Status:", swissSettings.forceDTVStatus, option == SET_FORCE_DTVSTATUS, enabledVideoPatches);
		drawSettingEntryString(page, &page_y_ofs, "Boot through IPL:", bs2BootStr[swissSettings.bs2Boot], option == SET_BS2BOOT, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Memory Card:", swissSettings.emulateMemoryCard, option == SET_EMULATE_MEMCARD, emulatedMemoryCard);
	}
	else if(page_num == PAGE_GAME_DEFAULTS) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Default Game Settings (4/5):"));
		bool enabledVideoPatches = swissSettings.disableVideoPatches < 2;
		bool emulatedReadSpeed = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED);
		drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[swissSettings.gameVMode], option == SET_DEFAULT_FORCE_VIDEOMODE, enabledVideoPatches);
		drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[swissSettings.forceHScale], option == SET_DEFAULT_HORIZ_SCALE, enabledVideoPatches);
		sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
		drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_DEFAULT_VERT_OFFSET, enabledVideoPatches);
		drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[swissSettings.forceVFilter], option == SET_DEFAULT_VERT_FILTER, enabledVideoPatches);
		drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", swissSettings.disableDithering, option == SET_DEFAULT_ALPHA_DITHER, enabledVideoPatches);
		drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", swissSettings.forceAnisotropy, option == SET_DEFAULT_ANISO_FILTER, true);
		drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[swissSettings.forceWidescreen], option == SET_DEFAULT_WIDESCREEN, true);
		drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[swissSettings.invertCStick], option == SET_DEFAULT_INVERT_CAMERA, true);
		sprintf(triggerLevelStr, "%hhu", swissSettings.triggerLevel);
		drawSettingEntryString(page, &page_y_ofs, "Digital Trigger Level:", triggerLevelStr, option == SET_DEFAULT_TRIGGER_LEVEL, true);
		drawSettingEntryString(page, &page_y_ofs, "Emulate Read Speed:", emulateReadSpeedStr[swissSettings.emulateReadSpeed], option == SET_DEFAULT_READ_SPEED, emulatedReadSpeed);
	}
	else if(page_num == PAGE_GAME) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Current Game Settings (5/5):"));
		bool enabledGamePatches = file != NULL && gameConfig != NULL;
		if(enabledGamePatches) {
			bool enabledVideoPatches = swissSettings.disableVideoPatches < 2;
			bool emulatedReadSpeed = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED);
			drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[gameConfig->gameVMode], option == SET_FORCE_VIDEOMODE, enabledVideoPatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[gameConfig->forceHScale], option == SET_HORIZ_SCALE, enabledVideoPatches);
			sprintf(forceVOffsetStr, "%+hi", gameConfig->forceVOffset);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_VERT_OFFSET, enabledVideoPatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[gameConfig->forceVFilter], option == SET_VERT_FILTER, enabledVideoPatches);
			drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", gameConfig->disableDithering, option == SET_ALPHA_DITHER, enabledVideoPatches);
			drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", gameConfig->forceAnisotropy, option == SET_ANISO_FILTER, true);
			drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[gameConfig->forceWidescreen], option == SET_WIDESCREEN, true);
			drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[gameConfig->invertCStick], option == SET_INVERT_CAMERA, true);
			sprintf(triggerLevelStr, "%hhu", gameConfig->triggerLevel);
			drawSettingEntryString(page, &page_y_ofs, "Digital Trigger Level:", triggerLevelStr, option == SET_TRIGGER_LEVEL, true);
			drawSettingEntryString(page, &page_y_ofs, "Emulate Read Speed:", emulateReadSpeedStr[gameConfig->emulateReadSpeed], option == SET_READ_SPEED, emulatedReadSpeed);
		}
		else {
			// Just draw the defaults again
			drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[swissSettings.gameVMode], option == SET_FORCE_VIDEOMODE, false);
			drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[swissSettings.forceHScale], option == SET_HORIZ_SCALE, false);
			sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_VERT_OFFSET, false);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[swissSettings.forceVFilter], option == SET_VERT_FILTER, false);
			drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", swissSettings.disableDithering, option == SET_ALPHA_DITHER, false);
			drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", swissSettings.forceAnisotropy, option == SET_ANISO_FILTER, false);
			drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[swissSettings.forceWidescreen], option == SET_WIDESCREEN, false);
			drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[swissSettings.invertCStick], option == SET_INVERT_CAMERA, false);
			sprintf(triggerLevelStr, "%hhu", swissSettings.triggerLevel);
			drawSettingEntryString(page, &page_y_ofs, "Digital Trigger Level:", triggerLevelStr, option == SET_TRIGGER_LEVEL, false);
			drawSettingEntryString(page, &page_y_ofs, "Emulate Read Speed:", emulateReadSpeedStr[swissSettings.emulateReadSpeed], option == SET_READ_SPEED, false);
		}
	}
	// If we have a tooltip for this page/option, add a fading label telling the user to press Y for help
	add_tooltip_label(page, page_num, option);
	
	DrawPublish(page);
	return page;
}

void settings_toggle(int page, int option, int direction, file_handle *file, ConfigEntry *gameConfig) {
	if(page == PAGE_GLOBAL) {
		switch(option) {
			case SET_SYS_SOUND:
				swissSettings.sramStereo ^= 1;
			break;
			case SET_SCREEN_POS:
				if(swissSettings.aveCompat == 1) {
					swissSettings.sramHOffset /= 2;
					swissSettings.sramHOffset += direction;
					swissSettings.sramHOffset *= 2;
				}
				else {
					swissSettings.sramHOffset += direction;
				}
				VIDEO_SetAdjustingValues(swissSettings.sramHOffset, 0);
			break;
			case SET_SYS_LANG:
				swissSettings.sramLanguage += direction;
				if(swissSettings.sramLanguage > SYS_LANG_DUTCH)
					swissSettings.sramLanguage = SYS_LANG_ENGLISH;
				if(swissSettings.sramLanguage < SYS_LANG_ENGLISH)
					swissSettings.sramLanguage = SYS_LANG_DUTCH;
			break;
			case SET_EXI_SPEED:
				swissSettings.exiSpeed ^= 1;
			break;
			case SET_SWISS_VIDEOMODE:
				swissSettings.uiVMode += direction;
				if(swissSettings.uiVMode > 4)
					swissSettings.uiVMode = 0;
				if(swissSettings.uiVMode < 0)
					swissSettings.uiVMode = 4;
			break;
			case SET_IGR:
				swissSettings.igrType += direction;
				if(swissSettings.igrType > 2)
					swissSettings.igrType = 0;
				if(swissSettings.igrType < 0)
					swissSettings.igrType = 2;
			break;
			case SET_CONFIG_DEV:
			{
				int curDevicePos = -1;
				
				// Set it to the first writable device available
				if(swissSettings.configDeviceId == DEVICE_ID_UNK) {
					for(int i = 0; i < MAX_DEVICES; i++) {
						if(allDevices[i] != NULL && (allDevices[i]->features & FEAT_CONFIG_DEVICE)) {
							swissSettings.configDeviceId = allDevices[i]->deviceUniqueId;
							return;
						}
					}
				}
				
				// get position in allDevices for current save device
				for(int i = 0; i < MAX_DEVICES; i++) {
					if(allDevices[i] != NULL && allDevices[i]->deviceUniqueId == swissSettings.configDeviceId) {
						curDevicePos = i;
						break;
					}
				}

				if(curDevicePos >= 0) {
					if(direction > 0) {
						curDevicePos = allDevices[curDevicePos+1] == NULL ? 0 : curDevicePos+1;
					}
					else {
						curDevicePos = curDevicePos > 0 ? curDevicePos-1 : 0;
					}
					// Go to next writable device
					while((allDevices[curDevicePos] == NULL) || !(allDevices[curDevicePos]->features & FEAT_CONFIG_DEVICE)) {
						curDevicePos += direction;
						if((curDevicePos < 0) || (curDevicePos >= MAX_DEVICES)){
							curDevicePos = direction > 0 ? 0 : MAX_DEVICES-1;
						}
					}
					if(allDevices[curDevicePos] != NULL) {
						swissSettings.configDeviceId = allDevices[curDevicePos]->deviceUniqueId;
					}
				}
			}
			break;
			case SET_AVE_COMPAT:
				swissSettings.aveCompat += direction;
				if(swissSettings.aveCompat > 3)
					swissSettings.aveCompat = 0;
				if(swissSettings.aveCompat < 0)
					swissSettings.aveCompat = 3;
			break;
			case SET_FILEBROWSER_TYPE:
				swissSettings.fileBrowserType ^= 1;
			break;
			case SET_SHOW_HIDDEN:
				swissSettings.showHiddenFiles ^= 1;
			break;
			case SET_RECENT_LIST:
				swissSettings.recentListLevel += direction;
				if(swissSettings.recentListLevel > 2)
					swissSettings.recentListLevel = 0;
				if(swissSettings.recentListLevel < 0)
					swissSettings.recentListLevel = 2;
			break;
		}	
	}
	else if(page == PAGE_NETWORK) {
		switch(option) {
			case SET_INIT_NET:
				swissSettings.initNetworkAtStart ^= 1;
			break;
			case SET_FTP_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "FTP Host IP", &swissSettings.ftpHostIp, sizeof(swissSettings.ftpHostIp));
			break;
			case SET_FTP_PORT:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "FTP Port", &swissSettings.ftpPort, 5);
			break;
			case SET_FTP_USER:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "FTP Username", &swissSettings.ftpUserName, sizeof(swissSettings.ftpUserName));
			break;
			case SET_FTP_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "FTP Password", &swissSettings.ftpPassword, sizeof(swissSettings.ftpPassword));
			break;
			case SET_FTP_PASV:
				swissSettings.ftpUsePasv ^= 1;
			break;
			case SET_FSP_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "FSP Host IP", &swissSettings.fspHostIp, sizeof(swissSettings.fspHostIp));
			break;
			case SET_FSP_PORT:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "FSP Port", &swissSettings.fspPort, 5);
			break;
			case SET_FSP_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "FSP Password", &swissSettings.fspPassword, sizeof(swissSettings.fspPassword));
			break;
			case SET_SMB_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "SMB Host IP", &swissSettings.smbServerIp, sizeof(swissSettings.smbServerIp));
			break;
			case SET_SMB_SHARE:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "SMB Share", &swissSettings.smbShare, sizeof(swissSettings.smbShare));
			break;
			case SET_SMB_USER:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "SMB Username", &swissSettings.smbUser, sizeof(swissSettings.smbUser));
			break;
			case SET_SMB_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "SMB Password", &swissSettings.smbPassword, sizeof(swissSettings.smbPassword));
			break;
		}
	}
	else if(page == PAGE_ADVANCED) {
		switch(option) {
			case SET_ENABLE_USBGECKODBG:
				swissSettings.debugUSB ^= 1;
			break;
			case SET_HIDE_UNK:
				swissSettings.hideUnknownFileTypes ^= 1;
			break;
			case SET_STOP_MOTOR:
				swissSettings.stopMotor ^= 1;
			break;
			case SET_WIIRDDBG:
				swissSettings.wiirdDebug ^=1;
			break;
			case SET_FILE_MGMT:
				swissSettings.enableFileManagement ^=1;
			break;
			case SET_ALL_CHEATS:
				swissSettings.autoCheats ^=1;
			break;
			case SET_ENABLE_VIDPATCH:
				swissSettings.disableVideoPatches += direction;
				if(swissSettings.disableVideoPatches > 2)
					swissSettings.disableVideoPatches = 0;
				if(swissSettings.disableVideoPatches < 0)
					swissSettings.disableVideoPatches = 2;
			break;
			case SET_FORCE_VIDACTIVE:
				if(swissSettings.disableVideoPatches < 2)
					swissSettings.forceVideoActive ^= 1;
			break;
			case SET_FORCE_DTVSTATUS:
				if(swissSettings.disableVideoPatches < 2)
					swissSettings.forceDTVStatus ^= 1;
			break;
			case SET_BS2BOOT:
				swissSettings.bs2Boot += direction;
				if(swissSettings.bs2Boot > 3)
					swissSettings.bs2Boot = 0;
				if(swissSettings.bs2Boot < 0)
					swissSettings.bs2Boot = 3;
			break;
			case SET_EMULATE_MEMCARD:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_MEMCARD))
					swissSettings.emulateMemoryCard ^= 1;
			break;
		}
	}
	else if(page == PAGE_GAME_DEFAULTS) {
		switch(option) {
			case SET_DEFAULT_FORCE_VIDEOMODE:
				if(swissSettings.disableVideoPatches < 2) {
					swissSettings.gameVMode += direction;
					if(swissSettings.gameVMode > 14)
						swissSettings.gameVMode = 0;
					if(swissSettings.gameVMode < 0)
						swissSettings.gameVMode = 14;
					if(swissSettings.aveCompat) {
						while(swissSettings.gameVMode >= 6 && swissSettings.gameVMode <= 7)
							swissSettings.gameVMode += direction;
						while(swissSettings.gameVMode >= 13 && swissSettings.gameVMode <= 14)
							swissSettings.gameVMode += direction;
					}
					if(!getDTVStatus()) {
						while(swissSettings.gameVMode >= 4 && swissSettings.gameVMode <= 7)
							swissSettings.gameVMode += direction;
						while(swissSettings.gameVMode >= 11 && swissSettings.gameVMode <= 14)
							swissSettings.gameVMode += direction;
					}
					if(swissSettings.gameVMode > 14)
						swissSettings.gameVMode = 0;
					if(swissSettings.gameVMode < 0)
						swissSettings.gameVMode = 14;
				}
			break;
			case SET_DEFAULT_HORIZ_SCALE:
				if(swissSettings.disableVideoPatches < 2) {
					swissSettings.forceHScale += direction;
					if(swissSettings.forceHScale > 8)
						swissSettings.forceHScale = 0;
					if(swissSettings.forceHScale < 0)
						swissSettings.forceHScale = 8;
				}
			break;
			case SET_DEFAULT_VERT_OFFSET:
				if(swissSettings.disableVideoPatches < 2)
					swissSettings.forceVOffset += direction;
			break;
			case SET_DEFAULT_VERT_FILTER:
				if(swissSettings.disableVideoPatches < 2) {
					swissSettings.forceVFilter += direction;
					if(swissSettings.forceVFilter > 3)
						swissSettings.forceVFilter = 0;
					if(swissSettings.forceVFilter < 0)
						swissSettings.forceVFilter = 3;
				}
			break;
			case SET_DEFAULT_ALPHA_DITHER:
				if(swissSettings.disableVideoPatches < 2)
					swissSettings.disableDithering ^= 1;
			break;
			case SET_DEFAULT_ANISO_FILTER:
				swissSettings.forceAnisotropy ^= 1;
			break;
			case SET_DEFAULT_WIDESCREEN:
				swissSettings.forceWidescreen += direction;
				if(swissSettings.forceWidescreen > 2)
					swissSettings.forceWidescreen = 0;
				if(swissSettings.forceWidescreen < 0)
					swissSettings.forceWidescreen = 2;
			break;
			case SET_DEFAULT_INVERT_CAMERA:
				swissSettings.invertCStick += direction;
				if(swissSettings.invertCStick > 3)
					swissSettings.invertCStick = 0;
				if(swissSettings.invertCStick < 0)
					swissSettings.invertCStick = 3;
			break;
			case SET_DEFAULT_TRIGGER_LEVEL:
				swissSettings.triggerLevel += direction * 10;
				if(swissSettings.triggerLevel > 200)
					swissSettings.triggerLevel = 0;
				if(swissSettings.triggerLevel < 0)
					swissSettings.triggerLevel = 200;
			break;
			case SET_DEFAULT_READ_SPEED:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED)) {
					swissSettings.emulateReadSpeed += direction;
					if(swissSettings.emulateReadSpeed > 2)
						swissSettings.emulateReadSpeed = 0;
					if(swissSettings.emulateReadSpeed < 0)
						swissSettings.emulateReadSpeed = 2;
				}
			break;
		}
	}
	else if(page == PAGE_GAME && file != NULL && gameConfig != NULL) {
		switch(option) {
			case SET_FORCE_VIDEOMODE:
				if(swissSettings.disableVideoPatches < 2) {
					gameConfig->gameVMode += direction;
					if(gameConfig->gameVMode > 14)
						gameConfig->gameVMode = 0;
					if(gameConfig->gameVMode < 0)
						gameConfig->gameVMode = 14;
					if(swissSettings.aveCompat) {
						while(gameConfig->gameVMode >= 6 && gameConfig->gameVMode <= 7)
							gameConfig->gameVMode += direction;
						while(gameConfig->gameVMode >= 13 && gameConfig->gameVMode <= 14)
							gameConfig->gameVMode += direction;
					}
					if(!getDTVStatus()) {
						while(gameConfig->gameVMode >= 4 && gameConfig->gameVMode <= 7)
							gameConfig->gameVMode += direction;
						while(gameConfig->gameVMode >= 11 && gameConfig->gameVMode <= 14)
							gameConfig->gameVMode += direction;
					}
					if(gameConfig->gameVMode > 14)
						gameConfig->gameVMode = 0;
					if(gameConfig->gameVMode < 0)
						gameConfig->gameVMode = 14;
				}
			break;
			case SET_HORIZ_SCALE:
				if(swissSettings.disableVideoPatches < 2) {
					gameConfig->forceHScale += direction;
					if(gameConfig->forceHScale > 8)
						gameConfig->forceHScale = 0;
					if(gameConfig->forceHScale < 0)
						gameConfig->forceHScale = 8;
				}
			break;
			case SET_VERT_OFFSET:
				if(swissSettings.disableVideoPatches < 2)
					gameConfig->forceVOffset += direction;
			break;
			case SET_VERT_FILTER:
				if(swissSettings.disableVideoPatches < 2) {
					gameConfig->forceVFilter += direction;
					if(gameConfig->forceVFilter > 3)
						gameConfig->forceVFilter = 0;
					if(gameConfig->forceVFilter < 0)
						gameConfig->forceVFilter = 3;
				}
			break;
			case SET_ALPHA_DITHER:
				if(swissSettings.disableVideoPatches < 2)
					gameConfig->disableDithering ^= 1;
			break;
			case SET_ANISO_FILTER:
				gameConfig->forceAnisotropy ^= 1;
			break;
			case SET_WIDESCREEN:
				gameConfig->forceWidescreen += direction;
				if(gameConfig->forceWidescreen > 2)
					gameConfig->forceWidescreen = 0;
				if(gameConfig->forceWidescreen < 0)
					gameConfig->forceWidescreen = 2;
			break;
			case SET_INVERT_CAMERA:
				gameConfig->invertCStick += direction;
				if(gameConfig->invertCStick > 3)
					gameConfig->invertCStick = 0;
				if(gameConfig->invertCStick < 0)
					gameConfig->invertCStick = 3;
			break;
			case SET_TRIGGER_LEVEL:
				gameConfig->triggerLevel += direction * 10;
				if(gameConfig->triggerLevel > 200)
					gameConfig->triggerLevel = 0;
				if(gameConfig->triggerLevel < 0)
					gameConfig->triggerLevel = 200;
			break;
			case SET_READ_SPEED:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED)) {
					gameConfig->emulateReadSpeed += direction;
					if(gameConfig->emulateReadSpeed > 2)
						gameConfig->emulateReadSpeed = 0;
					if(gameConfig->emulateReadSpeed < 0)
						gameConfig->emulateReadSpeed = 2;
				}
			break;
		}
	}
}

int show_settings(file_handle *file, ConfigEntry *config) {
	int page = PAGE_GLOBAL, option = SET_SYS_SOUND;
	
	// Copy current settings to a temp copy in case the user cancels out
	memcpy((void*)&tempSettings,(void*)&swissSettings, sizeof(SwissSettings));
	
	// Setup the settings for the current game
	if(config != NULL) {
		page = PAGE_GAME;
	}
		
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	while(1) {
		uiDrawObj_t* settingsPage = settings_draw_page(page, option, file, config);
		while (!((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_UP) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B)
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_Y)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_Y) {
			char *tooltip = get_tooltip(page, option);
			if(tooltip) {
				uiDrawObj_t* tooltipBox = DrawPublish(DrawTooltip(tooltip));
				while (PAD_ButtonsHeld(0) & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
				while (!((PAD_ButtonsHeld(0) & PAD_BUTTON_Y) || (PAD_ButtonsHeld(0) & PAD_BUTTON_B))){ VIDEO_WaitVSync (); }
				DrawDispose(tooltipBox);
			}
		}
		if(btns & PAD_BUTTON_RIGHT) {
			// If we're on a button (Back, Next, Save, Exit), allow left/right movement
			if((page == PAGE_MIN || page == PAGE_MAX) && (option >= settings_count_pp[page]-2) && option < settings_count_pp[page]) {
				option++;
			}
			else if((page != PAGE_MIN && page != PAGE_MAX) && (option >= settings_count_pp[page]-3) && option < settings_count_pp[page]) {
				option++;
			}
			else {
				settings_toggle(page, option, 1, file, config);
			}
		}
		if(btns & PAD_BUTTON_LEFT) {
			// If we're on a button (Back, Next, Save, Exit), allow left/right movement
			if((page == PAGE_MIN || page == PAGE_MAX) && (option > settings_count_pp[page]-2)) {
				option--;
			}
			else if((page != PAGE_MIN && page != PAGE_MAX) && (option > settings_count_pp[page]-3)) {
				option--;
			}
			else {
				settings_toggle(page, option, -1, file, config);
			}
		}
		if((btns & PAD_BUTTON_DOWN) && option < settings_count_pp[page])
			option++;
		if((btns & PAD_BUTTON_UP) && option > PAGE_MIN)
			option--;
		if((btns & PAD_TRIGGER_R) && page < PAGE_MAX) {
			page++; option = 0;
		}
		if((btns & PAD_TRIGGER_L) && page > PAGE_GLOBAL) {
			page--; option = 0;
		}
		if((btns & PAD_BUTTON_B))
			option = settings_count_pp[page];
		// Handle all options/buttons here
		if((btns & PAD_BUTTON_A)) {
			// Generic Save/Cancel/Back/Next button actions
			if(option == settings_count_pp[page]-1) {
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving changes ..."));
				// Change Swiss video mode if it was modified.
				if(tempSettings.uiVMode != swissSettings.uiVMode) {
					GXRModeObj *newmode = getVideoModeFromSwissSetting(swissSettings.uiVMode);
					DrawVideoMode(newmode);
				}
				// Save settings to SRAM
				if(swissSettings.uiVMode > 0) {
					swissSettings.sram60Hz = (swissSettings.uiVMode >= 1) && (swissSettings.uiVMode <= 2);
					swissSettings.sramProgressive = (swissSettings.uiVMode == 2) || (swissSettings.uiVMode == 4);
				}
				if(swissSettings.aveCompat == 1) {
					swissSettings.sramHOffset &= ~1;
				}
				VIDEO_SetAdjustingValues(swissSettings.sramHOffset, 0);
				sram = __SYS_LockSram();
				sram->display_offsetH = swissSettings.sramHOffset;
				sram->ntd = swissSettings.sram60Hz ? (sram->ntd|0x40):(sram->ntd&~0x40);
				sram->lang = swissSettings.sramLanguage;
				sram->flags = swissSettings.sramProgressive ? (sram->flags|0x80):(sram->flags&~0x80);
				sram->flags = swissSettings.sramStereo ? (sram->flags|0x04):(sram->flags&~0x04);
				sram->flags = (swissSettings.sramVideo&0x03)|(sram->flags&~0x03);
				__SYS_UnlockSram(1);
				while(!__SYS_SyncSram());
				sramex = __SYS_LockSramEx();
				sramex->__padding0 = swissSettings.configDeviceId;
				__SYS_UnlockSramEx(1);
				while(!__SYS_SyncSram());
				// Update our .ini (in memory)
				if(config != NULL) {
					config_update_game(config, true);
					DrawDispose(msgBox);
				}
				// flush settings to .ini
				if(config_update_global(true)) {
					DrawDispose(msgBox);
					msgBox = DrawPublish(DrawMessageBox(D_INFO,"Config Saved Successfully!"));
					sleep(1);
					DrawDispose(msgBox);
				}
				else {
					DrawDispose(msgBox);
					msgBox = DrawPublish(DrawMessageBox(D_INFO,"Config Failed to Save!"));
					sleep(1);
					DrawDispose(msgBox);
				}
				DrawDispose(settingsPage);
				return 1;
			}
			if(option == settings_count_pp[page]) {
				// Exit without saving (revert)
				memcpy((void*)&swissSettings, (void*)&tempSettings, sizeof(SwissSettings));
				VIDEO_SetAdjustingValues(swissSettings.sramHOffset, 0);
				DrawDispose(settingsPage);
				return 0;
			}
			if((page != PAGE_MAX) && (option == settings_count_pp[page]-2)) {
				page++; option = 0;
			}
			if((page != PAGE_MIN) && (option == settings_count_pp[page]-(page != PAGE_MAX ? 3:2))) {
				page--; option = 0;
			}
			// These use text input, allow them to be accessed with the A button
			if(page == PAGE_NETWORK && ((option >= SET_FTP_HOSTIP && option <= SET_FTP_PASS) || (option >= SET_FSP_HOSTIP && option <= SET_SMB_PASS))) {
				settings_toggle(page, option, -1, file, config);
			}
		}
		while ((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_UP) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_Y)
				|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
				|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L))
			{ VIDEO_WaitVSync (); }
		DrawDispose(settingsPage);
	}
}
