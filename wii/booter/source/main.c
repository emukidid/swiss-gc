/* 
 * Copyright (c) 2024-2025, Extrems <extrems@extremscorner.org>
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

#include <argz.h>
#include <gctypes.h>
#include <ogc/cache.h>
#include <ogc/conf.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include <ogc/wiilaunch.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dolformat.h"

static DOLImage *const dolHeader = (DOLImage *)0x80800000;
static void *const dolImage = dolHeader;
static char *const dolMagic = dolImage - 32;
void *__myArena1Hi = dolMagic;

extern syssram *__SYS_LockSram(void);
extern bool __SYS_UnlockSram(bool write);
extern bool __SYS_SyncSram(void);
extern bool __SYS_CheckSram(void);

static void initVideo(void)
{
	setenv("AVE", "AVE-RVL", 0);
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
	bool euRGB60 = SYS_GetEuRGB60();
	uint8_t language = SYS_GetLanguage();
	bool progressiveScan = SYS_GetProgressiveScan();
	uint8_t soundMode = SYS_GetSoundMode();
	uint8_t videoMode = SYS_GetVideoMode();

	CONF_GetCounterBias(&counterBias);
	CONF_GetDisplayOffsetH(&displayOffsetH);

	euRGB60 = CONF_GetEuRGB60() > CONF_ERR_OK;

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

	progressiveScan = CONF_GetProgressiveScan() > CONF_ERR_OK;

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
	SYS_SetEuRGB60(euRGB60);
	SYS_SetLanguage(language);
	SYS_SetProgressiveScan(progressiveScan);
	SYS_SetSoundMode(soundMode);
	SYS_SetVideoMode(videoMode);
}

int main(int argc, char **argv)
{
	char *argz, *envz;
	size_t argzlen, envzlen;

	if (strncmp(dolMagic, "gchomebrew dol", 32))
		return EXIT_FAILURE;

	initVideo();
	initSram();

	argz_create(argv, &argz, &argzlen);
	argz_create(environ, &envz, &envzlen);

	envz = memcpy(SYS_AllocArenaMem1Hi(envzlen, 1), envz, envzlen);
	argz = memcpy(SYS_AllocArenaMem1Hi(argzlen, 1), argz, argzlen);

	for (int i = 0; i < DOL_MAX_TEXT; i++) {
		if (dolHeader->textData[i]) {
			uint32_t *text = dolImage + dolHeader->textData[i];

			if (text[1] == ARGV_MAGIC) {
				struct __argv *argv = (struct __argv *)(text + 2);

				argv->argvMagic = ARGV_MAGIC;
				argv->commandLine = argz;
				argv->length = argzlen;

				DCBlockStore(argv);
				DCStoreRange(argz, argzlen);
			}

			if (text[9] == ENVP_MAGIC) {
				struct __argv *envp = (struct __argv *)(text + 10);

				envp->argvMagic = ENVP_MAGIC;
				envp->commandLine = envz;
				envp->length = envzlen;

				DCBlockStore(envp);
				DCStoreRange(envz, envzlen);
			}
		}
	}

	WII_LaunchTitle(0x100000100LL);

	return EXIT_SUCCESS;
}
