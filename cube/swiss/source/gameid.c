/* 
 * Copyright (c) 2022-2023, Extrems <extrems@extremscorner.org>
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
#include <string.h>
#include <ogc/si.h>
#include "gameid.h"
#include "gcm.h"
#include "mcp.h"

static u8 command[1 + 10] = {0x1D};

static void callback(s32 chan, u32 type)
{
}

static s32 onreset(s32 final)
{
	static s32 chan = SI_CHAN0;

	if (!final) {
		if (chan < SI_MAX_CHAN) {
			if (SI_Transfer(chan, &command, sizeof(command), NULL, 0, callback, 0))
				chan++;
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
	s32 chan = SI_CHAN0;

	while (chan < SI_MAX_CHAN)
		if (SI_Transfer(chan, &command, sizeof(command), NULL, 0, callback, 0))
			chan++;
	while (SI_Busy());

	SYS_RegisterResetFunc(&resetinfo);
}

__attribute((destructor))
static void gameID_deinit(void)
{
	SYS_UnregisterResetFunc(&resetinfo);
}

void gameID_early_set(const DiskHeader *header)
{
	for (s32 chan = 0; chan < 2; chan++) {
		u32 id;
		s32 ret;

		while ((ret = MCP_ProbeEx(chan)) == MCP_RESULT_BUSY);
		if (ret < 0) continue;
		while ((ret = MCP_GetDeviceID(chan, &id)) == MCP_RESULT_BUSY);
		if (ret < 0) continue;
		while ((ret = MCP_SetDiskID(chan, (dvddiskid *)header)) == MCP_RESULT_BUSY);
		if (ret < 0) continue;
		while ((ret = MCP_SetDiskInfo(chan, header->GameName)) == MCP_RESULT_BUSY);
		if (ret < 0) continue;
	}
}

void gameID_set(const DiskHeader *header, u64 hash)
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
