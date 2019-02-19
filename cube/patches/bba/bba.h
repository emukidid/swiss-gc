#ifndef BBA_H
#define BBA_H

#include <stdint.h>

typedef struct {
	uint32_t next   : 12;
	uint32_t length : 12;
	uint32_t status : 8;
	uint8_t data[];
} __attribute((packed, scalar_storage_order("little-endian"))) bba_header_t;

typedef uint32_t bba_page_t[64] __attribute((aligned(32)));

void bba_transmit(void *data, int size);
void bba_receive_end(bba_header_t *bba);

#endif /* BBA_H */
