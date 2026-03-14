/* cm_draw.c
	- Card Manager drawing and icon animation
	by Swiss contributors
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gccore.h>
#include "cm_internal.h"
#include "IPLFontWrite.h"
#include "images.h"

extern TPLFile imagesTPL;

// --- Flat rectangle drawing ---
static u16 cm_flat_pixels[16] ATTRIBUTE_ALIGN(32);  // 4x4 RGB5A3
static u16 cm_dim_pixels[16] ATTRIBUTE_ALIGN(32);   // 4x4 RGB5A3 (dark gray)
static u16 cm_highlight_pixels[16] ATTRIBUTE_ALIGN(32);  // 4x4 RGB5A3 (translucent blue)
static GXTexObj cm_flat_tex;
static GXTexObj cm_dim_tex;
static GXTexObj cm_highlight_tex;
static GXTexObj cm_memcard_tex;
static GXTexObj cm_panel_frame_phys_tex;
static GXTexObj cm_panel_frame_vmc_tex;
static bool cm_inited = false;

void cm_draw_init(void) {
	if (cm_inited) return;
	for (int i = 0; i < 16; i++)
		cm_flat_pixels[i] = 0x5FFF;  // opaque white (RGB555)
	DCFlushRange(cm_flat_pixels, sizeof(cm_flat_pixels));
	GX_InitTexObj(&cm_flat_tex, cm_flat_pixels, 4, 4, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&cm_flat_tex, GX_LINEAR, GX_NEAR);

	// Dim gray for inactive icons: RGB555 ~30% (~9/31 per channel)
	for (int i = 0; i < 16; i++)
		cm_dim_pixels[i] = 0xA529;  // RGB555 opaque, R=9 G=9 B=9 (~30%)
	DCFlushRange(cm_dim_pixels, sizeof(cm_dim_pixels));
	GX_InitTexObj(&cm_dim_tex, cm_dim_pixels, 4, 4, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&cm_dim_tex, GX_LINEAR, GX_NEAR);

	// Translucent blue highlight: A3R4G4B4 — alpha=2/7, R=6, G=6, B=10
	for (int i = 0; i < 16; i++)
		cm_highlight_pixels[i] = 0x266A;
	DCFlushRange(cm_highlight_pixels, sizeof(cm_highlight_pixels));
	GX_InitTexObj(&cm_highlight_tex, cm_highlight_pixels, 4, 4, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&cm_highlight_tex, GX_LINEAR, GX_NEAR);

	{
		GXTexObj tplTex;
		void *texData;

		TPL_GetTexture(&imagesTPL, memcard_mgr_icon, &tplTex);
		texData = GX_GetTexObjData(&tplTex);
		GX_InitTexObj(&cm_memcard_tex, texData, 64, 64, GX_TF_RGBA8,
			GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObjFilterMode(&cm_memcard_tex, GX_NEAR, GX_NEAR);

		TPL_GetTexture(&imagesTPL, panel_frame_phys, &tplTex);
		texData = GX_GetTexObjData(&tplTex);
		GX_InitTexObj(&cm_panel_frame_phys_tex, texData, 24, 24, GX_TF_RGBA8,
			GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObjFilterMode(&cm_panel_frame_phys_tex, GX_NEAR, GX_NEAR);

		TPL_GetTexture(&imagesTPL, panel_frame_vmc, &tplTex);
		texData = GX_GetTexObjData(&tplTex);
		GX_InitTexObj(&cm_panel_frame_vmc_tex, texData, 24, 24, GX_TF_RGBA8,
			GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_InitTexObjFilterMode(&cm_panel_frame_vmc_tex, GX_NEAR, GX_NEAR);
	}
	cm_inited = true;
}

uiDrawObj_t *cm_draw_rect(int x, int y, int w, int h) {
	return DrawTexObj(&cm_flat_tex, x, y, w, h, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

uiDrawObj_t *cm_draw_highlight(int x, int y, int w, int h) {
	return DrawTexObj(&cm_highlight_tex, x, y, w, h, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

// --- Message box (consistent with context menu styling) ---

#define CM_MSG_MAX_W    400
#define CM_MSG_PAD_X    20
#define CM_MSG_PAD_Y    16
#define CM_MSG_LINE_H   20
#define CM_MSG_FONT     0.55f

uiDrawObj_t *cm_draw_message(const char *msg) {
	char buf[512];
	strlcpy(buf, msg, sizeof(buf));

	const char *lines[16];
	int num_lines = 0;
	char *tok = strtok(buf, "\n");
	while (tok && num_lines < 16) {
		lines[num_lines++] = tok;
		tok = strtok(NULL, "\n");
	}

	int content_w = CM_MSG_MAX_W - 2 * CM_MSG_PAD_X;
	int box_h = num_lines * CM_MSG_LINE_H + 2 * CM_MSG_PAD_Y;
	int box_x = (640 - CM_MSG_MAX_W) / 2;
	int box_y = (480 - box_h) / 2;

	uiDrawObj_t *box = DrawEmptyBox(box_x, box_y,
		box_x + CM_MSG_MAX_W, box_y + box_h);

	for (int i = 0; i < num_lines; i++) {
		if (lines[i][0] == ' ' && lines[i][1] == '\0') continue;
		float scale = GetTextScaleToFitInWidthWithMax(lines[i], content_w, CM_MSG_FONT);
		int ly = box_y + CM_MSG_PAD_Y + i * CM_MSG_LINE_H + CM_MSG_LINE_H / 2;
		DrawAddChild(box, DrawStyledLabel(640 / 2, ly,
			lines[i], scale, ALIGN_CENTER, defaultColor));
	}

	return box;
}

// --- 9-slice panel frame ---

static void cm_draw_9slice(uiDrawObj_t *container, GXTexObj *tex,
	int x, int y, int w, int h, int tex_size, int corner) {
	float c = (float)corner / tex_size;
	float e = 1.0f - c;
	int mw = w - 2 * corner;
	int mh = h - 2 * corner;

	// Corners
	DrawAddChild(container, DrawTexObj(tex, x, y, corner, corner, 0, 0, c, 0, c, 0));
	DrawAddChild(container, DrawTexObj(tex, x+w-corner, y, corner, corner, 0, e, 1, 0, c, 0));
	DrawAddChild(container, DrawTexObj(tex, x, y+h-corner, corner, corner, 0, 0, c, e, 1, 0));
	DrawAddChild(container, DrawTexObj(tex, x+w-corner, y+h-corner, corner, corner, 0, e, 1, e, 1, 0));
	// Edges
	DrawAddChild(container, DrawTexObj(tex, x+corner, y, mw, corner, 0, c, e, 0, c, 0));
	DrawAddChild(container, DrawTexObj(tex, x+corner, y+h-corner, mw, corner, 0, c, e, e, 1, 0));
	DrawAddChild(container, DrawTexObj(tex, x, y+corner, corner, mh, 0, 0, c, c, e, 0));
	DrawAddChild(container, DrawTexObj(tex, x+w-corner, y+corner, corner, mh, 0, e, 1, c, e, 0));
	// Center
	DrawAddChild(container, DrawTexObj(tex, x+corner, y+corner, mw, mh, 0, c, e, c, e, 0));
}

void cm_draw_panel_frame(uiDrawObj_t *container, bool is_vmc, int x, int y, int w, int h) {
	cm_draw_init();
	GXTexObj *tex = is_vmc ? &cm_panel_frame_vmc_tex : &cm_panel_frame_phys_tex;
	cm_draw_9slice(container, tex, x, y, w, h, 24, 8);
}

static uiDrawObj_t *cm_draw_icon_rect(GXTexObj *tex, int x, int y, int w, int h) {
	return DrawTexObj(tex, x, y, w, h, 0, 0.0f, 1.0f, 0.0f, 1.0f, 0);
}

void cm_draw_title_bar(uiDrawObj_t *container, int view_mode) {
	cm_draw_init();
	DrawAddChild(container, DrawStyledLabel(BOX_X1 + 5, TITLE_Y + 12,
		"Save Manager", 0.75f, ALIGN_LEFT, defaultColor));

	// View mode icons (upper right) — Cards = two boxes, Library = three lines
	int ix = BOX_X2 - 40;
	int iy = TITLE_Y + 4;

	// Cards icon: two small rectangles side by side
	GXTexObj *ct = view_mode == 0 ? &cm_flat_tex : &cm_dim_tex;
	DrawAddChild(container, cm_draw_icon_rect(ct, ix, iy, 7, 10));
	DrawAddChild(container, cm_draw_icon_rect(ct, ix + 9, iy, 7, 10));

	// Library icon: three horizontal lines
	GXTexObj *lt = view_mode == 1 ? &cm_flat_tex : &cm_dim_tex;
	ix += 22;
	DrawAddChild(container, cm_draw_icon_rect(lt, ix, iy, 14, 2));
	DrawAddChild(container, cm_draw_icon_rect(lt, ix, iy + 4, 14, 2));
	DrawAddChild(container, cm_draw_icon_rect(lt, ix, iy + 8, 14, 2));
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

	bool is_vmc = (panel->source == CM_SRC_VMC);
	GXColor hdr_color;
	if (!active)
		hdr_color = (GXColor){160, 160, 160, 255};
	else if (is_vmc)
		hdr_color = (GXColor){140, 200, 255, 255};
	else
		hdr_color = defaultColor;

	// Align header text with icon grid edges
	int grid_w = GRID_COLS * GRID_CELL;
	int grid_lx = px + (pw - grid_w) / 2;
	int grid_rx = grid_lx + grid_w;

	// Header: vertically centered between PANEL_TOP_Y and GRID_TOP_Y
	// GRID_TOP_Y = PANEL_TOP_Y + 36, so 36px of header space
	int hdr_mid = PANEL_TOP_Y + (GRID_TOP_Y - PANEL_TOP_Y) / 2;

	// Slot label left-aligned with first icon
	DrawAddChild(container, DrawStyledLabel(grid_lx + 4, hdr_mid, panel->label,
		0.55f, ALIGN_LEFT, hdr_color));

	// Free blocks right-aligned with last icon
	if (panel->card_present) {
		int total = (panel->mem_size << 20 >> 3) / panel->sector_size - 5;
		int used = 0;
		for (int i = 0; i < panel->num_entries; i++) used += panel->entries[i].blocks;
		char blocks[32];
		snprintf(blocks, sizeof(blocks), "%d/%d free", total - used, total);
		DrawAddChild(container, DrawStyledLabel(grid_rx - 4, hdr_mid, blocks,
			0.5f, ALIGN_RIGHT, hdr_color));
	}

	// Separator line — centered between text bottom (~hdr_mid+7) and grid top
	int text_bot = hdr_mid + 7;
	int sep_y = text_bot + (GRID_TOP_Y - text_bot) / 2;
	DrawAddChild(container, DrawFlatColorRect(grid_lx, sep_y, grid_w, 1,
		(GXColor){120, 120, 120, 80}));

	// Edge glow for active panel
	if (active) {
		float flicker = 0.85f + 0.15f * sinf((float)anim_tick * 0.08f);
		u8 glow_i = (u8)(240.0f * flicker);
		GXColor glow_color = is_vmc ? (GXColor){80, 140, 220, 255}
			: (GXColor){80, 100, 180, 255};
		DrawAddChild(container, DrawEdgeGlow(px, PANEL_TOP_Y, pw,
			PANEL_BOTTOM_Y - PANEL_TOP_Y, 6.0f, glow_color, glow_i));
	}

	// Panel frame (9-slice: blue for VMC, grey for physical)
	GXTexObj *frame_tex = is_vmc ? &cm_panel_frame_vmc_tex : &cm_panel_frame_phys_tex;
	cm_draw_9slice(container, frame_tex, px, PANEL_TOP_Y, pw,
		PANEL_BOTTOM_Y - PANEL_TOP_Y, 24, 8);

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
	int total_rows = (panel->num_entries + GRID_COLS - 1) / GRID_COLS;
	int vis_rows = total_rows - panel->scroll_row;
	if (vis_rows > GRID_ROWS_VISIBLE_MAX) vis_rows = GRID_ROWS_VISIBLE_MAX;

	for (int row = 0; row < vis_rows; row++) {
		for (int col = 0; col < GRID_COLS; col++) {
			int idx = (panel->scroll_row + row) * GRID_COLS + col;
			if (idx >= panel->num_entries) break;

			int cx = grid_x + col * GRID_CELL;
			int cy = GRID_TOP_Y + row * GRID_CELL;
			int ix = cx + (GRID_CELL - GRID_ICON_SIZE) / 2;
			int iy = cy + (GRID_CELL - GRID_ICON_SIZE) / 2;

			// Selection glow (only on active panel's cursor)
			if (active && idx == panel->cursor) {
				float flicker = 0.85f + 0.15f * sinf((float)anim_tick * 0.12f);
				u8 glow_i = (u8)(255.0f * flicker);
				DrawAddChild(container, DrawGlow(
					cx + GRID_CELL / 2, cy + GRID_CELL / 2,
					GRID_CELL / 2 + 4, GRID_ICON_SIZE, (GXColor){100, 120, 200, 255}, glow_i));
			}

			// Icon or banner (only animate selected icon)
			card_entry *e = &panel->entries[idx];
			bool is_selected = (active && idx == panel->cursor);
			if (e->icon && e->icon->num_frames > 0) {
				int frame = is_selected
				? icon_anim_get_frame(e->icon, anim_tick - panel->anim_start) : 0;
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
	if (total_rows > GRID_ROWS_VISIBLE_MAX) {
		float pct = (float)panel->scroll_row / (float)(total_rows - GRID_ROWS_VISIBLE_MAX);
		DrawAddChild(container, DrawVertScrollBar(
			px + pw - 10, GRID_TOP_Y, 6,
			GRID_ROWS_VISIBLE_MAX * GRID_CELL, pct, 16));
	}
}

uiDrawObj_t *card_manager_draw(cm_panel *left, cm_panel *right,
	int active_panel, u32 anim_tick) {

	cm_draw_init();

	cm_panel *active_p = active_panel == 0 ? left : right;
	bool has_sel = active_p->card_present && active_p->num_entries > 0 &&
		active_p->cursor >= 0 && active_p->cursor < active_p->num_entries;

	uiDrawObj_t *container = DrawEmptyBox(BOX_X1, BOX_TOP_Y, BOX_X2, BOX_BOTTOM_Y);

	cm_draw_title_bar(container, 0);

	int left_x = BOX_X1;
	int right_x = BOX_X1 + PANEL_WIDTH + PANEL_GAP;

	card_manager_draw_panel(container, left, left_x, PANEL_WIDTH,
		active_panel == 0, anim_tick);
	card_manager_draw_panel(container, right, right_x, PANEL_WIDTH,
		active_panel == 1, anim_tick);

	// Hint bar
	const char *hints = has_sel
		? "A: Actions  L/R: Switch  Z: Card Source  X: Library  B: Back"
		: "L/R: Switch  Z: Card Source  X: Library  B: Back";
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
