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

#include <ogc/system.h>
#include <stdbool.h>
#include "deviceHandler.h"
#include "sram.h"
#include "swiss.h"

static s32 onreset(s32 final)
{
	if (!final) updateSRAM(&swissSettings, false);
	return TRUE;
}

static sys_resetinfo resetinfo = {
	{NULL, NULL}, onreset, 0
};

__attribute((constructor))
static void initSRAM(void)
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

	SYS_RegisterResetFunc(&resetinfo);
}

void refreshSRAM(SwissSettings *settings)
{
	settings->sramHOffset = SYS_GetDisplayOffsetH();
	settings->sramBoot = SYS_GetBootMode();
	settings->sram60Hz = SYS_GetEuRGB60();
	settings->sramLanguage = SYS_GetLanguage();
	if (settings->sramLanguage > SYS_LANG_ENGLISH_US)
		settings->sramLanguage = SYS_LANG_ENGLISH;
	settings->sramProgressive = SYS_GetProgressiveScan();
	settings->sramStereo = SYS_GetSoundMode();
	settings->sramVideo = SYS_GetVideoMode();

	syssramex *sramex = __SYS_LockSramEx();

	settings->configDeviceId = sramex->__padding0;
	if (settings->configDeviceId > DEVICE_ID_MAX || !(getDeviceByUniqueId(settings->configDeviceId)->features & FEAT_CONFIG_DEVICE))
		settings->configDeviceId = DEVICE_ID_UNK;

	__SYS_UnlockSramEx(FALSE);
}

void updateSRAM(SwissSettings *settings, bool syncSram)
{
	SYS_SetDisplayOffsetH(settings->sramHOffset);
	SYS_SetBootMode(settings->sramBoot);
	SYS_SetEuRGB60(settings->sram60Hz);
	SYS_SetLanguage(settings->sramLanguage);
	SYS_SetProgressiveScan(settings->sramProgressive);
	SYS_SetSoundMode(settings->sramStereo);
	SYS_SetVideoMode(settings->sramVideo);

	bool writeSram = false;
	syssramex *sramex = __SYS_LockSramEx();

	if (sramex->__padding0 != settings->configDeviceId) {
		sramex->__padding0 = settings->configDeviceId;
		writeSram = true;
	}

	__SYS_UnlockSramEx(writeSram);
	while (syncSram && !__SYS_SyncSram());
}
