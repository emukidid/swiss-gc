/* 
 * Copyright (c) 2020-2025, Extrems <extrems@extremscorner.org>
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

#ifndef __NKIT_H__
#define __NKIT_H__

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "gcm.h"

uint8_t fletcher8(const void *buffer, size_t size);
uint16_t fletcher16(const void *buffer, size_t size);

bool is_datel_disc(const DiskHeader *header);
bool is_diag_disc(const DiskHeader *header);
bool is_multi_disc(const file_meta *meta);
bool is_nkit_format(const DiskHeader *header);
bool is_redump_disc(const file_meta *meta);
bool is_streaming_disc(const DiskHeader *header);
bool is_verifiable_disc(const DiskHeader *header);

bool get_gcm_banner_fast(const DiskHeader *header, uint32_t *offset, uint32_t *size);
uint64_t get_gcm_boot_hash(const DiskHeader *header, const file_meta *meta);
const char *get_gcm_title(const DiskHeader *header, file_meta *meta);

int valid_dol_xxh3(const file_handle *file, uint64_t hash);
int valid_file_xxh3(const DiskHeader *header, const ExecutableFile *file);
bool valid_gcm_boot(const DiskHeader *header);
bool valid_gcm_crc32(const DiskHeader *header, uint32_t crc);
bool valid_gcm_magic(const DiskHeader *header);
bool valid_gcm_size(const DiskHeader *header, off_t size);
bool valid_gcm_size2(const DiskHeader *header, off_t size);

bool needs_flippy_bypass(const file_handle *file, uint64_t hash);
bool needs_nkit_reencode(const DiskHeader *header, off_t size);

#endif /* __NKIT_H__ */
