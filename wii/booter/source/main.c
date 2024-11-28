/* 
 * Copyright (c) 2024, Extrems <extrems@extremscorner.org>
 * 
 * This file is part of Swiss.
 * 
 * Swiss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Swiss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * with Swiss.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ogc/conf.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include <ogc/wiilaunch.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void *__myArena1Hi = (void *)0x807FFFE0;

extern syssram *__SYS_LockSram(void);
extern bool __SYS_UnlockSram(bool write);
extern bool __SYS_SyncSram(void);
extern bool __SYS_CheckSram(void);

static void initVideo(void)
{
	VIDEO_Init();

	unsigned tvMode = VIDEO_GetCurrentTvMode();
	unsigned scanMode = VIDEO_GetScanMode();

	switch (CONF_GetVideo()) {
		case CONF_VIDEO_NTSC:
			tvMode = VI_NTSC;
			break;
		case CONF_VIDEO_PAL:
			tvMode = VI_PAL;
			break;
		case CONF_VIDEO_MPAL:
			tvMode = VI_MPAL;
			break;
	}

	if (tvMode == VI_PAL && CONF_GetEuRGB60() > CONF_ERR_OK)
		tvMode  = VI_EURGB60;

	if (VIDEO_HaveComponentCable()) {
		if (tvMode != VI_PAL)
			tvMode  = VI_EURGB60;
		if (scanMode != VI_PROGRESSIVE && CONF_GetProgressiveScan() > CONF_ERR_OK)
			scanMode  = VI_PROGRESSIVE;
	}

	GXRModeObj rmode = {
		.viTVMode = VI_TVMODE(tvMode, scanMode)
	};

	VIDEO_Configure(&rmode);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
}

static void initSram(void)
{
	bool writeSram = false;
	syssram *sram = __SYS_LockSram();

	if (!__SYS_CheckSram()) {
		memset(sram, 0, sizeof(syssram));
		sram->flags |= 0x10;
		sram->flags |= 0x04;
		writeSram = true;
	}

	__SYS_UnlockSram(writeSram);
	while (!__SYS_SyncSram());

	uint32_t counterBias = SYS_GetCounterBias();
	int8_t displayOffsetH = SYS_GetDisplayOffsetH();
	uint8_t language = SYS_GetLanguage();
	uint8_t soundMode = SYS_GetSoundMode();
	uint8_t videoMode = SYS_GetVideoMode();

	CONF_GetCounterBias(&counterBias);
	CONF_GetDisplayOffsetH(&displayOffsetH);

	switch (CONF_GetLanguage()) {
		case CONF_LANG_JAPANESE:
		case CONF_LANG_ENGLISH:
			language = SYS_LANG_ENGLISH;
			break;
		case CONF_LANG_GERMAN:
			language = SYS_LANG_GERMAN;
			break;
		case CONF_LANG_FRENCH:
			language = SYS_LANG_FRENCH;
			break;
		case CONF_LANG_SPANISH:
			language = SYS_LANG_SPANISH;
			break;
		case CONF_LANG_ITALIAN:
			language = SYS_LANG_ITALIAN;
			break;
		case CONF_LANG_DUTCH:
			language = SYS_LANG_DUTCH;
			break;
	}

	switch (CONF_GetSoundMode()) {
		case CONF_SOUND_MONO:
			soundMode = SYS_SOUND_MONO;
			break;
		case CONF_SOUND_STEREO:
		case CONF_SOUND_SURROUND:
			soundMode = SYS_SOUND_STEREO;
			break;
	}

	switch (CONF_GetVideo()) {
		case CONF_VIDEO_NTSC:
			videoMode = SYS_VIDEO_NTSC;
			break;
		case CONF_VIDEO_PAL:
			videoMode = SYS_VIDEO_PAL;
			break;
		case CONF_VIDEO_MPAL:
			videoMode = SYS_VIDEO_MPAL;
			break;
	}

	SYS_SetCounterBias(counterBias);
	SYS_SetDisplayOffsetH(displayOffsetH);
	SYS_SetLanguage(language);
	SYS_SetSoundMode(soundMode);
	SYS_SetVideoMode(videoMode);
}

int main(int argc, char **argv)
{
	initVideo();
	initSram();

	WII_LaunchTitle(0x100000100LL);

	return EXIT_SUCCESS;
}
