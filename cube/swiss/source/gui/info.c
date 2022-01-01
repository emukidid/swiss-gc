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
#include "gcloader.h"
#include "wkf.h"

char topStr[256];

uiDrawObj_t * info_draw_page(int page_num) {
	uiDrawObj_t *container = DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 420);
	
	// System Info (Page 1/3)
	if(!page_num) {
		DrawAddChild(container, DrawLabel(30, 55, "System Info (1/3):"));
		// Model
		DrawAddChild(container, DrawStyledLabel(640/2, 90, (char*)"MODEL", 0.65f, true, defaultColor));
		if(mfpvr() == GC_CPU_VERSION01) {
			if(*(u16*)&driveVersion[2] == 0x0201) {
				strcpy(topStr, "NPDP-GDEV (GCT-0100)");
			}
			else if(!strncmp(&IPLInfo[0x55], "TDEV Revision 1.1", 17)) {
				strcpy(topStr, "Nintendo GameCube DOT-006");
			}
			else if(*(u16*)&driveVersion[2] == 0x0200) {
				if(!strncmp(&IPLInfo[0x55], "PAL  Revision 1.0", 17)) {
					strcpy(topStr, "Nintendo GameCube DOT-002P");
				}
				else {
					strcpy(topStr, "Nintendo GameCube DOT-002");
				}
			}
			else if(*(u16*)&driveVersion[2] == 0x0001) {
				if(!strncmp(&IPLInfo[0x55], "PAL  Revision 1.0", 17)) {
					strcpy(topStr, "Nintendo GameCube DOT-001P");
				}
				else {
					strcpy(topStr, "Nintendo GameCube DOT-001");
				}
			}
			else if(*(u16*)&driveVersion[2] == 0x0000 && driveVersion[9] == 'M') {
				strcpy(topStr, "Panasonic Q SL-GC10-S");
			}
			else if(!strncmp(&IPLInfo[0x55], "MPAL Revision 1.1", 17)) {
				strcpy(topStr, "Nintendo GameCube DOL-002 (BRA)");
			}
			else {
				strcpy(topStr, "Nintendo GameCube DOL-001");
			}
		}
		else if(mfpvr() == GC_CPU_VERSION02) {
			strcpy(topStr, "Nintendo GameCube DOL-101");
		}
		else {
			strcpy(topStr, "Nintendo Wii");
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 106, topStr, 0.75f, true, defaultColor));
		// IPL version string
		DrawAddChild(container, DrawStyledLabel(640/2, 130, (char*)"IPL VERSION", 0.65f, true, defaultColor));
		if(is_gamecube()) {
			if(!IPLInfo[0x55]) {
				strcpy(topStr, "NTSC Revision 1.0");
			}
			else {
				strcpy(topStr, &IPLInfo[0x55]);
			}
		}
		else {
			strcpy(topStr, "Wii");
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 146, topStr, 0.75f, true, defaultColor));
		
		DrawAddChild(container, DrawStyledLabel(640/2, 170, (char*)"VIDEO MODE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 186, getVideoModeString(), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 210, (char*)"AUDIO", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 226, (char*)(swissSettings.sramStereo ? "Stereo" : "Mono"), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 250, (char*)"LANGUAGE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 266, (char*)(swissSettings.sramLanguage > SRAM_LANG_MAX ? "Unknown" : sramLang[swissSettings.sramLanguage]), 0.75f, true, defaultColor));

		// GC 00083214, 00083410
		DrawAddChild(container, DrawStyledLabel(640/2, 290, (char*)"CPU PVR", 0.65f, true, defaultColor));
		sprintf(topStr,"%08X",mfpvr());
		DrawAddChild(container, DrawStyledLabel(640/2, 306, topStr, 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 330, (char*)"CPU ECID", 0.65f, true, defaultColor));
		sprintf(topStr,"%08X:%08X:%08X:%08X",mfspr(0x39C),mfspr(0x39D),mfspr(0x39E),mfspr(0x39F));
		DrawAddChild(container, DrawStyledLabel(640/2, 346, topStr, 0.75f, true, defaultColor));
	}
	else if(page_num == 1) {
		DrawAddChild(container, DrawLabel(30, 55, "Device Info (2/3):"));
		
		DrawAddChild(container, DrawStyledLabel(640/2, 90, (char*)"SLOT-A", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 106, getHwNameByLocation(LOC_MEMCARD_SLOT_A), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 130, (char*)"SLOT-B", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 146, getHwNameByLocation(LOC_MEMCARD_SLOT_B), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 170, (char*)"SERIAL PORT 1", 0.65f, true, defaultColor));
		if(bba_exists) {
			sprintf(topStr, "%s (%s)", getHwNameByLocation(LOC_SERIAL_PORT_1), !net_initialized ? "Not initialized" : bba_ip);
		}
		else {
			strcpy(topStr, getHwNameByLocation(LOC_SERIAL_PORT_1));
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 186, topStr, 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 210, (char*)"SERIAL PORT 2", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 226, getHwNameByLocation(LOC_SERIAL_PORT_2), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 250, (char*)"DRIVE INTERFACE", 0.65f, true, defaultColor));
		
		DEVICEHANDLER_INTERFACE *device = getDeviceByLocation(LOC_DVD_CONNECTOR);
		if(device == &__device_dvd) {
			sprintf(topStr, "%s %02X %02X%02X/%02X (%02X)",device->hwName,driveVersion[6],driveVersion[4],driveVersion[5],driveVersion[7],driveVersion[8]);
		}
		else if(device == &__device_gcloader) {
			char* gcloaderVersionStr = gcloaderGetVersion();
			sprintf(topStr, "%s %s",device->hwName,gcloaderVersionStr);
			free(gcloaderVersionStr);
		}
		else if(device == &__device_wkf) {
			sprintf(topStr, "%s (%s)",device->hwName,wkfGetSerial());
		}
		else {
			strcpy(topStr, getHwNameByLocation(LOC_DVD_CONNECTOR));
		}
		DrawAddChild(container, DrawStyledLabel(640/2, 266, topStr, 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 290, (char*)"PROGRESSIVE VIDEO", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 306, (char*)(getDTVStatus() ? "Yes" : "No"), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 330, (char*)"CURRENT DEVICE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 346, (char*)(devices[DEVICE_CUR] != NULL ? devices[DEVICE_CUR]->deviceName : "None"), 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 370, (char*)"CONFIGURATION DEVICE", 0.65f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 386, (char*)(devices[DEVICE_CONFIG] != NULL ? devices[DEVICE_CONFIG]->deviceName : "None"), 0.75f, true, defaultColor));
	}
	else if(page_num == 2) {
		DrawAddChild(container, DrawLabel(30, 55, "Credits (3/3):"));
		DrawAddChild(container, DrawStyledLabel(640/2, 115, "Swiss version 0.6", 1.0f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 140, "by emu_kidid & Extrems, 2022", 0.75f, true, defaultColor));
		sprintf(txtbuffer, "Commit %s Revision %s", GITREVISION, GITVERSION);
		DrawAddChild(container, DrawStyledLabel(640/2, 165, txtbuffer, 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 208, "Patreon supporters", 0.75f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 228, "meneerbeer, Dan Kunz, Heather Kent, Joshua Witt, Filyx20, SubElement, KirovAir,", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 244, "Cristofer Cruz, LemonMeringueTy, badsector, Fernando Avelino, RamblingOkie,", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 260, "Kory, Lindh0lm154, Alex Mitchell, Haymose, finnyguy, Marlon,", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 276, "HakanaiSeishin, Borg Number One (a.k.a. Steven Weiser)", 0.60f, true, defaultColor));
		
		DrawAddChild(container, DrawStyledLabel(640/2, 300, "Extra Greetz: FIX94, megalomaniac, sepp256, novenary", 0.60f, true, defaultColor));
		DrawAddChild(container, DrawStyledLabel(110, 320, "Web/Support", 0.64f, false, defaultColor));
		DrawAddChild(container, DrawStyledLabel(410, 320, "Source/Updates", 0.64f, false, defaultColor));
		DrawAddChild(container, DrawStyledLabel(85, 336, "www.gc-forever.com", 0.64f, false, defaultColor));
		DrawAddChild(container, DrawStyledLabel(355, 336, "github.com/emukidid/swiss-gc", 0.64f, false, defaultColor));
		DrawAddChild(container, DrawStyledLabel(640/2, 378, "Visit us on IRC at EFNet/#gc-forever", 0.75f, true, defaultColor));
	}
	if(page_num != 2) {
		DrawAddChild(container, DrawLabel(520, 400, "->"));
	}
	if(page_num != 0) {
		DrawAddChild(container, DrawLabel(100, 400, "<-"));
	}
	DrawAddChild(container, DrawStyledLabel(640/2, 410, "Press A or B to return", 1.0f, true, defaultColor));
	return container;
}

void show_info() {
	int page = 0;
	uiDrawObj_t* pagePanel = NULL;
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	while(1) {
		uiDrawObj_t* newPanel = info_draw_page(page);
		if(pagePanel != NULL) {
			DrawDispose(pagePanel);
		}
		pagePanel = newPanel;
		DrawPublish(pagePanel);
		while (!((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B)
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if(((btns & PAD_BUTTON_RIGHT) || (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)) && page < 2)
			page++;
		if(((btns & PAD_BUTTON_LEFT) || (PAD_ButtonsHeld(0) & PAD_TRIGGER_L)) && page > 0)
			page--;
		if((btns & PAD_BUTTON_A) || (btns & PAD_BUTTON_B))
			break;
		while ((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B)
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L))
			{ VIDEO_WaitVSync (); }
	}
	DrawDispose(pagePanel);
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
}
