/* 
 * Copyright (c) 2020, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#ifndef FIFO_H
#define FIFO_H

void fifo_reset(void);
int fifo_space(void);
int fifo_size(void);
void fifo_read(void *buffer, int length);
void fifo_write(void *buffer, int length);

#endif /* FIFO_H */
