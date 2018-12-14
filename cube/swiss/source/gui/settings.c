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

SwissSettings tempSettings;
char *uiVModeStr[] = {"Auto", "480i", "480p", "576i", "576p"};
char *gameVModeStr[] = {"No", "480i", "480sf", "240p", "1080i60", "480p", "576i", "576sf", "288p", "1080i50", "576p"};
char *forceHScaleStr[] = {"Auto", "1:1", "11:10", "9:8", "704px", "720px"};
char *forceVFilterStr[] = {"Auto", "0", "1", "2"};
char *forceWidescreenStr[] = {"No", "3D", "2D+3D"};
char *forceEncodingStr[] = {"Auto", "ANSI", "SJIS"};
char *igrTypeStr[] = {"Disabled", "Reboot", "igr.dol", "USB Flash"};
syssram* sram;
syssramex* sramex;

// Number of settings (including Back, Next, Save, Exit buttons) per page
int settings_count_pp[3] = {8, 10, 10};

void refreshSRAM() {
	sram = __SYS_LockSram();
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

uiDrawObj_t* settings_draw_page(int page_num, int option, file_handle *file) {
	uiDrawObj_t* page = DrawEmptyBox(20,60, vmode->fbWidth-20, 460);
	char forceVOffsetStr[8];
		
	// Save Settings to current device (**Shown on all tabs**)
	/** Global Settings (Page 1/) */
	// IPL/Game Language [English/German/French/Spanish/Italian/Dutch]
	// IPL/Game Audio [Mono/Stereo]
	// SD/IDE Speed [16/32 MHz]
	// Swiss Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), etc]
	// In Game Reset [Yes/No]
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
	// If Progressive, Soften [No/Yes/Soften]
	// Force Widescreen [Yes/No/Persp]
	// Force Anistropy [Yes/No]
	// Disable Audio Streaming [Yes/No]
	// Force Encoding [Auto/SJIS]

	if(!page_num) {
		DrawAddChild(page, DrawLabel(30, 65, "Global Settings (1/3):"));
		DrawAddChild(page, DrawStyledLabel(30, 120, "IPL/Game Language:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 120, -1, 150, getSramLang(swissSettings.sramLanguage), option == 0 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 160, "IPL/Game Audio:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 160, -1, 190, swissSettings.sramStereo ? "Stereo":"Mono", option == 1 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 200, "SD/IDE Speed:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 200, -1, 230, swissSettings.exiSpeed ? "32 MHz":"16 MHz", option == 2 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 240, "Swiss Video Mode:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 240, -1, 270, uiVModeStr[swissSettings.uiVMode], option == 3 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 280, "In-Game-Reset:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 280, -1, 310, igrTypeStr[swissSettings.igrType], option == 4 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 320, "Config Device:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(320, 320, -1, 350, getConfigDeviceName(), option == 5 ? B_SELECTED:B_NOSELECT));
	}
	else if(page_num == 1) {
		DrawAddChild(page, DrawLabel(30, 65, "Advanced Settings (2/3):"));
		DrawAddChild(page, DrawStyledLabel(30, 110, "Enable USB Gecko Debug via Slot B:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(500, 110, -1, 135, swissSettings.debugUSB ? "Yes":"No", option == 0 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 140, "Hide Unknown file types:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(500, 140, -1, 165, swissSettings.hideUnknownFileTypes ? "Yes":"No", option == 1 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 170, "Stop DVD Motor on startup:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(500, 170, -1, 195, swissSettings.stopMotor ? "Yes":"No", option == 2 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 200, "Enable WiiRD debugging in Games:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(500, 200, -1, 225, swissSettings.wiirdDebug ? "Yes":"No", option == 3 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 230, "Enable File Management:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(500, 230, -1, 255, swissSettings.enableFileManagement ? "Yes":"No", option == 4 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 260, "Auto-load all cheats:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(500, 260, -1, 285, swissSettings.autoCheats ? "Yes":"No", option == 5 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 290, "Init network at startup:", 1.0f, false, defaultColor));
		DrawAddChild(page, DrawSelectableButton(500, 290, -1, 315, swissSettings.initNetworkAtStart ? "Yes":"No", option == 6 ? B_SELECTED:B_NOSELECT));
	}
	else if(page_num == 2) {
		DrawAddChild(page, DrawLabel(30, 65, "Current Game Settings (3/3):"));
		DrawAddChild(page, DrawStyledLabel(30, 110, "Force Video Mode:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 110, -1, 135, gameVModeStr[swissSettings.gameVMode], option == 0 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 140, "Force Horizontal Scale:", 1.0f, false, file != NULL && swissSettings.gameVMode > 0 ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 140, -1, 165, forceHScaleStr[swissSettings.forceHScale], option == 1 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 170, "Force Vertical Offset:", 1.0f, false, file != NULL && swissSettings.gameVMode > 0 ? defaultColor : disabledColor));
		sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
		DrawAddChild(page, DrawSelectableButton(480, 170, -1, 195, forceVOffsetStr, option == 2 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 200, "Force Vertical Filter:", 1.0f, false, file != NULL && swissSettings.gameVMode > 0 ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 200, -1, 225, forceVFilterStr[swissSettings.forceVFilter], option == 3 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 230, "Force Anisotropic Filter:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 230, -1, 255, swissSettings.forceAnisotropy ? "Yes":"No", option == 4 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 260, "Force Widescreen:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 260, -1, 285, forceWidescreenStr[swissSettings.forceWidescreen], option == 5 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 290, "Force Text Encoding:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 290, -1, 315, forceEncodingStr[swissSettings.forceEncoding], option == 6 ? B_SELECTED:B_NOSELECT));
		DrawAddChild(page, DrawStyledLabel(30, 320, "Disable Audio Streaming:", 1.0f, false, file != NULL ? defaultColor : disabledColor));
		DrawAddChild(page, DrawSelectableButton(480, 320, -1, 345, swissSettings.muteAudioStreaming ? "Yes":"No", option == 7 ? B_SELECTED:B_NOSELECT));
	}
	if(page_num != 0) {
		DrawAddChild(page, DrawSelectableButton(40, 390, -1, 420, "Back", 
		option == settings_count_pp[page_num]-(page_num != 2 ? 3:2) ? B_SELECTED:B_NOSELECT));
	}
	if(page_num != 2) {
		DrawAddChild(page, DrawSelectableButton(510, 390, -1, 420, "Next", 
		option == settings_count_pp[page_num]-2 ? B_SELECTED:B_NOSELECT));
	}
	DrawAddChild(page, DrawSelectableButton(100, 425, -1, 455, "Save & Exit", option == settings_count_pp[page_num]-1 ? B_SELECTED:B_NOSELECT));
	DrawAddChild(page, DrawSelectableButton(320, 425, -1, 455, "Discard & Exit", option ==  settings_count_pp[page_num] ? B_SELECTED:B_NOSELECT));
	DrawPublish(page);
	return page;
}

void settings_toggle(int page, int option, int direction, file_handle *file) {
	if(page == 0) {
		switch(option) {
			case 0:
				swissSettings.sramLanguage += direction;
				if(swissSettings.sramLanguage > 5)
					swissSettings.sramLanguage = 0;
				if(swissSettings.sramLanguage < 0)
					swissSettings.sramLanguage = 5;
			break;
			case 1:
				swissSettings.sramStereo ^= 1;
			break;
			case 2:
				swissSettings.exiSpeed ^= 1;
			break;
			case 3:
				swissSettings.uiVMode += direction;
				if(swissSettings.uiVMode > 4)
					swissSettings.uiVMode = 0;
				if(swissSettings.uiVMode < 0)
					swissSettings.uiVMode = 4;
			break;
			case 4:
				swissSettings.igrType += direction;
				if(swissSettings.igrType > 3)
					swissSettings.igrType = 0;
				if(swissSettings.igrType < 0)
					swissSettings.igrType = 3;
			break;
			case 5:
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
	else if(page == 1) {
		switch(option) {
			case 0:
				swissSettings.debugUSB ^= 1;
			break;
			case 1:
				swissSettings.hideUnknownFileTypes ^= 1;
			break;
			case 2:
				swissSettings.stopMotor ^= 1;
			break;
			case 3:
				swissSettings.wiirdDebug ^=1;
			break;
			case 4:
				swissSettings.enableFileManagement ^=1;
			break;
			case 5:
				swissSettings.autoCheats ^=1;
			break;
			case 6:
				swissSettings.initNetworkAtStart ^= 1;
		}
	}
	else if(page == 2 && file != NULL) {
		switch(option) {
			case 0:
				swissSettings.gameVMode += direction;
				if(swissSettings.gameVMode > 10)
					swissSettings.gameVMode = 0;
				if(swissSettings.gameVMode < 0)
					swissSettings.gameVMode = 10;
			break;
			case 1:
				if(swissSettings.gameVMode > 0) {
					swissSettings.forceHScale += direction;
					if(swissSettings.forceHScale > 5)
						swissSettings.forceHScale = 0;
					if(swissSettings.forceHScale < 0)
						swissSettings.forceHScale = 5;
				}
			break;
			case 2:
				if(swissSettings.gameVMode > 0)
					swissSettings.forceVOffset += direction;
			break;
			case 3:
				if(swissSettings.gameVMode > 0) {
					swissSettings.forceVFilter += direction;
					if(swissSettings.forceVFilter > 3)
						swissSettings.forceVFilter = 0;
					if(swissSettings.forceVFilter < 0)
						swissSettings.forceVFilter = 3;
				}
			break;
			case 4:
				swissSettings.forceAnisotropy ^= 1;
			break;
			case 5:
				swissSettings.forceWidescreen += direction;
				if(swissSettings.forceWidescreen > 2)
					swissSettings.forceWidescreen = 0;
				if(swissSettings.forceWidescreen < 0)
					swissSettings.forceWidescreen = 2;
			break;
			case 6:
				swissSettings.forceEncoding += direction;
				if(swissSettings.forceEncoding > 2)
					swissSettings.forceEncoding = 0;
				if(swissSettings.forceEncoding < 0)
					swissSettings.forceEncoding = 2;
			break;
			case 7:
				swissSettings.muteAudioStreaming ^= 1;
			break;
		}
	}
}

int show_settings(file_handle *file, ConfigEntry *config) {
	int page = 0, option = 0;

	// Refresh SRAM in case user changed it from IPL
	refreshSRAM();
	
	// Copy current settings to a temp copy in case the user cancels out
	memcpy((void*)&tempSettings,(void*)&swissSettings, sizeof(SwissSettings));
	
	// Setup the settings for the current game
	if(config != NULL) {
		page = 2;
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
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_RIGHT) {
			// If we're on a button (Back, Next, Save, Exit), allow left/right movement
			if((page != 1) && (option >= settings_count_pp[page]-2) && option < settings_count_pp[page]) {
				option++;
			}
			else if((page == 1) && (option >= settings_count_pp[page]-3) && option < settings_count_pp[page]) {
				option++;
			}
			else {
				settings_toggle(page, option, 1, file);
			}
		}
		if(btns & PAD_BUTTON_LEFT) {
			// If we're on a button (Back, Next, Save, Exit), allow left/right movement
			if((page != 1) && (option > settings_count_pp[page]-2)) {
				option--;
			}
			else if((page == 1) && (option > settings_count_pp[page]-3)) {
				option--;
			}
			else {
				settings_toggle(page, option, -1, file);
			}
		}
		if((btns & PAD_BUTTON_DOWN) && option < settings_count_pp[page])
			option++;
		if((btns & PAD_BUTTON_UP) && option > 0)
			option--;
		if((btns & PAD_TRIGGER_R) && page < 2) {
			page++; option = 0;
		}
		if((btns & PAD_TRIGGER_L) && page > 0) {
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
			if((page != 2) && (option == settings_count_pp[page]-2)) {
				page++; option = 0;
			}
			if((page != 0) && (option == settings_count_pp[page]-(page != 2 ? 3:2))) {
				page--; option = 0;
			}
		}
		while ((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_UP) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
				|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
				|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L))
			{ VIDEO_WaitVSync (); }
		DrawDispose(settingsPage);
	}
}
