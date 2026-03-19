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
#include "deviceHandler.h"

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
		u8 gx_fmt = (bnr_fmt == CARD_BANNER_CI) ? GX_TF_CI8 : GX_TF_RGB5A3;
		entry->banner_snap = cm_snap_create(gfx_data, banner_size, gx_fmt,
			CARD_BANNER_W, CARD_BANNER_H);
	}

	// Extract icon frames
	if (total_frames > 0) {
		entry->icon = calloc(1, sizeof(icon_anim));
		if (entry->icon) {
			entry->icon->anim_type = banner_fmt & CARD_ANIM_MASK;
			entry->icon->num_frames = total_frames;
			int last_valid = -1;
			for (int i = 0; i < total_frames; i++) {
				entry->icon->frames[i].speed = ispeeds[i];

				if (ifmts[i] == CARD_ICON_NONE) {
					// Borrowed frame — share snapshot pointer from last valid
					if (last_valid >= 0)
						entry->icon->frames[i].snap = entry->icon->frames[last_valid].snap;
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

				// Build contiguous pixel+tlut buffer for snapshot
				u8 *tmp = (u8 *)memalign(32, alloc_size);
				if (!tmp) break;
				memcpy(tmp, gfx_data + poff, pix_size);
				if (has_tlut)
					memcpy(tmp + pix_size, gfx_data + icon_tlut_off[i], tlut_size);

				u8 gx_fmt = (ifmts[i] == CARD_ICON_RGB) ? GX_TF_RGB5A3 : GX_TF_CI8;
				entry->icon->frames[i].snap = cm_snap_create(tmp, alloc_size,
					gx_fmt, CARD_ICON_W, CARD_ICON_H);
				free(tmp);

				if (!entry->icon->frames[i].snap) break;
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

void card_manager_read_comment(int slot, card_entry *entry) {
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

// --- Graphics cache (SD) ---
// Caches raw banner/icon pixel data on SD to skip slow CARD reads on repeat visits.
// Cache key: gamecode + company + hex(filename). Staleness detected by timestamp.

#define GFX_CACHE_MAGIC 0x53474658  // "SGFX"
#define GFX_CACHE_VER   1

typedef struct __attribute__((packed)) {
	u32 magic;
	u8  version;
	u8  banner_fmt;
	u16 icon_fmt;
	u16 icon_speed;
	u32 time;
	u32 gfx_len;
} gfx_cache_hdr;

static DEVICEHANDLER_INTERFACE *cm_get_sd_dev(void) {
	return devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
}

static void cm_gfx_cache_path(char *out, size_t out_size,
	const card_entry *entry, DEVICEHANDLER_INTERFACE *dev) {
	// Hex-encode filename to avoid FAT issues with special characters
	char hex[CARD_FILENAMELEN * 2 + 1];
	for (int i = 0; i < CARD_FILENAMELEN && entry->filename[i]; i++)
		sprintf(hex + i * 2, "%02x", (u8)entry->filename[i]);
	char rel[128];
	snprintf(rel, sizeof(rel), "swiss/cache/gfx/%.4s%.2s-%s.bin",
		entry->gamecode, entry->company, hex);
	concat_path(out, dev->initial->name, rel);
}

static bool cm_gfx_cache_load(card_entry *entry) {
	DEVICEHANDLER_INTERFACE *dev = cm_get_sd_dev();
	if (!dev) return false;

	file_handle *f = calloc(1, sizeof(file_handle));
	if (!f) return false;
	cm_gfx_cache_path(f->name, sizeof(f->name), entry, dev);

	if (dev->statFile(f) || f->size < sizeof(gfx_cache_hdr)) {
		free(f);
		return false;
	}

	u32 file_size = f->size;
	u8 *buf = (u8 *)memalign(32, file_size);
	if (!buf) { free(f); return false; }

	dev->seekFile(f, 0, DEVICE_HANDLER_SEEK_SET);
	if (dev->readFile(f, buf, file_size) != file_size) {
		dev->closeFile(f);
		free(buf); free(f);
		return false;
	}
	dev->closeFile(f);
	free(f);

	gfx_cache_hdr *hdr = (gfx_cache_hdr *)buf;
	if (hdr->magic != GFX_CACHE_MAGIC || hdr->version != GFX_CACHE_VER
		|| hdr->time != entry->time
		|| hdr->banner_fmt != entry->banner_fmt
		|| hdr->icon_fmt != entry->icon_fmt
		|| hdr->icon_speed != entry->icon_speed
		|| sizeof(gfx_cache_hdr) + hdr->gfx_len > file_size) {
		free(buf);
		return false;
	}

	u8 *gfx = buf + sizeof(gfx_cache_hdr);
	cm_parse_save_graphics(gfx, hdr->gfx_len,
		entry->banner_fmt, entry->icon_fmt, entry->icon_speed, entry);

	free(buf);
	return true;
}

static void cm_gfx_cache_save(const card_entry *entry,
	const u8 *gfx, u32 gfx_len) {
	DEVICEHANDLER_INTERFACE *dev = cm_get_sd_dev();
	if (!dev) return;

	// Ensure cache directories exist (f_mkdir only creates one level)
	file_handle *dir = calloc(1, sizeof(file_handle));
	if (!dir) return;
	concat_path(dir->name, dev->initial->name, "swiss/cache");
	dev->makeDir(dir);
	concat_path(dir->name, dev->initial->name, "swiss/cache/gfx");
	dev->makeDir(dir);
	free(dir);

	file_handle *f = calloc(1, sizeof(file_handle));
	if (!f) return;
	cm_gfx_cache_path(f->name, sizeof(f->name), entry, dev);

	u32 total = sizeof(gfx_cache_hdr) + gfx_len;
	u8 *buf = (u8 *)memalign(32, total);
	if (!buf) { free(f); return; }

	gfx_cache_hdr *hdr = (gfx_cache_hdr *)buf;
	hdr->magic = GFX_CACHE_MAGIC;
	hdr->version = GFX_CACHE_VER;
	hdr->banner_fmt = entry->banner_fmt;
	hdr->icon_fmt = entry->icon_fmt;
	hdr->icon_speed = entry->icon_speed;
	hdr->time = entry->time;
	hdr->gfx_len = gfx_len;
	memcpy(buf + sizeof(gfx_cache_hdr), gfx, gfx_len);

	dev->writeFile(f, buf, total);
	dev->closeFile(f);
	free(buf);
	free(f);
}

void cm_gfx_cache_invalidate(const card_entry *entry) {
	DEVICEHANDLER_INTERFACE *dev = cm_get_sd_dev();
	if (!dev) return;
	file_handle *f = calloc(1, sizeof(file_handle));
	if (!f) return;
	cm_gfx_cache_path(f->name, sizeof(f->name), entry, dev);
	dev->deleteFile(f);
	free(f);
}

void card_manager_read_graphics(int slot, card_entry *entry) {
	if (entry->icon_addr == (u32)-1)
		return;

	// Try SD cache first — much faster than CARD reads
	if (cm_gfx_cache_load(entry))
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

	// Write to SD cache for next time
	cm_gfx_cache_save(entry, gfx, avail);

	free(buf);
}

// Deep-copy graphics from src to dst (which already has metadata copied).
// dst gets its own allocated snapshots — safe to free independently.
void cm_deep_copy_graphics(card_entry *dst, const card_entry *src) {
	dst->banner_snap = NULL;
	dst->icon = NULL;

	// Copy banner
	if (src->banner_snap) {
		dst->banner_snap = cm_snap_create(src->banner_snap->pixels,
			src->banner_snap->pixel_size, src->banner_snap->fmt,
			CARD_BANNER_W, CARD_BANNER_H);
	}

	// Copy icon
	if (src->icon && src->icon->num_frames > 0) {
		dst->icon = calloc(1, sizeof(icon_anim));
		if (!dst->icon) return;
		dst->icon->num_frames = src->icon->num_frames;
		dst->icon->anim_type = src->icon->anim_type;
		int last_valid = -1;
		for (int i = 0; i < src->icon->num_frames; i++) {
			dst->icon->frames[i].speed = src->icon->frames[i].speed;

			cm_tex_snapshot *ss = src->icon->frames[i].snap;
			if (!ss) continue;

			// Borrowed frame — shares snap pointer with a previous frame
			if (last_valid >= 0 && ss == src->icon->frames[last_valid].snap) {
				dst->icon->frames[i].snap = dst->icon->frames[last_valid].snap;
				continue;
			}

			dst->icon->frames[i].snap = cm_snap_create(ss->pixels,
				ss->pixel_size, ss->fmt, CARD_ICON_W, CARD_ICON_H);
			if (!dst->icon->frames[i].snap) break;
			last_valid = i;
		}
	}
}

// Free graphics for entries — snapshots go to retirement queue (safe for video thread).
void card_manager_free_graphics(card_entry *entries, int count) {
	for (int i = 0; i < count; i++) {
		if (entries[i].banner_snap) {
			cm_snap_retire(entries[i].banner_snap);
			entries[i].banner_snap = NULL;
		}
		if (entries[i].icon) {
			// Collect unique snapshots first, then retire.
			// Can't null-and-check because borrowed frames share pointers.
			cm_tex_snapshot *unique[CARD_MAXICONS];
			int nu = 0;
			for (int f = 0; f < entries[i].icon->num_frames; f++) {
				cm_tex_snapshot *s = entries[i].icon->frames[f].snap;
				if (!s) continue;
				bool dup = false;
				for (int u = 0; u < nu; u++) {
					if (unique[u] == s) { dup = true; break; }
				}
				if (!dup)
					unique[nu++] = s;
				entries[i].icon->frames[f].snap = NULL;
			}
			for (int u = 0; u < nu; u++)
				cm_snap_retire(unique[u]);
			free(entries[i].icon);
			entries[i].icon = NULL;
		}
	}
}

// --- Snapshot lifecycle ---
// Heap-allocated texture bundles with frame-based retirement queue.
// Snapshots are never freed immediately — they enter the retirement queue
// and are freed only after CM_RETIRE_DELAY frames have elapsed, guaranteeing
// the video thread is no longer rendering from them.

#define CM_RETIRE_DELAY 3

static struct {
	cm_tex_snapshot **items;
	u32 *stamps;
	int count, cap;
} s_retire;
static u32 s_frame_counter;

cm_tex_snapshot *cm_snap_create(const u8 *pixels, u16 size, u8 fmt,
	u16 w, u16 h) {
	cm_tex_snapshot *s = calloc(1, sizeof(cm_tex_snapshot));
	if (!s) return NULL;
	s->pixels = (u8 *)memalign(32, size);
	if (!s->pixels) { free(s); return NULL; }
	memcpy(s->pixels, pixels, size);
	s->pixel_size = size;
	s->fmt = fmt;
	DCFlushRange(s->pixels, size);

	if (fmt == GX_TF_CI8) {
		u16 pix_size = w * h;
		GX_InitTlutObj(&s->tlut, s->pixels + pix_size, GX_TL_RGB5A3, 256);
		GX_InitTexObjCI(&s->tex, s->pixels, w, h, GX_TF_CI8,
			GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
		GX_InitTexObjUserData(&s->tex, &s->tlut);
	} else {
		GX_InitTexObj(&s->tex, s->pixels, w, h, GX_TF_RGB5A3,
			GX_CLAMP, GX_CLAMP, GX_FALSE);
	}
	GX_InitTexObjFilterMode(&s->tex, GX_LINEAR, GX_NEAR);
	return s;
}

void cm_snap_retire(cm_tex_snapshot *snap) {
	if (!snap) return;
	if (s_retire.count >= s_retire.cap) {
		int new_cap = s_retire.cap ? s_retire.cap * 2 : 128;
		cm_tex_snapshot **ni = realloc(s_retire.items, new_cap * sizeof(*ni));
		if (!ni) {
			free(snap->pixels);
			free(snap);
			return;
		}
		s_retire.items = ni;
		u32 *ns = realloc(s_retire.stamps, new_cap * sizeof(*ns));
		if (!ns) {
			free(snap->pixels);
			free(snap);
			return;
		}
		s_retire.stamps = ns;
		s_retire.cap = new_cap;
	}
	s_retire.items[s_retire.count] = snap;
	s_retire.stamps[s_retire.count] = s_frame_counter;
	s_retire.count++;
}

void cm_retire_tick(void) {
	s_frame_counter++;
	int w = 0;
	for (int r = 0; r < s_retire.count; r++) {
		if (s_frame_counter - s_retire.stamps[r] > CM_RETIRE_DELAY) {
			free(s_retire.items[r]->pixels);
			free(s_retire.items[r]);
		} else {
			s_retire.items[w] = s_retire.items[r];
			s_retire.stamps[w] = s_retire.stamps[r];
			w++;
		}
	}
	s_retire.count = w;
}

void cm_retire_flush(void) {
	for (int i = 0; i < s_retire.count; i++) {
		free(s_retire.items[i]->pixels);
		free(s_retire.items[i]);
	}
	s_retire.count = 0;
	free(s_retire.items);
	free(s_retire.stamps);
	s_retire.items = NULL;
	s_retire.stamps = NULL;
	s_retire.cap = 0;
}

// --- Button wait helper ---
// Waits for any button in mask, keeps activity display refreshed during wait.
// Returns which button(s) were pressed.
u16 cm_wait_buttons(u16 mask) {
	while (1) {
		cm_retire_tick();
		cm_activity_pulse();
		u16 btns = padsButtonsHeld();
		if (btns & mask) {
			while (padsButtonsHeld() & mask) {
				cm_retire_tick();
				VIDEO_WaitVSync();
			}
			return btns & mask;
		}
		VIDEO_WaitVSync();
	}
}

// --- Incremental panel updates ---

bool cm_panel_add_from_gci(cm_panel *panel, GCI *gci, u8 *savedata, u32 save_len) {
	if (panel->num_entries >= 128) return false;

	card_entry *e = &panel->entries[panel->num_entries];
	memset(e, 0, sizeof(card_entry));

	snprintf(e->filename, CARD_FILENAMELEN + 1, "%.*s", CARD_FILENAMELEN, gci->filename);
	memcpy(e->gamecode, gci->gamecode, 4);
	e->gamecode[4] = '\0';
	memcpy(e->company, gci->company, 2);
	e->company[2] = '\0';
	e->filesize = (u32)gci->filesize8 * 8192;
	e->blocks = gci->filesize8;
	e->permissions = gci->permission;
	e->time = gci->time;
	e->banner_fmt = gci->banner_fmt;
	e->icon_addr = gci->icon_addr;
	e->icon_fmt = gci->icon_fmt;
	e->icon_speed = gci->icon_speed;

	// Parse comment (game name + description)
	cm_parse_comment(savedata, save_len, gci->comment_addr, e);

	// Parse graphics (banner + icons)
	if (e->icon_addr != (u32)-1 && e->icon_addr < save_len) {
		u8 *gfx = savedata + e->icon_addr;
		u32 gfx_len = save_len - e->icon_addr;
		cm_parse_save_graphics(gfx, gfx_len, e->banner_fmt, e->icon_fmt, e->icon_speed, e);
	}

	panel->num_entries++;

	// Update animated icons flag
	if (e->icon && e->icon->num_frames > 1)
		panel->has_animated_icons = true;

	return true;
}

void cm_panel_remove_entry(cm_panel *panel, int index) {
	if (index < 0 || index >= panel->num_entries) return;

	// Free graphics for this entry
	card_manager_free_graphics(&panel->entries[index], 1);

	// Shift remaining entries down
	for (int i = index; i < panel->num_entries - 1; i++)
		panel->entries[i] = panel->entries[i + 1];
	panel->num_entries--;
	memset(&panel->entries[panel->num_entries], 0, sizeof(card_entry));

	// Recompute animated icons flag
	panel->has_animated_icons = false;
	for (int i = 0; i < panel->num_entries; i++) {
		if (panel->entries[i].icon && panel->entries[i].icon->num_frames > 1) {
			panel->has_animated_icons = true;
			break;
		}
	}

	// Clamp cursor
	if (panel->cursor >= panel->num_entries && panel->num_entries > 0)
		panel->cursor = panel->num_entries - 1;
}

// Fast directory scan — RAM only, no CARD I/O.
// Populates metadata (filename, gamecode, size, format descriptors) but
// no comments or graphics. Returns entry count.
int card_manager_read_dir(int slot, card_entry *entries, int max_entries) {
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

	// Read banner/icon metadata via CARD_GetStatus
	for (int i = 0; i < count; i++) {
		entries[i].icon_addr = (u32)-1;
		CARD_SetGamecode(entries[i].gamecode);
		CARD_SetCompany(entries[i].company);
		card_file cardfile;
		if (CARD_Open(slot, entries[i].filename, &cardfile) == CARD_ERROR_READY) {
			card_stat cardstat;
			if (CARD_GetStatus(slot, cardfile.filenum, &cardstat) == CARD_ERROR_READY) {
				entries[i].banner_fmt = cardstat.banner_fmt;
				entries[i].time = cardstat.time;
				entries[i].icon_addr = cardstat.icon_addr;
				entries[i].icon_fmt = cardstat.icon_fmt;
				entries[i].icon_speed = cardstat.icon_speed;
			}
			CARD_Close(&cardfile);
		}
	}

	return count;
}

// Read comment text and graphics for a single save entry.
// This is the slow part — involves CARD I/O (or SD cache hit for graphics).
void card_manager_read_save_detail(int slot, card_entry *entry) {
	card_manager_read_comment(slot, entry);
	card_manager_read_graphics(slot, entry);
}

// Convenience wrapper — reads everything in one blocking call.
// Used by library view which doesn't need incremental loading.
int card_manager_read_saves(int slot, card_entry *entries, int max_entries) {
	int count = card_manager_read_dir(slot, entries, max_entries);
	for (int i = 0; i < count; i++) {
		card_manager_read_save_detail(slot, &entries[i]);
		cm_activity_pulse();
	}
	return count;
}
