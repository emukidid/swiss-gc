/*
 * eltorito.h
 *
 * Copyright (C) 2005-2006 The GameCube Linux Team
 * Copyright (C) 2005,2006 Albert Herranz
 * Copyright (C) 2020-2021 Extrems
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 */

#ifndef __ELTORITO_H
#define __ELTORITO_H

#include <stdint.h>

#define DI_SECTOR_SIZE		2048

/*
 * DVD data structures
 */

struct di_boot_record {
	uint8_t zero;
	uint8_t standard_id[5];	/* "CD001" */
	uint8_t version;	/* 1 */
	uint8_t boot_system_id[32];	/* "EL TORITO SPECIFICATION" */
	uint8_t boot_id[32];
	uint32_t boot_catalog_offset;	/* in media sectors */
	uint8_t align_1[21];
} __attribute__ ((__packed__, __scalar_storage_order__ ("little-endian")));

struct di_validation_entry {
	uint8_t header_id;	/* 1 */
	uint8_t platform_id;	/* 0=80x86,1=PowerPC,2=Mac */
	uint16_t reserved;
	uint8_t id_string[24];
	uint16_t checksum;
	uint8_t key_55;		/* 55 */
	uint8_t key_AA;		/* AA */
} __attribute__ ((__packed__, __scalar_storage_order__ ("little-endian")));

struct di_default_entry {
	uint8_t boot_indicator;	/* 0x88=bootable */
	uint8_t boot_media_type;	/* 0=no emulation */
	uint16_t load_segment;	/* multiply by 10 to get actual address */
	uint8_t system_type;
	uint8_t unused_1;
	uint16_t sector_count;	/* emulated sectors to load at segment */
	uint32_t load_rba;	/* in media sectors */
	uint8_t unused_2[20];
} __attribute__ ((__packed__, __scalar_storage_order__ ("little-endian")));

#endif /* __ELTORITO_H */
