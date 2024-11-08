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

#ifndef __ARAM_H__
#define __ARAM_H__

#include <ogc/disc_io.h>
#include "ff.h"
#include "diskio.h"

DRESULT ARAM_ioctl(BYTE ctrl, void *buff);

#define DEVICE_TYPE_GC_ARAM (('G'<<24)|('C'<<16)|('A'<<8)|'R')

extern DISC_INTERFACE __io_aram;

#endif
