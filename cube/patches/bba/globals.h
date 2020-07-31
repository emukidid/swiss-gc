/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "../../reservedarea.h"

static uint16_t *const _port      = (uint16_t *)VAR_SERVER_PORT;
static uint16_t *const _key       = (uint16_t *)VAR_FSP_KEY;
static uint8_t  *const _filelen   = (uint8_t  *)VAR_DISC_1_FNLEN;
static char     *const _file      = (char     *)VAR_DISC_1_FN;
static uint8_t  *const _file2len  = (uint8_t  *)VAR_DISC_2_FNLEN;
static char     *const _file2     = (char     *)VAR_DISC_2_FN;

#endif /* GLOBALS_H */
