/* cm_library.c
	- Save Library: full-screen unified save browser
	  Shows all saves across physical cards, VMCs, and SD backups
	  grouped by game, with per-save source badges and actions.
	by Swiss contributors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include "cm_internal.h"
#include "IPLFontWrite.h"
#include "swiss.h"

// Horizontal split: game list gets 200px, saves get the rest, shared gap
#define LIB_GAME_FW     200
#define LIB_GAME_FX     BOX_X1
#define LIB_SAVE_FX     (LIB_GAME_FX + LIB_GAME_FW + PANEL_GAP)
#define LIB_SAVE_FW     (BOX_X2 - LIB_SAVE_FX)

// Content inset within panel frames
#define LIB_INSET       8
#define LIB_HDR_H       20
#define LIB_LIST_TOP    (PANEL_TOP_Y + LIB_INSET + LIB_HDR_H)
#define LIB_LIST_BOT    (PANEL_BOTTOM_Y - LIB_INSET)

// Row sizes
#define LIB_ROW_H       28
#define LIB_ICON_SZ     24
#define LIB_SAVE_ROW_H  24

// Save panel: detail header (banner + game info above save rows)
#define LIB_DETAIL_H    52
#define LIB_BANNER_W    96
#define LIB_BANNER_H    32
#define LIB_SAVE_HDR_TOP  (PANEL_TOP_Y + LIB_INSET)
#define LIB_SAVE_LIST_TOP (LIB_SAVE_HDR_TOP + LIB_DETAIL_H)

// Visible row counts (derived)
#define LIB_MAX_VIS       ((LIB_LIST_BOT - LIB_LIST_TOP) / LIB_ROW_H)
#define LIB_SAVE_MAX_VIS  ((LIB_LIST_BOT - LIB_SAVE_LIST_TOP) / LIB_SAVE_ROW_H)

// GC epoch for date formatting
#define GC_EPOCH_OFFSET 946684800ULL

// Device indicator box dimensions (4 per save row)
#define LIB_IND_W   14
#define LIB_IND_H   14
#define LIB_IND_GAP 2

static const char *lib_ind_labels[LIB_NUM_DEVICES] = {"A", "B", "vA", "vB"};

static void lib_draw_thin_scrollbar(uiDrawObj_t *container,
	int x, int top_y, int track_h, int total, int visible, int scroll) {
	int thumb_w = 6, thumb_h = 6;
	float pct = (float)scroll / (float)(total - visible);
	int thumb_y = top_y + (int)((track_h - thumb_h) * pct);
	DrawAddChild(container, DrawFlatColorRect(x, top_y, 2, track_h,
		(GXColor){128, 128, 128, 128}));
	DrawAddChild(container, DrawFlatColorRect(x - (thumb_w / 2 - 1), thumb_y, thumb_w, thumb_h,
		(GXColor){200, 200, 200, 255}));
}

static void lib_format_date(u32 gc_time, char *buf, int buflen) {
	if (gc_time == 0 || gc_time == (u32)-1) {
		snprintf(buf, buflen, "----");
		return;
	}
	time_t unix_time = (time_t)((u64)gc_time + GC_EPOCH_OFFSET);
	struct tm *t = gmtime(&unix_time);
	if (!t) { snprintf(buf, buflen, "----"); return; }
	snprintf(buf, buflen, "%04d-%02d-%02d",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

// --- Device loading ---

// Load VMC device saves (no CARD hardware access)
static void lib_load_vmc_device(lib_device *dev) {
	card_manager_free_graphics(dev->entries, dev->num_entries);
	dev->num_entries = 0;
	dev->present = false;
	dev->mem_size = 0;
	dev->sector_size = 0;

	if (dev->vmc_path[0]) {
		int n = vmc_read_saves(dev->vmc_path, dev->entries, 128,
			&dev->mem_size, &dev->sector_size);
		if (n >= 0) {
			dev->num_entries = n;
			dev->present = true;
		}
	}

	dev->loaded = true;
	dev->needs_reload = false;
}

// Sync a physical lib_device from a panel that already loaded the slot.
// Deep-copies metadata and graphics — the library owns its own data,
// completely independent of panel state. No shared pointers.
void lib_sync_from_panel(lib_state *st, cm_panel *panel) {
	if (!st->initialized || panel->source != CM_SRC_PHYSICAL) return;

	int dev_idx = (panel->slot == CARD_SLOTA) ? LIB_DEV_PHYS_A : LIB_DEV_PHYS_B;
	lib_device *dev = &st->devices[dev_idx];

	// Free old library graphics before overwriting
	card_manager_free_graphics(dev->entries, dev->num_entries);

	dev->num_entries = 0;
	dev->present = panel->card_present;
	dev->mem_size = panel->mem_size;
	dev->sector_size = panel->sector_size;

	for (int i = 0; i < panel->num_entries && i < 128; i++) {
		dev->entries[i] = panel->entries[i];
		cm_deep_copy_graphics(&dev->entries[i], &panel->entries[i]);
	}
	dev->num_entries = panel->num_entries;
	dev->loaded = true;
	dev->needs_reload = false;
}

static void lib_init_devices(lib_state *st) {
	memset(st->devices, 0, sizeof(st->devices));

	st->devices[LIB_DEV_PHYS_A].slot = CARD_SLOTA;
	strlcpy(st->devices[LIB_DEV_PHYS_A].label, "Slot A", 32);

	st->devices[LIB_DEV_PHYS_B].slot = CARD_SLOTB;
	strlcpy(st->devices[LIB_DEV_PHYS_B].label, "Slot B", 32);

	st->devices[LIB_DEV_VMC_A].slot = -1;
	st->devices[LIB_DEV_VMC_B].slot = -1;

	vmc_file_entry vmcs[MAX_VMC_FILES];
	int n = vmc_scan_files(vmcs, MAX_VMC_FILES);

	for (int i = 0; i < n; i++) {
		char *name = strrchr(vmcs[i].path, '/');
		name = name ? name + 1 : vmcs[i].path;

		if (strncasecmp(name, "MemoryCardA", 11) == 0 && vmcs[i].filesize > 0
			&& !st->devices[LIB_DEV_VMC_A].vmc_path[0]) {
			strlcpy(st->devices[LIB_DEV_VMC_A].vmc_path, vmcs[i].path, PATHNAME_MAX);
			strlcpy(st->devices[LIB_DEV_VMC_A].label, vmcs[i].label, 32);
		}
		else if (strncasecmp(name, "MemoryCardB", 11) == 0 && vmcs[i].filesize > 0
			&& !st->devices[LIB_DEV_VMC_B].vmc_path[0]) {
			strlcpy(st->devices[LIB_DEV_VMC_B].vmc_path, vmcs[i].path, PATHNAME_MAX);
			strlcpy(st->devices[LIB_DEV_VMC_B].label, vmcs[i].label, 32);
		}
	}

	// Load VMC devices independently (no CARD hardware access)
	for (int d = 0; d < LIB_NUM_DEVICES; d++) {
		if (st->devices[d].vmc_path[0])
			lib_load_vmc_device(&st->devices[d]);
	}
	// Physical devices are synced from panels via lib_sync_from_panel
}

// --- Build unified save index ---

static int lib_build_index(lib_state *st) {
	int ns = 0;

	for (int d = 0; d < LIB_NUM_DEVICES; d++) {
		lib_device *dev = &st->devices[d];
		if (!dev->present) continue;

		for (int i = 0; i < dev->num_entries && ns < LIB_MAX_SAVES; i++) {
			lib_save *s = &st->saves[ns];
			s->entry = &dev->entries[i];
			s->device_idx = d;
			s->gci_idx = -1;

			if (d == LIB_DEV_PHYS_A)
				s->source = LIB_SRC_SLOT_A;
			else if (d == LIB_DEV_PHYS_B)
				s->source = LIB_SRC_SLOT_B;
			else
				s->source = LIB_SRC_VMC;

			strlcpy(s->source_label, dev->label, sizeof(s->source_label));
			strlcpy(s->display_name, dev->entries[i].filename,
				sizeof(s->display_name));
			ns++;
		}
	}

	for (int i = 0; i < st->num_gci && ns < LIB_MAX_SAVES; i++) {
		lib_save *s = &st->saves[ns];
		s->source = LIB_SRC_GCI;
		s->entry = &st->gci_files[i].entry;
		strlcpy(s->source_label, "SD Backup", sizeof(s->source_label));
		s->device_idx = -1;
		s->gci_idx = i;
		const char *slash = strrchr(st->gci_files[i].path, '/');
		strlcpy(s->display_name, slash ? slash + 1 : st->gci_files[i].path,
			sizeof(s->display_name));
		ns++;
	}

	// Build game groups
	int ng = 0;
	for (int i = 0; i < ns && ng < LIB_MAX_GROUPS; i++) {
		card_entry *e = st->saves[i].entry;
		int found = -1;
		for (int g = 0; g < ng; g++) {
			if (strcmp(st->groups[g].gamecode, e->gamecode) == 0) {
				found = g;
				break;
			}
		}
		if (found >= 0) {
			st->groups[found].num_saves++;
			// Prefer a rep with graphics (VMC/GCI entries have them, physical don't)
			if (!st->groups[found].rep->icon && !st->groups[found].rep->banner
				&& (e->icon || e->banner))
				st->groups[found].rep = e;
		} else {
			lib_game_group *g = &st->groups[ng];
			strlcpy(g->gamecode, e->gamecode, 5);
			strlcpy(g->game_name,
				e->game_name[0] ? e->game_name : e->filename,
				sizeof(g->game_name));
			g->rep = e;
			g->first_save = i;
			g->num_saves = 1;
			ng++;
		}
	}

	// Sort groups alphabetically by game name
	for (int i = 0; i < ng - 1; i++) {
		for (int j = i + 1; j < ng; j++) {
			if (strcasecmp(st->groups[i].game_name, st->groups[j].game_name) > 0) {
				lib_game_group tmp = st->groups[i];
				st->groups[i] = st->groups[j];
				st->groups[j] = tmp;
			}
		}
	}

	st->num_saves = ns;
	st->num_groups = ng;
	return ng;
}

static int lib_get_saves_for_game(lib_save *all_saves, int num_saves,
	const char *gamecode, int *indices, int max) {
	int count = 0;
	for (int i = 0; i < num_saves && count < max; i++) {
		if (strcmp(all_saves[i].entry->gamecode, gamecode) == 0)
			indices[count++] = i;
	}
	return count;
}

// --- Selection update ---

static void lib_update_selection(lib_state *st) {
	st->save_count = 0;
	st->sel_group = NULL;
	st->num_card_saves = 0;
	st->num_gci_saves = 0;
	st->total_rows = 0;

	if (st->num_groups > 0 && st->game_cursor < st->num_groups) {
		st->sel_group = &st->groups[st->game_cursor];
		st->save_count = lib_get_saves_for_game(st->saves, st->num_saves,
			st->sel_group->gamecode, st->save_indices, LIB_MAX_SAVES);

		for (int i = 0; i < st->save_count; i++) {
			int si = st->save_indices[i];
			lib_save *s = &st->saves[si];

			if (s->source == LIB_SRC_GCI) {
				st->gci_save_indices[st->num_gci_saves++] = si;
				continue;
			}

			// Group by filename
			int found = -1;
			for (int c = 0; c < st->num_card_saves; c++) {
				if (strcmp(st->card_saves[c].filename, s->entry->filename) == 0) {
					found = c;
					break;
				}
			}
			if (found < 0 && st->num_card_saves < LIB_MAX_CARD_SAVES) {
				found = st->num_card_saves++;
				lib_card_save *cs = &st->card_saves[found];
				strlcpy(cs->filename, s->entry->filename, sizeof(cs->filename));
				cs->blocks = s->entry->blocks;
				cs->time = s->entry->time;
				for (int d = 0; d < LIB_NUM_DEVICES; d++)
					cs->save_idx[d] = -1;
			}
			if (found >= 0) {
				if (s->device_idx >= 0 && s->device_idx < LIB_NUM_DEVICES)
					st->card_saves[found].save_idx[s->device_idx] = si;
				if (s->entry->time > st->card_saves[found].time)
					st->card_saves[found].time = s->entry->time;
			}
		}
	}

	st->total_rows = st->num_card_saves;
	if (st->num_gci_saves > 0)
		st->total_rows += 1 + st->num_gci_saves;

	if (st->save_cursor >= st->total_rows)
		st->save_cursor = st->total_rows > 0 ? st->total_rows - 1 : 0;
	if (st->num_card_saves > 0 && st->num_gci_saves > 0
		&& st->save_cursor == st->num_card_saves)
		st->save_cursor = st->num_card_saves > 0 ? st->num_card_saves - 1 : st->num_card_saves + 1;
}

// --- Rebuild helpers ---

void lib_rebuild_index(lib_state *st) {
	char saved_gamecode[5] = {0};
	if (st->sel_group)
		strlcpy(saved_gamecode, st->sel_group->gamecode, 5);

	card_manager_free_gci_files(st->gci_files, st->num_gci);
	st->num_gci = card_manager_scan_gci_files(st->gci_files, MAX_GCI_FILES);
	lib_build_index(st);

	if (saved_gamecode[0]) {
		for (int g = 0; g < st->num_groups; g++) {
			if (strcmp(st->groups[g].gamecode, saved_gamecode) == 0) {
				st->game_cursor = g;
				break;
			}
		}
	}
	if (st->game_cursor >= st->num_groups)
		st->game_cursor = st->num_groups > 0 ? st->num_groups - 1 : 0;
	if (st->game_scroll > st->game_cursor)
		st->game_scroll = st->game_cursor;
	st->save_cursor = 0;
	st->save_scroll = 0;
	lib_update_selection(st);
}

// --- Drawing ---

static void lib_draw_game_list(uiDrawObj_t *container,
	lib_game_group *groups, int num_groups,
	int cursor, int scroll, bool active, u32 anim_tick) {

	int lx = LIB_GAME_FX + LIB_INSET;
	int lw = LIB_GAME_FW - 2 * LIB_INSET;

	cm_draw_panel_frame(container, false,
		LIB_GAME_FX, PANEL_TOP_Y, LIB_GAME_FW, PANEL_BOTTOM_Y - PANEL_TOP_Y);

	DrawAddChild(container, DrawStyledLabel(LIB_GAME_FX + LIB_GAME_FW / 2,
		PANEL_TOP_Y + LIB_INSET + 6, "Games", 0.55f, ALIGN_CENTER,
		active ? defaultColor : (GXColor){140, 140, 140, 255}));

	if (num_groups == 0) {
		DrawAddChild(container, DrawStyledLabel(lx + lw / 2, LIB_LIST_TOP + 40,
			"No saves found", 0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}

	int visible = num_groups - scroll;
	if (visible > LIB_MAX_VIS) visible = LIB_MAX_VIS;
	int cw = (num_groups > LIB_MAX_VIS) ? lw - 8 : lw;

	for (int i = 0; i < visible; i++) {
		int idx = scroll + i;
		lib_game_group *g = &groups[idx];
		int ry = LIB_LIST_TOP + i * LIB_ROW_H;

		if (active && idx == cursor)
			DrawAddChild(container, cm_draw_highlight(lx, ry, cw, LIB_ROW_H));

		// Icon
		int ix = lx + 4;
		int iy = ry + (LIB_ROW_H - LIB_ICON_SZ) / 2;
		int icon_cx = ix + LIB_ICON_SZ / 2;
		int icon_cy = iy + LIB_ICON_SZ / 2;
		card_entry *e = g->rep;

		if (active && idx == cursor) {
			float flicker = 0.85f + 0.15f * sinf((float)anim_tick * 0.12f);
			u8 glow_i = (u8)(255.0f * flicker);
			DrawAddChild(container, DrawGlow(icon_cx, icon_cy,
				LIB_ICON_SZ, LIB_ICON_SZ, (GXColor){100, 120, 200, 255}, glow_i));
		}

		if (e->icon && e->icon->num_frames > 0 && e->icon->frames[0].data) {
			DrawAddChild(container, DrawTexObj(&e->icon->frames[0].tex,
				ix, iy, LIB_ICON_SZ, LIB_ICON_SZ,
				0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
		} else if (e->banner) {
			DrawAddChild(container, DrawTexObj(&e->banner_tex,
				ix, iy, LIB_ICON_SZ, LIB_ICON_SZ,
				0, 0.33f, 0.67f, 0.0f, 1.0f, 0));
		}

		int tx = ix + LIB_ICON_SZ + 6;
		GXColor name_color;
		if (active && idx == cursor)
			name_color = (GXColor){96, 107, 164, 255};
		else if (!active)
			name_color = (GXColor){160, 160, 160, 255};
		else
			name_color = defaultColor;

		float ns = GetTextScaleToFitInWidthWithMax(g->game_name,
			cw - LIB_ICON_SZ - 16, 0.5f);
		DrawAddChild(container, DrawStyledLabel(tx, ry + 9,
			g->game_name, ns, ALIGN_LEFT, name_color));

		char stats[24];
		snprintf(stats, sizeof(stats), "%d save%s",
			g->num_saves, g->num_saves > 1 ? "s" : "");
		DrawAddChild(container, DrawStyledLabel(tx, ry + 20,
			stats, 0.4f, ALIGN_LEFT, (GXColor){120, 120, 120, 255}));
	}

	if (num_groups > LIB_MAX_VIS)
		lib_draw_thin_scrollbar(container, lx + lw - 2, LIB_LIST_TOP,
			LIB_MAX_VIS * LIB_ROW_H, num_groups, LIB_MAX_VIS, scroll);
}

static void lib_draw_indicator(uiDrawObj_t *container, int x, int y,
	bool save_present, bool device_present, bool is_vmc, const char *label) {
	if (!device_present) {
		DrawAddChild(container, DrawFlatColorRect(x, y, LIB_IND_W, LIB_IND_H,
			(GXColor){40, 40, 40, 64}));
	} else if (save_present) {
		GXColor fill = is_vmc ? (GXColor){100, 160, 220, 255}
			: (GXColor){180, 180, 180, 255};
		DrawAddChild(container, DrawFlatColorRect(x, y, LIB_IND_W, LIB_IND_H, fill));
		DrawAddChild(container, DrawStyledLabel(x + LIB_IND_W / 2, y + LIB_IND_H / 2,
			label, 0.35f, ALIGN_CENTER, (GXColor){20, 20, 20, 255}));
	} else {
		DrawAddChild(container, DrawFlatColorRect(x, y, LIB_IND_W, LIB_IND_H,
			(GXColor){60, 60, 60, 128}));
		DrawAddChild(container, DrawStyledLabel(x + LIB_IND_W / 2, y + LIB_IND_H / 2,
			label, 0.35f, ALIGN_CENTER, (GXColor){60, 60, 60, 128}));
	}
}

static void lib_draw_save_list(uiDrawObj_t *container, lib_state *st,
	bool active) {

	int lx = LIB_SAVE_FX + LIB_INSET;
	int lw = LIB_SAVE_FW - 2 * LIB_INSET;

	cm_draw_panel_frame(container, false,
		LIB_SAVE_FX, PANEL_TOP_Y, LIB_SAVE_FW, PANEL_BOTTOM_Y - PANEL_TOP_Y);

	if (!st->sel_group) {
		DrawAddChild(container, DrawStyledLabel(lx + lw / 2, LIB_LIST_TOP + 40,
			"Select a game", 0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}

	lib_game_group *group = st->sel_group;

	// --- Detail header: banner + game info ---
	card_entry *rep = group->rep;
	int hdr_y = LIB_SAVE_HDR_TOP;
	int text_x = lx + 4;

	if (rep->banner) {
		DrawAddChild(container, DrawTexObj(&rep->banner_tex,
			lx + lw - LIB_BANNER_W - 4, hdr_y,
			LIB_BANNER_W, LIB_BANNER_H,
			0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	}

	int name_max_w = lw - LIB_BANNER_W - 16;
	float name_scale = GetTextScaleToFitInWidthWithMax(group->game_name,
		name_max_w, 0.6f);
	DrawAddChild(container, DrawStyledLabel(text_x, hdr_y + 8,
		group->game_name, name_scale, ALIGN_LEFT,
		active ? defaultColor : (GXColor){160, 160, 160, 255}));

	if (rep->game_desc[0]) {
		float desc_scale = GetTextScaleToFitInWidthWithMax(rep->game_desc,
			name_max_w, 0.45f);
		DrawAddChild(container, DrawStyledLabel(text_x, hdr_y + 22,
			rep->game_desc, desc_scale, ALIGN_LEFT,
			(GXColor){160, 160, 160, 255}));
	}

	char info[64];
	snprintf(info, sizeof(info), "%s  %d on cards  %d backup%s",
		group->gamecode, st->num_card_saves, st->num_gci_saves,
		st->num_gci_saves != 1 ? "s" : "");
	DrawAddChild(container, DrawStyledLabel(text_x, hdr_y + 36,
		info, 0.4f, ALIGN_LEFT, (GXColor){120, 120, 120, 255}));

	DrawAddChild(container, cm_draw_rect(lx, LIB_SAVE_LIST_TOP - 4, lw, 1));

	// --- Two-section save list ---
	if (st->total_rows == 0) {
		DrawAddChild(container, DrawStyledLabel(lx + lw / 2, LIB_SAVE_LIST_TOP + 20,
			"No saves", 0.5f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}

	int max_vis = LIB_SAVE_MAX_VIS;
	int visible = st->total_rows - st->save_scroll;
	if (visible > max_vis) visible = max_vis;
	int cw = (st->total_rows > max_vis) ? lw - 8 : lw;

	// 4 device indicators per card row
	int ind_total_w = LIB_NUM_DEVICES * LIB_IND_W + (LIB_NUM_DEVICES - 1) * LIB_IND_GAP;
	int ind_x = lx + cw - ind_total_w - 4;

	for (int i = 0; i < visible; i++) {
		int vi = st->save_scroll + i;
		int ry = LIB_SAVE_LIST_TOP + i * LIB_SAVE_ROW_H;

		if (vi < st->num_card_saves) {
			// --- Card status row ---
			lib_card_save *cs = &st->card_saves[vi];

			if (active && vi == st->save_cursor)
				DrawAddChild(container, cm_draw_highlight(lx, ry, cw, LIB_SAVE_ROW_H));

			GXColor name_color = (active && vi == st->save_cursor)
				? (GXColor){96, 107, 164, 255} : defaultColor;

			char row_text[80];
			snprintf(row_text, sizeof(row_text), "%s  %d blk", cs->filename, cs->blocks);
			int text_w = ind_x - lx - 8;
			float rs = GetTextScaleToFitInWidthWithMax(row_text, text_w, 0.42f);
			DrawAddChild(container, DrawStyledLabel(lx + 4, ry + LIB_SAVE_ROW_H / 2,
				row_text, rs, ALIGN_LEFT, name_color));

			// 4 device indicators
			int iy = ry + (LIB_SAVE_ROW_H - LIB_IND_H) / 2;
			for (int d = 0; d < LIB_NUM_DEVICES; d++) {
				int ix = ind_x + d * (LIB_IND_W + LIB_IND_GAP);
				lib_draw_indicator(container, ix, iy,
					cs->save_idx[d] >= 0,
					st->devices[d].present,
					d >= LIB_DEV_VMC_A,
					lib_ind_labels[d]);
			}

		} else if (st->num_card_saves > 0 && st->num_gci_saves > 0
				&& vi == st->num_card_saves) {
			// --- Divider row ---
			DrawAddChild(container, DrawStyledLabel(lx + 4, ry + LIB_SAVE_ROW_H / 2,
				"SD Backups", 0.4f, ALIGN_LEFT, (GXColor){140, 140, 140, 255}));
			DrawAddChild(container, cm_draw_rect(lx, ry + LIB_SAVE_ROW_H - 2, lw, 1));

		} else {
			// --- GCI backup row ---
			int gci_row = (st->num_card_saves > 0 && st->num_gci_saves > 0)
				? vi - st->num_card_saves - 1 : vi;
			if (gci_row < 0 || gci_row >= st->num_gci_saves) continue;

			int si = st->gci_save_indices[gci_row];
			lib_save *s = &st->saves[si];

			if (active && vi == st->save_cursor)
				DrawAddChild(container, cm_draw_highlight(lx, ry, cw, LIB_SAVE_ROW_H));

			GXColor name_color = (active && vi == st->save_cursor)
				? (GXColor){96, 107, 164, 255} : defaultColor;

			DrawAddChild(container, DrawStyledLabel(lx + 4, ry + LIB_SAVE_ROW_H / 2,
				"SD", 0.4f, ALIGN_LEFT, (GXColor){200, 200, 140, 255}));

			char datebuf[16];
			lib_format_date(s->entry->time, datebuf, sizeof(datebuf));
			char row_text[128];
			snprintf(row_text, sizeof(row_text), "%s  %s", s->display_name, datebuf);
			int tx = lx + 28;
			int text_w = cw - 36;
			float rs = GetTextScaleToFitInWidthWithMax(row_text, text_w, 0.42f);
			DrawAddChild(container, DrawStyledLabel(tx, ry + LIB_SAVE_ROW_H / 2,
				row_text, rs, ALIGN_LEFT, name_color));
		}
	}

	if (st->total_rows > max_vis)
		lib_draw_thin_scrollbar(container, lx + lw - 2, LIB_SAVE_LIST_TOP,
			max_vis * LIB_SAVE_ROW_H, st->total_rows, max_vis, st->save_scroll);
}

// --- Context menu for a save in the library ---

enum {
	LIB_ACT_COPY,
	LIB_ACT_EXPORT,
	LIB_ACT_DELETE,
	LIB_ACT_COUNT
};

// Set activity LED on device and any matching panel
static void lib_set_activity(lib_state *st, int dev_idx, cm_panel *panels[2], u8 activity) {
	if (dev_idx < 0) return;
	lib_device *dev = &st->devices[dev_idx];
	dev->activity = activity;
	for (int p = 0; p < 2; p++) {
		if (dev_idx < LIB_DEV_VMC_A) {
			if (panels[p]->source == CM_SRC_PHYSICAL && panels[p]->slot == dev->slot)
				panels[p]->activity = activity;
		} else {
			if (panels[p]->source == CM_SRC_VMC
				&& strcmp(panels[p]->vmc_path, dev->vmc_path) == 0)
				panels[p]->activity = activity;
		}
	}
}

// Flag device + matching panel(s) for reload after mutation
static void lib_flag_reload(lib_state *st, int dev_idx, cm_panel *panels[2]) {
	if (dev_idx < 0) return;
	lib_device *dev = &st->devices[dev_idx];
	dev->needs_reload = true;
	for (int p = 0; p < 2; p++) {
		if (dev_idx < LIB_DEV_VMC_A) {
			if (panels[p]->source == CM_SRC_PHYSICAL && panels[p]->slot == dev->slot)
				panels[p]->needs_reload = true;
		} else {
			if (panels[p]->source == CM_SRC_VMC
				&& strcmp(panels[p]->vmc_path, dev->vmc_path) == 0)
				panels[p]->needs_reload = true;
		}
	}
}

static bool lib_handle_action(lib_state *st, lib_save *s, cm_panel *panels[2]) {
	const char *items[LIB_ACT_COUNT];
	bool enabled[LIB_ACT_COUNT];

	bool is_gci = (s->source == LIB_SRC_GCI);
	lib_device *src_dev = (s->device_idx >= 0) ? &st->devices[s->device_idx] : NULL;

	items[LIB_ACT_COPY] = "Copy to...";
	enabled[LIB_ACT_COPY] = true;

	items[LIB_ACT_EXPORT] = is_gci ? "Rename" : "Export to SD (.gci)";
	enabled[LIB_ACT_EXPORT] = true;

	items[LIB_ACT_DELETE] = is_gci ? "Delete backup" : "Delete";
	enabled[LIB_ACT_DELETE] = true;

	int choice = cm_context_menu(items, enabled, LIB_ACT_COUNT);

	switch (choice) {
		case LIB_ACT_COPY: {
			vmc_file_entry *vmcs = calloc(MAX_VMC_FILES, sizeof(vmc_file_entry));
			int num_vmcs = vmcs ? vmc_scan_files(vmcs, MAX_VMC_FILES) : 0;

			const char *dest_items[MAX_VMC_FILES + 3];
			bool dest_enabled[MAX_VMC_FILES + 3];
			int dest_count = 0;

			// Physical slots (skip source device)
			for (int slot = 0; slot < 2; slot++) {
				if (s->device_idx == slot)
					continue;
				dest_items[dest_count] = slot == 0 ? "Physical Slot A" : "Physical Slot B";
				dest_enabled[dest_count] = true;
				dest_count++;
			}
			// VMC files (skip source VMC)
			for (int i = 0; i < num_vmcs; i++) {
				if (src_dev && s->source == LIB_SRC_VMC
					&& strcmp(src_dev->vmc_path, vmcs[i].path) == 0)
					continue;
				dest_items[dest_count] = vmcs[i].label;
				dest_enabled[dest_count] = true;
				dest_count++;
			}
			// SD backup
			if (!is_gci) {
				dest_items[dest_count] = "SD Backup (.gci)";
				dest_enabled[dest_count] = true;
				dest_count++;
			}

			if (dest_count == 0) {
				uiDrawObj_t *msg = cm_draw_message("No destinations available.");
				DrawPublish(msg);
				while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
				while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
				DrawDispose(msg);
				free(vmcs);
				return false;
			}

			int dest_choice = cm_context_menu(dest_items, dest_enabled, dest_count);
			if (dest_choice < 0) { free(vmcs); return false; }

			// Read source save as GCI buffer
			GCI gci;
			u8 *savedata = NULL;
			u32 save_len = 0;
			bool read_ok = false;


			lib_set_activity(st, s->device_idx, panels, CM_ACTIVITY_READ);

			if (s->source == LIB_SRC_SLOT_A || s->source == LIB_SRC_SLOT_B)
				read_ok = cm_read_physical_save(src_dev->slot, s->entry,
					src_dev->sector_size, &gci, &savedata, &save_len);
			else if (s->source == LIB_SRC_VMC)
				read_ok = vmc_read_save(src_dev->vmc_path, s->entry,
					&gci, &savedata, &save_len);
			else if (s->source == LIB_SRC_GCI && s->gci_idx >= 0) {
				DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG]
					? devices[DEVICE_CONFIG] : devices[DEVICE_CUR];
				if (dev) {
					file_handle *f = calloc(1, sizeof(file_handle));
					if (f) {
						strlcpy(f->name, st->gci_files[s->gci_idx].path, PATHNAME_MAX);
						if (!dev->statFile(f) && f->size > sizeof(GCI)) {
							u8 *buf = memalign(32, f->size);
							if (buf) {
								dev->readFile(f, buf, f->size);
								dev->closeFile(f);
								memcpy(&gci, buf, sizeof(GCI));
								save_len = f->size - sizeof(GCI);
								savedata = memalign(32, save_len);
								if (savedata) {
									memcpy(savedata, buf + sizeof(GCI), save_len);
									read_ok = true;
								}
								free(buf);
							}
						}
						free(f);
					}
				}
			}

			lib_set_activity(st, s->device_idx, panels, CM_ACTIVITY_IDLE);
			if (!read_ok) { free(vmcs); return false; }

			// Determine destination and write
			bool write_ok = false;
			int phys_offset = 0;
			for (int slot = 0; slot < 2; slot++) {
				if (s->device_idx == slot)
					continue;
				if (dest_choice == phys_offset) {
					int dest_dev_idx = (slot == 0) ? LIB_DEV_PHYS_A : LIB_DEV_PHYS_B;
					lib_set_activity(st, dest_dev_idx, panels, CM_ACTIVITY_WRITE);
					s32 dest_sector = 8192;
					CARD_ProbeEx(slot, NULL, &dest_sector);
					s32 init_ret = initialize_card(slot);
					if (init_ret == CARD_ERROR_READY)
						write_ok = card_manager_import_gci_buf(slot, &gci, savedata,
							save_len, dest_sector);
					lib_set_activity(st, dest_dev_idx, panels, CM_ACTIVITY_IDLE);
					if (write_ok)
						lib_flag_reload(st, dest_dev_idx, panels);
					free(savedata);
					free(vmcs);
					return write_ok;
				}
				phys_offset++;
			}

			// VMC destinations
			int vmc_offset = phys_offset;
			for (int i = 0; i < num_vmcs; i++) {
				if (src_dev && s->source == LIB_SRC_VMC
					&& strcmp(src_dev->vmc_path, vmcs[i].path) == 0)
					continue;
				if (dest_choice == vmc_offset) {
					// Find matching lib device for LED
					int dest_dev_idx = -1;
					for (int d = LIB_DEV_VMC_A; d <= LIB_DEV_VMC_B; d++) {
						if (strcmp(st->devices[d].vmc_path, vmcs[i].path) == 0) {
							dest_dev_idx = d;
							break;
						}
					}
					lib_set_activity(st, dest_dev_idx, panels, CM_ACTIVITY_WRITE);
					write_ok = vmc_import_save_buf(vmcs[i].path, &gci, savedata, save_len);
					lib_set_activity(st, dest_dev_idx, panels, CM_ACTIVITY_IDLE);
					if (write_ok && dest_dev_idx >= 0)
						lib_flag_reload(st, dest_dev_idx, panels);
					free(savedata);
					free(vmcs);
					return write_ok;
				}
				vmc_offset++;
			}

			// SD backup destination
			if (!is_gci && dest_choice == vmc_offset) {
				write_ok = cm_write_gci_to_sd(&gci, savedata, save_len);
				free(savedata);
				free(vmcs);
				return write_ok;
			}

			free(savedata);
			free(vmcs);
			return false;
		}

		case LIB_ACT_EXPORT:

			if (is_gci) {
				if (s->gci_idx >= 0)
					return cm_backup_rename(&st->gci_files[s->gci_idx]);
			} else if (s->source == LIB_SRC_VMC) {
				return vmc_export_save(src_dev->vmc_path, s->entry);
			} else if (src_dev) {
				return card_manager_export_save(src_dev->slot, s->entry,
					src_dev->sector_size);
			}
			return false;

		case LIB_ACT_DELETE:
			if (is_gci) {
				if (s->gci_idx >= 0) {
		
					return cm_backup_delete(&st->gci_files[s->gci_idx]);
				}
			} else if (s->source == LIB_SRC_VMC && src_dev) {
				if (card_manager_confirm_delete(s->entry->filename)) {
		
					lib_set_activity(st, s->device_idx, panels, CM_ACTIVITY_WRITE);
					bool ok = vmc_delete_save(src_dev->vmc_path, s->entry);
					lib_set_activity(st, s->device_idx, panels, CM_ACTIVITY_IDLE);
					if (ok) {
						lib_flag_reload(st, s->device_idx, panels);
						return true;
					}
				}
			} else if (src_dev) {
				if (card_manager_confirm_delete(s->entry->filename)) {
		
					lib_set_activity(st, s->device_idx, panels, CM_ACTIVITY_WRITE);
					bool ok = card_manager_delete_save(src_dev->slot, s->entry);
					lib_set_activity(st, s->device_idx, panels, CM_ACTIVITY_IDLE);
					if (ok) {
						lib_flag_reload(st, s->device_idx, panels);
						return true;
					}
				}
			}
			return false;

		default:
			return false;
	}
}

// --- State management ---

bool lib_state_init(lib_state *st) {
	memset(st, 0, sizeof(*st));

	st->gci_files = calloc(MAX_GCI_FILES, sizeof(gci_file_entry));
	st->saves = calloc(LIB_MAX_SAVES, sizeof(lib_save));
	st->groups = calloc(LIB_MAX_GROUPS, sizeof(lib_game_group));
	if (!st->gci_files || !st->saves || !st->groups) {
		free(st->gci_files); free(st->saves); free(st->groups);
		memset(st, 0, sizeof(*st));
		return false;
	}

	lib_init_devices(st);

	st->num_gci = card_manager_scan_gci_files(st->gci_files, MAX_GCI_FILES);
	lib_build_index(st);

	st->initialized = true;
	lib_update_selection(st);
	return true;
}

void lib_state_rebuild(lib_state *st) {
	for (int d = 0; d < LIB_NUM_DEVICES; d++) {
		if (st->devices[d].needs_reload && st->devices[d].vmc_path[0])
			lib_load_vmc_device(&st->devices[d]);
	}
	lib_rebuild_index(st);
}

void lib_reload_device(lib_state *st, int dev_idx) {
	lib_device *dev = &st->devices[dev_idx];
	if (dev->vmc_path[0])
		lib_load_vmc_device(dev);
	// Physical devices are synced via lib_sync_from_panel — nothing to do here
	lib_rebuild_index(st);
}

uiDrawObj_t *lib_draw_view(lib_state *st, u32 anim_tick) {
	uiDrawObj_t *container = DrawEmptyBox(BOX_X1, BOX_TOP_Y, BOX_X2, BOX_BOTTOM_Y);

	cm_draw_title_bar(container, 1);

	lib_draw_game_list(container, st->groups, st->num_groups,
		st->game_cursor, st->game_scroll, st->focus == 0, anim_tick);

	lib_draw_save_list(container, st, st->focus == 1);

	const char *hints = st->focus == 0
		? "A: View saves  X: Cards  B: Back"
		: "A: Actions  X: Cards  B: Games";
	DrawAddChild(container, DrawStyledLabel(640 / 2, HINTS_Y,
		hints, 0.5f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));

	return container;
}

int lib_handle_input(lib_state *st, cm_panel *panels[2], u16 btns, s8 stickY) {
	if (st->focus == 0) {
		if (((btns & PAD_BUTTON_UP) || stickY > 16) && st->game_cursor > 0) {
			st->game_cursor--;
			if (st->game_cursor < st->game_scroll)
				st->game_scroll = st->game_cursor;
			st->save_cursor = 0; st->save_scroll = 0;
			lib_update_selection(st);
			return LIB_INPUT_REDRAW;
		}
		if (((btns & PAD_BUTTON_DOWN) || stickY < -16)
				&& st->game_cursor < st->num_groups - 1) {
			st->game_cursor++;
			if (st->game_cursor >= st->game_scroll + LIB_MAX_VIS)
				st->game_scroll = st->game_cursor - LIB_MAX_VIS + 1;
			st->save_cursor = 0; st->save_scroll = 0;
			lib_update_selection(st);
			return LIB_INPUT_REDRAW;
		}
		if ((btns & (PAD_BUTTON_A | PAD_BUTTON_RIGHT)) && st->num_groups > 0) {
			st->focus = 1;
			st->save_cursor = 0; st->save_scroll = 0;
			return LIB_INPUT_REDRAW;
		}
		if (btns & PAD_BUTTON_B) {
			return LIB_INPUT_EXIT;
		}
	} else {
		int divider_row = (st->num_card_saves > 0 && st->num_gci_saves > 0)
			? st->num_card_saves : -1;

		if (((btns & PAD_BUTTON_UP) || stickY > 16) && st->save_cursor > 0) {
			st->save_cursor--;
			if (st->save_cursor == divider_row)
				st->save_cursor--;
			if (st->save_cursor < 0) st->save_cursor = 0;
			if (st->save_cursor < st->save_scroll)
				st->save_scroll = st->save_cursor;
			return LIB_INPUT_REDRAW;
		}
		if (((btns & PAD_BUTTON_DOWN) || stickY < -16)
				&& st->save_cursor < st->total_rows - 1) {
			st->save_cursor++;
			if (st->save_cursor == divider_row)
				st->save_cursor++;
			if (st->save_cursor >= st->total_rows)
				st->save_cursor = st->total_rows - 1;
			if (st->save_cursor >= st->save_scroll + LIB_SAVE_MAX_VIS)
				st->save_scroll = st->save_cursor - LIB_SAVE_MAX_VIS + 1;
			return LIB_INPUT_REDRAW;
		}
		if (btns & PAD_BUTTON_LEFT) {
			st->focus = 0;
			return LIB_INPUT_REDRAW;
		}
		if ((btns & PAD_BUTTON_A) && st->total_rows > 0) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }

			lib_save *target = NULL;
			if (st->save_cursor < st->num_card_saves) {
				// Card save row — pick source if on multiple devices
				lib_card_save *cs = &st->card_saves[st->save_cursor];
				int sources[LIB_NUM_DEVICES];
				int nsrc = 0;
				for (int d = 0; d < LIB_NUM_DEVICES; d++) {
					if (cs->save_idx[d] >= 0)
						sources[nsrc++] = d;
				}
				if (nsrc == 1) {
					target = &st->saves[cs->save_idx[sources[0]]];
				} else if (nsrc > 1) {
					const char *pick_items[LIB_NUM_DEVICES];
					bool pick_enabled[LIB_NUM_DEVICES];
					for (int i = 0; i < nsrc; i++) {
						pick_items[i] = st->saves[cs->save_idx[sources[i]]].source_label;
						pick_enabled[i] = true;
					}
					int pick = cm_context_menu(pick_items, pick_enabled, nsrc);
					if (pick >= 0)
						target = &st->saves[cs->save_idx[sources[pick]]];
				}
			} else {
				int gci_row = (divider_row >= 0)
					? st->save_cursor - st->num_card_saves - 1
					: st->save_cursor;
				if (gci_row >= 0 && gci_row < st->num_gci_saves) {
					int si = st->gci_save_indices[gci_row];
					target = &st->saves[si];
				}
			}

			if (target && lib_handle_action(st, target, panels)) {
				lib_state_rebuild(st);
			}
			return LIB_INPUT_REDRAW;
		}
		if (btns & PAD_BUTTON_B) {
			st->focus = 0;
			return LIB_INPUT_REDRAW;
		}
	}

	return LIB_INPUT_NONE;
}

void lib_state_free(lib_state *st) {
	for (int d = 0; d < LIB_NUM_DEVICES; d++)
		card_manager_free_graphics(st->devices[d].entries, st->devices[d].num_entries);
	if (st->gci_files) {
		card_manager_free_gci_files(st->gci_files, st->num_gci);
		free(st->gci_files);
	}
	free(st->saves);
	free(st->groups);
	memset(st, 0, sizeof(*st));
}
