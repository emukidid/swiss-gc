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

SwissSettings tempSettings;
char *uiVModeStr[] = {"Auto", "480i", "480p", "576i", "576p"};
char *gameVModeStr[] = {"No", "480i", "480sf", "240p", "1080i60", "480p", "576i", "576sf", "288p", "1080i50", "576p"};
char *forceHScaleStr[] = {"Auto", "1:1", "11:10", "9:8", "640px", "704px", "720px"};
char *forceVFilterStr[] = {"Auto", "0", "1", "2"};
char *forceWidescreenStr[] = {"No", "3D", "2D+3D"};
char *forceEncodingStr[] = {"Auto", "ANSI", "SJIS"};
char *igrTypeStr[] = {"Disabled", "Reboot", "igr.dol"};

char *tooltips_global[PAGE_GLOBAL_MAX+1] = {
	"System Sound:\n\nSets the default audio output type used by most games",
	"Screen Position:\n\nAdjusts the horizontal screen position in games.\nNote: This will take effect next boot.",
	"System Language:\n\nSystem language used in games, primarily multi-5 PAL games",
	"SD/IDE Speed:\n\nThe speed to try and use on the EXI bus for SD Gecko or IDE-EXI devices.\n32 MHz may not work on some SD cards.\n",
	 NULL,
	"In-Game Reset: (B + R + Z + DPad Down)\n\nReboot: Soft-Reset the GameCube\nigr.dol: 0x81300000 igr.dol at the root of SD Gecko",
	"Configuration Device:\n\nThe device that Swiss will use to load and save swiss.ini from.\nThis setting is stored in SRAM and will remain on reboot.",
	NULL,
	NULL,
	NULL
};

char *tooltips_advanced[PAGE_ADVANCED_MAX+1] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

char *tooltips_game[PAGE_GAME_MAX+1] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

syssram* sram;
syssramex* sramex;

// Number of settings (including Back, Next, Save, Exit buttons) per page
int settings_count_pp[3] = {PAGE_GLOBAL_MAX, PAGE_ADVANCED_MAX, PAGE_GAME_MAX};

void refreshSRAM() {
	sram = __SYS_LockSram();
	swissSettings.sramHOffset = sram->display_offsetH;
	swissSettings.sram60Hz = (sram->ntd >> 6) & 1;
	swissSettings.sramLanguage = sram->lang;
	swissSettings.sramProgressive = (sram->flags >> 7) & 1;
	swissSettings.sramStereo = (sram->flags >> 2) & 1;
	__SYS_UnlockSram(0);
	sramex = __SYS_LockSramEx();
	swissSettings.configDeviceId = sramex->__padding0;
	if(swissSettings.configDeviceId > DEVICE_ID_MAX || !(getDeviceByUniqueId(swissSettings.configDeviceId)->features & FEAT_WRITE)) {
		swissSettings.configDeviceId = DEVICE_ID_UNK;
	}
	__SYS_UnlockSramEx(0);
}

char* getConfigDeviceName() {
	DEVICEHANDLER_INTERFACE *configDevice = getDeviceByUniqueId(swissSettings.configDeviceId);
	return configDevice != NULL ? (char*)(configDevice->deviceName) : "None";
}

char* get_tooltip(int page_num, int option) {
	char *textPtr = NULL;
	if(page_num == PAGE_GLOBAL) {
		textPtr = tooltips_global[option];
	}
	else if(page_num == PAGE_ADVANCED) {
		textPtr = tooltips_advanced[option];
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

uiDrawObj_t* settings_draw_page(int page_num, int option, file_handle *file) {
	uiDrawObj_t* page = DrawEmptyBox(20,60, vmode->fbWidth-20, 460);
	char sramHOffsetStr[8];
	char forceVOffsetStr[8];
	
	// Save Settings to current device (**Shown on all tabs**)
	/** Global Settings (Page 1/) */
	// System Sound [Mono/Stereo]
	// Screen Position [+/-0]
	// System Language [English/German/French/Spanish/Italian/Dutch]
	// SD/IDE Speed [16/32 MHz]
	// Swiss Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), etc]
	// In-Game Reset [Yes/No]
	// Configuration Device [Writable device name]

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
	// Force Horizontal Scale [Auto/1:1/11:10/9:8/704px/720px]
	// Force Vertical Offset [+/-0]
	// Force Vertical Filter [Auto/0/1/2]
	// Force Anisotropic Filter [Yes/No]
	// Force Widescreen [No/3D/2D+3D]
	// Force Text Encoding [Auto/ANSI/SJIS]
	// Disable Audio Streaming [Yes/No]

	// Add paging and save/cancel buttons
	if(page_num != PAGE_MIN) {
		DrawAddChild(page, DrawSelectableButton(50, 390, -1, 420, "Back", 
		option == settings_count_pp[page_num]-(page_num != PAGE_MAX ? 3:2) ? B_SELECTED:B_NOSELECT));
	}
	if(page_num != PAGE_MAX) {
		DrawAddChild(page, DrawSelectableButton(510, 390, -1, 420, "Next", 
		option == settings_count_pp[page_num]-2 ? B_SELECTED:B_NOSELECT));
	}
	DrawAddChild(page, DrawSelectableButton(120, 425, -1, 455, "Save & Exit", option == settings_count_pp[page_num]-1 ? B_SELECTED:B_NOSELECT));
	DrawAddChild(page, DrawSelectableButton(320, 425, -1, 455, "Discard & Exit", option ==  settings_count_pp[page_num] ? B_SELECTED:B_NOSELECT));
	
	// Page specific buttons
	if(page_num == PAGE_GLOBAL) {
		DrawAddChild(page, DrawLabel(30, 65, "Global Settings (1/3):"));
		DrawAddChild(page, DrawStyledLabel(30, 110, "System Sound:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 110, -1, 135, swissSettings.sramStereo ? "Stereo":"Mono", option == SET_SYS_SOUND ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 140, "Screen Position:", 1.0f, false, defaultColor));
		sprintf(sramHOffsetStr, "%+hi", swissSettings.sramHOffset);
		DrawAddChild(page, DrawSelectableButton(320, 140, -1, 165, sramHOffsetStr, option == SET_SCREEN_POS ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 170, "System Language:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 170, -1, 195, getSramLang(swissSettings.sramLanguage), option == SET_SYS_LANG ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 200, "SD/IDE Speed:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 200, -1, 225, swissSettings.exiSpeed ? "32 MHz":"16 MHz", option == SET_EXI_SPEED ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 230, "Swiss Video Mode:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 230, -1, 255, uiVModeStr[swissSettings.uiVMode], option == SET_SWISS_VIDEOMODE ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 260, "In-Game Reset:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 260, -1, 285, igrTypeStr[swissSettings.igrType], option == SET_IGR ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 290, "Configuration Device:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 290, -1, 315, getConfigDeviceName(), option == SET_CONFIG_DEV ? B_SELECTED:B_NOSELECT));
	}
	else if(page_num == PAGE_ADVANCED) {
		DrawAddChild(page, DrawLabel(30, 65, "Advanced Settings (2/3):"));
		DrawAddChild(page, DrawStyledLabel(30, 110, "Enable USB Gecko Debug via Slot B:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 110, -1, 135, swissSettings.debugUSB ? "Yes":"No", option == SET_ENABLE_USBGECKODBG ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 140, "Hide Unknown file types:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 140, -1, 165, swissSettings.hideUnknownFileTypes ? "Yes":"No", option == SET_HIDE_UNK ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 170, "Stop DVD Motor on startup:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 170, -1, 195, swissSettings.stopMotor ? "Yes":"No", option == SET_STOP_MOTOR ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 200, "Enable WiiRD debugging in Games:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 200, -1, 225, swissSettings.wiirdDebug ? "Yes":"No", option == SET_WIIRDDBG ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 230, "Enable File Management:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 230, -1, 255, swissSettings.enableFileManagement ? "Yes":"No", option == SET_FILE_MGMT ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 260, "Auto-load all cheats:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 260, -1, 285, swissSettings.autoCheats ? "Yes":"No", option == SET_ALL_CHEATS ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 290, "Init network at startup:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 290, -1, 315, swissSettings.initNetworkAtStart ? "Yes":"No", option == SET_INIT_NET ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 320, "Disable Video Patches:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(510, 320, -1, 345, swissSettings.disableVideoPatches ? "Yes":"No", option == SET_ENABLE_VIDPATCH ? B_SELECTED:B_NOSELECT));
	}
	else if(page_num == PAGE_GAME) {
		DrawAddChild(page, DrawLabel(30, 65, "Current Game Settings (3/3):"));
		DrawAddChild(page, DrawStyledLabel(30, 110, "Force Video Mode:", 1.0f, false, file != NULL && !swissSettings.disableVideoPatches ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 110, -1, 135, gameVModeStr[swissSettings.gameVMode], option == SET_FORCE_VIDEOMODE ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 140, "Force Horizontal Scale:", 1.0f, false, file != NULL && !swissSettings.disableVideoPatches ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 140, -1, 165, forceHScaleStr[swissSettings.forceHScale], option == SET_HORIZ_SCALE ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 170, "Force Vertical Offset:", 1.0f, false, file != NULL && !swissSettings.disableVideoPatches ? defaultColor : disabledColor));
		sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
		DrawAddChild(page, DrawSelectableButton(480, 170, -1, 195, forceVOffsetStr, option == SET_VERT_OFFSET ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 200, "Force Vertical Filter:", 1.0f, false, file != NULL && !swissSettings.disableVideoPatches ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 200, -1, 225, forceVFilterStr[swissSettings.forceVFilter], option == SET_VERT_FILTER ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 230, "Disable Alpha Dithering:", 1.0f, false, file != NULL && !swissSettings.disableVideoPatches ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 230, -1, 255, swissSettings.disableDithering ? "Yes":"No", option == SET_ALPHA_DITHER ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 260, "Force Anisotropic Filter:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 260, -1, 285, swissSettings.forceAnisotropy ? "Yes":"No", option == SET_ANISO_FILTER ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 290, "Force Widescreen:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 290, -1, 315, forceWidescreenStr[swissSettings.forceWidescreen], option == SET_WIDESCREEN ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 320, "Force Text Encoding:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 320, -1, 345, forceEncodingStr[swissSettings.forceEncoding], option == SET_TEXT_ENCODING ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 350, "Disable Audio Streaming:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 350, -1, 375, swissSettings.muteAudioStreaming ? "Yes":"No", option == SET_AUDIO_STREAM ? B_SELECTED:B_NOSELECT));
	}
	// If we have a tooltip for this page/option, add a fading label telling the user to press Y for help
	add_tooltip_label(page, page_num, option);
	
	DrawPublish(page);
	return page;
}

void settings_toggle(int page, int option, int direction, file_handle *file) {
	if(page == PAGE_GLOBAL) {
		switch(option) {
			case SET_SYS_SOUND:
				swissSettings.sramStereo ^= 1;
			break;
			case SET_SCREEN_POS:
				swissSettings.sramHOffset += direction;
			break;
			case SET_SYS_LANG:
				swissSettings.sramLanguage += direction;
				if(swissSettings.sramLanguage > 5)
					swissSettings.sramLanguage = 0;
				if(swissSettings.sramLanguage < 0)
					swissSettings.sramLanguage = 5;
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
						if(allDevices[i] != NULL && (allDevices[i]->features & FEAT_WRITE)) {
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
					while((allDevices[curDevicePos] == NULL) || !(allDevices[curDevicePos]->features & FEAT_WRITE)) {
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
			case SET_INIT_NET:
				swissSettings.initNetworkAtStart ^= 1;
			break;
			case SET_ENABLE_VIDPATCH:
				swissSettings.disableVideoPatches ^= 1;
			break;
		}
	}
	else if(page == PAGE_GAME && file != NULL) {
		switch(option) {
			case SET_FORCE_VIDEOMODE:
				if(!swissSettings.disableVideoPatches) {
					swissSettings.gameVMode += direction;
					if(swissSettings.gameVMode > 10)
						swissSettings.gameVMode = 0;
					if(swissSettings.gameVMode < 0)
						swissSettings.gameVMode = 10;
				}
			break;
			case SET_HORIZ_SCALE:
				if(!swissSettings.disableVideoPatches) {
					swissSettings.forceHScale += direction;
					if(swissSettings.forceHScale > 6)
						swissSettings.forceHScale = 0;
					if(swissSettings.forceHScale < 0)
						swissSettings.forceHScale = 6;
				}
			break;
			case SET_VERT_OFFSET:
				if(!swissSettings.disableVideoPatches)
					swissSettings.forceVOffset += direction;
			break;
			case SET_VERT_FILTER:
				if(!swissSettings.disableVideoPatches) {
					swissSettings.forceVFilter += direction;
					if(swissSettings.forceVFilter > 3)
						swissSettings.forceVFilter = 0;
					if(swissSettings.forceVFilter < 0)
						swissSettings.forceVFilter = 3;
				}
			break;
			case SET_ALPHA_DITHER:
				if(!swissSettings.disableVideoPatches)
					swissSettings.disableDithering ^= 1;
			break;
			case SET_ANISO_FILTER:
				swissSettings.forceAnisotropy ^= 1;
			break;
			case SET_WIDESCREEN:
				swissSettings.forceWidescreen += direction;
				if(swissSettings.forceWidescreen > 2)
					swissSettings.forceWidescreen = 0;
				if(swissSettings.forceWidescreen < 0)
					swissSettings.forceWidescreen = 2;
			break;
			case SET_TEXT_ENCODING:
				swissSettings.forceEncoding += direction;
				if(swissSettings.forceEncoding > 2)
					swissSettings.forceEncoding = 0;
				if(swissSettings.forceEncoding < 0)
					swissSettings.forceEncoding = 2;
			break;
			case SET_AUDIO_STREAM:
				swissSettings.muteAudioStreaming ^= 1;
			break;
		}
	}
}

int show_settings(file_handle *file, ConfigEntry *config) {
	int page = PAGE_GLOBAL, option = SET_SYS_SOUND;

	// Refresh SRAM in case user changed it from IPL
	refreshSRAM();
	
	// Copy current settings to a temp copy in case the user cancels out
	memcpy((void*)&tempSettings,(void*)&swissSettings, sizeof(SwissSettings));
	
	// Setup the settings for the current game
	if(config != NULL) {
		page = PAGE_GAME;
	}
		
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	while(1) {
		uiDrawObj_t* settingsPage = settings_draw_page(page, option, file);
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
				settings_toggle(page, option, 1, file);
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
				settings_toggle(page, option, -1, file);
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
					GXRModeObj *newmode = getModeFromSwissSetting(swissSettings.uiVMode);
					initialise_video(newmode);
					vmode = newmode;
				}
				// Save settings to SRAM
				if(swissSettings.uiVMode > 0) {
					swissSettings.sram60Hz = (swissSettings.uiVMode >= 1) && (swissSettings.uiVMode <= 2);
					swissSettings.sramProgressive = (swissSettings.uiVMode == 2) || (swissSettings.uiVMode == 4);
				}
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
				// Update our .ini
				if(config != NULL) {
					config->gameVMode = swissSettings.gameVMode;
					config->forceHScale = swissSettings.forceHScale;
					config->forceVOffset = swissSettings.forceVOffset;
					config->forceVFilter = swissSettings.forceVFilter;
					config->disableDithering = swissSettings.disableDithering;
					config->forceAnisotropy = swissSettings.forceAnisotropy;
					config->forceWidescreen = swissSettings.forceWidescreen;
					config->forceEncoding = swissSettings.forceEncoding;
					config->muteAudioStreaming = swissSettings.muteAudioStreaming;
					DrawDispose(msgBox);
				}
				else {
					// Save the Swiss system settings since we're called from the main menu
					config_copy_swiss_settings(&swissSettings);
					if(config_update_file()) {
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
				}
				DrawDispose(settingsPage);
				return 1;
			}
			if(option == settings_count_pp[page]) {
				// Exit without saving (revert)
				memcpy((void*)&swissSettings, (void*)&tempSettings, sizeof(SwissSettings));
				DrawDispose(settingsPage);
				return 0;
			}
			if((page != PAGE_MAX) && (option == settings_count_pp[page]-2)) {
				page++; option = 0;
			}
			if((page != PAGE_MIN) && (option == settings_count_pp[page]-(page != PAGE_MAX ? 3:2))) {
				page--; option = 0;
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
