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

#ifndef __MCP_H__
#define __MCP_H__

#include <gctypes.h>
#include <ogc/dvd.h>

#define MCP_RESULT_READY        0
#define MCP_RESULT_BUSY        -1
#define MCP_RESULT_WRONGDEVICE -2
#define MCP_RESULT_NOCARD      -3
#define MCP_RESULT_FATAL_ERROR -128

#ifdef __cplusplus
extern "C" {
#endif

s32 MCP_ProbeEx(s32 chan);
s32 MCP_GetDeviceID(s32 chan, u32 *id);
s32 MCP_SetDiskID(s32 chan, const dvddiskid *diskID);
s32 MCP_GetDiskInfo(s32 chan, char diskInfo[64]);
s32 MCP_SetDiskInfo(s32 chan, const char diskInfo[64]);

#ifdef __cplusplus
}
#endif

#endif
