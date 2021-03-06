/* 
 * Copyright (c) 2019-2021, Extrems <extrems@extremscorner.org>
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

#define COMMAND_LATENCY_TICKS OSMicrosecondsToTicks(300)

extern OSAlarm bba_alarm;
extern OSAlarm di_alarm;
extern OSAlarm read_alarm;

typedef struct {
	uint32_t gpr[32];
	uint32_t cr, lr, ctr, xer;
	uint32_t srr0, srr1;
} ppc_context_t;

void perform_read(uint32_t address, uint32_t length, uint32_t offset);
void trickle_read(void);
bool change_disc(void);

void exi_interrupt(unsigned chan);
void exi_complete_transfer(unsigned chan);
void exi0_complete_transfer();
void exi1_complete_transfer();
void exi2_complete_transfer();
void exi_insert_device(unsigned chan);
void exi_remove_device(unsigned chan);

bool dtk_fill_buffer(void);

void di_error(uint32_t error);
void di_complete_transfer(void);
void di_open_cover(void);
void di_close_cover(void);

void dvd_schedule_read(uint32_t offset, uint32_t length, OSAlarmHandler handler);

#endif /* EMULATOR_H */
