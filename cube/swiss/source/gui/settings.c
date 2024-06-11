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
#include "bba.h"
#include "sram.h"

#define page_x_ofs_key (30)
#define page_x_ofs_val (410)
#define page_y_line (25)
#define page_nav_y (390)
#define page_saveexit_y (425)
#define label_size (0.75f)

ConfigEntry tempConfig;
SwissSettings tempSettings;
char *uiVModeStr[] = {"Auto", "480i", "480p", "576i", "576p"};
char *gameVModeStr[] = {"Auto", "480i", "480sf", "240p", "960i", "480p", "1080i60", "540p60", "576i", "576sf", "288p", "1152i", "576p", "1080i50", "540p50"};
char *forceHScaleStr[] = {"Auto", "1:1", "11:10", "9:8", "640px", "656px", "672px", "704px", "720px"};
char *forceVFilterStr[] = {"Auto", "0", "1", "2"};
char *forceVJitterStr[] = {"Auto", "On", "Off"};
char *forceWidescreenStr[] = {"No", "3D", "2D+3D"};
char *forcePollRateStr[] = {"No", "VSync", "1000Hz", "500Hz", "350Hz", "300Hz", "250Hz", "200Hz", "150Hz", "150Hz", "120Hz", "120Hz", "100Hz"};
char *invertCStickStr[] = {"No", "X", "Y", "X&Y"};
char *swapCStickStr[] = {"No", "X", "Y", "X&Y"};
char *disableMCPGameIDStr[] = {"No", "Slot A", "Slot B", "Slot A&B"};
char *disableVideoPatchesStr[] = {"None", "Game", "All"};
char *emulateAudioStreamStr[] = {"Off", "Auto", "On"};
char *emulateReadSpeedStr[] = {"No", "Yes", "Wii"};
char *igrTypeStr[] = {"Disabled", "Reboot", "igr.dol"};
char *aveCompatStr[] = {"CMPV-DOL", "GCVideo", "AVE-RVL", "AVE N-DOL", "AVE P-DOL"};
char *fileBrowserStr[] = {"Standard", "Fullwidth", "Carousel"};
char *bs2BootStr[] = {"No", "Yes", "Sound 1", "Sound 2"};
char *sramLang[] = {"English", "German", "French", "Spanish", "Italian", "Dutch", "Japanese", "English (US)"};
char *recentListLevelStr[] = {"Off", "Lazy", "On"};

static char *tooltips_global[PAGE_GLOBAL_MAX+1] = {
	"System Sound:\n\nSets the default audio output type used by most games",
	"Screen Position:\n\nAdjusts the horizontal screen position in games",
	"System Language:\n\nSystem language used in games, primarily multi-5 PAL games",
	"Configuration Device:\n\nThe device that Swiss will use to load and save swiss.ini from.\nThis setting is stored in SRAM and will remain on reboot.",
	NULL,
	"File Browser Type:\n\nStandard - Displays files with minimal detail (default)\n\nCarousel - Suited towards Game/DOL only use, consider combining\nthis option with the File Management setting turned off\nand Hide Unknown File Types turned on for a better experience.",
	"File Management:\n\nWhen enabled, pressing Z on an entry in the file browser will allow it to be managed.",
	"Recent List:\n\n(On) - Press Start while browsing to show a recent list.\n(Lazy) - Same as On but list updates only for new entries.\n(Off) - Recent list is completely disabled.\n\nThe lazy/off options exist to minimise SD card writes.",
	NULL,
	"Hide unknown file types:\n\nDisabled - Show all files (default)\nEnabled - Swiss will hide unknown file types from being displayed\n\nKnown file types are:\n GameCube Executables (.bin/.dol/.elf)\n Disc images (.gcm/.iso/.nkit.iso/.tgc)\n MP3 Music (.mp3)\n WASP/WKF Flash files (.fzn)\n GameCube Memory Card Files (.gci/.gcs/.sav)\n GameCube Executables with parameters appended (.dol+cli)",
	"Flatten directory:\n\nFlattens a directory structure matching a glob pattern.",
	"Stop DVD Motor at startup:\n\nDisabled - Leave it as-is (default)\nEnabled - Stop the DVD drive from spinning when Swiss starts\n\nThis option is mostly for users booting from game\nexploits where the disc will already be spinning.",
	"SD/IDE Speed:\n\nThe clock speed to try using on the EXI bus for SD Card Adapter\nand IDE-EXI devices. 32 MHz may not work with some SD cards\nor SD card adapters.",
	"AVE Compatibility:\n\nSets the compatibility mode for the used audio/video encoder.\n\nAVE N-DOL - Output PAL as NTSC 50\nAVE P-DOL - Disable progressive scan mode\nCMPV-DOL - Enable 1080i & 540p\nGCVideo - Apply general workarounds for GCVideo (default)\nAVE-RVL - Support 960i & 1152i without WiiVideo",
	"Force DTV Status:\n\nDisabled - Use detect signal from the Digital AV Out (default)\nEnabled - Force detection in the case of a hardware fault",
	"Optimise for RetroTINK-4K:\n\nRequires GCVideo-DVI v3.0 or later with Fix Resolution Off.",
	"Enable USB Gecko debug output:\n\nIf a USB Gecko is present in slot B, debug output from\nSwiss & in game (if the game supported output over OSReport)\nwill be output. If nothing is reading the data out from the\ndevice it may cause Swiss/games to hang."
};

static char *tooltips_network[PAGE_NETWORK_MAX+1] = {
	"Init network at startup:\n\nDisabled - Do not initialise the BBA even if present (default)\nEnabled - If a BBA is present, it will be initialised at startup\n\nIf initialised, navigate to the IP in a web browser to backup various data"
};

static char *tooltips_game_global[PAGE_GAME_GLOBAL_MAX+1] = {
	"In-Game Reset: (A + Z + Start)\n\nReboot: Soft-Reset the GameCube\nigr.dol: Low mem (< 0x81300000) igr.dol at the root of SD Card",
	"Boot through IPL:\n\nWhen enabled, games will be booted with the GameCube\nlogo screen and Main Menu accessible with patches applied.",
	NULL,
	NULL,
	NULL,
	"Force Video Active:\n\nA workaround for GCVideo-DVI v3.0 series, obsoleted by 3.1.",
	NULL,
	"Pause for resolution change:\n\nWhen enabled, a change in active video resolution will pause\nthe game for 2 seconds.",
	"Auto-load all cheats:\n\nIf enabled, and a cheats file for a particular game is found\ne.g. /swiss/cheats/GPOP8D.txt (on a compatible device)\nthen all cheats in the file will be enabled",
	"WiiRD debugging:\n\nDisabled - Boot as normal (default)\nEnabled - This will start a game with the WiiRD debugger enabled & paused\n\nThe WiiRD debugger takes up more memory and can cause issues."
};

static char *tooltips_game[PAGE_GAME_MAX+1] = {
	NULL,
	NULL,
	"Force Vertical Offset:\n\n+0 - Standard value\n-2 - GCVideo-DVI compatible (480i)\n-3 - GCVideo-DVI compatible (default)\n-4 - GCVideo-DVI compatible (240p)\n-12 - Datapath VisionRGB (480p)",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"Force Polling Rate:\n\nVSync - Highest compatibility\n1000Hz - Lowest input latency",
	"Invert Camera Stick:\n\nNo - Leave C Stick as-is (default)\nX - Invert X-axis of the C Stick\nY - Invert Y-axis of the C Stick\nX&Y - Invert both axes of the C Stick",
	"Swap Camera Stick:\n\nNo - Leave C Stick as-is (default)\nX - Swap X-axis of the C Stick with the Control Stick\nY - Swap Y-axis of the C Stick with the Control Stick\nX&Y - Swap both axes of the C Stick with the Control Stick",
	"Digital Trigger Level:\n\nSets the threshold where the L/R Button is fully pressed.",
	"Emulate Audio Streaming:\n\nAudio streaming is a hardware feature that allows a compressed\naudio track to be played in the background by the disc drive.\n\nEmulation is necessary for devices not attached to the\nDVD Interface, or for those not implementing it regardless.",
	"Emulate Read Speed:\n\nNo - Start transfer immediately (default)\nYes - Delay transfer to simulate the GameCube disc drive\nWii - Delay transfer to simulate the Wii disc drive\n\nThis is necessary to avoid programming mistakes obfuscated\nby the original medium, or for speedrunning.",
	"Emulate Broadband Adapter:\n\nOnly available with the File Service Protocol or an initialised\nETH2GC module, where memory constraints permit.\n\nPackets not destined for the hypervisor are forwarded to\nthe virtual MAC. The virtual MAC address is the same as\nthe physical MAC. The physical MAC/PHY retain their\nconfiguration from Swiss, including link speed.",
	"Prefer Clean Boot:\n\nWhen enabled, the GameCube will be reset and the game\nbooted through normal processes with no changes applied.\nRegion restrictions may be applicable.\n\nOnly available to devices attached to the DVD Interface."
};

// Number of settings (including Back, Next, Save, Exit buttons) per page
int settings_count_pp[5] = {PAGE_GLOBAL_MAX, PAGE_NETWORK_MAX, PAGE_GAME_GLOBAL_MAX, PAGE_GAME_DEFAULTS_MAX, PAGE_GAME_MAX};

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
	else if(page_num == PAGE_GAME_GLOBAL) {
		textPtr = tooltips_game_global[option];
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

void drawSettingEntryString(uiDrawObj_t* page, int *y, char *label, char *key, bool selected, bool enabled) {
	if(selected) {
		DrawAddChild(page, DrawStyledLabel(20, *y, "\225", label_size, false, enabled ? defaultColor:deSelectedColor));
	}
	DrawAddChild(page, DrawStyledLabel(page_x_ofs_key, *y, label, label_size, false, enabled ? defaultColor:deSelectedColor));
	DrawAddChild(page, DrawStyledLabel(page_x_ofs_val, *y, key, label_size, false, enabled && selected ? defaultColor:deSelectedColor));
	*y += page_y_line; 
}

void drawSettingEntryBoolean(uiDrawObj_t* page, int *y, char *label, bool boolval, bool selected, bool enabled) {
	drawSettingEntryString(page, y, label, boolval ? "Yes" : "No", selected, enabled);
}

void drawSettingEntryNumeric(uiDrawObj_t* page, int *y, char *label, int num, bool selected, bool enabled) {
	sprintf(txtbuffer, "%i", num);
	drawSettingEntryString(page, y, label, txtbuffer, selected, enabled);
}

uiDrawObj_t* settings_draw_page(int page_num, int option, ConfigEntry *gameConfig) {
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
		int settings_per_page = 10;
		int scrollBarHeight = 90+(settings_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)SET_PAGE_1_NEXT);
		DrawAddChild(page, DrawVertScrollBar(getVideoMode()->fbWidth-45, page_y_ofs, 25, scrollBarHeight, (float)((float)option/(float)(SET_PAGE_1_NEXT-1)),scrollBarTabHeight));
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Global Settings (1/5):"));
		bool dtvEnable = swissSettings.aveCompat < 3;
		bool rt4kEnable = swissSettings.aveCompat == 1;
		bool dbgEnable = devices[DEVICE_CUR] != &__device_usbgecko && deviceHandler_getDeviceAvailable(&__device_usbgecko);
		// TODO settings to a new typedef that ties type etc all together, then draw a "page" of these rather than this at some point.
		if(option < SET_STOP_MOTOR) {
			drawSettingEntryString(page, &page_y_ofs, "System Sound:", swissSettings.sramStereo ? "Stereo":"Mono", option == SET_SYS_SOUND, true);
			sprintf(sramHOffsetStr, "%+hi", swissSettings.sramHOffset);
			drawSettingEntryString(page, &page_y_ofs, "Screen Position:", sramHOffsetStr, option == SET_SCREEN_POS, true);
			drawSettingEntryString(page, &page_y_ofs, "System Language:", swissSettings.sramLanguage > SRAM_LANG_MAX ? "Unknown" : sramLang[swissSettings.sramLanguage], option == SET_SYS_LANG, true);
			drawSettingEntryString(page, &page_y_ofs, "Configuration Device:", getConfigDeviceName(&swissSettings), option == SET_CONFIG_DEV, true);
			drawSettingEntryString(page, &page_y_ofs, "Swiss Video Mode:", uiVModeStr[swissSettings.uiVMode], option == SET_SWISS_VIDEOMODE, true);
			drawSettingEntryString(page, &page_y_ofs, "File Browser Type:", fileBrowserStr[swissSettings.fileBrowserType], option == SET_FILEBROWSER_TYPE, true);
			drawSettingEntryBoolean(page, &page_y_ofs, "File Management:", swissSettings.enableFileManagement, option == SET_FILE_MGMT, true);
			drawSettingEntryString(page, &page_y_ofs, "Recent List:", recentListLevelStr[swissSettings.recentListLevel], option == SET_RECENT_LIST, true);
			drawSettingEntryBoolean(page, &page_y_ofs, "Show hidden files:", swissSettings.showHiddenFiles, option == SET_SHOW_HIDDEN, true);
			drawSettingEntryBoolean(page, &page_y_ofs, "Hide unknown file types:", swissSettings.hideUnknownFileTypes, option == SET_HIDE_UNK, true);
			drawSettingEntryString(page, &page_y_ofs, "Flatten directory:", swissSettings.flattenDir, option == SET_FLATTEN_DIR, true);
		} else {
			drawSettingEntryBoolean(page, &page_y_ofs, "Stop DVD Motor at startup:", swissSettings.stopMotor, option == SET_STOP_MOTOR, true);
			drawSettingEntryString(page, &page_y_ofs, "SD/IDE Speed:", swissSettings.exiSpeed ? "32 MHz":"16 MHz", option == SET_EXI_SPEED, true);
			drawSettingEntryString(page, &page_y_ofs, "AVE Compatibility:", aveCompatStr[swissSettings.aveCompat], option == SET_AVE_COMPAT, true);
			drawSettingEntryBoolean(page, &page_y_ofs, "Force DTV Status:", swissSettings.forceDTVStatus, option == SET_FORCE_DTVSTATUS, dtvEnable);
			drawSettingEntryBoolean(page, &page_y_ofs, "Optimise for RetroTINK-4K:", swissSettings.rt4kOptim, option == SET_RT4K_OPTIM, rt4kEnable);
			drawSettingEntryBoolean(page, &page_y_ofs, "USB Gecko debug output:", swissSettings.debugUSB, option == SET_ENABLE_USBGECKODBG, dbgEnable);
		}
	}
	else if(page_num == PAGE_NETWORK) {
		int settings_per_page = 10;
		int scrollBarHeight = 90+(settings_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)SET_PAGE_2_BACK);
		DrawAddChild(page, DrawVertScrollBar(getVideoMode()->fbWidth-45, page_y_ofs, 25, scrollBarHeight, (float)((float)option/(float)(SET_PAGE_2_BACK-1)),scrollBarTabHeight));
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Network Settings (2/5):"));
		bool netEnable = net_initialized || bba_exists(LOC_ANY);
		// TODO settings to a new typedef that ties type etc all together, then draw a "page" of these rather than this at some point.
		if(option < SET_FTP_USER) {
			drawSettingEntryBoolean(page, &page_y_ofs, "Init network at startup:", swissSettings.initNetworkAtStart, option == SET_INIT_NET, true);
			drawSettingEntryString(page, &page_y_ofs, "IPv4 Address:", swissSettings.bbaLocalIp, option == SET_BBA_LOCALIP, netEnable);
			drawSettingEntryNumeric(page, &page_y_ofs, "IPv4 Netmask:", swissSettings.bbaNetmask, option == SET_BBA_NETMASK, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "IPv4 Gateway:", swissSettings.bbaGateway, option == SET_BBA_GATEWAY, netEnable);
			drawSettingEntryBoolean(page, &page_y_ofs, "IPv4 uses DHCP:", swissSettings.bbaUseDhcp, option == SET_BBA_DHCP, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FSP Host IP:", swissSettings.fspHostIp, option == SET_FSP_HOSTIP, netEnable);
			drawSettingEntryNumeric(page, &page_y_ofs, "FSP Port:", swissSettings.fspPort, option == SET_FSP_PORT, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FSP Password:", "*****", option == SET_FSP_PASS, netEnable);
			drawSettingEntryNumeric(page, &page_y_ofs, "FSP Path MTU:", swissSettings.fspPathMtu, option == SET_FSP_PMTU, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FTP Host IP:", swissSettings.ftpHostIp, option == SET_FTP_HOSTIP, netEnable);
			drawSettingEntryNumeric(page, &page_y_ofs, "FTP Port:", swissSettings.ftpPort, option == SET_FTP_PORT, netEnable);
		} else {
			drawSettingEntryString(page, &page_y_ofs, "FTP Username:", swissSettings.ftpUserName, option == SET_FTP_USER, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "FTP Password:", "*****", option == SET_FTP_PASS, netEnable);
			drawSettingEntryBoolean(page, &page_y_ofs, "FTP PASV Mode:", swissSettings.ftpUsePasv, option == SET_FTP_PASV, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "SMB Host IP:", swissSettings.smbServerIp, option == SET_SMB_HOSTIP, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "SMB Share:", swissSettings.smbShare, option == SET_SMB_SHARE, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "SMB Username:", swissSettings.smbUser, option == SET_SMB_USER, netEnable);
			drawSettingEntryString(page, &page_y_ofs, "SMB Password:", "*****", option == SET_SMB_PASS, netEnable);
		}
	}
	else if(page_num == PAGE_GAME_GLOBAL) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Global Game Settings (3/5):"));
		bool enabledVideoPatches = swissSettings.disableVideoPatches < 2;
		bool emulatedMemoryCard = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_MEMCARD);
		bool dbgEnable = devices[DEVICE_CUR] != &__device_usbgecko && deviceHandler_getDeviceAvailable(&__device_usbgecko);
		drawSettingEntryString(page, &page_y_ofs, "In-Game Reset:", igrTypeStr[swissSettings.igrType], option == SET_IGR, true);
		drawSettingEntryString(page, &page_y_ofs, "Boot through IPL:", bs2BootStr[swissSettings.bs2Boot], option == SET_BS2BOOT, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Boot without prompts:", swissSettings.autoBoot, option == SET_AUTOBOOT, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Memory Card:", swissSettings.emulateMemoryCard, option == SET_EMULATE_MEMCARD, emulatedMemoryCard);
		drawSettingEntryString(page, &page_y_ofs, "Disable MemCard PRO GameID:", disableMCPGameIDStr[swissSettings.disableMCPGameID], option == SET_ENABLE_MCPGAMEID, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Force Video Active:", swissSettings.forceVideoActive, option == SET_FORCE_VIDACTIVE, enabledVideoPatches);
		drawSettingEntryString(page, &page_y_ofs, "Disable Video Patches:", disableVideoPatchesStr[swissSettings.disableVideoPatches], option == SET_ENABLE_VIDPATCH, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Pause for resolution change:", swissSettings.pauseAVOutput, option == SET_PAUSE_AVOUTPUT, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Auto-load all cheats:", swissSettings.autoCheats, option == SET_ALL_CHEATS, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "WiiRD debugging:", swissSettings.wiirdDebug, option == SET_WIIRDDBG, dbgEnable);
	}
	else if(page_num == PAGE_GAME_DEFAULTS) {
		int settings_per_page = 10;
		int scrollBarHeight = 90+(settings_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)SET_PAGE_4_BACK);
		DrawAddChild(page, DrawVertScrollBar(getVideoMode()->fbWidth-45, page_y_ofs, 25, scrollBarHeight, (float)((float)option/(float)(SET_PAGE_4_BACK-1)),scrollBarTabHeight));
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Default Game Settings (4/5):"));
		bool enabledVideoPatches = swissSettings.disableVideoPatches < 2;
		bool emulatedAudioStream = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_AUDIO_STREAMING);
		bool emulatedReadSpeed = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED);
		bool emulatedEthernet = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_ETHERNET);
		bool enabledCleanBoot = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR);
		if(option < SET_DEFAULT_TRIGGER_LEVEL) {
			drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[swissSettings.gameVMode], option == SET_DEFAULT_FORCE_VIDEOMODE, enabledVideoPatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[swissSettings.forceHScale], option == SET_DEFAULT_HORIZ_SCALE, enabledVideoPatches);
			sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_DEFAULT_VERT_OFFSET, enabledVideoPatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[swissSettings.forceVFilter], option == SET_DEFAULT_VERT_FILTER, enabledVideoPatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Field Rendering:", forceVJitterStr[swissSettings.forceVJitter], option == SET_FIELD_RENDER, enabledVideoPatches);
			drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", swissSettings.disableDithering, option == SET_DEFAULT_ALPHA_DITHER, enabledVideoPatches);
			drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", swissSettings.forceAnisotropy, option == SET_DEFAULT_ANISO_FILTER, true);
			drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[swissSettings.forceWidescreen], option == SET_DEFAULT_WIDESCREEN, true);
			drawSettingEntryString(page, &page_y_ofs, "Force Polling Rate:", forcePollRateStr[swissSettings.forcePollRate], option == SET_DEFAULT_POLL_RATE, true);
			drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[swissSettings.invertCStick], option == SET_DEFAULT_INVERT_CAMERA, true);
			drawSettingEntryString(page, &page_y_ofs, "Swap Camera Stick:", swapCStickStr[swissSettings.swapCStick], option == SET_DEFAULT_SWAP_CAMERA, true);
		} else {
			sprintf(triggerLevelStr, "%hhu", swissSettings.triggerLevel);
			drawSettingEntryString(page, &page_y_ofs, "Digital Trigger Level:", triggerLevelStr, option == SET_DEFAULT_TRIGGER_LEVEL, true);
			drawSettingEntryString(page, &page_y_ofs, "Emulate Audio Streaming:", emulateAudioStreamStr[swissSettings.emulateAudioStream], option == SET_DEFAULT_AUDIO_STREAM, emulatedAudioStream);
			drawSettingEntryString(page, &page_y_ofs, "Emulate Read Speed:", emulateReadSpeedStr[swissSettings.emulateReadSpeed], option == SET_DEFAULT_READ_SPEED, emulatedReadSpeed);
			drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Broadband Adapter:", swissSettings.emulateEthernet, option == SET_DEFAULT_EMULATE_ETHERNET, emulatedEthernet);
			drawSettingEntryBoolean(page, &page_y_ofs, "Prefer Clean Boot:", swissSettings.preferCleanBoot, option == SET_DEFAULT_CLEAN_BOOT, enabledCleanBoot);
		}
	}
	else if(page_num == PAGE_GAME) {
		int settings_per_page = 10;
		int scrollBarHeight = 90+(settings_per_page*20);
		int scrollBarTabHeight = (int)((float)scrollBarHeight/(float)SET_PAGE_5_BACK);
		DrawAddChild(page, DrawVertScrollBar(getVideoMode()->fbWidth-45, page_y_ofs, 25, scrollBarHeight, (float)((float)option/(float)(SET_PAGE_5_BACK-1)),scrollBarTabHeight));
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Current Game Settings (5/5):"));
		bool enabledGamePatches = gameConfig != NULL && !gameConfig->forceCleanBoot;
		if(enabledGamePatches) {
			bool enabledVideoPatches = swissSettings.disableVideoPatches < 2;
			bool emulatedAudioStream = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_AUDIO_STREAMING);
			bool emulatedReadSpeed = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED);
			bool emulatedEthernet = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_ETHERNET);
			bool enabledCleanBoot = devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR);
			if(option < SET_TRIGGER_LEVEL) {
				drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[gameConfig->gameVMode], option == SET_FORCE_VIDEOMODE, enabledVideoPatches);
				drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[gameConfig->forceHScale], option == SET_HORIZ_SCALE, enabledVideoPatches);
				sprintf(forceVOffsetStr, "%+hi", gameConfig->forceVOffset);
				drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_VERT_OFFSET, enabledVideoPatches);
				drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[gameConfig->forceVFilter], option == SET_VERT_FILTER, enabledVideoPatches);
				drawSettingEntryString(page, &page_y_ofs, "Force Field Rendering:", forceVJitterStr[gameConfig->forceVJitter], option == SET_FIELD_RENDER, enabledVideoPatches);
				drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", gameConfig->disableDithering, option == SET_ALPHA_DITHER, enabledVideoPatches);
				drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", gameConfig->forceAnisotropy, option == SET_ANISO_FILTER, true);
				drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[gameConfig->forceWidescreen], option == SET_WIDESCREEN, true);
				drawSettingEntryString(page, &page_y_ofs, "Force Polling Rate:", forcePollRateStr[gameConfig->forcePollRate], option == SET_POLL_RATE, true);
				drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[gameConfig->invertCStick], option == SET_INVERT_CAMERA, true);
				drawSettingEntryString(page, &page_y_ofs, "Swap Camera Stick:", swapCStickStr[gameConfig->swapCStick], option == SET_SWAP_CAMERA, true);
			} else {
				sprintf(triggerLevelStr, "%hhu", gameConfig->triggerLevel);
				drawSettingEntryString(page, &page_y_ofs, "Digital Trigger Level:", triggerLevelStr, option == SET_TRIGGER_LEVEL, true);
				drawSettingEntryString(page, &page_y_ofs, "Emulate Audio Streaming:", emulateAudioStreamStr[gameConfig->emulateAudioStream], option == SET_AUDIO_STREAM, emulatedAudioStream);
				drawSettingEntryString(page, &page_y_ofs, "Emulate Read Speed:", emulateReadSpeedStr[gameConfig->emulateReadSpeed], option == SET_READ_SPEED, emulatedReadSpeed);
				drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Broadband Adapter:", gameConfig->emulateEthernet, option == SET_EMULATE_ETHERNET, emulatedEthernet);
				drawSettingEntryBoolean(page, &page_y_ofs, "Prefer Clean Boot:", gameConfig->preferCleanBoot, option == SET_CLEAN_BOOT, enabledCleanBoot);
				drawSettingEntryString(page, &page_y_ofs, "Reset to defaults", NULL, option == SET_DEFAULTS, true);
			}
		}
		else {
			// Just draw the defaults again
			if(option < SET_TRIGGER_LEVEL) {
				drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[swissSettings.gameVMode], option == SET_FORCE_VIDEOMODE, false);
				drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[swissSettings.forceHScale], option == SET_HORIZ_SCALE, false);
				sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
				drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_VERT_OFFSET, false);
				drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[swissSettings.forceVFilter], option == SET_VERT_FILTER, false);
				drawSettingEntryString(page, &page_y_ofs, "Force Field Rendering:", forceVJitterStr[swissSettings.forceVJitter], option == SET_FIELD_RENDER, false);
				drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", swissSettings.disableDithering, option == SET_ALPHA_DITHER, false);
				drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", swissSettings.forceAnisotropy, option == SET_ANISO_FILTER, false);
				drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[swissSettings.forceWidescreen], option == SET_WIDESCREEN, false);
				drawSettingEntryString(page, &page_y_ofs, "Force Polling Rate:", forcePollRateStr[swissSettings.forcePollRate], option == SET_POLL_RATE, false);
				drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[swissSettings.invertCStick], option == SET_INVERT_CAMERA, false);
				drawSettingEntryString(page, &page_y_ofs, "Swap Camera Stick:", swapCStickStr[swissSettings.swapCStick], option == SET_SWAP_CAMERA, false);
			} else {
				sprintf(triggerLevelStr, "%hhu", swissSettings.triggerLevel);
				drawSettingEntryString(page, &page_y_ofs, "Digital Trigger Level:", triggerLevelStr, option == SET_TRIGGER_LEVEL, false);
				drawSettingEntryString(page, &page_y_ofs, "Emulate Audio Streaming:", emulateAudioStreamStr[swissSettings.emulateAudioStream], option == SET_AUDIO_STREAM, false);
				drawSettingEntryString(page, &page_y_ofs, "Emulate Read Speed:", emulateReadSpeedStr[swissSettings.emulateReadSpeed], option == SET_READ_SPEED, false);
				drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Broadband Adapter:", swissSettings.emulateEthernet, option == SET_EMULATE_ETHERNET, false);
				drawSettingEntryBoolean(page, &page_y_ofs, "Prefer Clean Boot:", swissSettings.preferCleanBoot, option == SET_CLEAN_BOOT, false);
				drawSettingEntryString(page, &page_y_ofs, "Reset to defaults", NULL, option == SET_DEFAULTS, false);
			}
		}
	}
	// If we have a tooltip for this page/option, add a fading label telling the user to press Y for help
	add_tooltip_label(page, page_num, option);
	
	DrawPublish(page);
	return page;
}

void settings_toggle(int page, int option, int direction, ConfigEntry *gameConfig) {
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
						curDevicePos = (curDevicePos + MAX_DEVICES) % MAX_DEVICES;
					}
					if(allDevices[curDevicePos] != NULL) {
						swissSettings.configDeviceId = allDevices[curDevicePos]->deviceUniqueId;
					}
				}
			}
			break;
			case SET_SWISS_VIDEOMODE:
				swissSettings.uiVMode += direction;
				swissSettings.uiVMode = (swissSettings.uiVMode + 5) % 5;
			break;
			case SET_FILEBROWSER_TYPE:
				swissSettings.fileBrowserType += direction;
				swissSettings.fileBrowserType = (swissSettings.fileBrowserType + 3) % 3;
			break;
			case SET_FILE_MGMT:
				swissSettings.enableFileManagement ^=1;
			break;
			case SET_RECENT_LIST:
				swissSettings.recentListLevel += direction;
				swissSettings.recentListLevel = (swissSettings.recentListLevel + 3) % 3;
			break;
			case SET_SHOW_HIDDEN:
				swissSettings.showHiddenFiles ^= 1;
			break;
			case SET_HIDE_UNK:
				swissSettings.hideUnknownFileTypes ^= 1;
			break;
			case SET_FLATTEN_DIR:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "Flatten directory", &swissSettings.flattenDir, sizeof(swissSettings.flattenDir) - 1);
			break;
			case SET_STOP_MOTOR:
				swissSettings.stopMotor ^= 1;
			break;
			case SET_EXI_SPEED:
				swissSettings.exiSpeed ^= 1;
			break;
			case SET_AVE_COMPAT:
				swissSettings.aveCompat += direction;
				swissSettings.aveCompat = (swissSettings.aveCompat + 5) % 5;
			break;
			case SET_FORCE_DTVSTATUS:
				if(swissSettings.aveCompat < 3)
					swissSettings.forceDTVStatus ^= 1;
			break;
			case SET_RT4K_OPTIM:
				if(swissSettings.aveCompat == 1)
					swissSettings.rt4kOptim ^= 1;
			break;
			case SET_ENABLE_USBGECKODBG:
				if(devices[DEVICE_CUR] != &__device_usbgecko && deviceHandler_getDeviceAvailable(&__device_usbgecko))
					swissSettings.debugUSB ^= 1;
			break;
		}
		switch(option) {
			case SET_SWISS_VIDEOMODE:
			case SET_AVE_COMPAT:
			case SET_FORCE_DTVSTATUS:
			case SET_RT4K_OPTIM:
			{
				// Change Swiss video mode if it was modified.
				GXRModeObj *forcedMode = getVideoModeFromSwissSetting(swissSettings.uiVMode);
				DrawVideoMode(forcedMode);
			}
			break;
		}
	}
	else if(page == PAGE_NETWORK) {
		switch(option) {
			case SET_INIT_NET:
				swissSettings.initNetworkAtStart ^= 1;
			break;
			case SET_BBA_LOCALIP:
				DrawGetTextEntry(ENTRYMODE_IP, "IPv4 Address", &swissSettings.bbaLocalIp, sizeof(swissSettings.bbaLocalIp) - 1);
			break;
			case SET_BBA_NETMASK:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "IPv4 Netmask", &swissSettings.bbaNetmask, 2);
			break;
			case SET_BBA_GATEWAY:
				DrawGetTextEntry(ENTRYMODE_IP, "IPv4 Gateway", &swissSettings.bbaGateway, sizeof(swissSettings.bbaGateway) - 1);
			break;
			case SET_BBA_DHCP:
				swissSettings.bbaUseDhcp ^= 1;
			break;
			case SET_FSP_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "FSP Host IP", &swissSettings.fspHostIp, sizeof(swissSettings.fspHostIp) - 1);
			break;
			case SET_FSP_PORT:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "FSP Port", &swissSettings.fspPort, 5);
			break;
			case SET_FSP_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "FSP Password", &swissSettings.fspPassword, sizeof(swissSettings.fspPassword) - 1);
			break;
			case SET_FSP_PMTU:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "FSP Path MTU", &swissSettings.fspPathMtu, 4);
			break;
			case SET_FTP_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "FTP Host IP", &swissSettings.ftpHostIp, sizeof(swissSettings.ftpHostIp) - 1);
			break;
			case SET_FTP_PORT:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "FTP Port", &swissSettings.ftpPort, 5);
			break;
			case SET_FTP_USER:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "FTP Username", &swissSettings.ftpUserName, sizeof(swissSettings.ftpUserName) - 1);
			break;
			case SET_FTP_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "FTP Password", &swissSettings.ftpPassword, sizeof(swissSettings.ftpPassword) - 1);
			break;
			case SET_FTP_PASV:
				swissSettings.ftpUsePasv ^= 1;
			break;
			case SET_SMB_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "SMB Host IP", &swissSettings.smbServerIp, sizeof(swissSettings.smbServerIp) - 1);
			break;
			case SET_SMB_SHARE:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "SMB Share", &swissSettings.smbShare, sizeof(swissSettings.smbShare) - 1);
			break;
			case SET_SMB_USER:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "SMB Username", &swissSettings.smbUser, sizeof(swissSettings.smbUser) - 1);
			break;
			case SET_SMB_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "SMB Password", &swissSettings.smbPassword, sizeof(swissSettings.smbPassword) - 1);
			break;
		}
	}
	else if(page == PAGE_GAME_GLOBAL) {
		switch(option) {
			case SET_IGR:
				swissSettings.igrType += direction;
				swissSettings.igrType = (swissSettings.igrType + 3) % 3;
			break;
			case SET_BS2BOOT:
				swissSettings.bs2Boot += direction;
				swissSettings.bs2Boot = (swissSettings.bs2Boot + 4) % 4;
			break;
			case SET_AUTOBOOT:
				swissSettings.autoBoot ^=1;
			break;
			case SET_EMULATE_MEMCARD:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_MEMCARD))
					swissSettings.emulateMemoryCard ^= 1;
			break;
			case SET_ENABLE_MCPGAMEID:
				swissSettings.disableMCPGameID += direction;
				swissSettings.disableMCPGameID = (swissSettings.disableMCPGameID + 4) % 4;
			break;
			case SET_FORCE_VIDACTIVE:
				if(swissSettings.disableVideoPatches < 2)
					swissSettings.forceVideoActive ^= 1;
			break;
			case SET_ENABLE_VIDPATCH:
				swissSettings.disableVideoPatches += direction;
				swissSettings.disableVideoPatches = (swissSettings.disableVideoPatches + 3) % 3;
			break;
			case SET_PAUSE_AVOUTPUT:
				swissSettings.pauseAVOutput ^=1;
			break;
			case SET_ALL_CHEATS:
				swissSettings.autoCheats ^=1;
			break;
			case SET_WIIRDDBG:
				if(devices[DEVICE_CUR] != &__device_usbgecko && deviceHandler_getDeviceAvailable(&__device_usbgecko))
					swissSettings.wiirdDebug ^=1;
			break;
		}
	}
	else if(page == PAGE_GAME_DEFAULTS) {
		switch(option) {
			case SET_DEFAULT_FORCE_VIDEOMODE:
				if(swissSettings.disableVideoPatches < 2) {
					swissSettings.gameVMode += direction;
					swissSettings.gameVMode = (swissSettings.gameVMode + 15) % 15;
					if(!getDTVStatus()) {
						while(in_range(swissSettings.gameVMode, 4, 7) || in_range(swissSettings.gameVMode, 11, 14)) {
							swissSettings.gameVMode += direction;
							swissSettings.gameVMode = (swissSettings.gameVMode + 15) % 15;
						}
					}
					else if(swissSettings.aveCompat != 0) {
						while(in_range(swissSettings.gameVMode, 6, 7) || in_range(swissSettings.gameVMode, 13, 14)) {
							swissSettings.gameVMode += direction;
							swissSettings.gameVMode = (swissSettings.gameVMode + 15) % 15;
						}
					}
				}
			break;
			case SET_DEFAULT_HORIZ_SCALE:
				if(swissSettings.disableVideoPatches < 2) {
					swissSettings.forceHScale += direction;
					swissSettings.forceHScale = (swissSettings.forceHScale + 9) % 9;
				}
			break;
			case SET_DEFAULT_VERT_OFFSET:
				if(swissSettings.disableVideoPatches < 2)
					swissSettings.forceVOffset += direction;
			break;
			case SET_DEFAULT_VERT_FILTER:
				if(swissSettings.disableVideoPatches < 2) {
					swissSettings.forceVFilter += direction;
					swissSettings.forceVFilter = (swissSettings.forceVFilter + 4) % 4;
				}
			break;
			case SET_DEFAULT_FIELD_RENDER:
				if(swissSettings.disableVideoPatches < 2) {
					swissSettings.forceVJitter += direction;
					swissSettings.forceVJitter = (swissSettings.forceVJitter + 3) % 3;
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
				swissSettings.forceWidescreen = (swissSettings.forceWidescreen + 3) % 3;
			break;
			case SET_DEFAULT_POLL_RATE:
				swissSettings.forcePollRate += direction;
				swissSettings.forcePollRate = (swissSettings.forcePollRate + 13) % 13;
			break;
			case SET_DEFAULT_INVERT_CAMERA:
				swissSettings.invertCStick += direction;
				swissSettings.invertCStick = (swissSettings.invertCStick + 4) % 4;
			break;
			case SET_DEFAULT_SWAP_CAMERA:
				swissSettings.swapCStick += direction;
				swissSettings.swapCStick = (swissSettings.swapCStick + 4) % 4;
			break;
			case SET_DEFAULT_TRIGGER_LEVEL:
				swissSettings.triggerLevel += direction * 10;
				swissSettings.triggerLevel = (swissSettings.triggerLevel + 210) % 210;
			break;
			case SET_DEFAULT_AUDIO_STREAM:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_AUDIO_STREAMING)) {
					swissSettings.emulateAudioStream += direction;
					swissSettings.emulateAudioStream = (swissSettings.emulateAudioStream + 3) % 3;
				}
			break;
			case SET_DEFAULT_READ_SPEED:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED)) {
					swissSettings.emulateReadSpeed += direction;
					swissSettings.emulateReadSpeed = (swissSettings.emulateReadSpeed + 3) % 3;
				}
			break;
			case SET_DEFAULT_EMULATE_ETHERNET:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_ETHERNET))
					swissSettings.emulateEthernet ^= 1;
			break;
			case SET_DEFAULT_CLEAN_BOOT:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR))
					swissSettings.preferCleanBoot ^= 1;
			break;
		}
	}
	else if(page == PAGE_GAME && gameConfig != NULL && !gameConfig->forceCleanBoot) {
		switch(option) {
			case SET_FORCE_VIDEOMODE:
				if(swissSettings.disableVideoPatches < 2) {
					gameConfig->gameVMode += direction;
					gameConfig->gameVMode = (gameConfig->gameVMode + 15) % 15;
					if(!getDTVStatus()) {
						while(in_range(gameConfig->gameVMode, 4, 7) || in_range(gameConfig->gameVMode, 11, 14)) {
							gameConfig->gameVMode += direction;
							gameConfig->gameVMode = (gameConfig->gameVMode + 15) % 15;
						}
					}
					else if(swissSettings.aveCompat != 0) {
						while(in_range(gameConfig->gameVMode, 6, 7) || in_range(gameConfig->gameVMode, 13, 14)) {
							gameConfig->gameVMode += direction;
							gameConfig->gameVMode = (gameConfig->gameVMode + 15) % 15;
						}
					}
				}
			break;
			case SET_HORIZ_SCALE:
				if(swissSettings.disableVideoPatches < 2) {
					gameConfig->forceHScale += direction;
					gameConfig->forceHScale = (gameConfig->forceHScale + 9) % 9;
				}
			break;
			case SET_VERT_OFFSET:
				if(swissSettings.disableVideoPatches < 2)
					gameConfig->forceVOffset += direction;
			break;
			case SET_VERT_FILTER:
				if(swissSettings.disableVideoPatches < 2) {
					gameConfig->forceVFilter += direction;
					gameConfig->forceVFilter = (gameConfig->forceVFilter + 4) % 4;
				}
			break;
			case SET_FIELD_RENDER:
				if(swissSettings.disableVideoPatches < 2) {
					gameConfig->forceVJitter += direction;
					gameConfig->forceVJitter = (gameConfig->forceVJitter + 3) % 3;
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
				gameConfig->forceWidescreen = (gameConfig->forceWidescreen + 3) % 3;
			break;
			case SET_POLL_RATE:
				gameConfig->forcePollRate += direction;
				gameConfig->forcePollRate = (gameConfig->forcePollRate + 13) % 13;
			break;
			case SET_INVERT_CAMERA:
				gameConfig->invertCStick += direction;
				gameConfig->invertCStick = (gameConfig->invertCStick + 4) % 4;
			break;
			case SET_SWAP_CAMERA:
				gameConfig->swapCStick += direction;
				gameConfig->swapCStick = (gameConfig->swapCStick + 4) % 4;
			break;
			case SET_TRIGGER_LEVEL:
				gameConfig->triggerLevel += direction * 10;
				gameConfig->triggerLevel = (gameConfig->triggerLevel + 210) % 210;
			break;
			case SET_AUDIO_STREAM:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_AUDIO_STREAMING)) {
					gameConfig->emulateAudioStream += direction;
					gameConfig->emulateAudioStream = (gameConfig->emulateAudioStream + 3) % 3;
				}
			break;
			case SET_READ_SPEED:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_READ_SPEED)) {
					gameConfig->emulateReadSpeed += direction;
					gameConfig->emulateReadSpeed = (gameConfig->emulateReadSpeed + 3) % 3;
				}
			break;
			case SET_EMULATE_ETHERNET:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->emulable & EMU_ETHERNET))
					gameConfig->emulateEthernet ^= 1;
			break;
			case SET_CLEAN_BOOT:
				if(devices[DEVICE_CUR] == NULL || (devices[DEVICE_CUR]->location == LOC_DVD_CONNECTOR))
					gameConfig->preferCleanBoot ^= 1;
			break;
			case SET_DEFAULTS:
				if(direction == 0)
					config_defaults(gameConfig);
			break;
		}
	}
}

int show_settings(int page, int option, ConfigEntry *config) {
	wait_network();
	// Copy current settings to a temp copy in case the user cancels out
	if(config != NULL) {
		memcpy(&tempConfig, config, sizeof(ConfigEntry));
	}
	memcpy(&tempSettings, &swissSettings, sizeof(SwissSettings));
	
	GXRModeObj *oldmode = getVideoMode();
	while (padsButtonsHeld() & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	while(1) {
		uiDrawObj_t* settingsPage = settings_draw_page(page, option, config);
		while (!((padsButtonsHeld() & PAD_BUTTON_RIGHT) 
			|| (padsButtonsHeld() & PAD_BUTTON_LEFT) 
			|| (padsButtonsHeld() & PAD_BUTTON_UP) 
			|| (padsButtonsHeld() & PAD_BUTTON_DOWN) 
			|| (padsButtonsHeld() & PAD_BUTTON_B)
			|| (padsButtonsHeld() & PAD_BUTTON_A)
			|| (padsButtonsHeld() & PAD_BUTTON_Y)
			|| (padsButtonsHeld() & PAD_TRIGGER_R)
			|| (padsButtonsHeld() & PAD_TRIGGER_L)))
			{ VIDEO_WaitVSync (); }
		u16 btns = padsButtonsHeld();
		if(btns & PAD_BUTTON_Y) {
			char *tooltip = get_tooltip(page, option);
			if(tooltip) {
				uiDrawObj_t* tooltipBox = DrawPublish(DrawTooltip(tooltip));
				while (padsButtonsHeld() & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
				while (!((padsButtonsHeld() & PAD_BUTTON_Y) || (padsButtonsHeld() & PAD_BUTTON_B))){ VIDEO_WaitVSync (); }
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
				settings_toggle(page, option, 1, config);
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
				settings_toggle(page, option, -1, config);
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
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving changes\205"));
				// Save settings to SRAM
				swissSettings.sram60Hz = getTVFormat() != VI_PAL;
				swissSettings.sramProgressive = getScanMode() == VI_PROGRESSIVE;
				if(swissSettings.aveCompat == 1) {
					swissSettings.sramHOffset &= ~1;
				}
				VIDEO_SetAdjustingValues(swissSettings.sramHOffset, 0);
				updateSRAM(&swissSettings, true);
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
				if(config != NULL) {
					memcpy(config, &tempConfig, sizeof(ConfigEntry));
				}
				memcpy(&swissSettings, &tempSettings, sizeof(SwissSettings));
				VIDEO_SetAdjustingValues(swissSettings.sramHOffset, 0);
				DrawDispose(settingsPage);
				DrawVideoMode(oldmode);
				return 0;
			}
			if((page != PAGE_MAX) && (option == settings_count_pp[page]-2)) {
				page++; option = 0;
			}
			if((page != PAGE_MIN) && (option == settings_count_pp[page]-(page != PAGE_MAX ? 3:2))) {
				page--; option = 0;
			}
			// These use text input, allow them to be accessed with the A button
			if(page == PAGE_GLOBAL && option == SET_FLATTEN_DIR) {
				settings_toggle(page, option, 0, config);
			}
			if(page == PAGE_NETWORK && (in_range(option, SET_BBA_LOCALIP, SET_BBA_GATEWAY) ||
										in_range(option, SET_FSP_HOSTIP,  SET_FTP_PASS) ||
										in_range(option, SET_SMB_HOSTIP,  SET_SMB_PASS))) {
				settings_toggle(page, option, 0, config);
			}
			if(page == PAGE_GAME && option == SET_DEFAULTS) {
				settings_toggle(page, option, 0, config);
			}
		}
		while ((padsButtonsHeld() & PAD_BUTTON_RIGHT) 
				|| (padsButtonsHeld() & PAD_BUTTON_LEFT) 
				|| (padsButtonsHeld() & PAD_BUTTON_UP) 
				|| (padsButtonsHeld() & PAD_BUTTON_DOWN) 
				|| (padsButtonsHeld() & PAD_BUTTON_B) 
				|| (padsButtonsHeld() & PAD_BUTTON_A)
				|| (padsButtonsHeld() & PAD_BUTTON_Y)
				|| (padsButtonsHeld() & PAD_TRIGGER_R)
				|| (padsButtonsHeld() & PAD_TRIGGER_L))
			{ VIDEO_WaitVSync (); }
		DrawDispose(settingsPage);
	}
}
