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
#include <string.h>
#include <ogc/card.h>
#include <ogc/exi.h>
#include "mcp.h"

static const char digits[16] = "0123456789ABCDEF";

s32 MCP_ProbeEx(s32 chan)
{
	return CARD_ProbeEx(chan, NULL, NULL);
}

s32 MCP_GetDeviceID(s32 chan, u32 *id)
{
	bool err = false;
	u8 cmd[2];

	if (!EXI_Lock(chan, EXI_DEVICE_0, NULL)) return MCP_RESULT_BUSY;
	if (!EXI_Select(chan, EXI_DEVICE_0, EXI_SPEED16MHZ)) {
		EXI_Unlock(chan);
		return MCP_RESULT_NOCARD;
	}

	cmd[0] = 0x8B;
	cmd[1] = 0x00;

	err |= !EXI_Imm(chan, cmd, 2, EXI_WRITE, NULL);
	err |= !EXI_Sync(chan);
	err |= !EXI_Imm(chan, id, 4, EXI_READ, NULL);
	err |= !EXI_Sync(chan);
	err |= !EXI_Deselect(chan);
	EXI_Unlock(chan);

	if (err)
		return MCP_RESULT_NOCARD;
	else if (*id >> 16 != 0x3842)
		return MCP_RESULT_WRONGDEVICE;
	else
		return MCP_RESULT_READY;
}

s32 MCP_SetDiskID(s32 chan, const dvddiskid *diskID)
{
	bool err = false;
	u8 cmd[12];

	if (!EXI_Lock(chan, EXI_DEVICE_0, NULL)) return MCP_RESULT_BUSY;
	if (!EXI_Select(chan, EXI_DEVICE_0, EXI_SPEED16MHZ)) {
		EXI_Unlock(chan);
		return MCP_RESULT_NOCARD;
	}

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x8B;
	cmd[1] = 0x11;

	if (diskID) {
		memcpy(&cmd[2], diskID->gamename, 4);
		memcpy(&cmd[6], diskID->company,  2);
		cmd[8]  = digits[diskID->disknum / 16];
		cmd[9]  = digits[diskID->disknum % 16];
		cmd[10] = digits[diskID->gamever / 16];
		cmd[11] = digits[diskID->gamever % 16];
	}

	err |= !EXI_ImmEx(chan, cmd, sizeof(cmd), EXI_WRITE);
	err |= !EXI_Deselect(chan);
	EXI_Unlock(chan);

	return err ? MCP_RESULT_NOCARD : MCP_RESULT_READY;
}

s32 MCP_GetDiskInfo(s32 chan, char diskInfo[64])
{
	bool err = false;
	u8 cmd[2];

	if (!EXI_Lock(chan, EXI_DEVICE_0, NULL)) return MCP_RESULT_BUSY;
	if (!EXI_Select(chan, EXI_DEVICE_0, EXI_SPEED16MHZ)) {
		EXI_Unlock(chan);
		return MCP_RESULT_NOCARD;
	}

	cmd[0] = 0x8B;
	cmd[1] = 0x12;

	err |= !EXI_Imm(chan, cmd, 2, EXI_WRITE, NULL);
	err |= !EXI_Sync(chan);
	err |= !EXI_ImmEx(chan, diskInfo, 64, EXI_READ);
	err |= !EXI_Deselect(chan);
	EXI_Unlock(chan);

	return err ? MCP_RESULT_NOCARD : MCP_RESULT_READY;
}

s32 MCP_SetDiskInfo(s32 chan, const char diskInfo[64])
{
	bool err = false;
	u8 cmd[2];

	if (!EXI_Lock(chan, EXI_DEVICE_0, NULL)) return MCP_RESULT_BUSY;
	if (!EXI_Select(chan, EXI_DEVICE_0, EXI_SPEED16MHZ)) {
		EXI_Unlock(chan);
		return MCP_RESULT_NOCARD;
	}

	cmd[0] = 0x8B;
	cmd[1] = 0x13;

	err |= !EXI_Imm(chan, cmd, 2, EXI_WRITE, NULL);
	err |= !EXI_Sync(chan);
	err |= !EXI_ImmEx(chan, (char *)diskInfo, 64, EXI_WRITE);
	err |= !EXI_Deselect(chan);
	EXI_Unlock(chan);

	return err ? MCP_RESULT_NOCARD : MCP_RESULT_READY;
}
