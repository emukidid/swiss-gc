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
#include "settings.h"
#include "main.h"
#include "ata.h"
#include "exi.h"
#include "bba.h"
#include "flippy.h"
#include "gcloader.h"
#include "wkf.h"

char topStr[256];

const char* getDeviceInfoString(u32 location) {
	DEVICEHANDLER_INTERFACE *device = getDeviceByLocation(location);
	s32 exi_channel, exi_device;
	if(location == bba_exists(LOC_ANY)) {
		sprintf(topStr, "%s (%s)", getHwNameByLocation(location), bba_address_str());
	}
	else if(device == &__device_dvd) {
		if(swissSettings.hasDVDDrive == 1) {
			u8* driveVersion = (u8*)DVDDriveInfo;
			sprintf(topStr,"%s %02X %02X%02X/%02X (%02X)",device->hwName,driveVersion[6],driveVersion[4],driveVersion[5],driveVersion[7],driveVersion[8]);
		}
		else {
			strcpy(topStr, device->hwName);
		}
	}
	else if(device == &__device_flippy) {
		flippyversion *version = (flippyversion*)DVDDriveInfo->pad;
		sprintf(topStr, "%s (%u.%u.%u%s)", device->hwName, version->major, version->minor, version->build, version->dirty ? "-dirtyboi" : "");

		device = &__device_dvd;
		if(deviceHandler_getDeviceAvailable(device)) {
			strcat(topStr, " + ");
			strcat(topStr, device->hwName);
		}
	}
	else if(device == &__device_gcloader) {
		if(gcloaderVersionStr != NULL) {
			sprintf(topStr, "%s HW%i (%s)", device->hwName, gcloaderHwVersion, gcloaderVersionStr);
		}
		else {
			strcpy(topStr, device->hwName);
		}
	}
	else if(device == &__device_sd_a || device == &__device_sd_b || device == &__device_sd_c) {
		if(getExiDeviceByLocation(location, &exi_channel, NULL)) {
			sprintf(topStr, "%u MHz %s", 1 << sdgecko_getSpeed(exi_channel), device->hwName);
		}
		else {
			strcpy(topStr, device->hwName);
		}
	}
	else if(device == &__device_wkf) {
		sprintf(topStr, "%s (%s)", device->hwName, wkfGetSerial());
	}
	else {
		strcpy(topStr, getHwNameByLocation(location));
	}
	if(!strcmp(topStr, "Unknown") && getExiDeviceByLocation(location, &exi_channel, &exi_device)) {
		u32 exi_id;
		if(EXI_GetID(exi_channel, exi_device, &exi_id)) {
			sprintf(topStr, "Unknown (0x%08X)", exi_id);
		}
	}
	return topStr;
}

uiDrawObj_t * info_draw_page(int page_num) {
	uiDrawObj_t *container = DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 420);
	
	// System Info (Page 1/4)
	if(!page_num) {
		DrawAddChild(container, DrawLabel(30, 55, "System Info (1/4):"));
		// Model
		DrawAddChild(container, DrawStyledLabel(640/2, 90, (char*)"MODEL", 0.65f, true, defaultColor));
		if((SYS_GetConsoleType() & SYS_CONSOLE_MASK) == SYS_CONSOLE_DEVELOPMENT) {
			if(*DVDDeviceCode == 0x8201) {
				strcpy(topStr, "NPDP-GDEV (GCT-0100)");
			}
			else if(*DVDDeviceCode == 0x8200) {
				strcpy(topStr, "NPDP-GBOX (GCT-0200)");
			}
			else {
				strcpy(topStr, "ArtX Orca");
			}
		}
		else if(!strncmp(IPLInfo, "(C) ", 4)) {
			if(!strncmp(&IPLInfo[0x55], "TDEV", 4) ||
				!strncmp(&IPLInfo[0x55], "DEV  Revision 0.1", 0x11)) {
				strcpy(topStr, "Nintendo GameCube DOT-006");
			}
			else if(*DVDDeviceCode == 0x8200) {
				if(!strncmp(&IPLInfo[0x55], "PAL ", 4)) {
					strcpy(topStr, "Nintendo GameCube DOT-002P");
				}
				else {
					strcpy(topStr, "Nintendo GameCube DOT-002");
				}
			}
			else if(*DVDDeviceCode == 0x8001) {
				if(!strncmp(&IPLInfo[0x55], "PAL ", 4)) {
					strcpy(topStr, "Nintendo GameCube DOT-001P");
				}
				else {
					strcpy(topStr, "Nintendo GameCube DOT-001");
				}
			}
			else if(*DVDDeviceCode == 0x8000 && DVDDriveInfo->pad[1] == 'M') {
				strcpy(topStr, "Panasonic Q SL-GC10-S");
			}
			else if(!strncmp(&IPLInfo[0x55], "PAL  Revision 1.2", 0x11)) {
				strcpy(topStr, "Nintendo GameCube DOL-101(EUR)");
			}
			else if(!strncmp(&IPLInfo[0x55], "NTSC Revision 1.2", 0x11)) {
				sprintf(topStr, "Nintendo GameCube DOL-101(%s)", getFontEncode() ? "JPN" : "USA");
			}
			else if(!strncmp(&IPLInfo[0x55], "MPAL", 4)) {
				strcpy(topStr, "Nintendo GameCube DOL-002(BRA)");
			}
			else if(!strncmp(&IPLInfo[0x55], "PAL ", 4)) {
				strcpy(topStr, "Nintendo GameCube DOL-001(EUR)");
			}
			else {
				sprintf(topStr, "Nintendo GameCube DOL-001(%s)", getFontEncode() ? "JPN" : "USA");
			}
		}
		else {
			strcpy(topStr, "Nintendo Wii");
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 106, topStr, 0.75f, true, defaultColor));
		// IPL version string
		DrawAddChild(container, DrawStyledLabel(640/2, 130, (char*)"IPL VERSION", 0.65f, true, defaultColor));
		if(!strncmp(IPLInfo, "(C) ", 4)) {
			if(!IPLInfo[0x55]) {
				strcpy(topStr, "NTSC Revision 1.0");
			}
			else {
				strcpy(topStr, &IPLInfo[0x55]);
			}
		}
		else {
			strcpy(topStr, "Dummy");
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 146, topStr, 0.75f, true, defaultColor));
		
		DrawAddChild(container, DrawStyledLabel(640/2, 170, (char*)"VIDEO MODE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 186, getVideoModeString(), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 210, (char*)"AUDIO", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 226, (char*)(swissSettings.sramStereo ? "Stereo" : "Mono"), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 250, (char*)"LANGUAGE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 266, (char*)(sramLang[swissSettings.sramLanguage]), 0.75f, true, defaultColor));

		// GC 00083214, 00083410
		DrawAddChild(container, DrawStyledLabel(640/2, 290, (char*)"CPU PVR", 0.65f, true, defaultColor));
		u32 pvr = mfpvr();
		if((pvr & 0xFFFFF000) == 0x00083000) {
			if((pvr & 0xFEF) == 0x203 || (pvr & 0xFFF) == 0x214) {
				sprintf(topStr, "IBM Gekko DD%X.%Xe", (pvr >> 8) & 0xF, pvr & 0xF);
			}
			else {
				sprintf(topStr, "IBM Gekko DD%X.%X", (pvr >> 8) & 0xF, pvr & 0xF);
			}
		}
		else if((pvr & 0xFFFFF000) == 0x00087000) {
			if((pvr & 0xFFF) == 0x110) {
				sprintf(topStr, "IBM Broadway DD%X.%X%X", (pvr >> 8) & 0xF, pvr & 0xF, (pvr >> 4) & 0xF);
			}
			else {
				sprintf(topStr, "IBM Broadway DD%X.%X", (pvr >> 8) & 0xF, pvr & 0xF);
			}
		}
		else {
			sprintf(topStr, "Unknown (0x%08X)", pvr);
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 306, topStr, 0.75f, true, defaultColor));
		
		DrawAddChild(container, DrawStyledLabel(640/2, 330, (char*)"CPU ECID", 0.65f, true, defaultColor));
		sprintf(topStr,"%08X:%08X:%08X:%08X",mfspr(ECID0),mfspr(ECID1),mfspr(ECID2),mfspr(ECID3));
		DrawAddChild(container, DrawStyledLabel(640/2, 346, topStr, 0.75f, true, defaultColor));
		
		DrawAddChild(container, DrawStyledLabel(640/2, 370, (char*)"SYSTEM-ON-CHIP", 0.65f, true, defaultColor));
		u32 chipid = ((vu32*)0xCC003000)[11];
		if((chipid & 0xFFFFFFF) == 0x46500B1) {
			if(is_gamecube()) {
				sprintf(topStr, "ArtX Flipper Rev.%c", 'A' + (chipid >> 28));
			}
			else {
				strcpy(topStr, "ATI Hollywood");
			}
		}
		else {
			sprintf(topStr, "Unknown (0x%08X)", chipid);
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 386, topStr, 0.75f, true, defaultColor));
	}
	else if(page_num == 1) {
		DrawAddChild(container, DrawLabel(30, 55, "Device Info (2/4):"));
		DrawAddChild(container, DrawStyledLabel(640/2, 90, (char*)"SLOT-A", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 106, getDeviceInfoString(LOC_MEMCARD_SLOT_A), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 130, (char*)"SLOT-B", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 146, getDeviceInfoString(LOC_MEMCARD_SLOT_B), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 170, (char*)"SERIAL PORT 1", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 186, getDeviceInfoString(LOC_SERIAL_PORT_1), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 210, (char*)"SERIAL PORT 2", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 226, getDeviceInfoString(LOC_SERIAL_PORT_2), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 250, (char*)"DRIVE INTERFACE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 266, getDeviceInfoString(LOC_DVD_CONNECTOR), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 290, (char*)"PROGRESSIVE VIDEO", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 306, (char*)(getDTVStatus() ? "Yes" : "No"), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 330, (char*)"CURRENT DEVICE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 346, (char*)(devices[DEVICE_CUR] != NULL ? devices[DEVICE_CUR]->deviceName : "None"), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 370, (char*)"CONFIGURATION DEVICE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 386, (char*)(devices[DEVICE_CONFIG] != NULL ? devices[DEVICE_CONFIG]->deviceName : "None"), 0.75f, true, defaultColor));
	}
	else if(page_num == 2) {
		DrawAddChild(container, DrawLabel(30, 55, "Version Info (3/4):"));
		DrawAddChild(container, DrawStyledLabel(640/2, 115, "Swiss version 0.6", 1.0f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 140, "by emu_kidid & Extrems, 2025", 0.75f, true, defaultColor));
		sprintf(topStr, "Commit %s Revision %s", GIT_COMMIT, GIT_REVISION);
		DrawAddChild(container, DrawStyledLabel(640/2, 165, topStr, 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 220, "Source/Updates/Issues", 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 244, "github.com/emukidid/swiss-gc", 0.64f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 310, "Web/Support", 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 334, "www.gc-forever.com", 0.64f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 378, "Visit us on IRC at EFNet/#gc-forever", 0.75f, true, defaultColor));
	}
	else if(page_num == 3) {
		DrawAddChild(container, DrawLabel(30, 55, "Greetings (4/4):"));
		DrawAddChild(container, DrawStyledLabel(640/2, 90, "Current patreon supporters", 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 114, "Borg Number One (a.k.a. Steven Weiser), Roman Antonacci, 8BitMods,", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 134, "CastleMania Ryan, Dan Kunz, Fernando Avelino, HakanaiSeishin, Haymose,", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 154, "Alex Mitchell, badsector, Jeffrey Pierce, Jon Moon, kevin,", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 174, "Kory, Marlon, silversteel, William Fowler", 0.60f, true, defaultColor));

		DrawAddChild(container, DrawStyledLabel(640/2, 210, "Historical Patreon supporters", 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 234, "meneerbeer, SubElement, KirovAir, Cristofer Cruz,", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 254, "RamblingOkie, Lindh0lm154, finnyguy, CtPG", 0.60f, true, defaultColor));

		DrawAddChild(container, DrawStyledLabel(640/2, 290, "Extra Greetz: FIX94, megalomaniac, sepp256, novenary", 0.60f, true, defaultColor));
		
		DrawAddChild(container, DrawStyledLabel(640/2, 330, "...and a big thanks to YOU, for using Swiss!", 0.75f, true, defaultColor));
	}
	if(page_num != 3) {
		DrawAddChild(container, DrawLabel(530, 400, "\233"));
	}
	if(page_num != 0) {
		DrawAddChild(container, DrawLabel(100, 400, "\213"));
	}
	DrawAddChild(container, DrawStyledLabel(640/2, 410, "Press A or B to return", 1.0f, true, defaultColor));
	return container;
}

void show_info() {
	int page = 0;
	uiDrawObj_t* pagePanel = NULL;
	while (padsButtonsHeld() & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	while(1) {
		uiDrawObj_t* newPanel = info_draw_page(page);
		if(pagePanel != NULL) {
			DrawDispose(pagePanel);
		}
		pagePanel = newPanel;
		DrawPublish(pagePanel);
		while (!((padsButtonsHeld() & PAD_BUTTON_RIGHT) 
			|| (padsButtonsHeld() & PAD_BUTTON_LEFT) 
			|| (padsButtonsHeld() & PAD_BUTTON_B)
			|| (padsButtonsHeld() & PAD_BUTTON_A)
			|| (padsButtonsHeld() & PAD_TRIGGER_R)
			|| (padsButtonsHeld() & PAD_TRIGGER_L)))
			{ VIDEO_WaitVSync (); }
		u16 btns = padsButtonsHeld();
		if(((btns & PAD_BUTTON_RIGHT) || (padsButtonsHeld() & PAD_TRIGGER_R)) && page < 3)
			page++;
		if(((btns & PAD_BUTTON_LEFT) || (padsButtonsHeld() & PAD_TRIGGER_L)) && page > 0)
			page--;
		if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_B))
			break;
		while ((padsButtonsHeld() & PAD_BUTTON_RIGHT) 
			|| (padsButtonsHeld() & PAD_BUTTON_LEFT) 
			|| (padsButtonsHeld() & PAD_BUTTON_B)
			|| (padsButtonsHeld() & PAD_BUTTON_A)
			|| (padsButtonsHeld() & PAD_TRIGGER_R)
			|| (padsButtonsHeld() & PAD_TRIGGER_L))
			{ VIDEO_WaitVSync (); }
	}
	DrawDispose(pagePanel);
	while (padsButtonsHeld() & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
}
