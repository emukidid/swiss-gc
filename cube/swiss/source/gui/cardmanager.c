/* cardmanager.c
	- Memory Card Manager UI — main loop and orchestration
	by Swiss contributors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include "cm_internal.h"
#include "swiss.h"
#include "IPLFontWrite.h"

// --- Panel helpers ---

static void cm_show_error(const char *msg) {
	uiDrawObj_t *msgBox = cm_draw_message(msg);
	DrawPublish(msgBox);
	cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
	DrawDispose(msgBox);
}

static void cm_panel_load(cm_panel *panel) {
	panel->num_entries = 0;
	panel->cursor = 0;
	panel->scroll_row = 0;
	panel->mem_size = 0;
	panel->sector_size = 0;
	panel->card_present = false;
	panel->has_animated_icons = false;

	if (panel->source == CM_SRC_PHYSICAL) {
		char slot_ch = panel->slot == CARD_SLOTA ? 'A' : 'B';
		snprintf(panel->label, sizeof(panel->label), "Slot %c", slot_ch);
		s32 ret = initialize_card(panel->slot);

		// Handle mount errors per Nintendo Memory Card Guidelines §3.6-3.8
		if (ret == CARD_ERROR_WRONGDEVICE) {
			char msg[128];
			sprintf(msg, "The device in Slot %c is not a\nMemory Card.", slot_ch);
			cm_show_error(msg);
		}
		else if (ret == CARD_ERROR_ENCODING) {
			char msg[128];
			sprintf(msg, "Memory Card in Slot %c uses an\nincompatible format.\n \nA: Format  |  B: Cancel", slot_ch);
			uiDrawObj_t *msgBox = cm_draw_message(msg);
			DrawPublish(msgBox);
			u16 btn = cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
			DrawDispose(msgBox);
			if (btn & PAD_BUTTON_A) {
				msgBox = cm_draw_message("Formatting Memory Card...\nDo not remove the Memory Card.");
				DrawPublish(msgBox);
				ret = CARD_Format(panel->slot);
				DrawDispose(msgBox);
				if (ret == CARD_ERROR_READY) {
					ret = initialize_card(panel->slot);
				} else {
					char fmsg[128];
					sprintf(fmsg, "Memory Card in Slot %c is damaged\nand cannot be used.", slot_ch);
					cm_show_error(fmsg);
				}
			}
		}
		else if (ret == CARD_ERROR_BROKEN) {
			char msg[128];
			sprintf(msg, "Memory Card in Slot %c has corrupted\ndata. Format the card?\n \nA: Format  |  B: Cancel", slot_ch);
			uiDrawObj_t *msgBox = cm_draw_message(msg);
			DrawPublish(msgBox);
			u16 btn = cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
			DrawDispose(msgBox);
			if (btn & PAD_BUTTON_A) {
				msgBox = cm_draw_message("Formatting Memory Card...\nDo not remove the Memory Card.");
				DrawPublish(msgBox);
				ret = CARD_Format(panel->slot);
				DrawDispose(msgBox);
				if (ret == CARD_ERROR_READY) {
					ret = initialize_card(panel->slot);
				} else {
					char fmsg[128];
					sprintf(fmsg, "Memory Card in Slot %c is damaged\nand cannot be used.", slot_ch);
					cm_show_error(fmsg);
				}
			}
		}
		if (ret == CARD_ERROR_READY) {
			CARD_ProbeEx(panel->slot, &panel->mem_size, &panel->sector_size);
			panel->card_present = true;
			panel->num_entries = card_manager_read_dir(panel->slot, panel->entries, 128);
			panel->load_cursor = 0;
		}
	}
	else if (panel->source == CM_SRC_VMC) {
		panel->num_entries = vmc_read_dir(panel->vmc_path, panel->entries, 128,
			&panel->mem_size, &panel->sector_size,
			&panel->_vmc_sysarea, &panel->_vmc_total_blocks);
		if (panel->num_entries >= 0) {
			panel->card_present = true;
			panel->load_cursor = 0;
		}
	}

	panel->needs_reload = false;
}

static void cm_panel_navigate(cm_panel *panel, u16 btns) {
	if (panel->num_entries == 0) return;
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

	cy = panel->cursor / GRID_COLS;
	if (cy < panel->scroll_row)
		panel->scroll_row = cy;
	if (cy >= panel->scroll_row + CARDS_GRID_ROWS)
		panel->scroll_row = cy - CARDS_GRID_ROWS + 1;
}

// --- Context menu handling ---

static void cm_handle_context_menu(cm_panel *ap, cm_panel *other, bool *needs_redraw) {
	card_entry *sel = &ap->entries[ap->cursor];

	// Build context menu items
	const char *items[CM_ACT_COUNT];
	bool enabled[CM_ACT_COUNT];

	items[CM_ACT_COPY] = "Copy...";
	enabled[CM_ACT_COPY] = true;

	items[CM_ACT_EXPORT] = "Export to SD (.gci)";
	enabled[CM_ACT_EXPORT] = true;

	items[CM_ACT_IMPORT] = "Import from SD (.gci)";
	enabled[CM_ACT_IMPORT] = ap->card_present;

	items[CM_ACT_DELETE] = "Delete";
	enabled[CM_ACT_DELETE] = true;

	items[CM_ACT_ASSIGN] = "Assign to game...";
	enabled[CM_ACT_ASSIGN] = (ap->source == CM_SRC_VMC);

	int choice = cm_context_menu(items, enabled, CM_ACT_COUNT);

	switch (choice) {
		case CM_ACT_COPY: {
			// Destination picker: list all available targets except source
			vmc_file_entry *vmcs = calloc(MAX_VMC_FILES, sizeof(vmc_file_entry));
			int num_vmcs = vmcs ? vmc_scan_files(vmcs, MAX_VMC_FILES) : 0;

			const char *dest_items[MAX_VMC_FILES + 2];
			bool dest_enabled[MAX_VMC_FILES + 2];
			int dest_count = 0;

			// Physical slots (skip if source is the same physical slot)
			for (int s = 0; s < 2; s++) {
				if (ap->source == CM_SRC_PHYSICAL && ap->slot == s)
					continue;
				char *label = (s == 0) ? "Physical Slot A" : "Physical Slot B";
				dest_items[dest_count] = label;
				dest_enabled[dest_count] = true;
				dest_count++;
			}
			// VMC files (skip if source is the same VMC)
			for (int i = 0; i < num_vmcs; i++) {
				if (ap->source == CM_SRC_VMC
					&& strcmp(ap->vmc_path, vmcs[i].path) == 0)
					continue;
				dest_items[dest_count] = vmcs[i].label;
				dest_enabled[dest_count] = true;
				dest_count++;
			}

			if (dest_count == 0) {
				uiDrawObj_t *msg = cm_draw_message("No copy destinations available.");
				DrawPublish(msg);
				cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
				DrawDispose(msg);
				free(vmcs);
				*needs_redraw = true;
				break;
			}

			int dest_choice = cm_context_menu(dest_items, dest_enabled, dest_count);
			if (dest_choice < 0) {
				free(vmcs);
				*needs_redraw = true;
				break;
			}

			// Determine destination type from the chosen item
			cm_source_type dest_src;
			int dest_slot = 0;
			const char *dest_vmc_path = NULL;

			// Map choice back to source: first come physical slots, then VMCs
			int phys_offset = 0;
			for (int s = 0; s < 2; s++) {
				if (ap->source == CM_SRC_PHYSICAL && ap->slot == s)
					continue;
				if (dest_choice == phys_offset) {
					dest_src = CM_SRC_PHYSICAL;
					dest_slot = s;
					goto dest_found;
				}
				phys_offset++;
			}
			{
				int vmc_idx = dest_choice - phys_offset;
				int vi = 0;
				for (int i = 0; i < num_vmcs; i++) {
					if (ap->source == CM_SRC_VMC
						&& strcmp(ap->vmc_path, vmcs[i].path) == 0)
						continue;
					if (vi == vmc_idx) {
						dest_src = CM_SRC_VMC;
						dest_vmc_path = vmcs[i].path;
						goto dest_found;
					}
					vi++;
				}
			}
			free(vmcs);
			*needs_redraw = true;
			break;

		dest_found: ;
			// Read source save as in-memory GCI
			GCI gci;
			u8 *savedata = NULL;
			u32 save_len = 0;
			bool read_ok;

			ap->activity = CM_ACTIVITY_READ;
			if (ap->source == CM_SRC_VMC)
				read_ok = vmc_read_save(ap->vmc_path, sel, &gci, &savedata, &save_len);
			else
				read_ok = cm_read_physical_save(ap->slot, sel, ap->sector_size,
					&gci, &savedata, &save_len);
			ap->activity = CM_ACTIVITY_IDLE;

			if (!read_ok) {
				free(vmcs);
				*needs_redraw = true;
				break;
			}

			// Write to destination — find dest panel for LED
			cm_panel *dest_panel = NULL;
			cm_panel *both_wr[] = { ap, other };
			for (int pi = 0; pi < 2; pi++) {
				if (dest_src == CM_SRC_PHYSICAL
					&& both_wr[pi]->source == CM_SRC_PHYSICAL
					&& both_wr[pi]->slot == dest_slot)
					dest_panel = both_wr[pi];
				else if (dest_src == CM_SRC_VMC && dest_vmc_path
					&& both_wr[pi]->source == CM_SRC_VMC
					&& strcmp(both_wr[pi]->vmc_path, dest_vmc_path) == 0)
					dest_panel = both_wr[pi];
			}
			if (dest_panel) dest_panel->activity = CM_ACTIVITY_WRITE;
			bool write_ok;
			if (dest_src == CM_SRC_VMC)
				write_ok = vmc_import_save_buf(dest_vmc_path, &gci, savedata, save_len);
			else {
				s32 dest_sector = 8192;
				CARD_ProbeEx(dest_slot, NULL, &dest_sector);
				s32 init_ret = initialize_card(dest_slot);
				if (init_ret != CARD_ERROR_READY) {
					uiDrawObj_t *msg = cm_draw_message("Destination card not ready.");
					DrawPublish(msg);
					cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
					DrawDispose(msg);
					write_ok = false;
				} else {
					write_ok = card_manager_import_gci_buf(dest_slot, &gci, savedata,
						save_len, dest_sector);
				}
			}
			if (dest_panel) dest_panel->activity = CM_ACTIVITY_IDLE;

			if (write_ok) {
				// Add entry to any visible panel matching the destination
				cm_panel *both[] = { ap, other };
				for (int pi = 0; pi < 2; pi++) {
					cm_panel *pp = both[pi];
					if (pp->source != dest_src) continue;
					if (dest_src == CM_SRC_PHYSICAL && pp->slot == dest_slot)
						cm_panel_add_from_gci(pp, &gci, savedata, save_len);
					else if (dest_src == CM_SRC_VMC && dest_vmc_path
						&& strcmp(pp->vmc_path, dest_vmc_path) == 0)
						cm_panel_add_from_gci(pp, &gci, savedata, save_len);
				}
			}
			free(savedata);
			free(vmcs);
			*needs_redraw = true;
			break;
		}

		case CM_ACT_EXPORT:
			ap->activity = CM_ACTIVITY_READ;
			if (ap->source == CM_SRC_VMC) {
				vmc_export_save(ap->vmc_path, sel);
			} else {
				card_manager_export_save(ap->slot, sel, ap->sector_size);
			}
			ap->activity = CM_ACTIVITY_IDLE;
			*needs_redraw = true;
			break;

		case CM_ACT_IMPORT:

			ap->activity = CM_ACTIVITY_WRITE;
			if (card_manager_backups(ap)) {
				ap->activity = CM_ACTIVITY_IDLE;
				ap->needs_reload = true;
			} else {
				ap->activity = CM_ACTIVITY_IDLE;
				*needs_redraw = true;
			}
			break;

		case CM_ACT_DELETE:
			if (card_manager_confirm_delete(sel->filename)) {
				ap->activity = CM_ACTIVITY_WRITE;
				bool deleted;
				if (ap->source == CM_SRC_VMC)
					deleted = vmc_delete_save(ap->vmc_path, sel);
				else
					deleted = card_manager_delete_save(ap->slot, sel);
				ap->activity = CM_ACTIVITY_IDLE;
				if (deleted) {
					cm_panel_remove_entry(ap, ap->cursor);
					*needs_redraw = true;
				}
			} else {
				*needs_redraw = true;
			}
			break;

		case CM_ACT_ASSIGN: {
			cm_game_entry *games = calloc(MAX_GAME_ENTRIES, sizeof(cm_game_entry));
			if (!games) { *needs_redraw = true; break; }
			int num_games = cm_scan_game_configs(games, MAX_GAME_ENTRIES);
			if (num_games == 0) {
				cm_show_error("No games found.\nLaunch a game first.");
				free(games);
				*needs_redraw = true;
				break;
			}
			int pick = cm_game_picker(games, num_games);
			if (pick >= 0) {
				// Determine slot from VMC filename: "MemoryCardB" → slot B, else slot A
				char *vmc_name = getRelativeName(ap->vmc_path);
				int assign_slot = (vmc_name && !strncasecmp(vmc_name, "MemoryCardB", 11)) ? 1 : 0;

				if (cm_assign_vmc_to_game(vmc_name, games[pick].game_id, assign_slot)) {
					char msg[128];
					snprintf(msg, sizeof(msg), "Assigned to %.40s\n(Slot %c)",
						games[pick].game_name, assign_slot ? 'B' : 'A');
					uiDrawObj_t *ok = cm_draw_message(msg);
					DrawPublish(ok);
					cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
					DrawDispose(ok);
				} else {
					cm_show_error("Failed to save assignment.");
				}
			}
			free(games);
			*needs_redraw = true;
			break;
		}

		default:
			*needs_redraw = true;
			break;
	}
}

// --- Main loop ---

// --- VMC picker ---
// Shows a scrollable list of .raw VMC files and lets the user pick one to mount.
// Returns true if panel was changed, false if cancelled.
static bool cm_pick_vmc(cm_panel *panel) {
	vmc_file_entry *vmcs = calloc(MAX_VMC_FILES, sizeof(vmc_file_entry));
	if (!vmcs) return false;

	int num = vmc_scan_files(vmcs, MAX_VMC_FILES);
	if (num == 0) {
		uiDrawObj_t *msg = cm_draw_message("No virtual card files found\nin swiss/saves/");
		DrawPublish(msg);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msg);
		free(vmcs);
		return false;
	}

	// Items: Physical slot + N VMC files + Create New
	int total_items = 1 + num + 1;
	char phys_label[48];
	snprintf(phys_label, sizeof(phys_label), "Physical Slot %c",
		panel->slot == CARD_SLOTA ? 'A' : 'B');

	// Build display labels with block info
	char item_labels[MAX_VMC_FILES][48];
	for (int i = 0; i < num; i++) {
		if (vmcs[i].free_blocks > 0 && vmcs[i].user_blocks > 0) {
			snprintf(item_labels[i], sizeof(item_labels[i]),
				"%s  %lu/%lu",
				vmcs[i].label,
				(unsigned long)(vmcs[i].user_blocks - vmcs[i].free_blocks),
				(unsigned long)(vmcs[i].user_blocks));
		} else {
			strlcpy(item_labels[i], vmcs[i].label, sizeof(item_labels[i]));
		}
	}

	int item_h = 22;
	int pad = 10;
	int menu_w = 320;
	int menu_h = (total_items < 14 ? total_items : 14) * item_h + pad * 2;
	if (menu_h > 400) menu_h = 400;
	int menu_x = (640 - menu_w) / 2;
	int menu_y = (480 - menu_h) / 2;
	int max_visible = (menu_h - pad * 2) / item_h;

	int cursor = 0, scroll = 0;
	// Pre-select current VMC if mounted
	if (panel->source == CM_SRC_VMC && panel->vmc_path[0]) {
		for (int i = 0; i < num; i++) {
			if (!strcmp(panel->vmc_path, vmcs[i].path)) {
				cursor = 1 + i;
				break;
			}
		}
	}
	if (cursor >= max_visible) scroll = cursor - max_visible + 1;

	uiDrawObj_t *overlay = NULL;
	bool needs_redraw = true;
	bool debounce = true;
	int choice = -1;

	while (1) {
		cm_retire_tick();

		if (needs_redraw) {
			uiDrawObj_t *fresh = DrawEmptyBox(menu_x, menu_y,
				menu_x + menu_w, menu_y + menu_h);

			for (int vi = 0; vi < max_visible && scroll + vi < total_items; vi++) {
				int idx = scroll + vi;
				int iy = menu_y + pad + vi * item_h + item_h / 2;
				GXColor color;
				const char *label;
				if (idx == 0) label = phys_label;
				else if (idx <= num) label = item_labels[idx - 1];
				else label = "Create New...";

				if (idx == cursor) {
					color = (GXColor){96, 107, 164, 255};
					DrawAddChild(fresh, cm_draw_highlight(
						menu_x + 6, menu_y + pad + vi * item_h,
						menu_w - 12, item_h));
				} else {
					color = defaultColor;
				}

				DrawAddChild(fresh, DrawStyledLabel(menu_x + 16, iy,
					label, 0.5f, ALIGN_LEFT, color));
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
			if (!(btns & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_A | PAD_BUTTON_B)))
				debounce = false;
			continue;
		}

		if ((btns & PAD_BUTTON_UP) || stickY > 16) {
			if (cursor > 0) {
				cursor--;
				if (cursor < scroll) scroll = cursor;
				needs_redraw = true;
			}
			debounce = true;
		}
		if ((btns & PAD_BUTTON_DOWN) || stickY < -16) {
			if (cursor < total_items - 1) {
				cursor++;
				if (cursor >= scroll + max_visible) scroll = cursor - max_visible + 1;
				needs_redraw = true;
			}
			debounce = true;
		}
		if (btns & PAD_BUTTON_A) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			choice = cursor;
			break;
		}
		if (btns & PAD_BUTTON_B) {
			while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
			choice = -1;
			break;
		}
	}

	DrawDispose(overlay);

	bool changed = false;
	if (choice == 0) {
		// Switch back to physical card
		if (panel->source != CM_SRC_PHYSICAL) {
			panel->source = CM_SRC_PHYSICAL;
			panel->vmc_path[0] = '\0';
			panel->needs_reload = true;
			changed = true;
		}
	}
	else if (choice >= 1 && choice <= num) {
		int vi = choice - 1;
		panel->source = CM_SRC_VMC;
		strlcpy(panel->vmc_path, vmcs[vi].path, PATHNAME_MAX);
		strlcpy(panel->label, vmcs[vi].label, sizeof(panel->label));
		panel->needs_reload = true;
		changed = true;
	}
	else if (choice == 1 + num) {
		// Create New...
		char new_filename[64];
		// Use region 1 (USA) as default — no game context in panel view
		if (cm_vmc_create(1, new_filename, sizeof(new_filename))) {
			// Mount the newly created card
			DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
				devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
			if (dev) {
				panel->source = CM_SRC_VMC;
				concatf_path(panel->vmc_path, dev->initial->name,
					"swiss/saves/%s", new_filename);
				// Build label from filename (strip .raw extension)
				strlcpy(panel->label, new_filename, sizeof(panel->label));
				char *dot = strrchr(panel->label, '.');
				if (dot) *dot = '\0';
				panel->needs_reload = true;
				changed = true;
			}
		}
	}

	free(vmcs);
	return changed;
}

void show_card_manager(void) {
	cm_panel *panels[2];
	panels[0] = calloc(1, sizeof(cm_panel));
	panels[1] = calloc(1, sizeof(cm_panel));
	if (!panels[0] || !panels[1]) {
		free(panels[0]);
		free(panels[1]);
		return;
	}
	panels[0]->source = CM_SRC_PHYSICAL;
	panels[0]->slot = CARD_SLOTA;
	panels[0]->needs_reload = true;
	panels[1]->source = CM_SRC_PHYSICAL;
	panels[1]->slot = CARD_SLOTB;
	panels[1]->needs_reload = true;

	int view_mode = 0;	// 0=cards, 1=library
	int active = 0;
	bool needs_redraw = false;
	int poll_counter = 0;
	u32 anim_tick = 0;
	uiDrawObj_t *page = NULL;

	lib_state *lib = calloc(1, sizeof(lib_state));

	cm_draw_init();
	cm_led_begin(panels);
	cm_led_update_card_status(
		CARD_ProbeEx(CARD_SLOTA, NULL, NULL) == CARD_ERROR_READY,
		CARD_ProbeEx(CARD_SLOTB, NULL, NULL) == CARD_ERROR_READY);
	// Wait for A button release
	while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }

	while (1) {
		cm_retire_tick();

		// Initiate reload for any panel that needs it
		for (int p = 0; p < 2; p++) {
			if (!panels[p]->needs_reload) continue;
			card_manager_free_graphics(panels[p]->entries, panels[p]->num_entries);
			panels[p]->loading = true;
			panels[p]->activity = CM_ACTIVITY_READ;
			cm_panel_load(panels[p]);
			// If no entries to detail-load, complete immediately
			if (!panels[p]->card_present || panels[p]->num_entries == 0) {
				panels[p]->loading = false;
				panels[p]->activity = CM_ACTIVITY_IDLE;
				if (panels[p]->_vmc_sysarea) {
					free(panels[p]->_vmc_sysarea);
					panels[p]->_vmc_sysarea = NULL;
				}
				if (lib->initialized) lib->needs_rebuild = true;
			}
			needs_redraw = true;
		}

		// Incremental detail loading — one save per frame per panel
		for (int p = 0; p < 2; p++) {
			if (!panels[p]->loading) continue;
			if (panels[p]->load_cursor < panels[p]->num_entries) {
				if (panels[p]->source == CM_SRC_VMC) {
					vmc_read_save_detail(panels[p]->vmc_path,
						&panels[p]->entries[panels[p]->load_cursor],
						panels[p]->_vmc_sysarea, panels[p]->_vmc_total_blocks);
					panels[p]->load_cursor++;
				} else {
					if (CARD_ProbeEx(panels[p]->slot, NULL, NULL) != CARD_ERROR_READY) {
						panels[p]->load_cursor = panels[p]->num_entries;
					} else {
						card_manager_read_save_detail(panels[p]->slot,
							&panels[p]->entries[panels[p]->load_cursor]);
						panels[p]->load_cursor++;
					}
				}
			}
			if (panels[p]->load_cursor >= panels[p]->num_entries) {
				panels[p]->loading = false;
				panels[p]->activity = CM_ACTIVITY_IDLE;
				if (panels[p]->_vmc_sysarea) {
					free(panels[p]->_vmc_sysarea);
					panels[p]->_vmc_sysarea = NULL;
				}
				for (int i = 0; i < panels[p]->num_entries; i++) {
					if (panels[p]->entries[i].icon
						&& panels[p]->entries[i].icon->num_frames > 1) {
						panels[p]->has_animated_icons = true;
						break;
					}
				}
				if (lib->initialized) lib->needs_rebuild = true;
			}
			needs_redraw = true;
		}

		// Reload any library devices that need it
		if (lib->initialized && lib->needs_rebuild) {
			lib_state_rebuild(lib);
			lib->needs_rebuild = false;
			needs_redraw = true;
		}

		// Draw current view
		bool has_anim = (view_mode == 0 &&
			(panels[0]->has_animated_icons || panels[1]->has_animated_icons
			|| panels[0]->loading || panels[1]->loading))
			|| view_mode == 1;
		if (needs_redraw || has_anim) {
			uiDrawObj_t *newPage;
			if (view_mode == 0)
				newPage = card_manager_draw(panels[0], panels[1], active, anim_tick);
			else
				newPage = lib_draw_view(lib, anim_tick);

			cm_draw_status_leds(newPage, panels);
			if (page) DrawDispose(page);
			page = newPage;
			DrawPublish(page);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();
		anim_tick++;

		// Poll both physical slots for hotswap
		poll_counter++;
		if (poll_counter >= CARD_POLL_INTERVAL) {
			poll_counter = 0;
			bool slot_present[2];
			for (int slot = 0; slot < 2; slot++) {
				s32 probe = CARD_ProbeEx(slot, NULL, NULL);
				slot_present[slot] = (probe == CARD_ERROR_READY || probe == CARD_ERROR_BUSY);

				for (int p = 0; p < 2; p++) {
					if (panels[p]->source == CM_SRC_PHYSICAL
						&& panels[p]->slot == slot
						&& panels[p]->card_present != slot_present[slot]) {
						panels[p]->needs_reload = true;
					}
				}
			}
			cm_led_update_card_status(slot_present[0], slot_present[1]);

			// Library polls independently — check if physical slots changed
			if (lib->initialized) {
				for (int d = 0; d < LIB_DEV_VMC_A; d++) {
					bool present = slot_present[lib->devices[d].slot];
					if (present != lib->devices[d].present)
						lib->devices[d].needs_reload = true;
				}
				for (int d = 0; d < LIB_NUM_DEVICES; d++) {
					if (lib->devices[d].needs_reload) {
						lib->needs_rebuild = true;
						break;
					}
				}
			}

			if (panels[0]->needs_reload || panels[1]->needs_reload)
				continue;
		}

		u16 btns = padsButtonsHeld();
		s8 stickX = padsStickX();
		s8 stickY = padsStickY();
		bool has_input = btns || stickY > 16 || stickY < -16 ||
			stickX > 16 || stickX < -16;
		if (!has_input) continue;

		// --- View toggle (X) — blocked during incremental loading (CARD I/O conflict) ---
		if ((btns & PAD_BUTTON_X)
			&& !panels[0]->loading && !panels[1]->loading) {
			while (padsButtonsHeld() & PAD_BUTTON_X) { VIDEO_WaitVSync(); }
			if (view_mode == 0) {
				if (!lib->initialized || lib->needs_rebuild) {
					uiDrawObj_t *loadMsg = cm_draw_message("Loading library...");
					DrawPublish(loadMsg);
					if (!lib->initialized) {
						lib_state_init(lib);
					} else {
						lib_state_rebuild(lib);
					}
					lib->needs_rebuild = false;
					DrawDispose(loadMsg);
				}
				view_mode = 1;
			} else {
				view_mode = 0;
			}
			needs_redraw = true;
			continue;
		}

		// --- Cards view input ---
		if (view_mode == 0) {
			cm_panel *ap = panels[active];
			cm_panel *other = panels[active ^ 1];

			// Grid navigation (d-pad + analog stick)
			u16 nav = btns & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT);
			if (stickY > 16)  nav |= PAD_BUTTON_UP;
			if (stickY < -16) nav |= PAD_BUTTON_DOWN;
			if (stickX < -16) nav |= PAD_BUTTON_LEFT;
			if (stickX > 16)  nav |= PAD_BUTTON_RIGHT;
			if (nav) {
				cm_panel_navigate(ap, nav);
				if (ap->cursor != ap->prev_cursor) {
					ap->anim_start = anim_tick;
					ap->prev_cursor = ap->cursor;
				}
				needs_redraw = true;
			}

			// Switch active panel (L/R)
			if (btns & (PAD_TRIGGER_L | PAD_TRIGGER_R)) {
				while (padsButtonsHeld() & (PAD_TRIGGER_L | PAD_TRIGGER_R)) { VIDEO_WaitVSync(); }
				active ^= 1;
				panels[active]->anim_start = anim_tick;
				needs_redraw = true;
				continue;
			}

			// Context menu (A) — requires save selected and no loading (CARD I/O)
			if ((btns & PAD_BUTTON_A) && ap->card_present && ap->num_entries > 0
				&& !panels[0]->loading && !panels[1]->loading) {
				while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
				cm_handle_context_menu(ap, other, &needs_redraw);
				needs_redraw = true;
				if (lib->initialized)
					lib->needs_rebuild = true;
				goto debounce;
			}

			// VMC picker (Z) — blocked during loading (CARD I/O conflict)
			if ((btns & PAD_TRIGGER_Z)
				&& !panels[0]->loading && !panels[1]->loading) {
				while (padsButtonsHeld() & PAD_TRIGGER_Z) { VIDEO_WaitVSync(); }
				cm_pick_vmc(ap);
				if (lib->initialized)
					lib->needs_rebuild = true;
				needs_redraw = true;
				goto debounce;
			}

			if (btns & PAD_BUTTON_B)
				break;
		}

		// --- Library view input ---
		if (view_mode == 1) {
			int result = lib_handle_input(lib, panels, btns, stickY);
			if (result == LIB_INPUT_EXIT) {
				view_mode = 0;
				needs_redraw = true;
			} else if (result == LIB_INPUT_REDRAW) {
				needs_redraw = true;
			}
		}

debounce:
		// Analog stick: speed-based repeat; d-pad/buttons: wait for release
		if ((stickY > 16 || stickY < -16 || stickX > 16 || stickX < -16)
			&& !(btns & ~(PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT))) {
			int mag = abs(stickY) > abs(stickX) ? abs(stickY) : abs(stickX);
			int wait = mag > 64 ? 2 : 5;
			for (int w = 0; w < wait; w++) VIDEO_WaitVSync();
		} else {
			while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN |
				PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT |
				PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_X |
				PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_TRIGGER_Z)) {
				VIDEO_WaitVSync();
			}
		}
	}

	// Cleanup
	if (page) DrawDispose(page);
	if (lib) {
		if (lib->initialized) lib_state_free(lib);
		free(lib);
	}
	for (int p = 0; p < 2; p++) {
		if (panels[p]->_vmc_sysarea) free(panels[p]->_vmc_sysarea);
		card_manager_free_graphics(panels[p]->entries, panels[p]->num_entries);
		free(panels[p]);
	}
	cm_retire_flush();
	cm_led_end();
	while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
}
