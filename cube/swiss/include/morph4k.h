/* 
 * Copyright (c) 2025, Extrems <extrems@extremscorner.org>, webhdx <webhdx@gmail.com>
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

#ifndef __MORPH4K_H__
#define __MORPH4K_H__

#include <stdbool.h>

#include "gcm.h"

bool morph4k_reset_gameid(void);
bool morph4k_send_gameid(const DiskHeader * header, uint64_t hash);
bool morph4k_apply_preset(char *preset_path);
bool is_morph4k_alive(void);
bool morph4k_init(void);
void morph4k_deinit(void);

#endif /* __MORPH4K_H__ */ 
