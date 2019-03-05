#include <stdint.h>
#include <string.h>
#include "../../reservedarea.h"
#include "../base/common.h"

enum {
	EXI_READ = 0,
	EXI_WRITE,
	EXI_READ_WRITE,
};

enum {
	EXI_CHANNEL_0 = 0,
	EXI_CHANNEL_1,
	EXI_CHANNEL_2,
	EXI_CHANNEL_MAX
};

enum {
	EXI_DEVICE_0 = 0,
	EXI_DEVICE_1,
	EXI_DEVICE_2,
	EXI_DEVICE_MAX
};

enum {
	EXI_SPEED_1MHZ = 0,
	EXI_SPEED_2MHZ,
	EXI_SPEED_4MHZ,
	EXI_SPEED_8MHZ,
	EXI_SPEED_16MHZ,
	EXI_SPEED_32MHZ,
};

typedef struct {
	uint32_t offset;
	uint32_t size;
} __attribute((packed, scalar_storage_order("little-endian"))) usb_request_t;

static void exi_select(void)
{
	EXI[EXI_CHANNEL_1][0] = (EXI[EXI_CHANNEL_1][0] & 0x405) | ((1 << EXI_DEVICE_0) << 7) | (EXI_SPEED_32MHZ << 4);
}

static void exi_deselect(void)
{
	EXI[EXI_CHANNEL_1][0] &= 0x405;
}

static uint32_t exi_imm_read_write(uint32_t data, int len)
{
	EXI[EXI_CHANNEL_1][4] = data;
	EXI[EXI_CHANNEL_1][3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 1;
	while (EXI[EXI_CHANNEL_1][3] & 1);
	return EXI[EXI_CHANNEL_1][4] >> ((4 - len) * 8);
}

static int usb_receive_byte(uint8_t *data)
{
	uint16_t val;

	exi_select();
	val = exi_imm_read_write(0xA << 28, 2); *data = val;
	exi_deselect();

	return !(val & 0x800);
}

static int usb_transmit_byte(uint8_t *data)
{
	uint16_t val;

	exi_select();
	val = exi_imm_read_write(0xB << 28 | *data << 20, 2);
	exi_deselect();

	return !(val & 0x400);
}

static int usb_transmit_check(void)
{
	uint8_t val;

	exi_select();
	val = exi_imm_read_write(0xC << 28, 1);
	exi_deselect();

	return !(val & 0x4);
}

static int usb_receive_check(void)
{
	uint8_t val;

	exi_select();
	val = exi_imm_read_write(0xD << 28, 1);
	exi_deselect();

	return !(val & 0x4);
}

static int usb_transmit(void *data, int size, int minsize)
{
	int i = 0, j = 0, check = 1;

	while (i < size) {
		if ((check && usb_transmit_check()) ||
			(check = usb_transmit_byte(data + i))) {
			j = i % 128;
			if (i < minsize)
				continue;
			else break;
		}

		i++;
		check = i % 128 == j;
	}

	return i;
}

static int usb_receive(void *data, int size, int minsize)
{
	int i = 0, j = 0, check = 1;

	while (i < size) {
		if ((check && usb_receive_check()) ||
			(check = usb_receive_byte(data + i))) {
			j = i % 64;
			if (i < minsize)
				continue;
			else break;
		}

		i++;
		check = i % 64 == j;
	}

	return i;
}

void usb_request(uint32_t offset, uint32_t size)
{
	usb_request_t request = {offset, size};
	usb_transmit(&request, sizeof(request), sizeof(request));
}

void exi_handler() {}

int exi_lock(int32_t channel, uint32_t device)
{
	return 1;
}

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

	if (!is_frag_read(off, len))
		usb_request(off, len);
}

void *tickle_read(void)
{
	uint32_t position  = *(uint32_t *)VAR_LAST_OFFSET;
	uint32_t remainder = *(uint32_t *)VAR_TMP2;
	uint8_t *data      = *(uint8_t **)VAR_TMP1;
	uint32_t data_size;

	if (remainder) {
		int frag = is_frag_read(position, remainder);
		if (frag)
			data_size = read_frag(data, remainder, position);
		else
			data_size = usb_receive(data, remainder, 0);

		position  += data_size;
		remainder -= data_size;

		*(uint32_t *)VAR_LAST_OFFSET = position;
		*(uint32_t *)VAR_TMP2 = remainder;
		*(uint8_t **)VAR_TMP1 = data + data_size;

		if (!remainder) trigger_dvd_interrupt();
		else if (frag && !is_frag_read(position, remainder))
			usb_request(position, remainder);
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
