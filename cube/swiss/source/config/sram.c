/* 
 * Copyright (c) 2024-2026, Extrems <extrems@extremscorner.org>
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
#include <ogc/timesupp.h>
#include <stdbool.h>
#include <stdckdint.h>
#include <stdlib.h>
#include <time.h>
#include "deviceHandler.h"
#include "sram.h"
#include "swiss.h"
#include "tzdata.h"

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

	u32 epoch, rtc;
	s32 bias = SYS_GetCounterBias();

	if (!ckd_sub(&epoch, GIT_TIMESTAMP, DIFF_SEC_1970_2000) && __SYS_GetRTC(&rtc)) {
		if (rtc < epoch) {
			__SYS_SetRTC(epoch);
			__SYS_SetPosixTime(secs_to_ticks(GIT_TIMESTAMP));
			__SYS_SetTime(secs_to_ticks(epoch));
			SYS_SetCounterBias(0);
		} else if (ckd_add(&rtc, rtc, bias)) {
			if (ckd_sub(&bias, rtc, epoch)) {
				__SYS_SetRTC(epoch);
				__SYS_SetPosixTime(secs_to_ticks(GIT_TIMESTAMP));
				__SYS_SetTime(secs_to_ticks(epoch));
				SYS_SetCounterBias(0);
			} else {
				__SYS_SetRTC(epoch);
				SYS_SetCounterBias(bias);
			}
		}
	}

	SYS_RegisterResetFunc(&resetinfo);
}

void refreshSRAM(SwissSettings *settings)
{
	settings->sramHOffset = SYS_GetDisplayOffsetH();
	settings->sramBoot = SYS_GetBootMode();
	settings->sram60Hz = SYS_GetEuRGB60();
	settings->sramTemperature = SYS_GetTAUCalibration();
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
	time_t now;
	struct tm tm;

	if (getenv("TZ")) {
		time(&now);
		gmtime_r(&now, &tm);
		tm.tm_isdst = -1;

		SYS_SetCounterBias(now - mktime(&tm));
	}

	SYS_SetDisplayOffsetH(settings->sramHOffset);
	SYS_SetBootMode(settings->sramBoot);
	SYS_SetEuRGB60(settings->sram60Hz);
	SYS_SetTAUCalibration(settings->sramTemperature);
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

void updateRTC(time_t now)
{
	u32 rtc = now - DIFF_SEC_1970_2000;

	now = (time_t)(getenv("TZ") ? rtc : rtc + SYS_GetCounterBias()) + DIFF_SEC_1970_2000;

	__SYS_SetRTC(rtc);
	__SYS_SetPosixTime(secs_to_ticks(now));
}

void updateTZ(const char *name)
{
	const char *tzenv = get_timezone(name);
	int adjust = 0;

	if (getenv("TZ")) {
		if (tzenv) {
			setenv("TZ", tzenv, 1);
			tzset();
		} else {
			unsetenv("TZ");
			tzset();
			adjust = 1;
		}
	} else if (tzenv) {
		setenv("TZ", tzenv, 0);
		tzset();
		adjust = -1;
	}

	if (adjust) {
		u32 sec0 = ticks_to_secs(gettime());
		u32 sec1 = sec0 + SYS_GetCounterBias() * adjust;

		s64 tick0 = secs_to_ticks(sec0);
		s64 tick1 = secs_to_ticks(sec1);

		__SYS_AdjustPosixTime(tick1 - tick0);
	}
}
