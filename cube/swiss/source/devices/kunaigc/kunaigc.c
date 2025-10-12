/*
 * kunaigc.c
 *
 *  Created on: Jul 11, 2022
 *      Author: mancloud
 */


#include "kunaigc.h"
#include "ogc/exi.h"
#include "spiflash.h"
#include "util.h"
#include <sys/unistd.h>


/* KUNAI functions */

void kunai_disable(void) {
    u32 addr = 0xc0000000;
    u32 data = 6 << 24;
    EXI_LockEx(EXI_CHANNEL_0, EXI_DEVICE_1);
    EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ);
    EXI_ImmEx(EXI_CHANNEL_0, &addr, 4, EXI_WRITE);
    EXI_ImmEx(EXI_CHANNEL_0, &data, 4, EXI_WRITE);
    EXI_Deselect(EXI_CHANNEL_0);
    EXI_Unlock(EXI_CHANNEL_0);
}

void kunai_reenable(void) {
    u32 addr = 0xc0000000;
    u32 data = 1 << 24;
    EXI_LockEx(EXI_CHANNEL_0, EXI_DEVICE_1);
    EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED8MHZ);
    EXI_ImmEx(EXI_CHANNEL_0, &addr, 4, EXI_WRITE);
    EXI_ImmEx(EXI_CHANNEL_0, &data, 4, EXI_WRITE);
    EXI_Deselect(EXI_CHANNEL_0);
    EXI_Unlock(EXI_CHANNEL_0);
}

void kunai_wait(void) {
    kunai_enable_passthrough();
    spiflash_wait();
    kunai_disable_passthrough();
}

void kunai_sector_erase(uint32_t addr) {
    kunai_enable_passthrough();
    spiflash_write_enable();
    kunai_disable_passthrough();
    kunai_enable_passthrough();
    spiflash_cmd_addr_start(W25Q80BV_CMD_ERASE_4K, addr);
    kunai_disable_passthrough();
    kunai_wait();
}


void kunai_disable_passthrough(void) {
    EXI_Deselect(EXI_CHANNEL_0);
    EXI_Unlock(EXI_CHANNEL_0);
}

void kunai_enable_passthrough(void) {
    u32 addr = 0x80000000; //for passthrough we need to send one '1' and 31 '0' and afterwards whatever we want
    EXI_LockEx(EXI_CHANNEL_0, EXI_DEVICE_1);
    EXI_Select(EXI_CHANNEL_0, EXI_DEVICE_1, EXI_SPEED32MHZ);
    EXI_ImmEx(EXI_CHANNEL_0, &addr, 4, EXI_WRITE);
}


uint32_t kunai_get_jedecID(void) {
    uint32_t jedecID = 0;
    kunai_reenable();
    kunai_enable_passthrough();
    jedecID = spiflash_jedec_id();
    kunai_disable_passthrough();
    kunai_disable();
    return jedecID;
}

/* LFS functions */

int kunai_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    int retVal = 0;
    if(size) {
        uint32_t * p_data = buffer;
        kunai_reenable();
        kunai_enable_passthrough();
        spiflash_read_start_fast((block * c->block_size) + off + KUNAI_OFFS);
        for(lfs_size_t i = size; i > 0; i-=(c->read_size)){
          *(p_data++) = spiflash_read_uint32();
        }
        kunai_disable_passthrough();
        kunai_disable();
    } else {
        retVal = LFS_ERR_IO;
    }

    return retVal;
}

int kunai_write(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    int retVal = 0;
    if(size) {
        uint32_t * p_data = (uint32_t *) buffer;
        kunai_reenable();

        for(lfs_size_t i = size; i > 0; i -= c->prog_size) {

            kunai_enable_passthrough();
            spiflash_write_enable();
            kunai_disable_passthrough();

            kunai_enable_passthrough();
            spiflash_cmd_addr_start(W25Q80BV_CMD_PAGE_PROG, (block * c->block_size) + KUNAI_OFFS + off);
            for(uint8_t ii = 0; ii < (W25Q80BV_PAGE_SIZE/4); ii++) {

                spiflash_write_uint32(*p_data);
                p_data++;
            }
            kunai_disable_passthrough();
            kunai_wait();
            off += c->prog_size;
        }

        kunai_disable();
    } else {
        retVal = LFS_ERR_IO;
    }
    return retVal;
}

int kunai_erase(const struct lfs_config *c, lfs_block_t block) {
    int retVal = 0;
    kunai_reenable();
    kunai_sector_erase(block * c->block_size + KUNAI_OFFS);
    kunai_disable();
    return retVal;
}

int kunai_sync(const struct lfs_config *c) { return 0; }
