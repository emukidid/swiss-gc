/* 
 * Copyright (c) 2017-2019, Extrems <extrems@extremscorner.org>
 * All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../base/common.h"
#include "../base/dvd.h"
#include "../base/exi.h"
#include "../base/os.h"
#include "bba.h"
#include "globals.h"
#include "tcpip.c"

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

static bool exi_selected(void)
{
	return !!(EXI[EXI_CHANNEL_0][0] & 0x380);
}

static void exi_clear_interrupts(int32_t chan, bool exi, bool tc, bool ext)
{
	EXI[chan][0] = (EXI[chan][0] & ~0x80A) | (ext << 11) | (tc << 3) | (exi << 1);
}

static void exi_select(void)
{
	EXI[EXI_CHANNEL_0][0] = (EXI[EXI_CHANNEL_0][0] & 0x405) | ((1 << EXI_DEVICE_2) << 7) | (EXI_SPEED_32MHZ << 4);
}

static void exi_deselect(void)
{
	EXI[EXI_CHANNEL_0][0] &= 0x405;
}

static void exi_imm_write(uint32_t data, uint32_t len)
{
	EXI[EXI_CHANNEL_0][4] = data;
	EXI[EXI_CHANNEL_0][3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 0b01;
	while (EXI[EXI_CHANNEL_0][3] & 0b01);
}

static uint32_t exi_imm_read(uint32_t len)
{
	EXI[EXI_CHANNEL_0][3] = ((len - 1) << 4) | (EXI_READ << 2) | 0b01;
	while (EXI[EXI_CHANNEL_0][3] & 0b01);
	return EXI[EXI_CHANNEL_0][4] >> ((4 - len) * 8);
}

static uint32_t exi_imm_read_write(uint32_t data, uint32_t len)
{
	EXI[EXI_CHANNEL_0][4] = data;
	EXI[EXI_CHANNEL_0][3] = ((len - 1) << 4) | (EXI_READ_WRITE << 2) | 0b01;
	while (EXI[EXI_CHANNEL_0][3] & 0b01);
	return EXI[EXI_CHANNEL_0][4] >> ((4 - len) * 8);
}

static void exi_immex_write(const void *buf, uint32_t len)
{
	do {
		uint32_t xlen = MIN(len, 4);
		exi_imm_write(*(uint32_t *)buf, xlen);
		buf += xlen;
		len -= xlen;
	} while (len);
}

static void exi_dma_read(void *buf, uint32_t len)
{
	EXI[EXI_CHANNEL_0][1] = (uint32_t)buf;
	EXI[EXI_CHANNEL_0][2] = (len + 31) & ~31;
	EXI[EXI_CHANNEL_0][3] = (EXI_READ << 2) | 0b11;
	while (EXI[EXI_CHANNEL_0][3] & 0b01);
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

static void bba_ins(uint16_t reg, void *val, uint32_t len)
{
	exi_select();
	exi_imm_write(0x80 << 24 | reg << 8, 4);
	exi_dma_read(val, len);
	exi_deselect();
}

static void bba_outs(uint16_t reg, const void *val, uint32_t len)
{
	exi_select();
	exi_imm_write(0xC0 << 24 | reg << 8, 4);
	exi_immex_write(val, len);
	exi_deselect();
}

bool bba_transmit(const void *data, size_t size)
{
	if (exi_selected())
		return false;

	while (bba_in8(BBA_NCRA) & (BBA_NCRA_ST0 | BBA_NCRA_ST1));
	bba_outs(BBA_WRTXFIFOD, data, size);
	bba_out8(BBA_NCRA, (bba_in8(BBA_NCRA) & ~BBA_NCRA_ST0) | BBA_NCRA_ST1);

	return true;
}

void bba_receive_end(bba_page_t page, void *data, size_t size)
{
	uint8_t rrp;

	page = OSCachedToUncached(page);

	while (size) {
		int page_size = MIN(size, sizeof(bba_page_t));

		rrp = bba_in8(BBA_RRP) % BBA_INIT_RHBP + 1;
		bba_out8(BBA_RRP, rrp);
		bba_ins(rrp << 8, page, page_size);

		memcpy(data, page, page_size);
		data += page_size;
		size -= page_size;
	}

	(*_received)++;
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

void exi_interrupt_handler(OSInterrupt interrupt, OSContext *context)
{
	uint8_t status = bba_cmd_in8(0x03);
	bba_cmd_out8(0x02, BBA_CMD_IRMASKALL);

	exi_clear_interrupts(EXI_CHANNEL_2, 1, 0, 0);

	if (status & 0x80)
		bba_interrupt();

	bba_cmd_out8(0x03, status);
	bba_cmd_out8(0x02, BBA_CMD_IRMASKNONE);
}

bool exi_trylock(int32_t chan, uint32_t dev, EXIControl *exi)
{
	if (!(exi->state & EXI_STATE_LOCKED) || exi->dev != dev)
		return false;
	if (chan == EXI_CHANNEL_0 && dev == EXI_DEVICE_2)
		return false;
	return true;
}

void di_complete_transfer(void);

void perform_read(uint32_t offset, uint32_t length, uint32_t address)
{
	*_position  = offset;
	*_remainder = length;
	*_data = (void *)address;

	if (!is_frag_read(offset, length))
		fsp_get_file(offset, length);
}

void trickle_read(void)
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
			else if (!is_frag_read(position, remainder))
				fsp_get_file(position, remainder);
		} else {
			tb_t end;
			mftb(&end);

			if (tb_diff_usec(&end, _start) > 1000000)
				fsp_get_file(position, remainder);
		}
	}
}
