/* 
 * Copyright (c) 2023, Extrems <extrems@extremscorner.org>
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

#include "dolphin/os.c"

void CallAlarmHandler(OSAlarm *alarm, OSContext *context, OSAlarmHandler handler)
{
	OSContext exceptionContext;

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(&exceptionContext);

	handler(alarm, context);

	OSClearContext(&exceptionContext);
	OSSetCurrentContext(context);
}
