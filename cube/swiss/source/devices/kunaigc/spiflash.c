/* (c) 2016-07-01 Jens Hauke <jens.hauke@4k2.de> */
#include "spiflash.h"

//#define SPI_DBG

uint8_t spiflash_is_busy(void) {
    uint8_t res;
    spiflash_write(W25Q80BV_CMD_READ_STAT1);
    res = spiflash_read_uint8();
    #ifdef SPI_DBG
    if (res != 0) 
        print_debug("SPI status Register is %d\n", res);
    #endif
    res &= W25Q80BV_MASK_STAT_BUSY;
    return res;
}


// Wait until not busy
void spiflash_wait(void) {
    while (spiflash_is_busy());
}

void spiflash_cmd_addr_start(uint8_t cmd, uint32_t addr) {
    uint32_t buff = ((uint32_t) cmd << 24) | addr;
#ifdef SPI_DBG
    print_debug("\tCommand is %x, Adress: %04x\n", cmd, buff);
#endif
    EXI_ImmEx(EXI_CHANNEL_0, &buff, 4, EXI_WRITE);
}


void spiflash_write_enable(void) {
    spiflash_write(W25Q80BV_CMD_WRITE_ENABLE);
}

void spiflash_write_disable(void) {
    spiflash_write(W25Q80BV_CMD_WRITE_DISABLE);
}


void spiflash_write_start(uint32_t addr) {
    spiflash_write_enable();
#ifdef SPI_DBG
    print_debug("Write start %04x\n", addr);
#endif
    spiflash_cmd_addr_start(W25Q80BV_CMD_PAGE_PROG, addr);
}


void spiflash_read_start(uint32_t addr) {
#ifdef SPI_DBG
    print_debug("Read start %04x\n", addr);
#endif
    spiflash_cmd_addr_start(W25Q80BV_CMD_READ_DATA, addr);
}

void spiflash_read_start_fast(uint32_t addr) {
#ifdef SPI_DBG
    print_debug("Read start fast %04x\n", addr);
#endif
    spiflash_cmd_addr_start(W25Q80BV_CMD_READ_FAST, addr);
    spiflash_read_uint8();
}

uint8_t spiflash_read_uint8(void) {
    uint8_t val = 0;
    EXI_ImmEx(EXI_CHANNEL_0, &val, 1, EXI_READ);
#ifdef SPI_DBG
    print_debug("Read u8 %01x\n", val);
#endif
    return val;
}


uint16_t spiflash_read_uint16(void) {
    uint16_t val = 0;
    EXI_ImmEx(EXI_CHANNEL_0, &val, 2, EXI_READ);
#ifdef SPI_DBG
    print_debug("Read u16 %02x\n", val);
#endif
    return val;
}


uint32_t spiflash_read_uint32(void) {
    uint32_t val = 0;
    EXI_ImmEx(EXI_CHANNEL_0, &val, 4, EXI_READ);
#ifdef SPI_DBG
    print_debug("Read u32 %04x\n", val);
#endif
    return val;
}


uint16_t spiflash_read_uint16_le(void) {
    uint16_t val = 0;
    val = spiflash_read_uint8() | (uint16_t)spiflash_read_uint8() << 8;
#ifdef SPI_DBG
    print_debug("Read u16_le %02x\n", val);
#endif
    return val;
}


uint32_t spiflash_read_uint32_le(void) {
    uint32_t val = 0;
    val = spiflash_read_uint16_le() | (uint32_t)spiflash_read_uint16_le() << 16;
#ifdef SPI_DBG
    print_debug("Read u32_le %04x\n", val);
#endif
    return val;
}


void spiflash_write_uint16(uint16_t val) {
#ifdef SPI_DBG
    print_debug("Write u16 : %02xl\n", val);
#endif
    EXI_ImmEx(EXI_CHANNEL_0, &val, 2, EXI_WRITE);
}


void spiflash_write_uint32(uint32_t val) {
#ifdef SPI_DBG
    print_debug("Write u32 : %04xl\n", val);
#endif
    EXI_ImmEx(EXI_CHANNEL_0, &val, 4, EXI_WRITE);
}


void spiflash_write_uint16_le(uint16_t val) {
#ifdef SPI_DBG
    print_debug("Write u16_le : %02xl\n", val);
#endif
    spiflash_write_uint8(val);
    spiflash_write_uint8(val >> 8);
}


void spiflash_write_uint32_le(uint32_t val) {
#ifdef SPI_DBG
    print_debug("Write u32_le : %02xl\n", val);
#endif
    spiflash_write_uint16_le(val);
    spiflash_write_uint16_le(val >> 16);
}


void spiflash_erase4k(uint32_t addr) {
    spiflash_write_enable();
#ifdef SPI_DBG
print_debug("Erase 4k\n");
#endif
    spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_4K, addr);
}


void spiflash_erase32k(uint32_t addr) {
    #ifdef SPI_DBG
print_debug("Erase 32k\n");
#endif
    spiflash_write_enable();
    spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_32K, addr);
}


void spiflash_erase64k(uint32_t addr) {
    spiflash_write_enable();
#ifdef SPI_DBG
    print_debug("Erase 64k\n");
#endif
    spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_64K, addr);
}


void spiflash_chip_erase(void) {
    spiflash_write_enable();
    spiflash_write(W25Q80BV_CMD_CHIP_ERASE);
}


uint16_t spiflash_device_id(void) {
    uint16_t id;
#ifdef SPI_DBG
    print_debug("Dev ID\n");
#endif
    spiflash_cmd_addr_start(W25Q80BV_CMD_READ_MAN_DEV_ID, 0x0000);
    id = spiflash_read_uint16();
    return id;
}

uint32_t spiflash_jedec_id(void) {
    uint32_t id;
    uint8_t cmd = W25Q80BV_CMD_READ_JEDEC_ID;
    EXI_ImmEx(EXI_CHANNEL_0, &cmd, 1, EXI_WRITE);
    id = spiflash_read_uint32() >> 8;
    return id;
}

uint64_t spiflash_unique_id(void) {
    uint64_t id;
#ifdef SPI_DBG
    print_debug("Unique ID\n");
#endif
    spiflash_cmd_addr_start(W25Q80BV_CMD_READ_UNIQUE_ID, 0x0000);
    spiflash_read();
    id = (uint64_t)spiflash_read_uint32() << 32;
    id |= spiflash_read_uint32();
    return id;
}
