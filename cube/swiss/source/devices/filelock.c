/* 
 * Copyright (c) 2024, Extrems <extrems@extremscorner.org>
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

#include <ogc/lwp.h>
#include <ogc/machine/processor.h>
#include <stdbool.h>
#include <stdint.h>
#include "deviceHandler.h"
#include "filelock.h"

static lwpq_t queue = LWP_TQUEUE_NULL;

__attribute((constructor))
static void initQueue(void)
{
	LWP_InitQueue(&queue);
}

void lockFile(file_handle *file)
{
	uint32_t level;

	_CPU_ISR_Disable(level);

	while (file->lockCount != 0 && file->thread != LWP_GetSelf())
		LWP_ThreadSleep(queue);

	file->lockCount++;
	file->thread = LWP_GetSelf();

	_CPU_ISR_Restore(level);
}

bool trylockFile(file_handle *file)
{
	uint32_t level;

	_CPU_ISR_Disable(level);

	if (file->lockCount != 0 && file->thread != LWP_GetSelf()) {
		_CPU_ISR_Restore(level);
		return false;
	}

	file->lockCount++;
	file->thread = LWP_GetSelf();

	_CPU_ISR_Restore(level);
	return true;
}

void unlockFile(file_handle *file)
{
	uint32_t level;

	_CPU_ISR_Disable(level);

	if (--file->lockCount == 0)
		LWP_ThreadBroadcast(queue);

	_CPU_ISR_Restore(level);
}
