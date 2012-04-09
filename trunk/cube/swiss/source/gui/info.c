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
#include "ata.h"
#include "exi.h"
#include "bba.h"

char topStr[256];

char *getSramLang(u8 lang) {
	switch(lang) {
		case 5:
			return "Dutch";
		case 4:
			return "Italian";
		case 3:
			return "Spanish";
		case 2:
			return "French";
		case 1:
			return "German";
		case 0:
			return "English";
	}
	return "Unknown";
}

void info_draw_page(int page_num) {
	doBackdrop();
	DrawEmptyBox(20,60, vmode->fbWidth-20, 420, COLOR_BLACK);
	syssram* sram = __SYS_LockSram();
	__SYS_UnlockSram(0);
	
	// System Info (Page 1/3)
	if(!page_num) {
		WriteFont(30, 65, "System Info (1/3):");
		// Model
		if(is_gamecube()) {
			if(*(u32*)&driveVersion[0] == 0x20010831) {
				sprintf(topStr, "Model: Panasonic Q SL-GC10-S");
			}
			else if(!IPLInfo[0x55] || IPLInfo[0x65]=='0' || (IPLInfo[0x65]=='1')) {	
				// NTSC 1.0,1.1 or PAL 1.0
				sprintf(topStr, "Model: Nintendo Gamecube DOL-001");
			}
			else if(IPLInfo[0x65]=='2') {
				// NTSC 1.2a/1.2b	- How can I detect PAL DOL-101's.. by drive too?
				sprintf(topStr, "Model: Nintendo Gamecube DOL-101");
			}
			else if(IPLInfo[0x65]=='3') {
				// Mythical NTSC 1.3 (Brazil)
				sprintf(topStr, "Model: Nintendo Gamecube DOL-102");	// Exists only in Brazil?
			}
		}
		else {
			sprintf(topStr, "Model: Nintendo Wii");
		}
		WriteFont(30, 115, topStr);
		// IPL version string
		if(is_gamecube()) {
			if(!IPLInfo[0x55]) {
				sprintf(topStr, "IPL: NTSC Revision 1.0");
			}
			else {
				sprintf(topStr, "IPL: %s", &IPLInfo[0x55]);
			}
		}
		else {
			sprintf(topStr, "IPL: Wii IPL");
		}
		WriteFont(30, 150, topStr);
		sprintf(topStr, "DVD: %02X %02X%02X/%02X (%02X)",driveVersion[2],driveVersion[0],driveVersion[1],driveVersion[3],driveVersion[4]);
		WriteFont(30, 185, topStr);
		sprintf(topStr, "Video: %s",videoStr);
		WriteFont(30, 220, topStr);
		sprintf(topStr,"ECID: %08X:%08X:%08X",mfspr(0x39C),mfspr(0x39D),mfspr(0x39E));
		WriteFontStyled(30, 260, topStr, 0.7f, false, defaultColor);
		sprintf(topStr,"PVR: %08X",mfpvr());
		WriteFontStyled(400, 260, topStr, 0.7f, false, defaultColor);
		sprintf(topStr,"Language: %s",getSramLang(sram->lang));
		WriteFont(30, 295, topStr);
		sprintf(topStr,"Audio: %s",sram->flags&4 ? "Stereo":"Mono");
		WriteFont(30, 330, topStr);
	}
	else if(page_num == 1) {
		WriteFont(30, 65, "Device Info (2/3):");
		sprintf(topStr,"BBA: %s", bba_exists ? "Installed":"Not Present");
		WriteFont(30, 115, topStr);
		if(exi_bba_exists()) {
			sprintf(topStr,"IP: %s", net_initialized ? bba_ip:"Not Available");
		}
		else {
			sprintf(topStr,"IP: Not Available");
		}
		WriteFont(30, 150, topStr);
		sprintf(topStr,"Component Cable Plugged in: %s",VIDEO_HaveComponentCable()?"Yes":"No");
		WriteFont(30, 185, topStr);
		if(usb_isgeckoalive(0)||usb_isgeckoalive(1)) {
			sprintf(topStr,"USB Gecko: Installed in %s",usb_isgeckoalive(0)?"Slot A":"Slot B");
		}
		else {
			sprintf(topStr,"USB Gecko: Not Present");
		}
		WriteFont(30, 220, topStr);
		if (!deviceHandler_initial) {
			sprintf(topStr, "Current Device: No Device Selected");
		}
		else if(deviceHandler_initial == &initial_SD0 || deviceHandler_initial == &initial_SD1) {
			int slot = (deviceHandler_initial->name[2] == 'b');
			sprintf(topStr, "Current Device: %s Card in %s @ %s",!SDHCCard?"SDHC":"SD",!slot?"Slot A":"Slot B",!swissSettings.exiSpeed?"16Mhz":"32Mhz");
		}
		else if(deviceHandler_initial == &initial_DVD) {
			sprintf(topStr, "Current Device: %s DVD Disc",dvdDiscTypeStr);
		}
		else if(deviceHandler_initial == &initial_IDE0 || deviceHandler_initial == &initial_IDE1) {
			int slot = (deviceHandler_initial->name[3] == 'b');
			sprintf(topStr, "Current Device: IDE-EXI Device (%d GB HDD) in %s",ataDriveInfo.sizeInGigaBytes,!slot?"Slot A":"Slot B");
		}
		else if(deviceHandler_initial == &initial_Qoob) {
			sprintf(topStr, "Current Device: Qoob IPL Replacement");
		}
		else if(deviceHandler_initial == &initial_WODE) {
			sprintf(topStr, "Current Device: Wode Jukebox");
		}
		else if(deviceHandler_initial == &initial_CARDA || deviceHandler_initial == &initial_CARDB) {
			sprintf(topStr, "Current Device: Memory Card in %s",!deviceHandler_initial->fileBase?"Slot A":"Slot B");
		}
		WriteFont(30, 295, topStr);
	}
	else if(page_num == 2) {
		WriteFont(30, 65, "Credits (3/3):");
		WriteFontStyled(640/2, 115, "Swiss ver 0.2", 1.0f, true, defaultColor);
		WriteFontStyled(640/2, 140, "by emu_kidid 2012", 0.75f, true, defaultColor);
		WriteFontStyled(640/2, 210, "Thanks to", 0.75f, true, defaultColor);
		WriteFontStyled(640/2, 228, "Testers & libOGC/dkPPC authors", 0.75f, true, defaultColor);
		WriteFontStyled(640/2, 246, "sepp256 for the wonderful GX conversion", 0.75f, true, defaultColor);
		WriteFontStyled(640/2, 300, "Web/Support http://www.gc-forever.com/", 0.75f, true, defaultColor);
		WriteFontStyled(640/2, 318, "Source at http://code.google.com/p/swiss-gc/", 0.75f, true, defaultColor);
		WriteFontStyled(640/2, 370, "Press A to return", 1.0f, true, defaultColor);
	}
	if(page_num != 2) {
		WriteFont(520, 390, "->");
	}
	if(page_num != 0) {
		WriteFont(100, 390, "<-");
	}
	DrawFrameFinish();
}

void show_info() {
	int page = 0;
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A);
	while(1) {
		info_draw_page(page);
		while (!((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B)
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L)));
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
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L));
	}
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A);
}
