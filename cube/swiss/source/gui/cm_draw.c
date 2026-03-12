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
static u16 cm_flat_pixels[16] ATTRIBUTE_ALIGN(32);  // 4x4 RGB5A3
static u16 cm_highlight_pixels[16] ATTRIBUTE_ALIGN(32);  // 4x4 RGB5A3 (translucent blue)
static GXTexObj cm_flat_tex;
static GXTexObj cm_highlight_tex;
static GXTexObj cm_memcard_tex;
static bool cm_inited = false;

void cm_draw_init(void) {
	if (cm_inited) return;
	for (int i = 0; i < 16; i++)
		cm_flat_pixels[i] = 0x5FFF;  // opaque white (RGB555)
	DCFlushRange(cm_flat_pixels, sizeof(cm_flat_pixels));
	GX_InitTexObj(&cm_flat_tex, cm_flat_pixels, 4, 4, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&cm_flat_tex, GX_LINEAR, GX_NEAR);

	// Translucent blue highlight: A3R4G4B4 — alpha=2/7, R=6, G=6, B=10
	for (int i = 0; i < 16; i++)
		cm_highlight_pixels[i] = 0x266A;
	DCFlushRange(cm_highlight_pixels, sizeof(cm_highlight_pixels));
	GX_InitTexObj(&cm_highlight_tex, cm_highlight_pixels, 4, 4, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&cm_highlight_tex, GX_LINEAR, GX_NEAR);
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

uiDrawObj_t *cm_draw_rect(int x, int y, int w, int h) {
	return DrawTexObj(&cm_flat_tex, x, y, w, h, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

uiDrawObj_t *cm_draw_highlight(int x, int y, int w, int h) {
	return DrawTexObj(&cm_highlight_tex, x, y, w, h, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
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

	if (pos < fwd_ticks) {
		u32 accum = 0;
		frame = n - 1;
		for (int i = 0; i < n; i++) {
			accum += icon_frame_dur(icon, i);
			if (pos < accum) { frame = i; break; }
		}
	} else {
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

	// Header — use panel label + free blocks info
	char header[64];
	if (panel->card_present) {
		int total = (panel->mem_size << 20 >> 3) / panel->sector_size - 5;
		int used = 0;
		for (int i = 0; i < panel->num_entries; i++) used += panel->entries[i].blocks;
		snprintf(header, sizeof(header), "%s  %d/%d free", panel->label, total - used, total);
	} else {
		snprintf(header, sizeof(header), "%s", panel->label);
	}
	GXColor hdr_color = active ? defaultColor : (GXColor){160, 160, 160, 255};
	float hdr_scale = GetTextScaleToFitInWidthWithMax(header, pw - 8, 0.6f);
	DrawAddChild(container, DrawStyledLabel(px + pw / 2, PANEL_TOP_Y + 9, header,
		hdr_scale, ALIGN_CENTER, hdr_color));

	// Panel border
	DrawAddChild(container, cm_draw_rect(px, PANEL_TOP_Y, pw, 1));
	DrawAddChild(container, cm_draw_rect(px, PANEL_BOTTOM_Y, pw, 1));
	DrawAddChild(container, cm_draw_rect(px, PANEL_TOP_Y, 1, PANEL_BOTTOM_Y - PANEL_TOP_Y));
	DrawAddChild(container, cm_draw_rect(px + pw - 1, PANEL_TOP_Y, 1, PANEL_BOTTOM_Y - PANEL_TOP_Y));

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

			if (active && idx == panel->cursor) {
				int sx = ix - 2, sy = iy - 2;
				int sw = GRID_ICON_SIZE + 4, sh = GRID_ICON_SIZE + 4;
				DrawAddChild(container, cm_draw_rect(sx, sy, sw, 1));
				DrawAddChild(container, cm_draw_rect(sx, sy + sh - 1, sw, 1));
				DrawAddChild(container, cm_draw_rect(sx, sy, 1, sh));
				DrawAddChild(container, cm_draw_rect(sx + sw - 1, sy, 1, sh));
			}

			card_entry *e = &panel->entries[idx];
			if (e->icon && e->icon->num_frames > 0) {
				int frame = icon_anim_get_frame(e->icon, anim_tick);
				if (e->icon->frames[frame].data) {
					DrawAddChild(container, DrawTexObj(&e->icon->frames[frame].tex,
						ix, iy, GRID_ICON_SIZE, GRID_ICON_SIZE,
						0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
				}
			} else if (e->banner) {
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

	int text_w = detail_w;
	if (sel->banner) {
		int bx = 640 - PANEL_OUTER_X - DETAIL_BANNER_W - 10;
		DrawAddChild(container, DrawTexObj(&sel->banner_tex,
			bx, dy, DETAIL_BANNER_W, DETAIL_BANNER_H,
			0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		text_w = detail_w - DETAIL_BANNER_W - 16;
	}

	char *name = sel->game_name[0] ? sel->game_name : sel->filename;
	float name_scale = GetTextScaleToFitInWidthWithMax(name, text_w, 0.65f);
	DrawAddChild(container, DrawStyledLabel(dx, dy + 6, name,
		name_scale, ALIGN_LEFT, defaultColor));

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

	int total_w = PANEL_WIDTH * 2 + PANEL_GAP;
	int left_x = (640 - total_w) / 2;
	int right_x = left_x + PANEL_WIDTH + PANEL_GAP;

	card_manager_draw_panel(container, left, left_x, PANEL_WIDTH,
		active_panel == 0, anim_tick);
	card_manager_draw_panel(container, right, right_x, PANEL_WIDTH,
		active_panel == 1, anim_tick);

	if (has_sel) {
		card_manager_draw_detail(container, &active_p->entries[active_p->cursor]);
	}

	// Hint bar
	const char *hints = has_sel
		? "A: Actions  X: Backups  L/R: Switch  B: Back"
		: "X: Backups  L/R: Switch  B: Back";
	DrawAddChild(container, DrawStyledLabel(640 / 2, HINTS_Y,
		hints, 0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));

	return container;
}

// --- Context menu ---
// Draws a small popup menu centered on screen.
// Returns selected index or -1 if cancelled.

int cm_context_menu(const char *items[], const bool enabled[], int count) {
	int cursor = 0;
	// Start cursor on first enabled item
	while (cursor < count && !enabled[cursor]) cursor++;
	if (cursor >= count) return -1;

	// Menu dimensions
	int menu_w = 240;
	int item_h = 26;
	int pad = 12;
	int menu_h = count * item_h + pad * 2;
	int menu_x = (640 - menu_w) / 2;
	int menu_y = (480 - menu_h) / 2;

	while (1) {
		uiDrawObj_t *overlay = DrawEmptyBox(menu_x, menu_y,
			menu_x + menu_w, menu_y + menu_h);

		for (int i = 0; i < count; i++) {
			int iy = menu_y + pad + i * item_h + item_h / 2;
			GXColor color;
			if (!enabled[i]) {
				color = (GXColor){80, 80, 80, 255};
			} else if (i == cursor) {
				color = (GXColor){96, 107, 164, 255};
				DrawAddChild(overlay, cm_draw_highlight(
					menu_x + 6, menu_y + pad + i * item_h,
					menu_w - 12, item_h));
			} else {
				color = defaultColor;
			}
			DrawAddChild(overlay, DrawStyledLabel(menu_x + 20, iy,
				items[i], 0.55f, ALIGN_LEFT, color));
		}

		DrawPublish(overlay);

		while (1) {
			VIDEO_WaitVSync();
			u16 btns = padsButtonsHeld();
			s8 stickY = padsStickY();
			bool has_input = btns || stickY > 16 || stickY < -16;
			if (!has_input) continue;

			bool redraw = false;
			if ((btns & PAD_BUTTON_UP) || stickY > 16) {
				int next = cursor - 1;
				while (next >= 0 && !enabled[next]) next--;
				if (next >= 0) { cursor = next; redraw = true; }
			}
			if ((btns & PAD_BUTTON_DOWN) || stickY < -16) {
				int next = cursor + 1;
				while (next < count && !enabled[next]) next++;
				if (next < count) { cursor = next; redraw = true; }
			}
			if (btns & PAD_BUTTON_A) {
				while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
				DrawDispose(overlay);
				return cursor;
			}
			if (btns & PAD_BUTTON_B) {
				while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
				DrawDispose(overlay);
				return -1;
			}
			if (redraw) {
				while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN)) {
					VIDEO_WaitVSync();
				}
				break;
			}
		}
		DrawDispose(overlay);
	}
}
