/* 
 * Copyright (c) 2019-2024, Extrems <extrems@extremscorner.org>
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

#include "common.h"
#include "dolphin/os.h"
#include "dolphin/pad.h"

void CheckStatus(s32 chan, PADStatus *status)
{
	if (*VAR_TRIGGER_LEVEL > 0) {
		if (status->button & PAD_BUTTON_L)
			status->triggerL = *VAR_TRIGGER_LEVEL;
		if (status->triggerL >= *VAR_TRIGGER_LEVEL) {
			status->triggerL = *VAR_TRIGGER_LEVEL;
			status->button |= PAD_BUTTON_L;
		}

		if (status->button & PAD_BUTTON_R)
			status->triggerR = *VAR_TRIGGER_LEVEL;
		if (status->triggerR >= *VAR_TRIGGER_LEVEL) {
			status->triggerR = *VAR_TRIGGER_LEVEL;
			status->button |= PAD_BUTTON_R;
		}
	}

	u8 igr_type = *VAR_IGR_TYPE;

	if (igr_type != IGR_OFF) {
		if ((status->button & PAD_COMBO_EXIT1) == PAD_COMBO_EXIT1 ||
			(status->button & PAD_COMBO_EXIT2) == PAD_COMBO_EXIT2) {
			if (OSResetSystem) {
				*VAR_DRIVE_FLAGS |= 0b0010;
				*VAR_IGR_TYPE = IGR_OFF;
				enable_interrupts();
				if (igr_type == IGR_APPLOADER)
					OSResetSystem(OS_RESET_RESTART, 0, 0);
				else
					OSResetSystem(OS_RESET_HOTRESET, 0, 0);
				__builtin_trap();
			}
		} else if ((status->button & PAD_COMBO_RESTART) == PAD_COMBO_RESTART) {
			if (OSResetSystem) {
				enable_interrupts();
				OSResetSystem(OS_RESET_RESTART, 0, 0);
				__builtin_trap();
			}
		}
	}

	status->button &= ~PAD_USE_ORIGIN;
}
