/* 
 * Copyright (c) 2019-2022, Extrems <extrems@extremscorner.org>
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

extern OSAlarm di_alarm;
extern OSAlarm cover_alarm;
extern OSAlarm read_alarm;

typedef struct {
	uint32_t gpr[32];
	uint32_t cr, lr, ctr, xer;
	uint32_t srr0, srr1;
} ppc_context_t;

void perform_read(uint32_t address, uint32_t length, uint32_t offset);
void trickle_read();
bool change_disc(void);
void reset_device(void);

void exi_interrupt(unsigned chan);
void exi_complete_transfer(unsigned chan);
void exi0_complete_transfer();
void exi1_complete_transfer();
void exi2_complete_transfer();
void exi_insert_device(unsigned chan);
void exi_remove_device(unsigned chan);

bool dtk_fill_buffer(void);

#define DI_CMD_INQUIRY              0x12
#define DI_CMD_READ                 0xA8
#define DI_CMD_SEEK                 0xAB
#define DI_CMD_REQUEST_ERROR        0xE0
#define DI_CMD_AUDIO_STREAM         0xE1
#define DI_CMD_REQUEST_AUDIO_STATUS 0xE2
#define DI_CMD_STOP_MOTOR           0xE3
#define DI_CMD_AUDIO_BUFFER_CONFIG  0xE4

#define DI_CMD_GCODE_READ           0xB2
#define DI_CMD_GCODE_SET_DISC_FRAGS 0xB3
#define DI_CMD_GCODE_WRITE_BUFFER   0xB9
#define DI_CMD_GCODE_WRITE          0xBA

void di_error(uint32_t error);
void di_complete_transfer();
void di_open_cover(void);
void di_close_cover();

void dvd_schedule_read(uint32_t offset, uint32_t length, OSAlarmHandler handler);

#endif /* EMULATOR_H */
