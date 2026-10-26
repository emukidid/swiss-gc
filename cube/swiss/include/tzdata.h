/* 
 * Copyright (c) 2026, Extrems <extrems@extremscorner.org>
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

#ifndef __TZDATA_H__
#define __TZDATA_H__

struct TZData {
	const char *name;
	union {
		const char *timezone;
		struct TZData *timezones;
	};

	int count;
};

extern struct TZData TZData;

const char *get_timezone(const char *name);

#endif /* __TZDATA_H__ */
