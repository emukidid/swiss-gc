/* cm_draw.c
	- Card Manager drawing and icon animation
	by Swiss contributors
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gccore.h>
#include <ogc/lwp_watchdog.h>
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

// Activity display context — keeps screen refreshed during blocking I/O
static char s_activity_msg[256];
static uiDrawObj_t *s_activity_page = NULL;

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
		float scale = GetTextScaleToFitInWidthWithMax((char *)lines[i], content_w, CM_MSG_FONT);
		int ly = box_y + CM_MSG_PAD_Y + i * CM_MSG_LINE_H + CM_MSG_LINE_H / 2;
		DrawAddChild(box, DrawStyledLabel(640 / 2, ly,
			lines[i], scale, ALIGN_CENTER, defaultColor));
	}

	return box;
}

#define CM_PROG_BAR_H    8
#define CM_PROG_BAR_PAD  6
#define CM_PROG_LINES    2  // Fixed layout: 2 text lines + progress bar

uiDrawObj_t *cm_draw_progress(const char *msg, int current, int total) {
	char buf[512];
	strlcpy(buf, msg, sizeof(buf));

	const char *lines[16];
	int num_lines = 0;
	char *tok = strtok(buf, "\n");
	while (tok && num_lines < 16) {
		lines[num_lines++] = tok;
		tok = strtok(NULL, "\n");
	}

	// Fixed box size regardless of actual line count
	int content_w = CM_MSG_MAX_W - 2 * CM_MSG_PAD_X;
	int box_h = CM_PROG_LINES * CM_MSG_LINE_H + 2 * CM_MSG_PAD_Y
		+ CM_PROG_BAR_H + CM_PROG_BAR_PAD;
	int box_x = (640 - CM_MSG_MAX_W) / 2;
	int box_y = (480 - box_h) / 2;

	uiDrawObj_t *box = DrawEmptyBox(box_x, box_y,
		box_x + CM_MSG_MAX_W, box_y + box_h);

	for (int i = 0; i < num_lines && i < CM_PROG_LINES; i++) {
		if (lines[i][0] == ' ' && lines[i][1] == '\0') continue;
		float scale = GetTextScaleToFitInWidthWithMax((char *)lines[i], content_w, CM_MSG_FONT);
		int ly = box_y + CM_MSG_PAD_Y + i * CM_MSG_LINE_H + CM_MSG_LINE_H / 2;
		DrawAddChild(box, DrawStyledLabel(640 / 2, ly,
			lines[i], scale, ALIGN_CENTER, defaultColor));
	}

	// Progress bar — always at the same Y position
	int bar_x = box_x + CM_MSG_PAD_X;
	int bar_y = box_y + CM_MSG_PAD_Y + CM_PROG_LINES * CM_MSG_LINE_H + CM_PROG_BAR_PAD;
	int bar_w = content_w;
	float frac = (total > 0) ? (float)current / (float)total : 0.0f;
	if (frac > 1.0f) frac = 1.0f;
	int fill_w = (int)(bar_w * frac);

	// Background track
	GXColor track = {40, 40, 40, 200};
	DrawAddChild(box, DrawFlatColorRect(bar_x, bar_y, bar_w, CM_PROG_BAR_H, track));
	// Filled portion
	if (fill_w > 0) {
		GXColor fill = {100, 180, 100, 255};
		DrawAddChild(box, DrawFlatColorRect(bar_x, bar_y, fill_w, CM_PROG_BAR_H, fill));
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

// --- Toast notification ---
// Non-blocking transient message in the title bar area.

static char s_toast_msg[64];
static int s_toast_frames = 0;
#define TOAST_FADE_FRAMES 30

void cm_toast(const char *msg, int frames) {
	strlcpy(s_toast_msg, msg, sizeof(s_toast_msg));
	s_toast_frames = frames;
}

bool cm_toast_tick(void) {
	if (s_toast_frames > 0) s_toast_frames--;
	return s_toast_frames > 0;
}

void cm_draw_toast(uiDrawObj_t *container) {
	if (s_toast_frames <= 0) return;
	u8 alpha = s_toast_frames > TOAST_FADE_FRAMES
		? 220 : (u8)(220 * s_toast_frames / TOAST_FADE_FRAMES);
	GXColor color = {180, 200, 220, alpha};
	DrawAddChild(container, DrawStyledLabel(BOX_X2 - 46, TITLE_Y + 12,
		s_toast_msg, 0.4f, ALIGN_RIGHT, color));
}

// --- Activity display ---
// Keeps the screen refreshed during blocking I/O (save reads, writes, etc.)

void cm_activity_show(const char *msg) {
	strlcpy(s_activity_msg, msg, sizeof(s_activity_msg));
	cm_activity_pulse();
}

void cm_activity_hide(void) {
	if (s_activity_page) {
		DrawDispose(s_activity_page);
		s_activity_page = NULL;
	}
	s_activity_msg[0] = '\0';
}

void cm_activity_pulse(void) {
	if (!s_activity_msg[0]) return;
	cm_retire_tick();
	uiDrawObj_t *fresh = cm_draw_message(s_activity_msg);
	if (s_activity_page) DrawDispose(s_activity_page);
	s_activity_page = fresh;
	DrawPublish(fresh);
	VIDEO_WaitVSync();
}

void cm_activity_progress(const char *msg, int current, int total) {
	cm_retire_tick();
	uiDrawObj_t *fresh = cm_draw_progress(msg, current, total);
	if (s_activity_page) DrawDispose(s_activity_page);
	s_activity_page = fresh;
	DrawPublish(fresh);
	VIDEO_WaitVSync();
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
	if (icon->frames[frame].snap) return frame;
	for (int i = frame + 1; i < icon->num_frames; i++) {
		if (icon->frames[i].snap) return i;
	}
	for (int i = 0; i < frame; i++) {
		if (icon->frames[i].snap) return i;
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

// --- Cards view detail bar ---
// Grid shows fewer rows in cards view to make room for the detail strip
#define CARDS_PANEL_BOT   (GRID_TOP_Y + CARDS_GRID_ROWS * GRID_CELL + 2)
#define DETAIL_Y          (CARDS_PANEL_BOT + 8)
#define DETAIL_BANNER_W   96
#define DETAIL_BANNER_H   32

// --- Panel drawing ---

static void card_manager_draw_panel(uiDrawObj_t *container, cm_panel *panel,
	int px, int pw, bool active, u32 anim_tick, int cols) {

	bool is_vmc = (panel->source == CM_SRC_VMC);
	GXColor hdr_color;
	if (!active)
		hdr_color = (GXColor){160, 160, 160, 255};
	else if (is_vmc)
		hdr_color = (GXColor){140, 200, 255, 255};
	else
		hdr_color = defaultColor;

	// Align header text with icon grid edges
	int grid_w = cols * GRID_CELL;
	int grid_lx = px + (pw - grid_w) / 2;
	int grid_rx = grid_lx + grid_w;

	// Header: vertically centered between PANEL_TOP_Y and GRID_TOP_Y
	// GRID_TOP_Y = PANEL_TOP_Y + 36, so 36px of header space
	int hdr_mid = PANEL_TOP_Y + (GRID_TOP_Y - PANEL_TOP_Y) / 2;

	// Slot label left-aligned with first icon
	DrawAddChild(container, DrawStyledLabel(grid_lx + 4, hdr_mid, panel->label,
		0.55f, ALIGN_LEFT, hdr_color));

	// Free blocks right-aligned with last icon (or progress during loading)
	if (panel->card_present) {
		char blocks[32];
		if (panel->loading) {
			snprintf(blocks, sizeof(blocks), "Reading %d/%d...",
				panel->load_cursor, panel->num_entries);
		} else {
			int total = (panel->mem_size << 20 >> 3) / panel->sector_size - 5;
			int used = 0;
			for (int i = 0; i < panel->num_entries; i++) used += panel->entries[i].blocks;
			snprintf(blocks, sizeof(blocks), "%d/%d free", total - used, total);
		}
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
			CARDS_PANEL_BOT - PANEL_TOP_Y, 6.0f, glow_color, glow_i));
	}

	// Panel frame (9-slice: blue for VMC, grey for physical)
	GXTexObj *frame_tex = is_vmc ? &cm_panel_frame_vmc_tex : &cm_panel_frame_phys_tex;
	cm_draw_9slice(container, frame_tex, px, PANEL_TOP_Y, pw,
		CARDS_PANEL_BOT - PANEL_TOP_Y, 24, 8);

	if (panel->loading && panel->num_entries == 0) {
		DrawAddChild(container, DrawStyledLabel(px + pw / 2, GRID_TOP_Y + 60,
			"Reading...", 0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}
	if (!panel->card_present) {
		int icon_sz = 64;
		int icon_x = px + (pw - icon_sz) / 2;
		int grid_h = CARDS_PANEL_BOT - GRID_TOP_Y;
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
	int grid_x = px + (pw - cols * GRID_CELL) / 2;
	int total_rows = (panel->num_entries + cols - 1) / cols;
	int vis_rows = total_rows - panel->scroll_row;
	if (vis_rows > CARDS_GRID_ROWS) vis_rows = CARDS_GRID_ROWS;

	for (int row = 0; row < vis_rows; row++) {
		for (int col = 0; col < cols; col++) {
			int idx = (panel->scroll_row + row) * cols + col;
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
				if (e->icon->frames[frame].snap) {
					DrawAddChild(container, DrawTexObj(&e->icon->frames[frame].snap->tex,
						ix, iy, GRID_ICON_SIZE, GRID_ICON_SIZE,
						0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
				}
			} else if (e->banner_snap) {
				DrawAddChild(container, DrawTexObj(&e->banner_snap->tex,
					ix, iy, GRID_ICON_SIZE, GRID_ICON_SIZE,
					0, 0.33f, 0.67f, 0.0f, 1.0f, 0));
			}
		}
	}

	// Scroll indicator
	if (total_rows > CARDS_GRID_ROWS) {
		float pct = (float)panel->scroll_row / (float)(total_rows - CARDS_GRID_ROWS);
		DrawAddChild(container, DrawVertScrollBar(
			px + pw - 10, GRID_TOP_Y, 6,
			CARDS_GRID_ROWS * GRID_CELL, pct, 16));
	}
}

// --- Card shelf drawing ---

static void cm_draw_shelf(uiDrawObj_t *container, cm_shelf *shelf,
	cm_focus focus, u32 anim_tick) {

	int sx = BOX_X1;
	int sy = PANEL_TOP_Y;
	int sw = SHELF_WIDTH;
	int sh = PANEL_BOTTOM_Y - PANEL_TOP_Y;

	// Panel frame
	cm_draw_panel_frame(container, false, sx, sy, sw, sh);

	// Edge glow when shelf is focused
	if (focus == CM_FOCUS_SHELF) {
		float flicker = 0.85f + 0.15f * sinf((float)anim_tick * 0.08f);
		u8 glow_i = (u8)(240.0f * flicker);
		DrawAddChild(container, DrawEdgeGlow(sx, sy, sw, sh,
			6.0f, (GXColor){80, 100, 180, 255}, glow_i));
	}

	// Header: "Cards"
	GXColor hdr_color = focus == CM_FOCUS_SHELF ? defaultColor
		: (GXColor){140, 140, 140, 255};
	DrawAddChild(container, DrawStyledLabel(sx + sw / 2, sy + SHELF_HDR_H / 2 + 2,
		"Cards", 0.55f, ALIGN_CENTER, hdr_color));

	// Separator
	int sep_y = sy + SHELF_HDR_H - 2;
	DrawAddChild(container, DrawFlatColorRect(sx + 8, sep_y, sw - 16, 1,
		(GXColor){120, 120, 120, 80}));

	// List items
	int max_vis = SHELF_MAX_VIS;
	int visible = shelf->count - shelf->scroll;
	if (visible > max_vis) visible = max_vis;

	for (int i = 0; i < visible; i++) {
		int idx = shelf->scroll + i;
		shelf_item *item = &shelf->items[idx];
		int ry = SHELF_LIST_TOP + i * SHELF_ROW_H;
		int rmid = ry + SHELF_ROW_H / 2;

		// Highlight selected item
		if (focus == CM_FOCUS_SHELF && idx == shelf->cursor)
			DrawAddChild(container, cm_draw_highlight(
				sx + 4, ry, sw - 8, SHELF_ROW_H));

		// Presence dot or "+" marker
		int dot_x = sx + 10;
		int dot_y = rmid - 2;
		int dot_sz = 5;

		if (item->type == SHELF_ITEM_CREATE) {
			DrawAddChild(container, DrawStyledLabel(dot_x + 2, rmid,
				"+", 0.5f, ALIGN_CENTER, (GXColor){180, 180, 180, 255}));
		} else {
			GXColor dot_color;
			if (item->present) {
				dot_color = (GXColor){60, 200, 80, 220};
				DrawAddChild(container, DrawFlatColorRect(
					dot_x, dot_y, dot_sz, dot_sz, dot_color));
				DrawAddChild(container, DrawEdgeGlow(
					dot_x, dot_y, dot_sz, dot_sz,
					2.0f, (GXColor){60, 200, 80, 255}, 40));
			} else {
				dot_color = (GXColor){60, 60, 60, 100};
				DrawAddChild(container, DrawFlatColorRect(
					dot_x, dot_y, dot_sz, dot_sz, dot_color));
			}
		}

		// Label
		int label_x = sx + 22;
		int label_w = sw - 28;
		GXColor label_color;
		if (focus == CM_FOCUS_SHELF && idx == shelf->cursor)
			label_color = (GXColor){96, 107, 164, 255};
		else if (item->type == SHELF_ITEM_VMC)
			label_color = (GXColor){140, 200, 255, 255};
		else
			label_color = defaultColor;

		float ls = GetTextScaleToFitInWidthWithMax(item->label, label_w, 0.45f);
		DrawAddChild(container, DrawStyledLabel(label_x, rmid,
			item->label, ls, ALIGN_LEFT, label_color));
	}

	// Scroll indicator
	if (shelf->count > max_vis) {
		float pct = (float)shelf->scroll / (float)(shelf->count - max_vis);
		DrawAddChild(container, DrawVertScrollBar(
			sx + sw - 8, SHELF_LIST_TOP, 6,
			max_vis * SHELF_ROW_H, pct, 12));
	}
}

uiDrawObj_t *card_manager_draw(cm_shelf *shelf, cm_panel *detail,
	cm_focus focus, u32 anim_tick) {

	cm_draw_init();

	bool has_sel = detail->card_present && detail->num_entries > 0 &&
		detail->cursor >= 0 && detail->cursor < detail->num_entries;

	uiDrawObj_t *container = DrawEmptyBox(BOX_X1, BOX_TOP_Y, BOX_X2, BOX_BOTTOM_Y);

	cm_draw_title_bar(container, 0);

	// Draw shelf on the left
	cm_draw_shelf(container, shelf, focus, anim_tick);

	// Draw detail panel on the right
	card_manager_draw_panel(container, detail, DETAIL_X, DETAIL_WIDTH,
		focus == CM_FOCUS_DETAIL, anim_tick, DETAIL_GRID_COLS);

	// Detail bar — selected save info below the grid
	if (has_sel) {
		card_entry *sel = &detail->entries[detail->cursor];
		int dx = DETAIL_X + 8;
		int dy = DETAIL_Y;
		int detail_w = DETAIL_WIDTH - 16;

		// Banner or animated icon as thumbnail
		int thumb_w = 0;
		if (sel->banner_snap) {
			DrawAddChild(container, DrawTexObj(&sel->banner_snap->tex,
				dx, dy, DETAIL_BANNER_W, DETAIL_BANNER_H,
				0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
			thumb_w = DETAIL_BANNER_W + 8;
		} else if (sel->icon && sel->icon->num_frames > 0) {
			int frame = icon_anim_get_frame(sel->icon,
				anim_tick - detail->anim_start);
			if (sel->icon->frames[frame].snap) {
				DrawAddChild(container, DrawTexObj(
					&sel->icon->frames[frame].snap->tex,
					dx, dy, DETAIL_BANNER_H, DETAIL_BANNER_H,
					0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
				thumb_w = DETAIL_BANNER_H + 8;
			}
		}

		int tx = dx + thumb_w;
		int tw = detail_w - thumb_w;

		// Game name (large)
		if (sel->game_name[0]) {
			float ns = GetTextScaleToFitInWidthWithMax(sel->game_name, tw, 0.6f);
			DrawAddChild(container, DrawStyledLabel(tx, dy + 8,
				sel->game_name, ns, ALIGN_LEFT, defaultColor));
		}

		// Blocks + filename/description
		char info[96];
		if (sel->game_desc[0])
			snprintf(info, sizeof(info), "%d  %s: %s",
				sel->blocks, sel->filename, sel->game_desc);
		else
			snprintf(info, sizeof(info), "%d  %s", sel->blocks, sel->filename);
		float is = GetTextScaleToFitInWidthWithMax(info, tw, 0.45f);
		DrawAddChild(container, DrawStyledLabel(tx, dy + 24,
			info, is, ALIGN_LEFT, (GXColor){180, 180, 180, 255}));
	}

	// Hint bar
	const char *hints;
	if (focus == CM_FOCUS_SHELF) {
		shelf_item *sel_item = &shelf->items[shelf->cursor];
		if (sel_item->type == SHELF_ITEM_VMC)
			hints = "A: View Card  Y: Delete  X: Library  B: Back";
		else
			hints = "A: View Card  X: Library  B: Back";
	} else if (has_sel)
		hints = "A: Actions  B: Cards  X: Library";
	else
		hints = "B: Cards  X: Library";
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

	// Menu dimensions — cap height so long lists scroll
	int menu_w = 240;
	int item_h = 26;
	int pad = 12;
	int max_visible_items = 14;
	int visible = count < max_visible_items ? count : max_visible_items;
	int menu_h = visible * item_h + pad * 2;
	if (menu_h > 400) menu_h = 400;
	int menu_x = (640 - menu_w) / 2;
	int menu_y = (480 - menu_h) / 2;
	int max_visible = (menu_h - pad * 2) / item_h;
	int scroll = 0;
	if (cursor >= max_visible) scroll = cursor - max_visible + 1;

	uiDrawObj_t *overlay = NULL;
	bool needs_redraw = true;
	bool debounce = false;

	while (1) {
		cm_retire_tick();

		if (needs_redraw) {
			uiDrawObj_t *fresh = DrawEmptyBox(menu_x, menu_y,
				menu_x + menu_w, menu_y + menu_h);

			for (int vi = 0; vi < max_visible && scroll + vi < count; vi++) {
				int i = scroll + vi;
				int iy = menu_y + pad + vi * item_h + item_h / 2;
				GXColor color;
				if (!enabled[i]) {
					color = (GXColor){80, 80, 80, 255};
				} else if (i == cursor) {
					color = (GXColor){96, 107, 164, 255};
					DrawAddChild(fresh, cm_draw_highlight(
						menu_x + 6, menu_y + pad + vi * item_h,
						menu_w - 12, item_h));
				} else {
					color = defaultColor;
				}
				DrawAddChild(fresh, DrawStyledLabel(menu_x + 20, iy,
					items[i], 0.55f, ALIGN_LEFT, color));
			}

			if (overlay) DrawDispose(overlay);
			overlay = fresh;
			DrawPublish(fresh);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();
		u16 btns = padsButtonsHeld();
		s8 stickY = padsStickY();

		if (debounce) {
			if (!(btns & (PAD_BUTTON_UP | PAD_BUTTON_DOWN)))
				debounce = false;
			continue;
		}

		if ((btns & PAD_BUTTON_UP) || stickY > 16) {
			int next = cursor - 1;
			while (next >= 0 && !enabled[next]) next--;
			if (next >= 0) {
				cursor = next;
				if (cursor < scroll) scroll = cursor;
				needs_redraw = true; debounce = true;
			}
		}
		if ((btns & PAD_BUTTON_DOWN) || stickY < -16) {
			int next = cursor + 1;
			while (next < count && !enabled[next]) next++;
			if (next < count) {
				cursor = next;
				if (cursor >= scroll + max_visible) scroll = cursor - max_visible + 1;
				needs_redraw = true; debounce = true;
			}
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
	}
}
