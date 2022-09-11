/* 
 * Copyright (c) 2022, Extrems <extrems@extremscorner.org>
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
#include <stdint.h>
#include <string.h>
#include <ogc/si.h>
#include "gameid.h"
#include "gcm.h"

static uint8_t command[1 + 10] = {0x1D};

static void callback(s32 chan, u32 type)
{
}

static s32 onreset(s32 final)
{
	static bool sent = false;

	if (!final) {
		if (!sent) {
			sent = SI_Transfer(SI_CHAN0, &command, sizeof(command), NULL, 0, callback, 0);
			return FALSE;
		} else if (SI_Busy())
			return FALSE;
	}

	return TRUE;
}

static sys_resetinfo resetinfo = {
	{NULL, NULL}, onreset, 0
};

__attribute((constructor))
static void gameID_init(void)
{
	while (SI_Busy());

	if (SI_Transfer(SI_CHAN0, &command, sizeof(command), NULL, 0, NULL, 0))
		SI_Sync();

	SYS_RegisterResetFunc(&resetinfo);
}

__attribute((destructor))
static void gameID_deinit(void)
{
	SYS_UnregisterResetFunc(&resetinfo);
}

void gameID_set(const DiskHeader *header, uint64_t hash)
{
	memcpy(&command[1], &hash, 8);

	if (header) {
		command[ 9] = header->ConsoleID;
		command[10] = header->CountryCode;
	} else {
		command[ 9] = 
		command[10] = '\0';
	}
}

void gameID_unset(void)
{
	memset(&command[1], 0, 10);
}
