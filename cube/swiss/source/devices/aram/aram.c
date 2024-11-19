/* 
 * Copyright (c) 2023-2024, Extrems <extrems@extremscorner.org>
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

#include <stdbool.h>
#include <ogc/aram.h>
#include <ogc/arqueue.h>
#include <ogc/cache.h>
#define LIBOGC_INTERNAL
#include <ogc/disc_io.h>
#include "aram.h"

static bool __aram_Startup(DISC_INTERFACE *disc)
{
	static bool initialized;

	if (initialized) return true;

	if (!AR_CheckInit()) {
		AR_Init(NULL, 0);
		AR_Reset();
	}

	if (!ARQ_CheckInit()) {
		ARQ_Init();
		ARQ_Reset();
	}

	initialized = true;
	return true;
}

static bool __aram_IsInserted(DISC_INTERFACE *disc)
{
	disc->numberOfSectors = (AR_GetSize() - AR_GetInternalSize()) / disc->bytesPerSector;

	return !!disc->numberOfSectors;
}

static bool __aram_ReadSectors(DISC_INTERFACE *disc, sec_t sector, sec_t numSectors, void *buffer)
{
	ARQRequest req;

	if (sector & ~0xFFFF) return false;
	if (numSectors & ~0x1FFFF) return false;
	if ((u32)buffer & 0x1F) return false;

	DCInvalidateRange(buffer, numSectors << 9);
	ARQ_PostRequest(&req, sector, ARQ_ARAMTOMRAM, ARQ_PRIO_LO, AR_GetInternalSize() + (sector << 9), (u32)buffer, numSectors << 9);

	return true;
}

static bool __aram_WriteSectors(DISC_INTERFACE *disc, sec_t sector, sec_t numSectors, const void *buffer)
{
	ARQRequest req;

	if (sector & ~0xFFFF) return false;
	if (numSectors & ~0x1FFFF) return false;
	if ((u32)buffer & 0x1F) return false;

	DCFlushRange((void *)buffer, numSectors << 9);
	ARQ_PostRequest(&req, sector, ARQ_MRAMTOARAM, ARQ_PRIO_LO, AR_GetInternalSize() + (sector << 9), (u32)buffer, numSectors << 9);

	return true;
}

static bool __aram_ClearStatus(DISC_INTERFACE *disc)
{
	return true;
}

static bool __aram_Shutdown(DISC_INTERFACE *disc)
{
	return true;
}

DISC_INTERFACE __io_aram = {
	DEVICE_TYPE_GC_ARAM,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
	__aram_Startup,
	__aram_IsInserted,
	__aram_ReadSectors,
	__aram_WriteSectors,
	__aram_ClearStatus,
	__aram_Shutdown,
	0,
	512
};
