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

#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "dolphin/os.h"

extern OSAlarm bba_alarm;
extern OSAlarm read_alarm;

void perform_read(uint32_t address, uint32_t length, uint32_t offset);
void trickle_read(void);
bool change_disc(void);

bool dtk_fill_buffer(void);

void di_complete_transfer(void);
void di_defer_transfer(uint32_t offset, uint32_t length);

#endif /* EMULATOR_H */
