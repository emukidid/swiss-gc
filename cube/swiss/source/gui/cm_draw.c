/* cm_draw.c
	- Card Manager drawing and icon animation
	by Swiss contributors
 */

#include <stdio.h>
#include <string.h>
#include <gccore.h>
#include "cm_internal.h"
#include "IPLFontWrite.h"
#include "images.h"

extern TPLFile imagesTPL;

// --- Flat rectangle drawing ---
// DrawEmptyColouredBox adds 3-10px borders, making it unusable for thin lines.
// Instead, use a tiny solid-color texture with DrawTexObj for pixel-exact rectangles.
static u16 cm_flat_pixels[16] ATTRIBUTE_ALIGN(32);  // 4x4 RGB5A3
static GXTexObj cm_flat_tex;
static GXTexObj cm_memcard_tex;
static bool cm_inited = false;

void cm_draw_init(void) {
	if (cm_inited) return;
	// Fill with white, semi-transparent (RGB5A3: bit15=0 → ARGB3444)
	// Alpha=5 (~71%), R=F, G=F, B=F → 0x5FFF
	for (int i = 0; i < 16; i++)
		cm_flat_pixels[i] = 0x5FFF;
	DCFlushRange(cm_flat_pixels, sizeof(cm_flat_pixels));
	GX_InitTexObj(&cm_flat_tex, cm_flat_pixels, 4, 4, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&cm_flat_tex, GX_LINEAR, GX_NEAR);
	// Load memcard manager icon from TPL, then re-init GXTexObj from scratch.
	// TPL_GetTexture sets internal fields (UserData, LOD, etc.) that corrupt
	// GX state when used with DrawTexObj's GX_NEAR path. Extracting the raw
	// pixel data and re-initializing avoids all TPL-specific GXTexObj baggage.
	{
		GXTexObj tplTex;
		TPL_GetTexture(&imagesTPL, memcard_mgr_icon, &tplTex);
		void *texData = GX_GetTexObjData(&tplTex);
		GX_InitTexObj(&cm_memcard_tex, texData, 64, 64, GX_TF_RGBA8,
			GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObjFilterMode(&cm_memcard_tex, GX_NEAR, GX_NEAR);
	}
	cm_inited = true;
}

// Draw a flat semi-transparent white rectangle at exact pixel coordinates
uiDrawObj_t *cm_draw_rect(int x, int y, int w, int h) {
	return DrawTexObj(&cm_flat_tex, x, y, w, h, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

// --- Icon animation ---

static u32 icon_frame_dur(icon_anim *icon, int i) {
	switch (icon->frames[i].speed) {
		case CARD_SPEED_FAST:   return ICON_SPEED_FAST_TICKS;
		case CARD_SPEED_MIDDLE: return ICON_SPEED_MIDDLE_TICKS;
		case CARD_SPEED_SLOW:   return ICON_SPEED_SLOW_TICKS;
		default:                return ICON_SPEED_MIDDLE_TICKS;
	}
}

// Resolve a NONE frame (fmt=0) to the next non-NONE frame in index order.
// NONE frames contribute to timing but display the texture of the nearest
// subsequent real frame, wrapping around if needed.
static int icon_resolve_frame(icon_anim *icon, int frame) {
	if (icon->frames[frame].fmt != 0) return frame;
	for (int i = frame + 1; i < icon->num_frames; i++) {
		if (icon->frames[i].fmt != 0) return i;
	}
	for (int i = 0; i < frame; i++) {
		if (icon->frames[i].fmt != 0) return i;
	}
	return frame;
}

static int icon_anim_get_frame(icon_anim *icon, u32 tick) {
	if (!icon || icon->num_frames <= 1) return 0;
	int n = icon->num_frames;

	// All frames (including NONE) contribute to timing
	u32 fwd_ticks = 0;
	for (int i = 0; i < n; i++)
		fwd_ticks += icon_frame_dur(icon, i);
	if (fwd_ticks == 0) return 0;

	u32 cycle_ticks;
	if (icon->anim_type & CARD_ANIM_BOUNCE) {
		u32 rev_ticks = 0;
		for (int i = n - 2; i >= 1; i--)
			rev_ticks += icon_frame_dur(icon, i);
		cycle_ticks = fwd_ticks + rev_ticks;
	} else {
		cycle_ticks = fwd_ticks;
	}
	if (cycle_ticks == 0) return 0;

	u32 pos = tick % cycle_ticks;
	int frame;

	// Forward phase
	if (pos < fwd_ticks) {
		u32 accum = 0;
		frame = n - 1;
		for (int i = 0; i < n; i++) {
			accum += icon_frame_dur(icon, i);
			if (pos < accum) { frame = i; break; }
		}
	} else {
		// Reverse phase (bounce)
		pos -= fwd_ticks;
		u32 accum = 0;
		frame = 1;
		for (int i = n - 2; i >= 1; i--) {
			accum += icon_frame_dur(icon, i);
			if (pos < accum) { frame = i; break; }
		}
	}

	return icon_resolve_frame(icon, frame);
}

// --- Panel drawing ---

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
	DrawAddChild(container, DrawStyledLabel(px + pw / 2, PANEL_TOP_Y + 9, header,
		0.6f, ALIGN_CENTER, hdr_color));

	// Panel border (1px outline, always visible)
	DrawAddChild(container, cm_draw_rect(px, PANEL_TOP_Y, pw, 1));                          // top
	DrawAddChild(container, cm_draw_rect(px, PANEL_BOTTOM_Y, pw, 1));                       // bottom
	DrawAddChild(container, cm_draw_rect(px, PANEL_TOP_Y, 1, PANEL_BOTTOM_Y - PANEL_TOP_Y)); // left
	DrawAddChild(container, cm_draw_rect(px + pw - 1, PANEL_TOP_Y, 1, PANEL_BOTTOM_Y - PANEL_TOP_Y)); // right

	if (panel->loading) {
		DrawAddChild(container, DrawStyledLabel(px + pw / 2, GRID_TOP_Y + 60,
			"Reading...", 0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}
	if (!panel->card_present) {
		int icon_sz = 64;
		int icon_x = px + (pw - icon_sz) / 2;
		int grid_h = PANEL_BOTTOM_Y - GRID_TOP_Y;
		int icon_y = GRID_TOP_Y + (grid_h - icon_sz) / 2 - 8;
		DrawAddChild(container, DrawTexObj(&cm_memcard_tex,
			icon_x, icon_y, icon_sz, icon_sz, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		DrawAddChild(container, DrawStyledLabel(px + pw / 2, icon_y + icon_sz + 6,
			"No Card", 0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}
	if (panel->num_entries == 0) {
		DrawAddChild(container, DrawStyledLabel(px + pw / 2, GRID_TOP_Y + 60,
			"Empty", 0.65f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}

	// Icon grid
	int grid_x = px + (pw - GRID_COLS * GRID_CELL) / 2;
	for (int row = 0; row < GRID_ROWS_VISIBLE_MAX; row++) {
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
	if (total_rows > GRID_ROWS_VISIBLE_MAX) {
		float pct = (float)panel->scroll_row / (float)(total_rows - GRID_ROWS_VISIBLE_MAX);
		DrawAddChild(container, DrawVertScrollBar(
			px + pw - 10, GRID_TOP_Y, 6,
			GRID_ROWS_VISIBLE_MAX * GRID_CELL, pct, 16));
	}
}

static void card_manager_draw_detail(uiDrawObj_t *container, card_entry *sel) {
	int dx = PANEL_OUTER_X + 10;
	int dy = DETAIL_TOP_Y;
	int detail_w = 640 - 2 * PANEL_OUTER_X - 20;

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

	// Stats line with file attributes (§5)
	char stats[96];
	char attribs[32] = "";
	if (sel->permissions & CARD_ATTRIB_NOCOPY) strcat(attribs, " NoCopy");
	if (sel->permissions & CARD_ATTRIB_NOMOVE) strcat(attribs, " NoMove");
	snprintf(stats, sizeof(stats), "%s  %d blocks  %dK%s",
		sel->gamecode, sel->blocks, sel->filesize / 1024, attribs);
	DrawAddChild(container, DrawStyledLabel(dx, dy + 22, stats,
		0.5f, ALIGN_LEFT, (GXColor){160, 160, 160, 255}));
}

uiDrawObj_t *card_manager_draw(cm_panel *left, cm_panel *right,
	int active_panel, u32 anim_tick) {

	cm_draw_init();

	cm_panel *active_p = active_panel == 0 ? left : right;
	bool has_sel = active_p->card_present && active_p->num_entries > 0 &&
		active_p->cursor >= 0 && active_p->cursor < active_p->num_entries;

	uiDrawObj_t *container = DrawEmptyBox(PANEL_OUTER_X, BOX_TOP_Y,
		640 - PANEL_OUTER_X, BOX_BOTTOM_Y);

	// Title with memcard icon
	DrawAddChild(container, DrawTexObj(&cm_memcard_tex,
		PANEL_OUTER_X + 5, TITLE_Y, 24, 24, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	DrawAddChild(container, DrawStyledLabel(PANEL_OUTER_X + 34, TITLE_Y + 12,
		"Memory Card Manager", 0.75f, ALIGN_LEFT, defaultColor));

	// Panel positions: centered within content area
	int total_w = PANEL_WIDTH * 2 + PANEL_GAP;
	int left_x = (640 - total_w) / 2;
	int right_x = left_x + PANEL_WIDTH + PANEL_GAP;

	card_manager_draw_panel(container, left, left_x, PANEL_WIDTH,
		active_panel == 0, anim_tick);
	card_manager_draw_panel(container, right, right_x, PANEL_WIDTH,
		active_panel == 1, anim_tick);

	// Shared detail area below both panels
	if (has_sel) {
		card_manager_draw_detail(container, &active_p->entries[active_p->cursor]);
	}

	// Button hints below detail area
	DrawAddChild(container, DrawStyledLabel(640 / 2, HINTS_Y,
		"A: Export  X: Import  Z: Delete  L/R: Switch  B: Back",
		0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));

	return container;
}
