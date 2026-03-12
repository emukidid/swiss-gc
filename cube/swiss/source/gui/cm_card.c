/* cm_card.c
	- Card Manager card reading and graphics parsing
	by Swiss contributors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include "cm_internal.h"

// --- Shared graphics parser ---
// Parses banner and icon textures from a raw graphics buffer.
// Works for physical cards (after CARD_Read), VMC (direct pointer), and GCI files.
// The buffer starts at icon_addr within the save file.

void cm_parse_save_graphics(u8 *gfx_data, u32 gfx_len,
	u8 banner_fmt, u16 icon_fmt, u16 icon_speed, card_entry *entry) {

	// Unpack per-frame format and speed from bitfields (2 bits each, 8 frames)
	u8 ifmts[CARD_MAXICONS];
	u8 ispeeds[CARD_MAXICONS];
	int total_frames = 0;
	int nicons_ci_shared = 0;
	for (int i = 0; i < CARD_MAXICONS; i++) {
		ispeeds[i] = (icon_speed >> (2 * i)) & CARD_SPEED_MASK;
		ifmts[i] = (icon_fmt >> (2 * i)) & CARD_ICON_MASK;
		if (ispeeds[i] == CARD_SPEED_END) {
			ifmts[i] = CARD_ICON_NONE;  // speed=END forces format to NONE
			break;
		}
		total_frames++;
		if (ifmts[i] == CARD_ICON_CI) nicons_ci_shared++;
	}

	// Banner size
	u8 bnr_fmt = banner_fmt & CARD_BANNER_MASK;
	u32 banner_size = 0;
	if (bnr_fmt == CARD_BANNER_CI)
		banner_size = CARD_BANNER_W * CARD_BANNER_H + 256 * 2;  // 3584
	else if (bnr_fmt == CARD_BANNER_RGB)
		banner_size = CARD_BANNER_W * CARD_BANNER_H * 2;  // 6144

	// Compute icon pixel offsets (relative to start of gfx_data)
	// Layout: [banner] [icon0 pixels] [icon1 pixels] ... [shared CI TLUT]
	u32 icon_pixel_off[CARD_MAXICONS];
	u32 icon_tlut_off[CARD_MAXICONS];
	u32 accum = banner_size;
	for (int i = 0; i < total_frames; i++) {
		if (ifmts[i] == CARD_ICON_NONE) {
			icon_pixel_off[i] = (u32)-1;
			icon_tlut_off[i] = (u32)-1;
			continue;
		}
		icon_pixel_off[i] = accum;
		icon_tlut_off[i] = (u32)-1;
		if (ifmts[i] == CARD_ICON_RGB) {
			accum += CARD_ICON_W * CARD_ICON_H * 2;  // 2048
		} else if (ifmts[i] == CARD_ICON_CI) {
			accum += CARD_ICON_W * CARD_ICON_H;  // 1024 (shared TLUT at end)
		} else {
			// Format 3: CI with individual TLUT right after pixels
			accum += CARD_ICON_W * CARD_ICON_H;  // 1024 pixels
			icon_tlut_off[i] = accum;
			accum += 256 * 2;  // 512 TLUT
		}
	}
	// Shared CI TLUT comes after all icon pixel data
	u32 shared_tlut_off = (u32)-1;
	if (nicons_ci_shared > 0) {
		shared_tlut_off = accum;
		accum += 256 * 2;  // 512
		for (int i = 0; i < total_frames; i++) {
			if (ifmts[i] == CARD_ICON_CI)
				icon_tlut_off[i] = shared_tlut_off;
		}
	}

	u32 total_size = accum;
	if (total_size == 0 || total_size > gfx_len)
		total_size = gfx_len;

	// Extract banner
	if (banner_size > 0 && banner_size <= total_size) {
		entry->banner = (u8 *)memalign(32, banner_size);
		if (entry->banner) {
			memcpy(entry->banner, gfx_data, banner_size);
			entry->banner_size = banner_size;
			DCFlushRange(entry->banner, banner_size);
			if (bnr_fmt == CARD_BANNER_CI) {
				GX_InitTlutObj(&entry->banner_tlut, entry->banner + CARD_BANNER_W * CARD_BANNER_H, GX_TL_RGB5A3, 256);
				GX_InitTexObjCI(&entry->banner_tex, entry->banner, CARD_BANNER_W, CARD_BANNER_H, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
				GX_InitTexObjFilterMode(&entry->banner_tex, GX_LINEAR, GX_NEAR);
				GX_InitTexObjUserData(&entry->banner_tex, &entry->banner_tlut);
			} else {
				GX_InitTexObj(&entry->banner_tex, entry->banner, CARD_BANNER_W, CARD_BANNER_H, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
				GX_InitTexObjFilterMode(&entry->banner_tex, GX_LINEAR, GX_NEAR);
			}
		}
	}

	// Extract icon frames
	if (total_frames > 0) {
		entry->icon = calloc(1, sizeof(icon_anim));
		if (entry->icon) {
			entry->icon->anim_type = banner_fmt & CARD_ANIM_MASK;
			entry->icon->num_frames = total_frames;
			int last_valid = -1;
			for (int i = 0; i < total_frames; i++) {
				entry->icon->frames[i].fmt = ifmts[i];
				entry->icon->frames[i].speed = ispeeds[i];

				if (ifmts[i] == CARD_ICON_NONE) {
					if (last_valid >= 0) {
						entry->icon->frames[i].data = entry->icon->frames[last_valid].data;
						entry->icon->frames[i].data_size = 0;  // borrowed
						entry->icon->frames[i].tex = entry->icon->frames[last_valid].tex;
						entry->icon->frames[i].tlut = entry->icon->frames[last_valid].tlut;
					}
					continue;
				}

				u32 poff = icon_pixel_off[i];
				u16 pix_size = (ifmts[i] == CARD_ICON_RGB)
					? CARD_ICON_W * CARD_ICON_H * 2
					: CARD_ICON_W * CARD_ICON_H;
				bool has_tlut = (ifmts[i] != CARD_ICON_RGB && icon_tlut_off[i] != (u32)-1);
				u16 tlut_size = has_tlut ? 256 * 2 : 0;
				u16 alloc_size = pix_size + tlut_size;

				if (poff + pix_size > total_size) break;
				if (has_tlut && icon_tlut_off[i] + tlut_size > total_size) break;

				entry->icon->frames[i].data = (u8 *)memalign(32, alloc_size);
				if (!entry->icon->frames[i].data) break;
				entry->icon->frames[i].data_size = alloc_size;

				memcpy(entry->icon->frames[i].data, gfx_data + poff, pix_size);
				if (has_tlut)
					memcpy(entry->icon->frames[i].data + pix_size, gfx_data + icon_tlut_off[i], tlut_size);
				DCFlushRange(entry->icon->frames[i].data, alloc_size);

				if (ifmts[i] == CARD_ICON_RGB) {
					GX_InitTexObj(&entry->icon->frames[i].tex,
						entry->icon->frames[i].data,
						CARD_ICON_W, CARD_ICON_H, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
				} else {
					GX_InitTlutObj(&entry->icon->frames[i].tlut,
						entry->icon->frames[i].data + pix_size, GX_TL_RGB5A3, 256);
					GX_InitTexObjCI(&entry->icon->frames[i].tex,
						entry->icon->frames[i].data,
						CARD_ICON_W, CARD_ICON_H, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
					GX_InitTexObjUserData(&entry->icon->frames[i].tex,
						&entry->icon->frames[i].tlut);
				}
				GX_InitTexObjFilterMode(&entry->icon->frames[i].tex, GX_LINEAR, GX_NEAR);
				last_valid = i;
			}
			if (last_valid < 0) {
				free(entry->icon);
				entry->icon = NULL;
			}
		}
	}
}

// --- Shared comment parser ---
// Parses game name and description from the comment block within save file data.

void cm_parse_comment(u8 *file_data, u32 file_len, u32 comment_addr,
	card_entry *entry) {
	if (comment_addr == (u32)-1 || comment_addr + 64 > file_len)
		return;
	char *comment = (char *)(file_data + comment_addr);
	snprintf(entry->game_name, sizeof(entry->game_name), "%.32s", comment);
	snprintf(entry->game_desc, sizeof(entry->game_desc), "%.32s", comment + 32);
}

// --- Physical card reading ---

static void card_manager_read_comment(int slot, card_entry *entry) {
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);

	card_file cardfile;
	if (CARD_Open(slot, entry->filename, &cardfile) != CARD_ERROR_READY)
		return;

	card_stat cardstat;
	if (CARD_GetStatus(slot, cardfile.filenum, &cardstat) == CARD_ERROR_READY
		&& cardstat.comment_addr != (u32)-1) {
		u32 sector_offset = cardstat.comment_addr & ~(8192 - 1);
		u8 *buf = (u8 *)memalign(32, 8192);
		if (buf) {
			if (CARD_Read(&cardfile, buf, 8192, sector_offset) == CARD_ERROR_READY) {
				u32 off_in_sector = cardstat.comment_addr - sector_offset;
				char *comment = (char *)(buf + off_in_sector);
				snprintf(entry->game_name, sizeof(entry->game_name), "%.32s", comment);
				snprintf(entry->game_desc, sizeof(entry->game_desc), "%.32s", comment + 32);
			}
			free(buf);
		}
	}
	CARD_Close(&cardfile);
}

static void card_manager_read_graphics(int slot, card_entry *entry) {
	if (entry->icon_addr == (u32)-1)
		return;

	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);

	card_file cardfile;
	if (CARD_Open(slot, entry->filename, &cardfile) != CARD_ERROR_READY)
		return;

	// Read all graphics data from card in sector-aligned chunks
	u32 sector_offset = entry->icon_addr & ~(8192 - 1);
	u32 off_in_sector = entry->icon_addr - sector_offset;
	// Estimate max graphics size: banner(6144) + 8 icons(2048 each) + tlut(512) ≈ 23KB
	u32 est_size = 24576;
	u32 read_len = (off_in_sector + est_size + 8191) & ~(8192 - 1);
	u32 max_read = entry->filesize > sector_offset ? entry->filesize - sector_offset : 0;
	max_read &= ~(8192 - 1);
	if (max_read == 0) max_read = 8192;
	if (read_len > max_read) read_len = max_read;

	u8 *buf = (u8 *)memalign(32, read_len);
	if (!buf) {
		CARD_Close(&cardfile);
		return;
	}

	bool ok = true;
	for (u32 off = 0; off < read_len; off += 8192) {
		if (CARD_Read(&cardfile, buf + off, 8192, sector_offset + off) != CARD_ERROR_READY) {
			ok = false;
			break;
		}
	}
	CARD_Close(&cardfile);

	if (!ok) {
		free(buf);
		return;
	}

	u8 *gfx = buf + off_in_sector;
	u32 avail = read_len - off_in_sector;

	cm_parse_save_graphics(gfx, avail,
		entry->banner_fmt, entry->icon_fmt, entry->icon_speed, entry);

	free(buf);
}

void card_manager_free_graphics(card_entry *entries, int count) {
	for (int i = 0; i < count; i++) {
		if (entries[i].banner) {
			free(entries[i].banner);
			entries[i].banner = NULL;
		}
		if (entries[i].icon) {
			for (int f = 0; f < entries[i].icon->num_frames; f++) {
				if (entries[i].icon->frames[f].data_size > 0)
					free(entries[i].icon->frames[f].data);
			}
			free(entries[i].icon);
			entries[i].icon = NULL;
		}
	}
}

int card_manager_read_saves(int slot, card_entry *entries, int max_entries) {
	int count = 0;
	card_dir carddir;

	CARD_SetGamecode("SWIS");
	CARD_SetCompany("S0");

	memset(&carddir, 0, sizeof(card_dir));
	s32 ret = CARD_FindFirst(slot, &carddir, true);
	while (ret != CARD_ERROR_NOFILE && count < max_entries) {
		memset(&entries[count], 0, sizeof(card_entry));
		snprintf(entries[count].filename, CARD_FILENAMELEN + 1, "%.*s",
			CARD_FILENAMELEN, carddir.filename);
		memcpy(entries[count].gamecode, carddir.gamecode, 4);
		entries[count].gamecode[4] = '\0';
		memcpy(entries[count].company, carddir.company, 2);
		entries[count].company[2] = '\0';
		entries[count].filesize = carddir.filelen;
		entries[count].blocks = carddir.filelen / 8192;
		entries[count].fileno = carddir.fileno;
		entries[count].permissions = carddir.permissions;
		count++;
		ret = CARD_FindNext(&carddir);
	}

	// Read banner_fmt and icon_addr from raw directory entries in sys_area
	unsigned char *sysarea = get_card_sys_area(slot);
	for (int i = 0; i < count; i++) {
		entries[i].icon_addr = (u32)-1;
		if (sysarea) {
			u32 entry_off = entries[i].fileno * 64;
			for (int blk = 0; blk < 5; blk++) {
				GCI *raw = (GCI *)(sysarea + blk * 8192 + entry_off);
				if (memcmp(raw->gamecode, entries[i].gamecode, 4) == 0
					&& memcmp(raw->company, entries[i].company, 2) == 0
					&& strncmp(raw->filename, entries[i].filename, CARD_FILENAMELEN) == 0) {
					entries[i].banner_fmt = raw->banner_fmt;
					entries[i].time = raw->time;
					entries[i].icon_addr = raw->icon_addr;
					entries[i].icon_fmt = raw->icon_fmt;
					entries[i].icon_speed = raw->icon_speed;
					break;
				}
			}
		}
	}

	for (int i = 0; i < count; i++) {
		card_manager_read_comment(slot, &entries[i]);
	}

	for (int i = 0; i < count; i++) {
		card_manager_read_graphics(slot, &entries[i]);
	}

	return count;
}
