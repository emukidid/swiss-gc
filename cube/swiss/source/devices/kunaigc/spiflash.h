/* (c) 2016-07-01 Jens Hauke <jens.hauke@4k2.de>              -*- linux-c -*- */
#ifndef _SPIFLASH_H_
#define _SPIFLASH_H_

#include <inttypes.h>
#include <gccore.h>

#ifdef __cplusplus
#define _spiflash_h_ {
extern "C" _spiflash_h_
#endif


#define W25Q80BV_CMD_WRITE_ENABLE	0x06
#define W25Q80BV_CMD_WRITE_DISABLE	0x04
#define W25Q80BV_CMD_PAGE_PROG	0x02
#define W25Q80BV_CMD_READ_DATA	0x03
#define W25Q80BV_CMD_READ_FAST	0x0B
#define W25Q80BV_CMD_READ_STAT1	0x05
#define W25Q80BV_MASK_STAT_BUSY	(1<<0)
#define W25Q80BV_CMD_ERASE_4K	0x20
#define W25Q80BV_CMD_ERASE_32K	0x52
#define W25Q80BV_CMD_ERASE_64K	0xD8
#define W25Q80BV_CMD_CHIP_ERASE	0xC7 /* alternative 0x60 */
#define W25Q80BV_CMD_READ_MAN_DEV_ID	0x90
#define W25Q80BV_CMD_READ_JEDEC_ID	0x9F
#define W25Q80BV_CMD_READ_UNIQUE_ID	0x4B
#define W25Q80BV_PAGE_SIZE	256
#define W25Q80BV_CAPACITY	(1L * 1024L * 1024L)

#define SPIFLASH_PAGE_SIZE	W25Q80BV_PAGE_SIZE

/*
 * Generic commands
 */
void spiflash_cmd_start(uint8_t cmd);
void spiflash_cmd_addr_start(uint8_t cmd, uint32_t addr);

void spiflash_end(void);
void spiflash_end_wait(void);


/*
 * Writing
 */
void spiflash_write_enable(void);
void spiflash_write_disable(void);
void spiflash_write_start(uint32_t addr);

static inline
void spiflash_write_uint8(uint8_t val) {
	EXI_Imm(EXI_CHANNEL_0, &val, 1, EXI_WRITE, NULL);
	EXI_Sync(EXI_CHANNEL_0);
}


static inline
void spiflash_write(uint8_t val) {
	spiflash_write_uint8(val);
}


void spiflash_write_uint16(uint16_t val);
void spiflash_write_uint32(uint32_t val);

// little-endian write
void spiflash_write_uint16_le(uint16_t val);
void spiflash_write_uint32_le(uint32_t val);


static inline
void spiflash_write_end(void) {
	spiflash_end_wait();
}


/*
 * Reading
 */
void spiflash_read_start(uint32_t addr);
void spiflash_read_start_fast(uint32_t addr);

uint8_t spiflash_read_uint8(void);
uint16_t spiflash_read_uint16(void);
uint32_t spiflash_read_uint32(void);

// little-endian read
uint16_t spiflash_read_uint16_le(void);
uint32_t spiflash_read_uint32_le(void);

static inline
uint8_t spiflash_read(void) {
	return spiflash_read_uint8();
}

static inline
void spiflash_read_end(void) {
	spiflash_end();
}


/*
 * Erase
 */
void spiflash_erase4k(uint32_t addr);
void spiflash_erase32k(uint32_t addr);
void spiflash_erase64k(uint32_t addr);
void spiflash_chip_erase(void);


/*
 * Misc
 */

// spiflash_device_id: return manufacturer/device id
// W25Q80BV: 0xEF13
uint16_t spiflash_device_id(void);
uint32_t spiflash_jedec_id(void);
uint64_t spiflash_unique_id(void);

// Status-1 BUSY-bit set?
uint8_t spiflash_is_busy(void);

// Wait until not busy
void spiflash_wait(void);

static inline
uint32_t spiflash_capacity(void) {
	if (spiflash_device_id() == 0xEF13) {
		return W25Q80BV_CAPACITY;
	} else {
		return ~0L;
	}
}

#ifdef __cplusplus
}
#endif
#endif /* _SPIFLASH_H_ */
