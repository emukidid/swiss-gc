/***************************************************************************
* Network Read code for GC via Broadband Adapter
* Extrems 2017
***************************************************************************/

#include <stdint.h>
#include <string.h>
#include "../../reservedarea.h"
#include "../base/common.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

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
	uint32_t next   : 12;
	uint32_t length : 12;
	uint32_t status : 8;
	uint8_t data[];
} __attribute((packed, scalar_storage_order("little-endian"))) bba_header_t;

static int exi_selected(void)
{
	volatile uint32_t *exi = (uint32_t *)0xCC006800;
	return !!(exi[0] & 0x380);
}

static void exi_select(void)
{
	volatile uint32_t *exi = (uint32_t *)0xCC006800;
	exi[0] = (exi[0] & 0x405) | ((1 << EXI_DEVICE_2) << 7) | (EXI_SPEED_32MHZ << 4);
}

static void exi_deselect(void)
{
	volatile uint32_t *exi = (uint32_t *)0xCC006800;
	exi[0] &= 0x405;
}

static void exi_imm_write(uint32_t data, int len)
{
	volatile uint32_t *exi = (uint32_t *)0xCC006800;
	exi[4] = data;
	exi[3] = ((len - 1) << 4) | (EXI_WRITE << 2) | 1;
	while (exi[3] & 1);
}

static uint32_t exi_imm_read(int len)
{
	volatile uint32_t *exi = (uint32_t *)0xCC006800;
	exi[3] = ((len - 1) << 4) | (EXI_READ << 2) | 1;
	while (exi[3] & 1);
	return exi[4] >> ((4 - len) * 8);
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
	volatile uint32_t *exi = (uint32_t *)0xCC006800;
	exi[1] = (uint32_t)data;
	exi[2] = len;
	exi[3] = (EXI_READ << 2) | 3;
	while (exi[3] & 1);
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
	exi_imm_write(0x00 << 24 | reg << 24, 2);
	val = exi_imm_read(1);
	exi_deselect();

	return val;
}

static void bba_cmd_out8(uint8_t reg, uint8_t val)
{
	exi_select();
	exi_imm_write(0x40 << 24 | reg << 24, 2);
	exi_imm_write(val << 24, 1);
	exi_deselect();
}

static void bba_insregister(uint16_t reg)
{
	exi_imm_write(0x80 << 24 | reg << 8, 4);
}

static void bba_outsregister(uint16_t reg)
{
	exi_imm_write(0xC0 << 24 | reg << 8, 4);
}

static void bba_insdata(void *data, int size)
{
	exi_dma_read(data, size);
}

static void bba_outsdata(void *data, int size)
{
	exi_immex_write(data, size);
}

void bba_transmit(void *data, int size)
{
	while (bba_in8(BBA_NCRA) & (BBA_NCRA_ST0 | BBA_NCRA_ST1));

	exi_select();
	bba_outsregister(BBA_WRTXFIFOD);
	bba_outsdata(data, size);
	exi_deselect();

	bba_out8(BBA_NCRA, (bba_in8(BBA_NCRA) & ~BBA_NCRA_ST0) | BBA_NCRA_ST1);
}

void fsp_output(const char *file, uint8_t filelen, uint32_t offset, uint32_t size);
void eth_input(void *eth, uint16_t size);

static void bba_receive(void)
{
	uint8_t rwp = bba_in8(BBA_RWP);
	uint8_t rrp = bba_in8(BBA_RRP);

	while (rrp != rwp) {
		uint8_t buffer[8][256] __attribute((aligned(32)));
		uint8_t (*page)[256] = buffer;
		bba_header_t *bba = (bba_header_t *)page;

		dcache_flush_icache_inv(page, sizeof(*page));

		exi_select();
		bba_insregister(rrp << 8);
		bba_insdata(page, sizeof(*page));
		exi_deselect();

		rrp = rrp == BBA_INIT_RHBP ? BBA_INIT_RRP : rrp + 1;
		page++;

		if (bba->status & (BBA_RX_STATUS_RERR | BBA_RX_STATUS_FAE)) {
			rwp = bba_in8(BBA_RWP);
			rrp = bba_in8(BBA_RRP);
		} else {
			int count = (bba->length - 1) / sizeof(*page);
			int left = BBA_INIT_RHBP + 1 - rrp;

			if (count > 0) {
				dcache_flush_icache_inv(page, sizeof(*page) * count);

				exi_select();
				bba_insregister(rrp << 8);

				if (count > left) {
					bba_insdata(page, sizeof(*page) * left);
					exi_deselect();

					rrp    = BBA_INIT_RRP;
					page  += left;
					count -= left;

					exi_select();
					bba_insregister(rrp << 8);
				}

				bba_insdata(page, sizeof(*page) * count);
				exi_deselect();

				rrp  += count;
				page += count;
			}

			bba_out8(BBA_RRP, rrp = bba->next);
			eth_input((void *)bba->data, bba->length - sizeof(*bba));
			rwp = bba_in8(BBA_RWP);
		}
	}
}

static void bba_interrupt(void)
{
	uint8_t ir = bba_in8(BBA_IR);

	while (ir) {
		if (ir & BBA_IR_RI)
			bba_receive();

		bba_out8(BBA_IR, ir);
		ir = bba_in8(BBA_IR);
	}
}

static void bba_poll(void)
{
	uint8_t status = bba_cmd_in8(0x03);

	while (status) {
		if (status & 0x80)
			bba_interrupt();

		bba_cmd_out8(0x03, status);
		status = bba_cmd_in8(0x03);
	}
}

void trigger_dvd_interrupt(void)
{
	volatile uint32_t *dvd = (uint32_t *)0xCC006000;

	uint32_t dst = dvd[5] | 0x80000000;
	uint32_t len = dvd[6];

	dvd[2] = 0xE0000000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[8] = 0;
	dvd[7] = 1;

	dcache_flush_icache_inv((void *)dst, len);
}

void perform_read(void)
{
	volatile uint32_t *dvd = (uint32_t *)0xCC006000;

	uint32_t off = dvd[3] << 2;
	uint32_t len = dvd[4];
	uint32_t dst = dvd[5] | 0x80000000;

	*(uint32_t *)VAR_FSP_POSITION = off;
	*(uint32_t *)VAR_TMP2 = len;
	*(uint32_t *)VAR_TMP1 = dst;
	*(uint16_t *)VAR_FSP_DATA_LENGTH = 0;

	if (!is_frag_read(off, len) && !exi_selected())
		fsp_output((const char *)VAR_FILENAME, *(uint8_t *)VAR_FILENAME_LEN, off, len);
}

void *tickle_read(void)
{
	if (!exi_selected()) {
		bba_poll();

		uint32_t position    = *(uint32_t *)VAR_FSP_POSITION;
		uint32_t remainder   = *(uint32_t *)VAR_TMP2;
		uint8_t *data        = *(uint8_t **)VAR_TMP1;
		uint32_t data_length = read_frag(data, remainder, position);

		if (data_length) {
			data      += data_length;
			position  += data_length;
			remainder -= data_length;

			*(uint32_t *)VAR_FSP_POSITION = position;
			*(uint32_t *)VAR_TMP2 = remainder;
			*(uint8_t **)VAR_TMP1 = data;
			*(uint16_t *)VAR_FSP_DATA_LENGTH = 0;

			if (!remainder) trigger_dvd_interrupt();
		} else if (remainder) {
			tb_t end, *start = (tb_t *)VAR_TIMER_START;
			mftb(&end);

			if (tb_diff_usec(&end, start) > 1000000)
				fsp_output((const char *)VAR_FILENAME, *(uint8_t *)VAR_FILENAME_LEN, position, remainder);
		}
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
