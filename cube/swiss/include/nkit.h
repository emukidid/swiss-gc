/* 
 * Copyright (c) 2020-2022, Extrems <extrems@extremscorner.org>
 * 
 * This file is part of Swiss.
 * 
 * Swiss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Swiss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * with Swiss.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __NKIT_H
#define __NKIT_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "gcm.h"

uint16_t fletcher16(const void *buffer, size_t size);

bool is_multi_disc(const file_meta *meta);
bool is_redump_disc(const file_meta *meta);
bool is_streaming_disc(const DiskHeader *header);
bool is_verifiable_disc(const DiskHeader *header);

bool get_gcm_banner_fast(const DiskHeader *header, uint32_t *offset, uint32_t *size);
uint64_t get_gcm_boot_hash(const DiskHeader *header);
const char *get_gcm_title(const DiskHeader *header, file_meta *meta);

bool valid_gcm_boot(const DiskHeader *header);
bool valid_gcm_crc32(const DiskHeader *header, uint32_t crc);
bool valid_gcm_magic(DiskHeader *header);
bool valid_gcm_size(const DiskHeader *header, off_t size);

#endif /* __NKIT_H */
