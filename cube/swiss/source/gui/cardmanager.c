/* cardmanager.c
	- Memory Card Manager UI
	by Swiss contributors
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <ogc/card.h>
#include <ogc/timesupp.h>
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "devices/memcard/deviceHandler-CARD.h"

#define CARD_POLL_INTERVAL 30
#define ICON_SPEED_FAST_TICKS 4
#define ICON_SPEED_MIDDLE_TICKS 8
#define ICON_SPEED_SLOW_TICKS 12

// Grid layout
#define GRID_COLS 6
#define GRID_CELL 42
#define GRID_ROWS_VISIBLE 4
#define GRID_ICON_SIZE 32

// Dual-panel layout (640x480 screen)
// NOTE: DrawEmptyColouredBox adds 10px border on large boxes, 3px on small.
// Panel coords are the INNER content area; rendered box is 10px larger on each side.
#define PANEL_OUTER_X 20
#define PANEL_GAP 40
#define PANEL_WIDTH 280
#define PANEL_TOP_Y 96
#define GRID_TOP_Y 114
#define PANEL_BOTTOM_Y (GRID_TOP_Y + GRID_ROWS_VISIBLE * GRID_CELL + 2)
#define DETAIL_TOP_Y (PANEL_BOTTOM_Y + 6)
#define DETAIL_BANNER_W 96
#define DETAIL_BANNER_H 32

typedef struct {
	u8 num_frames;
	u8 anim_type;	// CARD_ANIM_LOOP or CARD_ANIM_BOUNCE
	struct {
		u8 *data;
		u16 data_size;
		u8 fmt;
		u8 speed;
		GXTexObj tex;
		GXTlutObj tlut;
	} frames[CARD_MAXICONS];
} icon_anim;

typedef struct {
	char filename[CARD_FILENAMELEN + 1];
	char gamecode[5];
	char company[3];
	char game_name[33];
	char game_desc[33];
	u32 filesize;
	u16 blocks;
	u32 fileno;
	u8 permissions;
	u8 banner_fmt;
	u32 icon_addr;
	u16 icon_fmt;
	u16 icon_speed;
	u8 *banner;
	u16 banner_size;
	GXTexObj banner_tex;
	GXTlutObj banner_tlut;
	icon_anim *icon;
} card_entry;

typedef struct {
	int slot;
	card_entry entries[128];
	int num_entries;
	int cursor;
	int scroll_row;
	s32 mem_size;
	s32 sector_size;
	bool card_present;
	bool has_animated_icons;
	bool needs_reload;
} cm_panel;

// --- Flat rectangle drawing ---
// DrawEmptyColouredBox adds 3-10px borders, making it unusable for thin lines.
// Instead, use a tiny solid-color texture with DrawTexObj for pixel-exact rectangles.
static u16 cm_flat_pixels[16] ATTRIBUTE_ALIGN(32);  // 4x4 RGB5A3
static GXTexObj cm_flat_tex;
static bool cm_flat_inited = false;

static void cm_flat_init(void) {
	if (cm_flat_inited) return;
	// Fill with white, semi-transparent (RGB5A3: bit15=0 → ARGB3444)
	// Alpha=5 (~71%), R=F, G=F, B=F → 0x5FFF
	for (int i = 0; i < 16; i++)
		cm_flat_pixels[i] = 0x5FFF;
	DCFlushRange(cm_flat_pixels, sizeof(cm_flat_pixels));
	GX_InitTexObj(&cm_flat_tex, cm_flat_pixels, 4, 4, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&cm_flat_tex, GX_LINEAR, GX_NEAR);
	cm_flat_inited = true;
}

// Draw a flat semi-transparent white rectangle at exact pixel coordinates
static uiDrawObj_t *cm_draw_rect(int x, int y, int w, int h) {
	return DrawTexObj(&cm_flat_tex, x, y, w, h, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

// --- Logging ---
// Uses Swiss device handler API (not fopen) since FAT devoptab isn't available

#define CM_LOG_MAX 16384
static char cm_log_buf[CM_LOG_MAX];
static int cm_log_len = 0;
static DEVICEHANDLER_INTERFACE *cm_log_dev = NULL;

static void cm_log_open(void) {
	cm_log_len = 0;
	cm_log_dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (cm_log_dev) {
		cm_log_len += snprintf(cm_log_buf + cm_log_len, CM_LOG_MAX - cm_log_len,
			"\n--- Card Manager session started ---\n");
	}
}

static void cm_log(const char *fmt, ...) {
	if (!cm_log_dev || cm_log_len >= CM_LOG_MAX - 1) return;
	va_list args;
	va_start(args, fmt);
	cm_log_len += vsnprintf(cm_log_buf + cm_log_len, CM_LOG_MAX - cm_log_len, fmt, args);
	va_end(args);
	if (cm_log_len < CM_LOG_MAX - 1) {
		cm_log_buf[cm_log_len++] = '\n';
		cm_log_buf[cm_log_len] = '\0';
	}
}

static void cm_log_close(void) {
	if (!cm_log_dev || cm_log_len == 0) return;
	cm_log_len += snprintf(cm_log_buf + cm_log_len, CM_LOG_MAX - cm_log_len,
		"--- Card Manager session ended ---\n");

	file_handle *logFile = calloc(1, sizeof(file_handle));
	if (logFile) {
		concat_path(logFile->name, cm_log_dev->initial->name, "cardmanager.log");
		// Read existing content if present
		char *existing = NULL;
		u32 existing_len = 0;
		if (!cm_log_dev->statFile(logFile) && logFile->size) {
			existing = calloc(1, logFile->size);
			if (existing) {
				cm_log_dev->readFile(logFile, existing, logFile->size);
				existing_len = logFile->size;
			}
			cm_log_dev->closeFile(logFile);
		}
		// Combine existing + new into single buffer and write once
		u32 total_len = existing_len + cm_log_len;
		char *combined = calloc(1, total_len);
		if (combined) {
			if (existing && existing_len)
				memcpy(combined, existing, existing_len);
			memcpy(combined + existing_len, cm_log_buf, cm_log_len);
			cm_log_dev->deleteFile(logFile);
			cm_log_dev->writeFile(logFile, combined, total_len);
			cm_log_dev->closeFile(logFile);
			free(combined);
		}
		free(existing);
		free(logFile);
	}
	cm_log_dev = NULL;
	cm_log_len = 0;
}

static void cm_log_card_info(int slot, s32 mem_size, s32 sector_size, int num_entries) {
	cm_log("Slot %c: mem_size=%d, sector_size=%d, entries=%d",
		slot == CARD_SLOTA ? 'A' : 'B', (int)mem_size, (int)sector_size, num_entries);
}

static void cm_log_entry(int idx, card_entry *entry) {
	if (entry->game_name[0]) {
		cm_log("  [%d] \"%s\" (%s) code=%s company=%s blocks=%d size=%dK",
			idx, entry->filename, entry->game_name, entry->gamecode,
			entry->company, entry->blocks, entry->filesize / 1024);
	}
	else {
		cm_log("  [%d] \"%s\" code=%s company=%s blocks=%d size=%dK",
			idx, entry->filename, entry->gamecode, entry->company,
			entry->blocks, entry->filesize / 1024);
	}
}

// --- Card reading ---

static void card_manager_read_comment(int slot, card_entry *entry) {
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);

	card_file cardfile;
	if (CARD_Open(slot, entry->filename, &cardfile) != CARD_ERROR_READY)
		return;

	card_stat cardstat;
	if (CARD_GetStatus(slot, cardfile.filenum, &cardstat) == CARD_ERROR_READY
		&& cardstat.comment_addr != (u32)-1) {
		// Read 64-byte comment block: first 32 = game name, next 32 = description
		// CARD_Read requires sector-aligned offsets (8192 bytes)
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

	// Compute offsets ourselves from raw GCI fields (bypass CARD_GetStatus bugs)
	// Unpack per-frame format and speed from bitfields (2 bits each, 8 frames)
	u8 ifmts[CARD_MAXICONS];
	u8 ispeeds[CARD_MAXICONS];
	int total_frames = 0;
	int nicons_ci_shared = 0;
	for (int i = 0; i < CARD_MAXICONS; i++) {
		ispeeds[i] = (entry->icon_speed >> (2 * i)) & CARD_SPEED_MASK;
		ifmts[i] = (entry->icon_fmt >> (2 * i)) & CARD_ICON_MASK;
		if (ispeeds[i] == CARD_SPEED_END) {
			ifmts[i] = CARD_ICON_NONE;  // speed=END forces format to NONE
			break;
		}
		total_frames++;
		if (ifmts[i] == CARD_ICON_CI) nicons_ci_shared++;
	}

	// Banner size
	u8 bnr_fmt = entry->banner_fmt & CARD_BANNER_MASK;
	u32 banner_size = 0;
	if (bnr_fmt == CARD_BANNER_CI)
		banner_size = CARD_BANNER_W * CARD_BANNER_H + 256 * 2;  // 3584
	else if (bnr_fmt == CARD_BANNER_RGB)
		banner_size = CARD_BANNER_W * CARD_BANNER_H * 2;  // 6144

	// Compute icon pixel offsets (relative to icon_addr)
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
		// Point all CI shared frames to the shared TLUT
		for (int i = 0; i < total_frames; i++) {
			if (ifmts[i] == CARD_ICON_CI)
				icon_tlut_off[i] = shared_tlut_off;
		}
	}

	u32 total_size = accum;  // total graphics data from icon_addr
	if (total_size == 0) {
		CARD_Close(&cardfile);
		return;
	}

	// Read all graphics data from card in sector-aligned chunks
	u32 sector_offset = entry->icon_addr & ~(8192 - 1);
	u32 off_in_sector = entry->icon_addr - sector_offset;
	u32 read_len = (off_in_sector + total_size + 8191) & ~(8192 - 1);
	// Don't exceed file size (read starts at sector_offset, not 0)
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

	u8 *gfx = buf + off_in_sector;  // pointer to start of graphics data at icon_addr
	u32 avail = read_len - off_in_sector;
	if (total_size > avail) total_size = avail;

	// Extract banner
	if (banner_size > 0 && banner_size <= total_size) {
		entry->banner = (u8 *)memalign(32, banner_size);
		if (entry->banner) {
			memcpy(entry->banner, gfx, banner_size);
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
	// NONE-format frames mean "hold previous frame" — we store the frame index
	// they should display as a mapping in the icon_anim structure.
	if (total_frames > 0) {
		entry->icon = calloc(1, sizeof(icon_anim));
		if (entry->icon) {
			entry->icon->anim_type = entry->banner_fmt & CARD_ANIM_MASK;
			entry->icon->num_frames = total_frames;
			int last_valid = -1;
			for (int i = 0; i < total_frames; i++) {
				entry->icon->frames[i].fmt = ifmts[i];
				entry->icon->frames[i].speed = ispeeds[i];

				if (ifmts[i] == CARD_ICON_NONE) {
					// Hold previous frame: copy its texture references
					if (last_valid >= 0) {
						entry->icon->frames[i].data = entry->icon->frames[last_valid].data;
						entry->icon->frames[i].data_size = 0;  // 0 = borrowed, don't free
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

				// Bounds check
				if (poff + pix_size > total_size) break;
				if (has_tlut && icon_tlut_off[i] + tlut_size > total_size) break;

				entry->icon->frames[i].data = (u8 *)memalign(32, alloc_size);
				if (!entry->icon->frames[i].data) break;
				entry->icon->frames[i].data_size = alloc_size;

				memcpy(entry->icon->frames[i].data, gfx + poff, pix_size);
				if (has_tlut)
					memcpy(entry->icon->frames[i].data + pix_size, gfx + icon_tlut_off[i], tlut_size);
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
			// Check if we got at least one valid frame
			if (last_valid < 0) {
				free(entry->icon);
				entry->icon = NULL;
			}
		}
	}

	free(buf);
}

static void card_manager_free_graphics(card_entry *entries, int count) {
	for (int i = 0; i < count; i++) {
		if (entries[i].banner) {
			free(entries[i].banner);
			entries[i].banner = NULL;
		}
		if (entries[i].icon) {
			for (int f = 0; f < entries[i].icon->num_frames; f++) {
				// data_size==0 means borrowed pointer (NONE frame), don't free
				if (entries[i].icon->frames[f].data_size > 0)
					free(entries[i].icon->frames[f].data);
			}
			free(entries[i].icon);
			entries[i].icon = NULL;
		}
	}
}

static int card_manager_read_saves(int slot, card_entry *entries, int max_entries) {
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
					&& memcmp(raw->filename, entries[i].filename, CARD_FILENAMELEN) == 0) {
					entries[i].banner_fmt = raw->banner_fmt;
					entries[i].icon_addr = raw->icon_addr;
					entries[i].icon_fmt = raw->icon_fmt;
					entries[i].icon_speed = raw->icon_speed;
					break;
				}
			}
		}
	}

	// Read comment blocks for game names
	for (int i = 0; i < count; i++) {
		card_manager_read_comment(slot, &entries[i]);
	}

	// Read banner images
	for (int i = 0; i < count; i++) {
		card_manager_read_graphics(slot, &entries[i]);
	}

	return count;
}

// --- Drawing ---
// Uses a single fixed-size container for all states to avoid pop-in/out

static int icon_anim_get_frame(icon_anim *icon, u32 tick) {
	if (!icon || icon->num_frames <= 1) return 0;
	// Calculate total ticks per animation cycle
	u32 total_ticks = 0;
	for (int i = 0; i < icon->num_frames; i++) {
		switch (icon->frames[i].speed) {
			case CARD_SPEED_FAST:   total_ticks += ICON_SPEED_FAST_TICKS; break;
			case CARD_SPEED_MIDDLE: total_ticks += ICON_SPEED_MIDDLE_TICKS; break;
			case CARD_SPEED_SLOW:   total_ticks += ICON_SPEED_SLOW_TICKS; break;
			default: total_ticks += ICON_SPEED_MIDDLE_TICKS; break;
		}
	}
	if (total_ticks == 0) return 0;
	u32 cycle_ticks = (icon->anim_type & CARD_ANIM_BOUNCE)
		? total_ticks * 2 - 2  // bounce: 0,1,2,...,N-1,N-2,...,1
		: total_ticks;
	u32 pos = tick % cycle_ticks;
	// Map position to frame
	bool reverse = false;
	if ((icon->anim_type & CARD_ANIM_BOUNCE) && pos >= total_ticks) {
		pos = cycle_ticks - pos;
		reverse = true;
	}
	(void)reverse;
	u32 accum = 0;
	for (int i = 0; i < icon->num_frames; i++) {
		u32 dur;
		switch (icon->frames[i].speed) {
			case CARD_SPEED_FAST:   dur = ICON_SPEED_FAST_TICKS; break;
			case CARD_SPEED_MIDDLE: dur = ICON_SPEED_MIDDLE_TICKS; break;
			case CARD_SPEED_SLOW:   dur = ICON_SPEED_SLOW_TICKS; break;
			default: dur = ICON_SPEED_MIDDLE_TICKS; break;
		}
		accum += dur;
		if (pos < accum) return i;
	}
	return 0;
}

static void card_manager_draw_panel(uiDrawObj_t *container, cm_panel *panel,
	int px, int pw, bool active, u32 anim_tick) {

	// Header
	char header[64];
	if (panel->card_present) {
		int total = (panel->mem_size << 20 >> 3) / panel->sector_size - 5;
		int used = 0;
		for (int i = 0; i < panel->num_entries; i++) used += panel->entries[i].blocks;
		sprintf(header, "Slot %c  %d/%d free",
			panel->slot == CARD_SLOTA ? 'A' : 'B', total - used, total);
	} else {
		sprintf(header, "Slot %c", panel->slot == CARD_SLOTA ? 'A' : 'B');
	}
	GXColor hdr_color = active ? defaultColor : (GXColor){160, 160, 160, 255};
	DrawAddChild(container, DrawStyledLabel(px + pw / 2, PANEL_TOP_Y + 8, header,
		0.55f, ALIGN_CENTER, hdr_color));

	// Panel border (1px outline, always visible)
	DrawAddChild(container, cm_draw_rect(px, PANEL_TOP_Y, pw, 1));                          // top
	DrawAddChild(container, cm_draw_rect(px, PANEL_BOTTOM_Y, pw, 1));                       // bottom
	DrawAddChild(container, cm_draw_rect(px, PANEL_TOP_Y, 1, PANEL_BOTTOM_Y - PANEL_TOP_Y)); // left
	DrawAddChild(container, cm_draw_rect(px + pw - 1, PANEL_TOP_Y, 1, PANEL_BOTTOM_Y - PANEL_TOP_Y)); // right

	if (!panel->card_present) {
		DrawAddChild(container, DrawStyledLabel(px + pw / 2, GRID_TOP_Y + 60,
			"No Card", 0.65f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}
	if (panel->num_entries == 0) {
		DrawAddChild(container, DrawStyledLabel(px + pw / 2, GRID_TOP_Y + 60,
			"Empty", 0.65f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}

	// Icon grid
	int grid_x = px + (pw - GRID_COLS * GRID_CELL) / 2;
	for (int row = 0; row < GRID_ROWS_VISIBLE; row++) {
		for (int col = 0; col < GRID_COLS; col++) {
			int idx = (panel->scroll_row + row) * GRID_COLS + col;
			if (idx >= panel->num_entries) break;

			int cx = grid_x + col * GRID_CELL;
			int cy = GRID_TOP_Y + row * GRID_CELL;
			int ix = cx + (GRID_CELL - GRID_ICON_SIZE) / 2;
			int iy = cy + (GRID_CELL - GRID_ICON_SIZE) / 2;

			// Selection highlight (1px border, pixel-exact via cm_draw_rect)
			if (active && idx == panel->cursor) {
				int sx = ix - 2, sy = iy - 2;
				int sw = GRID_ICON_SIZE + 4, sh = GRID_ICON_SIZE + 4;
				DrawAddChild(container, cm_draw_rect(sx, sy, sw, 1));       // top
				DrawAddChild(container, cm_draw_rect(sx, sy + sh - 1, sw, 1)); // bottom
				DrawAddChild(container, cm_draw_rect(sx, sy, 1, sh));       // left
				DrawAddChild(container, cm_draw_rect(sx + sw - 1, sy, 1, sh)); // right
			}

			// Draw icon (animated)
			card_entry *e = &panel->entries[idx];
			if (e->icon && e->icon->num_frames > 0) {
				int frame = icon_anim_get_frame(e->icon, anim_tick);
				if (e->icon->frames[frame].data) {
					DrawAddChild(container, DrawTexObj(&e->icon->frames[frame].tex,
						ix, iy, GRID_ICON_SIZE, GRID_ICON_SIZE,
						0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
				}
			} else if (e->banner) {
				// Fallback: show banner cropped to square
				DrawAddChild(container, DrawTexObj(&e->banner_tex,
					ix, iy, GRID_ICON_SIZE, GRID_ICON_SIZE,
					0, 0.33f, 0.67f, 0.0f, 1.0f, 0));
			}
		}
	}

	// Scroll indicator
	int total_rows = (panel->num_entries + GRID_COLS - 1) / GRID_COLS;
	if (total_rows > GRID_ROWS_VISIBLE) {
		float pct = (float)panel->scroll_row / (float)(total_rows - GRID_ROWS_VISIBLE);
		DrawAddChild(container, DrawVertScrollBar(
			px + pw - 10, GRID_TOP_Y, 6,
			GRID_ROWS_VISIBLE * GRID_CELL, pct, 16));
	}

}

static void card_manager_draw_detail(uiDrawObj_t *container, card_entry *sel) {
	int dx = PANEL_OUTER_X + 10;
	int dy = DETAIL_TOP_Y;
	int detail_w = 640 - 2 * PANEL_OUTER_X - 20;

	// Separator line spanning full width
	DrawAddChild(container, cm_draw_rect(PANEL_OUTER_X + 4, dy - 4,
		640 - 2 * PANEL_OUTER_X - 8, 1));

	// Banner on the right
	int text_w = detail_w;
	if (sel->banner) {
		int bx = 640 - PANEL_OUTER_X - DETAIL_BANNER_W - 10;
		DrawAddChild(container, DrawTexObj(&sel->banner_tex,
			bx, dy, DETAIL_BANNER_W, DETAIL_BANNER_H,
			0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		text_w = detail_w - DETAIL_BANNER_W - 16;
	}

	// Title (larger font)
	char *name = sel->game_name[0] ? sel->game_name : sel->filename;
	float name_scale = GetTextScaleToFitInWidthWithMax(name, text_w, 0.65f);
	DrawAddChild(container, DrawStyledLabel(dx, dy + 6, name,
		name_scale, ALIGN_LEFT, defaultColor));

	// Stats line
	char stats[64];
	snprintf(stats, sizeof(stats), "%s  %d blocks  %dK",
		sel->gamecode, sel->blocks, sel->filesize / 1024);
	DrawAddChild(container, DrawStyledLabel(dx, dy + 22, stats,
		0.5f, ALIGN_LEFT, (GXColor){160, 160, 160, 255}));
}

static uiDrawObj_t *card_manager_draw(cm_panel *left, cm_panel *right,
	int active_panel, u32 anim_tick) {

	// Detail area height depends on whether there's a selection
	cm_panel *active_p = active_panel == 0 ? left : right;
	bool has_sel = active_p->card_present && active_p->num_entries > 0 &&
		active_p->cursor >= 0 && active_p->cursor < active_p->num_entries;
	int bottom_y = has_sel ? DETAIL_TOP_Y + 40 : PANEL_BOTTOM_Y + 4;

	uiDrawObj_t *container = DrawEmptyBox(PANEL_OUTER_X, 74,
		640 - PANEL_OUTER_X, bottom_y + 22);

	// Title
	DrawAddChild(container, DrawStyledLabel(640 / 2, 84,
		"Memory Card Manager", 0.6f, ALIGN_CENTER, defaultColor));

	int left_x = PANEL_OUTER_X + 2;
	int right_x = PANEL_OUTER_X + PANEL_WIDTH + PANEL_GAP;

	card_manager_draw_panel(container, left, left_x, PANEL_WIDTH,
		active_panel == 0, anim_tick);
	card_manager_draw_panel(container, right, right_x, PANEL_WIDTH,
		active_panel == 1, anim_tick);

	// Shared detail area below both panels
	if (has_sel) {
		card_manager_draw_detail(container, &active_p->entries[active_p->cursor]);
	}

	// Button hints
	DrawAddChild(container, DrawStyledLabel(640 / 2, bottom_y + 12,
		"A: Export  X: Import  Z: Delete  L/R: Switch  B: Back",
		0.4f, ALIGN_CENTER, (GXColor){160, 160, 160, 255}));

	return container;
}

// --- Delete ---

static bool card_manager_confirm_delete(const char *filename) {
	char msg[256];
	sprintf(msg, "Delete \"%s\"?\n \nPress L + A to confirm, or B to cancel.", filename);
	uiDrawObj_t *msgBox = DrawMessageBox(D_WARN, msg);
	DrawPublish(msgBox);

	bool confirmed = false;
	while (1) {
		u16 btns = padsButtonsHeld();
		if ((btns & (PAD_BUTTON_A | PAD_TRIGGER_L)) == (PAD_BUTTON_A | PAD_TRIGGER_L)) {
			confirmed = true;
			break;
		}
		if (btns & PAD_BUTTON_B) break;
		VIDEO_WaitVSync();
	}
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_TRIGGER_L | PAD_BUTTON_B)) {
		VIDEO_WaitVSync();
	}
	DrawDispose(msgBox);
	return confirmed;
}

static bool card_manager_delete_save(int slot, card_entry *entry) {
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	s32 ret = CARD_Delete(slot, entry->filename);
	if (ret != CARD_ERROR_READY) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "Delete failed.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & PAD_BUTTON_A)) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}
	return true;
}

// --- Export (card → GCI on SD) ---

static bool card_manager_export_save(int slot, card_entry *entry, s32 sector_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "No storage device available.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Show progress
	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Exporting save...");
	DrawPublish(msgBox);

	// Open card file
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	card_file cardfile;
	s32 ret = CARD_Open(slot, entry->filename, &cardfile);
	if (ret != CARD_ERROR_READY) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to open card file.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Build GCI header from the raw directory entry in sys_area.
	// CARD_GetStatus transforms some fields (strips banner animation bits),
	// so we read directly from sys_area for a faithful copy.
	GCI gci;
	memset(&gci, 0, sizeof(GCI));
	gci.reserved01 = 0xFF;
	gci.reserved02 = 0xFFFF;
	gci.filesize8 = entry->filesize / 8192;

	bool got_raw = false;
	unsigned char *sysarea = get_card_sys_area(slot);
	if (sysarea) {
		// Find raw directory entry at the specific fileno position
		u32 entry_off = cardfile.filenum * 64;
		GCI *raw = NULL;
		for (int blk = 0; blk < 5; blk++) {
			GCI *candidate = (GCI *)(sysarea + blk * 8192 + entry_off);
			if (memcmp(candidate->gamecode, entry->gamecode, 4) == 0
				&& memcmp(candidate->company, entry->company, 2) == 0
				&& memcmp(candidate->filename, entry->filename, CARD_FILENAMELEN) == 0) {
				raw = candidate;
			}
		}
		if (raw) {
			memcpy(gci.gamecode, raw->gamecode, 4);
			memcpy(gci.company, raw->company, 2);
			memcpy(gci.filename, raw->filename, CARD_FILENAMELEN);
			gci.banner_fmt = raw->banner_fmt;
			gci.time = raw->time;
			gci.icon_addr = raw->icon_addr;
			gci.icon_fmt = raw->icon_fmt;
			gci.icon_speed = raw->icon_speed;
			gci.permission = raw->permission;
			gci.copy_counter = raw->copy_counter;
			gci.comment_addr = raw->comment_addr;
			got_raw = true;
			cm_log("  Read raw direntry from sys_area");
		}
	}
	if (!got_raw) {
		// Fallback to CARD_GetStatus (may lose banner animation bits)
		card_stat cardstat;
		CARD_GetStatus(slot, cardfile.filenum, &cardstat);
		u8 file_attr = 0;
		CARD_GetAttributes(slot, cardfile.filenum, &file_attr);
		memcpy(gci.gamecode, cardstat.gamecode, 4);
		memcpy(gci.company, cardstat.company, 2);
		memcpy(gci.filename, cardstat.filename, CARD_FILENAMELEN);
		gci.banner_fmt = cardstat.banner_fmt;
		gci.time = cardstat.time;
		gci.icon_addr = cardstat.icon_addr;
		gci.icon_fmt = cardstat.icon_fmt;
		gci.icon_speed = cardstat.icon_speed;
		gci.permission = file_attr;
		gci.comment_addr = cardstat.comment_addr;
		cm_log("  WARNING: raw direntry not found, fell back to CARD_GetStatus");
	}

	cm_log("  GCI header: filesize8=%d banner_fmt=0x%02x icon_addr=0x%x icon_fmt=0x%04x icon_speed=0x%04x comment_addr=0x%x perms=0x%02x copy=%d",
		gci.filesize8, gci.banner_fmt, gci.icon_addr, gci.icon_fmt, gci.icon_speed, gci.comment_addr, gci.permission, gci.copy_counter);

	// Read save data from card
	u8 *savedata = (u8 *)memalign(32, entry->filesize);
	if (!savedata) {
		CARD_Close(&cardfile);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Out of memory.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	u8 *read_buf = (u8 *)memalign(32, sector_size);
	bool read_ok = true;
	u32 offset = 0;
	while (offset < entry->filesize) {
		ret = CARD_Read(&cardfile, read_buf, sector_size, offset);
		if (ret != CARD_ERROR_READY) {
			// Retry once
			usleep(2000);
			ret = CARD_Read(&cardfile, read_buf, sector_size, offset);
		}
		cm_log("  CARD_Read offset=%u len=%d ret=%d", offset, sector_size, (int)ret);
		if (ret != CARD_ERROR_READY) {
			read_ok = false;
			break;
		}
		u32 copy_len = entry->filesize - offset;
		if (copy_len > (u32)sector_size) copy_len = sector_size;
		memcpy(savedata + offset, read_buf, copy_len);
		offset += sector_size;
	}
	free(read_buf);
	CARD_Close(&cardfile);

	// Log data fingerprint for verification
	if (read_ok && entry->filesize >= 16) {
		cm_log("  Export data: first4=%02x%02x%02x%02x last4=%02x%02x%02x%02x total=%u",
			savedata[0], savedata[1], savedata[2], savedata[3],
			savedata[entry->filesize-4], savedata[entry->filesize-3],
			savedata[entry->filesize-2], savedata[entry->filesize-1],
			entry->filesize);
	}

	if (!read_ok) {
		free(savedata);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to read card data.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Write GCI file to SD: swiss/saves/<gamecode>-<filename>.gci
	file_handle *outFile = calloc(1, sizeof(file_handle));
	if (!outFile) {
		free(savedata);
		DrawDispose(msgBox);
		return false;
	}

	// Ensure swiss/saves/ directory exists
	file_handle *savesDir = calloc(1, sizeof(file_handle));
	if (savesDir) {
		concat_path(savesDir->name, dev->initial->name, "swiss/saves");
		dev->makeDir(savesDir);
		free(savesDir);
	}

	char gci_name[64];
	snprintf(gci_name, sizeof(gci_name), "swiss/saves/%.4s-%.32s.gci",
		entry->gamecode, entry->filename);
	concat_path(outFile->name, dev->initial->name, gci_name);

	// Check if file already exists
	if (!dev->statFile(outFile)) {
		char overwrite_msg[128];
		snprintf(overwrite_msg, sizeof(overwrite_msg),
			"%.4s-%.32s.gci already exists.\n \nA: Overwrite  |  X: Keep Both  |  B: Cancel",
			entry->gamecode, entry->filename);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_WARN, overwrite_msg);
		DrawPublish(msgBox);
		int choice = 0; // 0=pending, 1=overwrite, 2=keep both, 3=cancel
		while (!choice) {
			u16 btn = padsButtonsHeld();
			if (btn & PAD_BUTTON_A) choice = 1;
			else if (btn & PAD_BUTTON_X) choice = 2;
			else if (btn & PAD_BUTTON_B) choice = 3;
			VIDEO_WaitVSync();
		}
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_X | PAD_BUTTON_B)) {
			VIDEO_WaitVSync();
		}
		DrawDispose(msgBox);
		if (choice == 3) {
			free(savedata);
			free(outFile);
			return false;
		}
		if (choice == 2) {
			// Find next available numbered filename
			for (int n = 2; n <= 99; n++) {
				snprintf(gci_name, sizeof(gci_name), "swiss/saves/%.4s-%.32s (%d).gci",
					entry->gamecode, entry->filename, n);
				concat_path(outFile->name, dev->initial->name, gci_name);
				if (dev->statFile(outFile)) break; // doesn't exist, use it
			}
		}
		else {
			// Overwrite: delete existing
			dev->deleteFile(outFile);
		}
		msgBox = DrawMessageBox(D_INFO, "Exporting save...");
		DrawPublish(msgBox);
	}

	// Combine GCI header + save data into single buffer for one atomic write
	u32 gci_total = sizeof(GCI) + entry->filesize;
	u8 *gcibuf = (u8 *)memalign(32, gci_total);
	if (!gcibuf) {
		free(savedata);
		free(outFile);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Out of memory.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}
	memcpy(gcibuf, &gci, sizeof(GCI));
	memcpy(gcibuf + sizeof(GCI), savedata, entry->filesize);
	free(savedata);

	s32 write_ret = dev->writeFile(outFile, gcibuf, gci_total);
	dev->closeFile(outFile);
	cm_log("  Wrote GCI: %u bytes (header=%u + data=%u), writeFile ret=%d",
		gci_total, (u32)sizeof(GCI), entry->filesize, (int)write_ret);
	free(gcibuf);
	free(outFile);

	DrawDispose(msgBox);
	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Exported %.4s-%.32s.gci",
		entry->gamecode, entry->filename);
	msgBox = DrawMessageBox(D_INFO, done_msg);
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
	DrawDispose(msgBox);
	return true;
}

// --- Import (GCI from SD → card) ---

#define MAX_GCI_FILES 64

typedef struct {
	char path[PATHNAME_MAX];
	char display[48];
	u32 filesize;
} gci_file_entry;

static int card_manager_scan_gci_files(gci_file_entry *gci_files, int max_files) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return 0;

	file_handle *dir = calloc(1, sizeof(file_handle));
	if (!dir) return 0;
	concat_path(dir->name, dev->initial->name, "swiss/saves");

	if (dev->statFile(dir)) {
		free(dir);
		return 0;
	}

	// Read directory
	file_handle *dirEntries = NULL;
	int count = 0;

	s32 num_dir = dev->readDir(dir, &dirEntries, -1);
	if (num_dir > 0 && dirEntries) {
		for (int i = 0; i < num_dir && count < max_files; i++) {
			char *name = getRelativeName(dirEntries[i].name);
			int len = strlen(name);
			if (len > 4 && !strcasecmp(name + len - 4, ".gci")) {
				strlcpy(gci_files[count].path, dirEntries[i].name, PATHNAME_MAX);
				snprintf(gci_files[count].display, sizeof(gci_files[count].display),
					"%.47s", name);
				gci_files[count].filesize = dirEntries[i].size;
				count++;
			}
		}
		free(dirEntries);
	}
	free(dir);
	return count;
}

static int card_manager_pick_gci(gci_file_entry *files, int num_files) {
	if (num_files == 0) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "No .gci files found in swiss/saves/");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return -1;
	}

	int cursor = 0;
	int scroll = 0;
	int max_visible = 8;

	while (1) {
		uiDrawObj_t *container = DrawEmptyBox(40, 80, 600, 400);
		DrawAddChild(container, DrawStyledLabel(640 / 2, 90,
			"Select GCI to import", 0.7f, ALIGN_CENTER, defaultColor));

		int visible = num_files - scroll;
		if (visible > max_visible) visible = max_visible;

		for (int i = 0; i < visible; i++) {
			int idx = scroll + i;
			int y = 120 + i * 30 + 15;
			GXColor color = (idx == cursor) ?
				(GXColor){96, 107, 164, 255} : defaultColor;
			if (idx == cursor) {
				DrawAddChild(container, DrawEmptyColouredBox(
					50, 120 + i * 30, 590, 120 + (i + 1) * 30,
					(GXColor){96, 107, 164, 80}));
			}
			DrawAddChild(container, DrawStyledLabel(60, y,
				files[idx].display, 0.55f, ALIGN_LEFT, color));
		}

		DrawAddChild(container, DrawStyledLabel(640 / 2, 385,
			"A: Import  |  B: Cancel", 0.55f, ALIGN_CENTER, defaultColor));
		DrawPublish(container);

		while (1) {
			VIDEO_WaitVSync();
			u16 btns = padsButtonsHeld();
			if (!btns) continue;

			bool redraw = false;
			if ((btns & PAD_BUTTON_UP) && cursor > 0) {
				cursor--;
				if (cursor < scroll) scroll = cursor;
				redraw = true;
			}
			if ((btns & PAD_BUTTON_DOWN) && cursor < num_files - 1) {
				cursor++;
				if (cursor >= scroll + max_visible) scroll = cursor - max_visible + 1;
				redraw = true;
			}
			if (btns & PAD_BUTTON_A) {
				while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
				DrawDispose(container);
				return cursor;
			}
			if (btns & PAD_BUTTON_B) {
				while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
				DrawDispose(container);
				return -1;
			}
			if (redraw) {
				while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN)) {
					VIDEO_WaitVSync();
				}
				break;
			}
		}
		DrawDispose(container);
	}
}

static bool card_manager_import_gci(int slot, gci_file_entry *gci_entry, s32 sector_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Importing save...");
	DrawPublish(msgBox);

	// Read GCI file
	file_handle *inFile = calloc(1, sizeof(file_handle));
	if (!inFile) {
		DrawDispose(msgBox);
		return false;
	}
	strlcpy(inFile->name, gci_entry->path, PATHNAME_MAX);

	if (dev->statFile(inFile)) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "File not found.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		free(inFile);
		return false;
	}

	u32 total_size = inFile->size;
	if (total_size <= sizeof(GCI)) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Invalid GCI file (too small).");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		free(inFile);
		return false;
	}

	u8 *filebuf = (u8 *)memalign(32, total_size);
	if (!filebuf) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Out of memory.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		free(inFile);
		return false;
	}

	s32 read_ret = dev->readFile(inFile, filebuf, total_size);
	dev->closeFile(inFile);
	free(inFile);
	cm_log("  Read GCI: requested=%u, readFile ret=%d", total_size, (int)read_ret);

	// Parse GCI header
	GCI *gci = (GCI *)filebuf;
	u8 *savedata = filebuf + sizeof(GCI);
	u32 save_len = total_size - sizeof(GCI);
	u32 expected_len = gci->filesize8 * 8192;

	cm_log("  GCI header: filesize8=%d banner_fmt=0x%02x icon_addr=0x%x icon_fmt=0x%04x icon_speed=0x%04x comment_addr=0x%x perms=0x%02x",
		gci->filesize8, gci->banner_fmt, gci->icon_addr, gci->icon_fmt, gci->icon_speed, gci->comment_addr, gci->permission);

	if (expected_len >= 16) {
		cm_log("  Import data: first4=%02x%02x%02x%02x last4=%02x%02x%02x%02x total=%u",
			savedata[0], savedata[1], savedata[2], savedata[3],
			savedata[expected_len-4], savedata[expected_len-3],
			savedata[expected_len-2], savedata[expected_len-1],
			expected_len);
	}

	if (save_len != expected_len) {
		free(filebuf);
		DrawDispose(msgBox);
		char errmsg[128];
		snprintf(errmsg, sizeof(errmsg), "GCI size mismatch: got %dK, expected %dK",
			(int)(save_len / 1024), (int)(expected_len / 1024));
		msgBox = DrawMessageBox(D_FAIL, errmsg);
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Set gamecode/company from GCI
	char gamecode[8], company[4];
	memset(gamecode, 0, sizeof(gamecode));
	memset(company, 0, sizeof(company));
	memcpy(gamecode, gci->gamecode, 4);
	memcpy(company, gci->company, 2);
	CARD_SetGamecode(gamecode);
	CARD_SetCompany(company);

	// Get filename from GCI
	char filename[CARD_FILENAMELEN + 1];
	memset(filename, 0, sizeof(filename));
	memcpy(filename, gci->filename, CARD_FILENAMELEN);

	// Open or create file on card
	card_file cardfile;
	bool is_overwrite = false;
	s32 ret = CARD_Open(slot, filename, &cardfile);
	if (ret == CARD_ERROR_NOFILE) {
		ret = CARD_Create(slot, filename, expected_len, &cardfile);
		if (ret != CARD_ERROR_READY) {
			free(filebuf);
			DrawDispose(msgBox);
			msgBox = DrawMessageBox(D_FAIL, "Failed to create file on card.\nCard may be full.");
			DrawPublish(msgBox);
			while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
			while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
			DrawDispose(msgBox);
			return false;
		}
		ret = CARD_Open(slot, filename, &cardfile);
		cm_log("  Created new file on card");
	}
	else if (ret == CARD_ERROR_READY) {
		// File exists — confirm overwrite
		CARD_Close(&cardfile);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_WARN, "Save already exists on card.\n \nA: Overwrite  |  B: Cancel");
		DrawPublish(msgBox);
		int choice = 0;
		while (!choice) {
			u16 btn = padsButtonsHeld();
			if (btn & PAD_BUTTON_A) choice = 1;
			if (btn & PAD_BUTTON_B) choice = 2;
			VIDEO_WaitVSync();
		}
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		if (choice == 2) {
			free(filebuf);
			return false;
		}
		msgBox = DrawMessageBox(D_INFO, "Importing save...");
		DrawPublish(msgBox);
		// Re-open for writing
		ret = CARD_Open(slot, filename, &cardfile);
		is_overwrite = true;
		cm_log("  Overwriting existing file on card");
	}

	if (ret != CARD_ERROR_READY) {
		free(filebuf);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to open card file.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Write data sector by sector
	u8 *write_buf = (u8 *)memalign(32, sector_size);
	bool write_ok = true;
	u32 offset = 0;
	while (offset < expected_len) {
		u32 chunk = expected_len - offset;
		if (chunk > (u32)sector_size) chunk = sector_size;
		memset(write_buf, 0, sector_size);
		memcpy(write_buf, savedata + offset, chunk);
		ret = CARD_Write(&cardfile, write_buf, sector_size, offset);
		cm_log("  CARD_Write offset=%u len=%d ret=%d", offset, sector_size, (int)ret);
		if (ret != CARD_ERROR_READY) {
			write_ok = false;
			break;
		}
		offset += sector_size;
	}
	free(write_buf);

	if (!write_ok) {
		CARD_Close(&cardfile);
		free(filebuf);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to write data to card.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Verify write by reading back and comparing
	u8 *verify_buf = (u8 *)memalign(32, sector_size);
	bool verify_ok = true;
	if (verify_buf) {
		offset = 0;
		while (offset < expected_len) {
			ret = CARD_Read(&cardfile, verify_buf, sector_size, offset);
			if (ret != CARD_ERROR_READY) {
				cm_log("  VERIFY: CARD_Read failed at offset=%u ret=%d", offset, (int)ret);
				verify_ok = false;
				break;
			}
			u32 chunk = expected_len - offset;
			if (chunk > (u32)sector_size) chunk = sector_size;
			if (memcmp(verify_buf, savedata + offset, chunk) != 0) {
				cm_log("  VERIFY: DATA MISMATCH at offset=%u!", offset);
				// Log first differing byte
				for (u32 j = 0; j < chunk; j++) {
					if (verify_buf[j] != savedata[offset + j]) {
						cm_log("    byte %u: wrote=0x%02x read=0x%02x",
							offset + j, savedata[offset + j], verify_buf[j]);
						break;
					}
				}
				verify_ok = false;
				break;
			}
			offset += sector_size;
		}
		free(verify_buf);
		if (verify_ok) {
			cm_log("  VERIFY: all %u bytes match", expected_len);
		}
	}

	// Patch raw directory entry in sys_area to restore ALL GCI fields.
	// CARD_SetStatus has bugs: it rejects icon_addr >= 512, overwrites
	// timestamp with current time, and strips banner animation bits.
	// Instead, we find the raw 64-byte directory entry in the card's
	// system area and patch it directly, then flush via CARD_SetAttributes.
	if (!is_overwrite) {
		unsigned char *sysarea = get_card_sys_area(slot);
		int patches = 0;
		if (sysarea) {
			// Patch ALL matching directory entries in sys_area.
			// There are two directory blocks (at offsets 0x2000 and 0x4000).
			// We must patch BOTH because we don't know which one curr_dir
			// points to — if we only patch the backup, __card_updatedir
			// would read unpatched values from curr_dir and discard our fix.
			u32 entry_off = cardfile.filenum * 64;
			for (int blk = 0; blk < 5; blk++) {
				GCI *candidate = (GCI *)(sysarea + blk * 8192 + entry_off);
				if (memcmp(candidate->gamecode, gci->gamecode, 4) == 0
					&& memcmp(candidate->company, gci->company, 2) == 0
					&& memcmp(candidate->filename, gci->filename, CARD_FILENAMELEN) == 0) {
					if (patches == 0) {
						cm_log("  Before: banner_fmt=0x%02x time=0x%08x icon_addr=0x%08x icon_fmt=0x%04x icon_speed=0x%04x perm=0x%02x copy=%d comment=0x%08x",
							candidate->banner_fmt, candidate->time, candidate->icon_addr,
							candidate->icon_fmt, candidate->icon_speed, candidate->permission,
							candidate->copy_counter, candidate->comment_addr);
					}
					candidate->banner_fmt = gci->banner_fmt;
					candidate->time = gci->time;
					candidate->icon_addr = gci->icon_addr;
					candidate->icon_fmt = gci->icon_fmt;
					candidate->icon_speed = gci->icon_speed;
					candidate->comment_addr = gci->comment_addr;
					candidate->permission = gci->permission;
					candidate->copy_counter = gci->copy_counter;
					patches++;
				}
			}
		}
		if (patches > 0) {
			cm_log("  Patched %d direntry copies in sys_area", patches);
			cm_log("  After:  banner_fmt=0x%02x time=0x%08x icon_addr=0x%08x icon_fmt=0x%04x icon_speed=0x%04x perm=0x%02x copy=%d comment=0x%08x",
				gci->banner_fmt, gci->time, gci->icon_addr,
				gci->icon_fmt, gci->icon_speed, gci->permission,
				gci->copy_counter, gci->comment_addr);
			// Flush directory to card via CARD_SetAttributes (triggers __card_updatedir)
			s32 attr_ret = CARD_SetAttributes(slot, cardfile.filenum, gci->permission);
			cm_log("  CARD_SetAttributes (flush) ret=%d", (int)attr_ret);
		} else {
			cm_log("  WARNING: Could not find direntry in sys_area, falling back to CARD_SetStatus");
			card_stat cardstat;
			CARD_GetStatus(slot, cardfile.filenum, &cardstat);
			cardstat.banner_fmt = gci->banner_fmt;
			cardstat.icon_addr = gci->icon_addr;
			cardstat.icon_fmt = gci->icon_fmt;
			cardstat.icon_speed = gci->icon_speed;
			cardstat.comment_addr = gci->comment_addr;
			u64 real_time = gettime();
			settime(secs_to_ticks(gci->time));
			s32 status_ret = CARD_SetStatus(slot, cardfile.filenum, &cardstat);
			settime(real_time);
			cm_log("  CARD_SetStatus ret=%d", (int)status_ret);
			CARD_SetAttributes(slot, cardfile.filenum, gci->permission);
		}
	} else {
		cm_log("  Skipping directory update (overwrite mode)");
	}

	CARD_Close(&cardfile);
	free(filebuf);

	DrawDispose(msgBox);
	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Imported \"%s\" successfully.", filename);
	msgBox = DrawMessageBox(D_INFO, done_msg);
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
	DrawDispose(msgBox);
	return true;
}

// --- Card status polling ---

static bool card_manager_check_status(int slot, bool was_present) {
	s32 ret = CARD_ProbeEx(slot, NULL, NULL);
	bool is_present = (ret == CARD_ERROR_READY || ret == CARD_ERROR_BUSY);
	return is_present != was_present;
}

// --- Panel helpers ---

static void cm_panel_load(cm_panel *panel) {
	panel->num_entries = 0;
	panel->cursor = 0;
	panel->scroll_row = 0;
	panel->mem_size = 0;
	panel->sector_size = 0;
	panel->card_present = false;
	panel->has_animated_icons = false;

	cm_log("Loading Slot %c...", panel->slot == CARD_SLOTA ? 'A' : 'B');

	s32 ret = initialize_card(panel->slot);
	if (ret == CARD_ERROR_READY) {
		CARD_ProbeEx(panel->slot, &panel->mem_size, &panel->sector_size);
		panel->num_entries = card_manager_read_saves(panel->slot, panel->entries, 128);
		panel->card_present = true;
		cm_log_card_info(panel->slot, panel->mem_size, panel->sector_size, panel->num_entries);
		for (int i = 0; i < panel->num_entries; i++) {
			cm_log_entry(i, &panel->entries[i]);
		}
		for (int i = 0; i < panel->num_entries; i++) {
			if (panel->entries[i].icon && panel->entries[i].icon->num_frames > 1) {
				panel->has_animated_icons = true;
				break;
			}
		}
	}
	else {
		cm_log("No card detected in Slot %c (error=%d)",
			panel->slot == CARD_SLOTA ? 'A' : 'B', (int)ret);
	}
	panel->needs_reload = false;
}

static void cm_panel_navigate(cm_panel *panel, u16 btns) {
	if (panel->num_entries == 0) return;
	int cx = panel->cursor % GRID_COLS;
	int cy = panel->cursor / GRID_COLS;
	int total_rows = (panel->num_entries + GRID_COLS - 1) / GRID_COLS;

	if (btns & PAD_BUTTON_UP) {
		if (cy > 0) {
			panel->cursor -= GRID_COLS;
		}
	}
	if (btns & PAD_BUTTON_DOWN) {
		if (cy < total_rows - 1) {
			int new_cursor = panel->cursor + GRID_COLS;
			panel->cursor = (new_cursor < panel->num_entries) ? new_cursor : panel->num_entries - 1;
		}
	}
	if (btns & PAD_BUTTON_LEFT) {
		if (panel->cursor > 0) panel->cursor--;
	}
	if (btns & PAD_BUTTON_RIGHT) {
		if (panel->cursor < panel->num_entries - 1) panel->cursor++;
	}

	// Update scroll to keep cursor visible
	cy = panel->cursor / GRID_COLS;
	if (cy < panel->scroll_row)
		panel->scroll_row = cy;
	if (cy >= panel->scroll_row + GRID_ROWS_VISIBLE)
		panel->scroll_row = cy - GRID_ROWS_VISIBLE + 1;
}

// --- Main loop ---

void show_card_manager(void) {
	cm_panel *panels[2];
	panels[0] = calloc(1, sizeof(cm_panel));
	panels[1] = calloc(1, sizeof(cm_panel));
	if (!panels[0] || !panels[1]) {
		free(panels[0]);
		free(panels[1]);
		return;
	}
	panels[0]->slot = CARD_SLOTA;
	panels[0]->needs_reload = true;
	panels[1]->slot = CARD_SLOTB;
	panels[1]->needs_reload = true;

	int active = 0;
	bool needs_redraw = false;
	int poll_counter = 0;
	u32 anim_tick = 0;
	uiDrawObj_t *pagePanel = NULL;

	cm_flat_init();
	cm_log_open();
	cm_log("Opened card manager (dual-panel)");

	// Wait for A button release
	while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }

	while (1) {
		// Reload any panel that needs it
		for (int p = 0; p < 2; p++) {
			if (panels[p]->needs_reload) {
				if (pagePanel) {
					DrawDispose(pagePanel);
					pagePanel = NULL;
				}
				card_manager_free_graphics(panels[p]->entries, panels[p]->num_entries);
				cm_panel_load(panels[p]);
				needs_redraw = true;
			}
		}

		bool has_anim = panels[0]->has_animated_icons || panels[1]->has_animated_icons;
		if (needs_redraw || has_anim) {
			uiDrawObj_t *newPanel = card_manager_draw(panels[0], panels[1], active, anim_tick);
			if (pagePanel) DrawDispose(pagePanel);
			pagePanel = newPanel;
			DrawPublish(pagePanel);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();
		anim_tick++;

		// Poll both slots for hotswap
		poll_counter++;
		if (poll_counter >= CARD_POLL_INTERVAL) {
			poll_counter = 0;
			for (int p = 0; p < 2; p++) {
				if (card_manager_check_status(panels[p]->slot, panels[p]->card_present)) {
					cm_log("Card %s in Slot %c",
						panels[p]->card_present ? "removed" : "inserted",
						panels[p]->slot == CARD_SLOTA ? 'A' : 'B');
					panels[p]->needs_reload = true;
				}
			}
			if (panels[0]->needs_reload || panels[1]->needs_reload)
				continue;
		}

		u16 btns = padsButtonsHeld();
		if (!btns) continue;

		cm_panel *ap = panels[active];

		// Grid navigation
		if (btns & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT)) {
			cm_panel_navigate(ap, btns);
			needs_redraw = true;
		}

		// Switch active panel
		if ((btns & PAD_TRIGGER_L) && active != 0) {
			active = 0;
			needs_redraw = true;
		}
		if ((btns & PAD_TRIGGER_R) && active != 1) {
			active = 1;
			needs_redraw = true;
		}

		// Export
		if ((btns & PAD_BUTTON_A) && ap->card_present && ap->num_entries > 0) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			card_entry *sel = &ap->entries[ap->cursor];
			cm_log("Exporting [%d] \"%s\" from Slot %c",
				ap->cursor, sel->filename,
				ap->slot == CARD_SLOTA ? 'A' : 'B');
			if (card_manager_export_save(ap->slot, sel, ap->sector_size)) {
				cm_log("Export successful");
			} else {
				cm_log("Export failed or cancelled");
			}
			needs_redraw = true;
			goto debounce;
		}

		// Import
		if ((btns & PAD_BUTTON_X) && ap->card_present) {
			while (padsButtonsHeld() & PAD_BUTTON_X) { VIDEO_WaitVSync(); }
			gci_file_entry *gci_files = calloc(MAX_GCI_FILES, sizeof(gci_file_entry));
			if (gci_files) {
				int num_gci = card_manager_scan_gci_files(gci_files, MAX_GCI_FILES);
				cm_log("Found %d GCI files for import", num_gci);
				int pick = card_manager_pick_gci(gci_files, num_gci);
				if (pick >= 0) {
					cm_log("Importing \"%s\" to Slot %c",
						gci_files[pick].display,
						ap->slot == CARD_SLOTA ? 'A' : 'B');
					if (card_manager_import_gci(ap->slot, &gci_files[pick], ap->sector_size)) {
						cm_log("Import successful");
						ap->needs_reload = true;
					} else {
						cm_log("Import failed or cancelled");
						needs_redraw = true;
					}
				} else {
					needs_redraw = true;
				}
				free(gci_files);
			}
			goto debounce;
		}

		// Delete
		if ((btns & PAD_TRIGGER_Z) && ap->card_present && ap->num_entries > 0) {
			while (padsButtonsHeld() & PAD_TRIGGER_Z) { VIDEO_WaitVSync(); }
			card_entry *sel = &ap->entries[ap->cursor];
			if (card_manager_confirm_delete(sel->filename)) {
				cm_log("Deleting [%d] \"%s\" from Slot %c",
					ap->cursor, sel->filename,
					ap->slot == CARD_SLOTA ? 'A' : 'B');
				if (card_manager_delete_save(ap->slot, sel)) {
					cm_log("Delete successful");
					ap->needs_reload = true;
				} else {
					cm_log("Delete failed");
				}
			} else {
				cm_log("Delete cancelled for \"%s\"", sel->filename);
				needs_redraw = true;
			}
			goto debounce;
		}

		if (btns & PAD_BUTTON_B)
			break;

debounce:
		while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN |
			PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT |
			PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_X |
			PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_TRIGGER_Z)) {
			VIDEO_WaitVSync();
		}
	}

	// Cleanup: dispose UI before freeing texture data
	if (pagePanel) DrawDispose(pagePanel);
	for (int p = 0; p < 2; p++) {
		card_manager_free_graphics(panels[p]->entries, panels[p]->num_entries);
		free(panels[p]);
	}
	cm_log("Closed card manager");
	cm_log_close();
	while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
}
