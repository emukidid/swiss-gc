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
char *uiVModeStr[] = {"Auto", "480i", "240p", "480p", "576i", "288p", "576p"};
char *softProgressiveStr[] = {"No", "Light", "Yes"};
char *forceWidescreenStr[] = {"No", "Persp", "Yes"};
char *forceEncodingStr[] = {"Auto", "ANSI", "SJIS"};
syssram* sram;

// Number of settings (including Back, Next, Save, Exit buttons) per page
int settings_count_pp[3] = {7, 9, 9};

void refreshSRAM() {
	sram = __SYS_LockSram();
	swissSettings.sramStereo = sram->flags & 4;
	swissSettings.sramLanguage = sram->lang;
	__SYS_UnlockSram(0);
}

void settings_draw_page(int page_num, int option, file_handle *file) {
	doBackdrop();
	DrawEmptyBox(20,60, vmode->fbWidth-20, 460, COLOR_BLACK);
		
	// Save Settings to current device (**Shown on all tabs**)
	/** Global Settings (Page 1/) */
	// IPL/Game Language [English/German/French/Spanish/Italian/Dutch]
	// IPL/Game Audio [Mono/Stereo]
	// Default Device [SD A/SD B/IDE A/IDE B/Qoob/Ask]
	// SD/IDE Speed [16/32 MHz]
	// Swiss Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz)]
	// Stop DVD Motor on startup [Yes/No]

	/** Advanced Settings (Page 2/) */
	// Enable USB Gecko Debug via Slot B [Yes/No]
	// Force No DVD Drive Mode [Yes/No]
	// Hide Unknown file types [Yes/No]	// TO BE IMPLEMENTED
	
	/** Current Game Settings - only if a valid GCM file is highlighted (Page 3/) */
	// Force Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), Auto]
	// Mute Audio Streaming [Yes/No]
	// Try to mute audio stutter [Yes/No]

	if(!page_num) {
		WriteFont(30, 65, "Global Settings (1/3):");
		WriteFontStyled(30, 120, "IPL/Game Language:", 1.0f, false, defaultColor);
		DrawSelectableButton(400, 120, -1, 150, getSramLang(swissSettings.sramLanguage), option == 0 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 160, "IPL/Game Audio:", 1.0f, false, defaultColor);
		DrawSelectableButton(400, 160, -1, 190, swissSettings.sramStereo ? "Stereo":"Mono", option == 1 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 200, "Default Device:", 1.0f, false, defaultColor);
		DrawSelectableButton(400, 200, -1, 230, swissSettings.defaultDevice ? "Yes":"No", option == 2 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 240, "SD/IDE Speed:", 1.0f, false, defaultColor);
		DrawSelectableButton(400, 240, -1, 270, swissSettings.exiSpeed ? "32 MHz":"16 MHz", option == 3 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 280, "Swiss Video Mode:", 1.0f, false, defaultColor);
		DrawSelectableButton(400, 280, -1, 310, uiVModeStr[swissSettings.uiVMode], option == 4 ? B_SELECTED:B_NOSELECT,-1);
	}
	else if(page_num == 1) {
		WriteFont(30, 65, "Advanced Settings (2/3):");
		WriteFontStyled(30, 120, "Enable USB Gecko Debug via Slot B:", 1.0f, false, defaultColor);
		DrawSelectableButton(500, 120, -1, 150, swissSettings.debugUSB ? "Yes":"No", option == 0 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 160, "Force No DVD Drive Mode:", 1.0f, false, defaultColor);
		DrawSelectableButton(500, 160, -1, 190, swissSettings.hasDVDDrive ? "No":"Yes", option == 1 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 200, "Hide Unknown file types:", 1.0f, false, defaultColor);
		DrawSelectableButton(500, 200, -1, 230, swissSettings.hideUnknownFileTypes ? "Yes":"No", option == 2 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 240, "Stop DVD Motor on startup:", 1.0f, false, defaultColor);
		DrawSelectableButton(500, 240, -1, 270, swissSettings.stopMotor ? "Yes":"No", option == 3 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 280, "Enable WiiRD debugging in Games:", 1.0f, false, defaultColor);
		DrawSelectableButton(500, 280, -1, 310, swissSettings.wiirdDebug ? "Yes":"No", option == 4 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 320, "Enable File Management:", 1.0f, false, defaultColor);
		DrawSelectableButton(500, 320, -1, 350, swissSettings.enableFileManagement ? "Yes":"No", option == 5 ? B_SELECTED:B_NOSELECT,-1);
	}
	else if(page_num == 2) {
		WriteFont(30, 65, "Current Game Settings (3/3):");
		WriteFontStyled(30, 110, "Force Video Mode:", 1.0f, false, file != NULL ? defaultColor : disabledColor);
		DrawSelectableButton(480, 110, -1, 135, uiVModeStr[swissSettings.gameVMode], option == 0 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 140, "If Progressive, Soften:", 1.0f, false, file != NULL ? defaultColor : disabledColor);
		DrawSelectableButton(480, 140, -1, 165, softProgressiveStr[swissSettings.softProgressive], option == 1 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 170, "Force Widescreen:", 1.0f, false, file != NULL ? defaultColor : disabledColor);
		DrawSelectableButton(480, 170, -1, 195, forceWidescreenStr[swissSettings.forceWidescreen], option == 2 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 200, "Force Anisotropy:", 1.0f, false, file != NULL ? defaultColor : disabledColor);
		DrawSelectableButton(480, 200, -1, 225, swissSettings.forceAnisotropy ? "Yes":"No", option == 3 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 230, "Mute Audio Streaming:", 1.0f, false, file != NULL ? defaultColor : disabledColor);
		DrawSelectableButton(480, 230, -1, 255, swissSettings.muteAudioStreaming ? "Yes":"No", option == 4 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 260, "Emulate Memory Card via SDGecko:", 1.0f, false, file != NULL ? defaultColor : disabledColor);
		DrawSelectableButton(480, 260, -1, 285, swissSettings.emulatemc ? "Yes":"No", option == 5 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 290, "Force Encoding:", 1.0f, false, file != NULL ? defaultColor : disabledColor);
		DrawSelectableButton(480, 290, -1, 315, forceEncodingStr[swissSettings.forceEncoding], option == 6 ? B_SELECTED:B_NOSELECT,-1);
	}
	if(page_num != 0) {
		DrawSelectableButton(40, 390, -1, 420, "Back", 
		option == settings_count_pp[page_num]-(page_num != 2 ? 3:2) ? B_SELECTED:B_NOSELECT,-1);
	}
	if(page_num != 2) {
		DrawSelectableButton(510, 390, -1, 420, "Next", 
		option == settings_count_pp[page_num]-2 ? B_SELECTED:B_NOSELECT,-1);
	}
	DrawSelectableButton(100, 425, -1, 455, "Save & Exit", option == settings_count_pp[page_num]-1 ? B_SELECTED:B_NOSELECT,-1);
	DrawSelectableButton(320, 425, -1, 455, "Discard & Exit", option ==  settings_count_pp[page_num] ? B_SELECTED:B_NOSELECT,-1);
	DrawFrameFinish();
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
				swissSettings.sramStereo ^= 4;
			break;
			case 2:
				swissSettings.defaultDevice ^= 1;
			break;
			case 3:
				swissSettings.exiSpeed ^= 1;
			break;
			case 4:
				swissSettings.uiVMode += direction;
				if(swissSettings.uiVMode == 2 || swissSettings.uiVMode == 5)
				swissSettings.uiVMode += direction;
				if(swissSettings.uiVMode > 6)
					swissSettings.uiVMode = 0;
				if(swissSettings.uiVMode < 0)
					swissSettings.uiVMode = 6;
			break;
		}	
	}
	else if(page == 1) {
		switch(option) {
			case 0:
				swissSettings.debugUSB ^= 1;
			break;
			case 1:
				swissSettings.hasDVDDrive ^= 1;
			break;
			case 2:
				swissSettings.hideUnknownFileTypes ^= 1;
			break;
			case 3:
				swissSettings.stopMotor ^= 1;
			break;
			case 4:
				swissSettings.wiirdDebug ^=1;
			break;
			case 5:
				swissSettings.enableFileManagement ^=1;
			break;
		}
	}
	else if(page == 2 && file != NULL) {
		switch(option) {
			case 0:
				swissSettings.gameVMode += direction;
				if(swissSettings.gameVMode > 6)
					swissSettings.gameVMode = 0;
				if(swissSettings.gameVMode < 0)
					swissSettings.gameVMode = 6;
			break;
			case 1:
				swissSettings.softProgressive += direction;
				if(swissSettings.softProgressive > 2)
					swissSettings.softProgressive = 0;
				if(swissSettings.softProgressive < 0)
					swissSettings.softProgressive = 2;
			break;
			case 2:
				swissSettings.forceWidescreen += direction;
				if(swissSettings.forceWidescreen > 2)
					swissSettings.forceWidescreen = 0;
				if(swissSettings.forceWidescreen < 0)
					swissSettings.forceWidescreen = 2;
			break;
			case 3:
				swissSettings.forceAnisotropy ^= 1;
			break;
			case 4:
				swissSettings.muteAudioStreaming ^= 1;
			break;
			case 5:
				swissSettings.emulatemc ^= 1;
			break;
			case 6:
				swissSettings.forceEncoding += direction;
				if(swissSettings.forceEncoding > 2)
					swissSettings.forceEncoding = 0;
				if(swissSettings.forceEncoding < 0)
					swissSettings.forceEncoding = 2;
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
		settings_draw_page(page, option, file);
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
				DrawFrameStart();
				DrawMessageBox(D_INFO,"Saving changes!");
				DrawFrameFinish();
				// Change Swiss video mode if it was modified.
				if(tempSettings.uiVMode != swissSettings.uiVMode) {
					GXRModeObj *newmode = getModeFromSwissSetting(swissSettings.uiVMode);
					initialise_video(newmode);
					vmode = newmode;
				}
				// Save settings to SRAM
				sram = __SYS_LockSram();
				sram->flags = swissSettings.sramStereo ? (sram->flags|4):(sram->flags&~4);
				sram->lang = swissSettings.sramLanguage;
				__SYS_UnlockSram(1);
				while(!__SYS_SyncSram());
				// Update our .ini
				if(config != NULL) {
					config->gameVMode = swissSettings.gameVMode;
					config->softProgressive = swissSettings.softProgressive;
					config->muteAudioStreaming = swissSettings.muteAudioStreaming;
					config->forceWidescreen = swissSettings.forceWidescreen;
					config->forceAnisotropy = swissSettings.forceAnisotropy;
					config->forceEncoding = swissSettings.forceEncoding;
					config->emulatemc = swissSettings.emulatemc;
				}
				else {
					// Save the Swiss system settings since we're called from the main menu
					if((curDevice != SD_CARD)&&((curDevice != IDEEXI))) {
						// If the device is Read-Only, warn/etc
						DrawFrameStart();
						DrawMessageBox(D_INFO,"Cannot save config on read-only device!");
						DrawFrameFinish();
					}
					else {
						DrawFrameStart();
						DrawMessageBox(D_INFO,"Saving Config ...");
						DrawFrameFinish();
						config_copy_swiss_settings(&swissSettings);
						if(config_update_file()) {
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
				return 1;
			}
			if(option == settings_count_pp[page]) {
				// Exit without saving (revert)
				memcpy((void*)&swissSettings, (void*)&tempSettings, sizeof(SwissSettings));
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
	}
}
