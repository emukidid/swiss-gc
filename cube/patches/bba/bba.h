/* 
 * Copyright (c) 2019-2020, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#ifndef BBA_H
#define BBA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint32_t next   : 12;
	uint32_t length : 12;
	uint32_t status : 8;
	uint8_t data[];
} __attribute((packed, scalar_storage_order("little-endian"))) bba_header_t;

typedef uint8_t bba_page_t[256] __attribute((aligned(32)));

bool bba_transmit(const void *data, size_t size);
void bba_receive_end(bba_page_t page, void *data, size_t size);

#endif /* BBA_H */
