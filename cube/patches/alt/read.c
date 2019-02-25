#include <stdint.h>
#include <string.h>
#include "../../reservedarea.h"
#include "../base/common.h"

void exi_handler() {}

void trigger_dvd_interrupt(void)
{
	uint32_t dst = (*DI)[5] | 0x80000000;
	uint32_t len = (*DI)[6];

	(*DI)[2] = 0xE0000000;
	(*DI)[3] = 0;
	(*DI)[4] = 0;
	(*DI)[5] = 0;
	(*DI)[6] = 0;
	(*DI)[8] = 0;
	(*DI)[7] = 1;

	dcache_flush_icache_inv((void *)dst, len);
}

void perform_read(void)
{
	uint32_t off = (*DI)[3] << 2;
	uint32_t len = (*DI)[4];
	uint32_t dst = (*DI)[5] | 0x80000000;

	*(uint32_t *)VAR_LAST_OFFSET = off;
	*(uint32_t *)VAR_TMP2 = len;
	*(uint32_t *)VAR_TMP1 = dst;
}

void *tickle_read(void)
{
	uint32_t position  = *(uint32_t *)VAR_LAST_OFFSET;
	uint32_t remainder = *(uint32_t *)VAR_TMP2;
	uint8_t *data      = *(uint8_t **)VAR_TMP1;
	uint32_t data_size;

	if (remainder) {
		data_size = read_frag(data, remainder, position);

		position   += data_size;
		remainder  -= data_size;

		*(uint32_t *)VAR_LAST_OFFSET = position;
		*(uint32_t *)VAR_TMP2 = remainder;
		*(uint8_t **)VAR_TMP1 = data + data_size;

		if (!remainder) trigger_dvd_interrupt();
	}

	return NULL;
}

void tickle_read_hook(uint32_t enable)
{
	tickle_read();
	restore_interrupts(enable);
}

void tickle_read_idle(void)
{
	disable_interrupts();
	tickle_read();
	enable_interrupts();
}
