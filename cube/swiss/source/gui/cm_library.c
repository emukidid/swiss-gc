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
#define LIB_SAVE_LIST_TOP (LIB_LIST_TOP + LIB_DETAIL_H)

// Visible row counts (derived)
#define LIB_MAX_VIS       ((LIB_LIST_BOT - LIB_LIST_TOP) / LIB_ROW_H)
#define LIB_SAVE_MAX_VIS  ((LIB_LIST_BOT - LIB_SAVE_LIST_TOP) / LIB_SAVE_ROW_H)

// GC epoch for date formatting
#define GC_EPOCH_OFFSET 946684800ULL

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

// --- Build unified save index ---

static int lib_build_index(cm_panel *panels[2],
	gci_file_entry *gci_files, int num_gci,
	lib_save *saves, int max_saves,
	lib_game_group *groups, int max_groups,
	int *out_num_saves, int *out_num_groups) {

	int ns = 0;

	// Add saves from both panels
	for (int p = 0; p < 2; p++) {
		cm_panel *panel = panels[p];
		if (!panel->card_present) continue;

		for (int i = 0; i < panel->num_entries && ns < max_saves; i++) {
			lib_save *s = &saves[ns];
			s->entry = &panel->entries[i];
			s->slot = panel->slot;
			s->sector_size = panel->sector_size;
			s->gci_idx = -1;

			if (panel->source == CM_SRC_PHYSICAL) {
				s->source = panel->slot == CARD_SLOTA ? LIB_SRC_SLOT_A : LIB_SRC_SLOT_B;
				snprintf(s->source_label, sizeof(s->source_label), "Slot %c",
					panel->slot == CARD_SLOTA ? 'A' : 'B');
				s->vmc_path[0] = '\0';
			} else {
				s->source = LIB_SRC_VMC;
				strlcpy(s->source_label, panel->label, sizeof(s->source_label));
				strlcpy(s->vmc_path, panel->vmc_path, PATHNAME_MAX);
			}
			ns++;
		}
	}

	// Add GCI backups from SD
	for (int i = 0; i < num_gci && ns < max_saves; i++) {
		lib_save *s = &saves[ns];
		s->source = LIB_SRC_GCI;
		s->entry = &gci_files[i].entry;
		strlcpy(s->source_label, "SD Backup", sizeof(s->source_label));
		s->slot = -1;
		s->vmc_path[0] = '\0';
		s->gci_idx = i;
		s->sector_size = 0;
		ns++;
	}

	// Build game groups
	int ng = 0;
	for (int i = 0; i < ns && ng < max_groups; i++) {
		card_entry *e = saves[i].entry;
		int found = -1;
		for (int g = 0; g < ng; g++) {
			if (strcmp(groups[g].gamecode, e->gamecode) == 0) {
				found = g;
				break;
			}
		}
		if (found >= 0) {
			groups[found].num_saves++;
		} else {
			lib_game_group *g = &groups[ng];
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
			if (strcasecmp(groups[i].game_name, groups[j].game_name) > 0) {
				lib_game_group tmp = groups[i];
				groups[i] = groups[j];
				groups[j] = tmp;
			}
		}
	}

	*out_num_saves = ns;
	*out_num_groups = ng;
	return ng;
}

// Get filtered save list for a specific game group
static int lib_get_saves_for_game(lib_save *all_saves, int num_saves,
	const char *gamecode, int *indices, int max) {
	int count = 0;
	for (int i = 0; i < num_saves && count < max; i++) {
		if (strcmp(all_saves[i].entry->gamecode, gamecode) == 0)
			indices[count++] = i;
	}
	return count;
}

// Source badge color
static GXColor lib_source_color(lib_source_type src) {
	switch (src) {
		case LIB_SRC_SLOT_A: return (GXColor){200, 200, 200, 255};
		case LIB_SRC_SLOT_B: return (GXColor){200, 200, 200, 255};
		case LIB_SRC_VMC:    return (GXColor){140, 200, 255, 255};
		case LIB_SRC_GCI:    return (GXColor){200, 200, 140, 255};
		default:             return (GXColor){160, 160, 160, 255};
	}
}

// Source short label for the right panel
static const char *lib_source_short(lib_save *s) {
	switch (s->source) {
		case LIB_SRC_SLOT_A: return "A";
		case LIB_SRC_SLOT_B: return "B";
		case LIB_SRC_VMC:    return "VMC";
		case LIB_SRC_GCI:    return "SD";
		default:             return "?";
	}
}

// --- Drawing ---

static void lib_draw_game_list(uiDrawObj_t *container,
	lib_game_group *groups, int num_groups,
	int cursor, int scroll, bool active, u32 anim_tick) {

	int lx = LIB_GAME_FX + LIB_INSET;
	int lw = LIB_GAME_FW - 2 * LIB_INSET;

	// Panel frame
	cm_draw_panel_frame(container, false,
		LIB_GAME_FX, PANEL_TOP_Y, LIB_GAME_FW, PANEL_BOTTOM_Y - PANEL_TOP_Y);

	// Panel header
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

		// Glow behind selected icon
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

		// Game name
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
		DrawAddChild(container, DrawStyledLabel(tx, ry + 10,
			g->game_name, ns, ALIGN_LEFT, name_color));

		// Save count
		char stats[24];
		snprintf(stats, sizeof(stats), "%d save%s",
			g->num_saves, g->num_saves > 1 ? "s" : "");
		DrawAddChild(container, DrawStyledLabel(tx, ry + 23,
			stats, 0.4f, ALIGN_LEFT, (GXColor){120, 120, 120, 255}));
	}

	if (num_groups > LIB_MAX_VIS)
		lib_draw_thin_scrollbar(container, lx + lw - 2, LIB_LIST_TOP,
			LIB_MAX_VIS * LIB_ROW_H, num_groups, LIB_MAX_VIS, scroll);
}

static void lib_draw_save_list(uiDrawObj_t *container,
	lib_save *all_saves, int *indices, int count,
	int cursor, int scroll, bool active,
	lib_game_group *group) {

	int lx = LIB_SAVE_FX + LIB_INSET;
	int lw = LIB_SAVE_FW - 2 * LIB_INSET;

	// Panel frame
	cm_draw_panel_frame(container, false,
		LIB_SAVE_FX, PANEL_TOP_Y, LIB_SAVE_FW, PANEL_BOTTOM_Y - PANEL_TOP_Y);

	if (!group) {
		DrawAddChild(container, DrawStyledLabel(lx + lw / 2, LIB_LIST_TOP + 40,
			"Select a game", 0.55f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}

	// --- Detail header: banner + game info ---
	card_entry *rep = group->rep;
	int hdr_y = LIB_LIST_TOP;
	int text_x = lx + 4;

	// Banner (if available)
	if (rep->banner) {
		DrawAddChild(container, DrawTexObj(&rep->banner_tex,
			lx + lw - LIB_BANNER_W - 4, hdr_y,
			LIB_BANNER_W, LIB_BANNER_H,
			0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
	}

	// Game name (title)
	int name_max_w = lw - LIB_BANNER_W - 16;
	float name_scale = GetTextScaleToFitInWidthWithMax(group->game_name,
		name_max_w, 0.6f);
	DrawAddChild(container, DrawStyledLabel(text_x, hdr_y + 8,
		group->game_name, name_scale, ALIGN_LEFT,
		active ? defaultColor : (GXColor){160, 160, 160, 255}));

	// Game desc (from representative entry)
	if (rep->game_desc[0]) {
		float desc_scale = GetTextScaleToFitInWidthWithMax(rep->game_desc,
			name_max_w, 0.45f);
		DrawAddChild(container, DrawStyledLabel(text_x, hdr_y + 22,
			rep->game_desc, desc_scale, ALIGN_LEFT,
			(GXColor){160, 160, 160, 255}));
	}

	// Gamecode + save count
	char info[48];
	snprintf(info, sizeof(info), "%s  %d save%s",
		group->gamecode, group->num_saves, group->num_saves > 1 ? "s" : "");
	DrawAddChild(container, DrawStyledLabel(text_x, hdr_y + 36,
		info, 0.4f, ALIGN_LEFT, (GXColor){120, 120, 120, 255}));

	// Divider under header
	DrawAddChild(container, cm_draw_rect(lx, LIB_SAVE_LIST_TOP - 4, lw, 1));

	// --- Save list ---
	if (count == 0) {
		DrawAddChild(container, DrawStyledLabel(lx + lw / 2, LIB_SAVE_LIST_TOP + 20,
			"No saves", 0.5f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));
		return;
	}

	int visible = count - scroll;
	if (visible > LIB_SAVE_MAX_VIS) visible = LIB_SAVE_MAX_VIS;
	int cw = (count > LIB_SAVE_MAX_VIS) ? lw - 8 : lw;

	for (int i = 0; i < visible; i++) {
		int vi = scroll + i;
		int si = indices[vi];
		lib_save *s = &all_saves[si];
		card_entry *e = s->entry;
		int ry = LIB_SAVE_LIST_TOP + i * LIB_SAVE_ROW_H;

		if (active && vi == cursor)
			DrawAddChild(container, cm_draw_highlight(lx, ry, cw, LIB_SAVE_ROW_H));

		// Source badge
		GXColor badge_color = lib_source_color(s->source);
		const char *badge = lib_source_short(s);
		DrawAddChild(container, DrawStyledLabel(lx + 4, ry + LIB_SAVE_ROW_H / 2,
			badge, 0.4f, ALIGN_LEFT, badge_color));

		// Filename + date + blocks — single compact row
		int tx = lx + 30;
		int text_w = cw - 38;
		GXColor name_color;
		if (active && vi == cursor)
			name_color = (GXColor){96, 107, 164, 255};
		else
			name_color = defaultColor;

		// Build display: "filename  |  3 blk  2025-01-15  Slot A"
		char datebuf[16];
		lib_format_date(e->time, datebuf, sizeof(datebuf));
		char row_text[128];
		snprintf(row_text, sizeof(row_text), "%-20.20s  %d blk  %s  %s",
			e->filename, e->blocks, datebuf, s->source_label);
		float rs = GetTextScaleToFitInWidthWithMax(row_text, text_w, 0.42f);
		DrawAddChild(container, DrawStyledLabel(tx, ry + LIB_SAVE_ROW_H / 2,
			row_text, rs, ALIGN_LEFT, name_color));
	}

	if (count > LIB_SAVE_MAX_VIS)
		lib_draw_thin_scrollbar(container, lx + lw - 2, LIB_SAVE_LIST_TOP,
			LIB_SAVE_MAX_VIS * LIB_SAVE_ROW_H, count, LIB_SAVE_MAX_VIS, scroll);
}

// --- Context menu for a save in the library ---

enum {
	LIB_ACT_COPY,
	LIB_ACT_EXPORT,
	LIB_ACT_DELETE,
	LIB_ACT_COUNT
};

static bool lib_handle_action(lib_save *s, cm_panel *panels[2],
	gci_file_entry *gci_files, int num_gci) {

	const char *items[LIB_ACT_COUNT];
	bool enabled[LIB_ACT_COUNT];

	bool is_gci = (s->source == LIB_SRC_GCI);

	items[LIB_ACT_COPY] = "Copy to...";
	enabled[LIB_ACT_COPY] = true;

	items[LIB_ACT_EXPORT] = is_gci ? "Rename" : "Export to SD (.gci)";
	enabled[LIB_ACT_EXPORT] = true;

	items[LIB_ACT_DELETE] = is_gci ? "Delete backup" : "Delete";
	enabled[LIB_ACT_DELETE] = true;

	int choice = cm_context_menu(items, enabled, LIB_ACT_COUNT);

	switch (choice) {
		case LIB_ACT_COPY: {
			// Build destination list: physical slots + VMCs, excluding source
			vmc_file_entry *vmcs = calloc(MAX_VMC_FILES, sizeof(vmc_file_entry));
			int num_vmcs = vmcs ? vmc_scan_files(vmcs, MAX_VMC_FILES) : 0;

			const char *dest_items[MAX_VMC_FILES + 3];
			bool dest_enabled[MAX_VMC_FILES + 3];
			int dest_count = 0;

			// Physical slots
			for (int slot = 0; slot < 2; slot++) {
				if ((s->source == LIB_SRC_SLOT_A && slot == 0) ||
					(s->source == LIB_SRC_SLOT_B && slot == 1))
					continue;
				dest_items[dest_count] = slot == 0 ? "Physical Slot A" : "Physical Slot B";
				dest_enabled[dest_count] = true;
				dest_count++;
			}
			// VMC files
			for (int i = 0; i < num_vmcs; i++) {
				if (s->source == LIB_SRC_VMC && strcmp(s->vmc_path, vmcs[i].path) == 0)
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

			if (s->source == LIB_SRC_SLOT_A || s->source == LIB_SRC_SLOT_B)
				read_ok = cm_read_physical_save(s->slot, s->entry, s->sector_size,
					&gci, &savedata, &save_len);
			else if (s->source == LIB_SRC_VMC)
				read_ok = vmc_read_save(s->vmc_path, s->entry, &gci, &savedata, &save_len);
			else if (s->source == LIB_SRC_GCI && s->gci_idx >= 0) {
				// For GCI backups, read from file
				// Re-use import path: the GCI is already on SD
				// Read the file directly
				DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG]
					? devices[DEVICE_CONFIG] : devices[DEVICE_CUR];
				if (dev) {
					file_handle *f = calloc(1, sizeof(file_handle));
					if (f) {
						strlcpy(f->name, gci_files[s->gci_idx].path, PATHNAME_MAX);
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

			if (!read_ok) { free(vmcs); return false; }

			// Determine destination and write
			bool write_ok = false;
			int phys_offset = 0;
			for (int slot = 0; slot < 2; slot++) {
				if ((s->source == LIB_SRC_SLOT_A && slot == 0) ||
					(s->source == LIB_SRC_SLOT_B && slot == 1))
					continue;
				if (dest_choice == phys_offset) {
					s32 dest_sector = 8192;
					CARD_ProbeEx(slot, NULL, &dest_sector);
					s32 init_ret = initialize_card(slot);
					if (init_ret == CARD_ERROR_READY)
						write_ok = card_manager_import_gci_buf(slot, &gci, savedata,
							save_len, dest_sector);
					if (write_ok) {
						for (int p = 0; p < 2; p++) {
							if (panels[p]->source == CM_SRC_PHYSICAL && panels[p]->slot == slot)
								cm_panel_add_from_gci(panels[p], &gci, savedata, save_len);
						}
					}
					free(savedata);
					free(vmcs);
					return write_ok;
				}
				phys_offset++;
			}

			// Check VMC destinations
			int vmc_offset = phys_offset;
			for (int i = 0; i < num_vmcs; i++) {
				if (s->source == LIB_SRC_VMC && strcmp(s->vmc_path, vmcs[i].path) == 0)
					continue;
				if (dest_choice == vmc_offset) {
					write_ok = vmc_import_save_buf(vmcs[i].path, &gci, savedata, save_len);
					if (write_ok) {
						for (int p = 0; p < 2; p++) {
							if (panels[p]->source == CM_SRC_VMC
								&& strcmp(panels[p]->vmc_path, vmcs[i].path) == 0)
								cm_panel_add_from_gci(panels[p], &gci, savedata, save_len);
						}
					}
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
					return cm_backup_rename(&gci_files[s->gci_idx]);
			} else if (s->source == LIB_SRC_VMC) {
				return vmc_export_save(s->vmc_path, s->entry);
			} else {
				return card_manager_export_save(s->slot, s->entry, s->sector_size);
			}
			return false;

		case LIB_ACT_DELETE:
			if (is_gci) {
				if (s->gci_idx >= 0)
					return cm_backup_delete(&gci_files[s->gci_idx]);
			} else if (s->source == LIB_SRC_VMC) {
				if (card_manager_confirm_delete(s->entry->filename)
					&& vmc_delete_save(s->vmc_path, s->entry)) {
					for (int p = 0; p < 2; p++) {
						if (panels[p]->source == CM_SRC_VMC
							&& strcmp(panels[p]->vmc_path, s->vmc_path) == 0) {
							int idx = (int)(s->entry - panels[p]->entries);
							if (idx >= 0 && idx < panels[p]->num_entries)
								cm_panel_remove_entry(panels[p], idx);
						}
					}
					return true;
				}
			} else {
				if (card_manager_confirm_delete(s->entry->filename)
					&& card_manager_delete_save(s->slot, s->entry)) {
					for (int p = 0; p < 2; p++) {
						if (panels[p]->source == CM_SRC_PHYSICAL
							&& panels[p]->slot == s->slot) {
							int idx = (int)(s->entry - panels[p]->entries);
							if (idx >= 0 && idx < panels[p]->num_entries)
								cm_panel_remove_entry(panels[p], idx);
						}
					}
					return true;
				}
			}
			return false;

		default:
			return false;
	}
}

// --- State management (called from main loop in cardmanager.c) ---

static void lib_update_selection(lib_state *st) {
	st->save_count = 0;
	st->sel_group = NULL;
	if (st->num_groups > 0 && st->game_cursor < st->num_groups) {
		st->sel_group = &st->groups[st->game_cursor];
		st->save_count = lib_get_saves_for_game(st->saves, st->num_saves,
			st->sel_group->gamecode, st->save_indices, LIB_MAX_SAVES);
	}
	if (st->save_cursor >= st->save_count)
		st->save_cursor = st->save_count > 0 ? st->save_count - 1 : 0;
}

bool lib_state_init(lib_state *st, cm_panel *panels[2]) {
	memset(st, 0, sizeof(*st));

	st->gci_files = calloc(MAX_GCI_FILES, sizeof(gci_file_entry));
	st->saves = calloc(LIB_MAX_SAVES, sizeof(lib_save));
	st->groups = calloc(LIB_MAX_GROUPS, sizeof(lib_game_group));
	if (!st->gci_files || !st->saves || !st->groups) {
		free(st->gci_files); free(st->saves); free(st->groups);
		memset(st, 0, sizeof(*st));
		return false;
	}

	st->num_gci = card_manager_scan_gci_files(st->gci_files, MAX_GCI_FILES);
	lib_build_index(panels, st->gci_files, st->num_gci,
		st->saves, LIB_MAX_SAVES, st->groups, LIB_MAX_GROUPS,
		&st->num_saves, &st->num_groups);

	st->initialized = true;
	lib_update_selection(st);
	cm_log("Library init: %d games, %d saves", st->num_groups, st->num_saves);
	return true;
}

void lib_state_rebuild(lib_state *st, cm_panel *panels[2]) {
	card_manager_free_gci_files(st->gci_files, st->num_gci);
	st->num_gci = card_manager_scan_gci_files(st->gci_files, MAX_GCI_FILES);
	lib_build_index(panels, st->gci_files, st->num_gci,
		st->saves, LIB_MAX_SAVES, st->groups, LIB_MAX_GROUPS,
		&st->num_saves, &st->num_groups);

	if (st->game_cursor >= st->num_groups)
		st->game_cursor = st->num_groups > 0 ? st->num_groups - 1 : 0;
	st->save_cursor = 0;
	st->save_scroll = 0;
	st->needs_rebuild = false;
	lib_update_selection(st);
}

uiDrawObj_t *lib_draw_view(lib_state *st, u32 anim_tick) {
	uiDrawObj_t *container = DrawEmptyBox(BOX_X1, BOX_TOP_Y, BOX_X2, BOX_BOTTOM_Y);

	cm_draw_title_bar(container, 1);

	lib_draw_game_list(container, st->groups, st->num_groups,
		st->game_cursor, st->game_scroll, st->focus == 0, anim_tick);

	lib_draw_save_list(container, st->saves, st->save_indices, st->save_count,
		st->save_cursor, st->save_scroll, st->focus == 1, st->sel_group);

	const char *hints = st->focus == 0
		? "A: View saves  X: Cards  B: Back"
		: "A: Actions  X: Cards  B: Games";
	DrawAddChild(container, DrawStyledLabel(640 / 2, HINTS_Y,
		hints, 0.5f, ALIGN_CENTER, (GXColor){140, 140, 140, 255}));

	return container;
}

int lib_handle_input(lib_state *st, cm_panel *panels[2], u16 btns, s8 stickY) {
	if (st->focus == 0) {
		// Game list navigation
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
		// Save list navigation
		if (((btns & PAD_BUTTON_UP) || stickY > 16) && st->save_cursor > 0) {
			st->save_cursor--;
			if (st->save_cursor < st->save_scroll)
				st->save_scroll = st->save_cursor;
			return LIB_INPUT_REDRAW;
		}
		if (((btns & PAD_BUTTON_DOWN) || stickY < -16)
				&& st->save_cursor < st->save_count - 1) {
			st->save_cursor++;
			if (st->save_cursor >= st->save_scroll + LIB_SAVE_MAX_VIS)
				st->save_scroll = st->save_cursor - LIB_SAVE_MAX_VIS + 1;
			return LIB_INPUT_REDRAW;
		}
		if (btns & PAD_BUTTON_LEFT) {
			st->focus = 0;
			return LIB_INPUT_REDRAW;
		}
		if ((btns & PAD_BUTTON_A) && st->save_count > 0) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			int si = st->save_indices[st->save_cursor];
			if (lib_handle_action(&st->saves[si], panels,
					st->gci_files, st->num_gci)) {
				st->needs_rebuild = true;
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
	if (st->gci_files) {
		card_manager_free_gci_files(st->gci_files, st->num_gci);
		free(st->gci_files);
	}
	free(st->saves);
	free(st->groups);
	memset(st, 0, sizeof(*st));
}
