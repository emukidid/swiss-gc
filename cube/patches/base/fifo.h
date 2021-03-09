/* 
 * Copyright (c) 2020, Extrems <extrems@extremscorner.org>
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

#ifndef FIFO_H
#define FIFO_H

typedef struct {
	int used, size;
	void *start_ptr, *end_ptr;
	void *read_ptr, *write_ptr;
} fifo_t;

void fifo_init(fifo_t *fifo, void *buffer, int size);
void fifo_reset(fifo_t *fifo);
int fifo_space(fifo_t *fifo);
int fifo_size(fifo_t *fifo);
void fifo_read(fifo_t *fifo, void *buffer, int length);
void fifo_write(fifo_t *fifo, void *buffer, int length);

#endif /* FIFO_H */
