/* 
 * Copyright (c) 2017-2019, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../base/common.h"
#include "../base/dvd.h"
#include "../base/os.h"
#include "bba.h"
#include "globals.h"

#define BBA_CMD_IRMASKALL		0x00
#define BBA_CMD_IRMASKNONE		0xF8

#define BBA_NCRA				0x00		/* Network Control Register A, RW */
#define BBA_NCRA_RESET			(1<<0)	/* RESET */
#define BBA_NCRA_ST0			(1<<1)	/* ST0, Start transmit command/status */
#define BBA_NCRA_ST1			(1<<2)	/* ST1,  " */
#define BBA_NCRA_SR				(1<<3)	/* SR, Start Receive */

#define BBA_IR 0x09		/* Interrupt Register, RW, 00h */
#define BBA_IR_FRAGI       (1<<0)	/* FRAGI, Fragment Counter Interrupt */
#define BBA_IR_RI          (1<<1)	/* RI, Receive Interrupt */
#define BBA_IR_TI          (1<<2)	/* TI, Transmit Interrupt */
#define BBA_IR_REI         (1<<3)	/* REI, Receive Error Interrupt */
#define BBA_IR_TEI         (1<<4)	/* TEI, Transmit Error Interrupt */
#define BBA_IR_FIFOEI      (1<<5)	/* FIFOEI, FIFO Error Interrupt */
#define BBA_IR_BUSEI       (1<<6)	/* BUSEI, BUS Error Interrupt */
#define BBA_IR_RBFI        (1<<7)	/* RBFI, RX Buffer Full Interrupt */

#define BBA_RWP  0x16/*+0x17*/	/* Receive Buffer Write Page Pointer Register */
#define BBA_RRP  0x18/*+0x19*/	/* Receive Buffer Read Page Pointer Register */

#define BBA_WRTXFIFOD 0x48/*-0x4b*/	/* Write TX FIFO Data Port Register */

#define BBA_RX_STATUS_BF      (1<<0)
#define BBA_RX_STATUS_CRC     (1<<1)
#define BBA_RX_STATUS_FAE     (1<<2)
#define BBA_RX_STATUS_FO      (1<<3)
#define BBA_RX_STATUS_RW      (1<<4)
#define BBA_RX_STATUS_MF      (1<<5)
#define BBA_RX_STATUS_RF      (1<<6)
#define BBA_RX_STATUS_RERR    (1<<7)

#define BBA_INIT_TLBP	0x00
#define BBA_INIT_BP		0x01
#define BBA_INIT_RHBP	0x0f
#define BBA_INIT_RWP	BBA_INIT_BP
#define BBA_INIT_RRP	BBA_INIT_BP

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

static bool exi_selected(void)
{
	return !!((*EXI)[0] & 0x380);
}

static void exi_select(void)
{
	(*EXI)[0] = ((*EXI)[0] & 0x405) | ((1 << EXI_DEVICE_2) << 7) | (EXI_SPEED_32MHZ << 4);
}

static void exi_deselect(void)
{
	(*EXI)[0] &= 0x405;
}

static void exi_imm_write(uint32_t data, int len)
{
	(*EXI)[4] = data;
	(*EXI)[3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 1;
	while ((*EXI)[3] & 1);
}

static uint32_t exi_imm_read(int len)
{
	(*EXI)[3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;
	while ((*EXI)[3] & 1);
	return (*EXI)[4] >> ((4 - len) * 8);
}

static uint32_t exi_imm_read_write(uint32_t data, int len)
{
	(*EXI)[4] = data;
	(*EXI)[3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 1;
	while ((*EXI)[3] & 1);
	return (*EXI)[4] >> ((4 - len) * 8);
}

static void exi_immex_write(void *buffer, int length)
{
	do {
		int count = MIN(length, 4);

		exi_imm_write(*(uint32_t *)buffer, count);

		buffer += count;
		length -= count;
	} while (length);
}

static void exi_dma_read(void *data, int len)
{
	(*EXI)[1] = (uint32_t)data;
	(*EXI)[2] = (len + 31) & ~31;
	(*EXI)[3] = (EXI_READ << 2) | 3;
	while ((*EXI)[3] & 1);
}

static uint8_t bba_in8(uint16_t reg)
{
	uint8_t val;

	exi_select();
	exi_imm_write(0x80 << 24 | reg << 8, 4);
	val = exi_imm_read(1);
	exi_deselect();

	return val;
}

static void bba_out8(uint16_t reg, uint8_t val)
{
	exi_select();
	exi_imm_write(0xC0 << 24 | reg << 8, 4);
	exi_imm_write(val << 24, 1);
	exi_deselect();
}

static uint8_t bba_cmd_in8(uint8_t reg)
{
	uint8_t val;

	exi_select();
	val = exi_imm_read_write(0x00 << 24 | reg << 24, 4);
	exi_deselect();

	return val;
}

static void bba_cmd_out8(uint8_t reg, uint8_t val)
{
	exi_select();
	exi_imm_write(0x40 << 24 | reg << 24 | val, 4);
	exi_deselect();
}

static void bba_ins(uint16_t reg, void *val, int len)
{
	exi_select();
	exi_imm_write(0x80 << 24 | reg << 8, 4);
	exi_dma_read(val, len);
	exi_deselect();
}

static void bba_outs(uint16_t reg, void *val, int len)
{
	exi_select();
	exi_imm_write(0xC0 << 24 | reg << 8, 4);
	exi_immex_write(val, len);
	exi_deselect();
}

void bba_transmit(void *data, int size)
{
	while (bba_in8(BBA_NCRA) & (BBA_NCRA_ST0 | BBA_NCRA_ST1));
	bba_outs(BBA_WRTXFIFOD, data, size);
	bba_out8(BBA_NCRA, (bba_in8(BBA_NCRA) & ~BBA_NCRA_ST0) | BBA_NCRA_ST1);
}

void fsp_output(const char *file, uint8_t filelen, uint32_t offset, size_t size);
void eth_input(void *page, void *eth, size_t size);

void bba_receive_end(bba_page_t page, void *data, int size)
{
	int chunk;
	int rrp;

	page += 0x40000000;

	if (size) {
		do {
			chunk = MIN(size, sizeof(bba_page_t));

			rrp = bba_in8(BBA_RRP) % BBA_INIT_RHBP + 1;
			bba_out8(BBA_RRP, rrp);
			bba_ins(rrp << 8, page, chunk);

			memcpy(data, page, chunk);

			data += chunk;
			size -= chunk;
		} while (size);

		(*_received)++;
	}
}

static void bba_receive(void)
{
	uint8_t rwp = bba_in8(BBA_RWP);
	uint8_t rrp = bba_in8(BBA_RRP);

	*_received = 0;

	while (rrp != rwp) {
		bba_page_t page;
		bba_header_t *bba = (bba_header_t *)page;
		size_t size = sizeof(bba_page_t);

		dcache_flush_icache_inv(page, size);
		bba_ins(rrp << 8, page, size);

		size = bba->length - sizeof(*bba);

		eth_input(page, (void *)bba->data, size);
		bba_out8(BBA_RRP, rrp = bba->next);
		rwp = bba_in8(BBA_RWP);
	}
}

static void bba_interrupt(void)
{
	uint8_t ir = bba_in8(BBA_IR);

	if (ir & BBA_IR_RI)
		bba_receive();

	bba_out8(BBA_IR, ir);
}

void exi_handler(int32_t channel, uint32_t device)
{
	uint8_t status = bba_cmd_in8(0x03);
	bba_cmd_out8(0x02, BBA_CMD_IRMASKALL);

	if (status & 0x80)
		bba_interrupt();

	bba_cmd_out8(0x03, status);
	bba_cmd_out8(0x02, BBA_CMD_IRMASKNONE);
}

bool exi_lock(int32_t channel, uint32_t device)
{
	if (channel == EXI_CHANNEL_0 &&
		device  == EXI_DEVICE_2)
		return false;
	return true;
}

void di_complete_transfer(void);

void perform_read(uint32_t offset, uint32_t length, uint32_t address)
{
	*_position  = offset;
	*_remainder = length;
	*_data = (void *)address;
	*_data_size = 0;

	if (!is_frag_read(offset, length) && !exi_selected())
		fsp_output(_file, *_filelen, offset, length);
}

void tickle_read(void)
{
	uint32_t position  = *_position;
	uint32_t remainder = *_remainder;
	uint8_t *data      = *_data;
	uint32_t data_size;

	if (remainder) {
		if (is_frag_read(position, remainder)) {
			data_size = read_frag(data, remainder, position);

			position  += data_size;
			remainder -= data_size;

			*_position  = position;
			*_remainder = remainder;
			*_data = data + data_size;
			*_data_size = 0;

			if (!remainder) di_complete_transfer();
			else if (!is_frag_read(position, remainder) && !exi_selected())
				fsp_output(_file, *_filelen, position, remainder);
		} else {
			tb_t end;
			mftb(&end);

			if (tb_diff_usec(&end, _start) > 1000000 && !exi_selected())
				fsp_output(_file, *_filelen, position, remainder);
		}
	}
}

void tickle_read_idle(void)
{
	disable_interrupts();
	tickle_read();
	enable_interrupts();
}
