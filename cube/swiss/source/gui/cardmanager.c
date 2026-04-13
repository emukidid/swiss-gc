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

		if (ret == CARD_ERROR_WRONGDEVICE) {
			char msg[128];
			sprintf(msg, "The device in Slot %c is not a\nMemory Card.", slot_ch);
			cm_show_error(msg);
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

static void cm_panel_navigate(cm_panel *panel, u16 btns, int cols) {
	if (panel->num_entries == 0) return;
	int cy = panel->cursor / cols;
	int total_rows = (panel->num_entries + cols - 1) / cols;

	if (btns & PAD_BUTTON_UP) {
		if (cy > 0) {
			panel->cursor -= cols;
		}
	}
	if (btns & PAD_BUTTON_DOWN) {
		if (cy < total_rows - 1) {
			int new_cursor = panel->cursor + cols;
			panel->cursor = (new_cursor < panel->num_entries) ? new_cursor : panel->num_entries - 1;
		}
	}
	if (btns & PAD_BUTTON_LEFT) {
		if (panel->cursor > 0) panel->cursor--;
	}
	if (btns & PAD_BUTTON_RIGHT) {
		if (panel->cursor < panel->num_entries - 1) panel->cursor++;
	}

	cy = panel->cursor / cols;
	if (cy < panel->scroll_row)
		panel->scroll_row = cy;
	if (cy >= panel->scroll_row + CARDS_GRID_ROWS)
		panel->scroll_row = cy - CARDS_GRID_ROWS + 1;
}

// --- Shelf ---

static void cm_shelf_populate(cm_shelf *shelf) {
	int saved_cursor = shelf->cursor;
	shelf->count = 0;

	// Physical Slot A
	shelf_item *it = &shelf->items[shelf->count++];
	memset(it, 0, sizeof(*it));
	it->type = SHELF_ITEM_PHYSICAL;
	it->slot = CARD_SLOTA;
	strlcpy(it->label, "Slot A", sizeof(it->label));
	s32 probe = CARD_ProbeEx(CARD_SLOTA, NULL, NULL);
	it->present = (probe == CARD_ERROR_READY || probe == CARD_ERROR_BUSY);

	// Physical Slot B
	it = &shelf->items[shelf->count++];
	memset(it, 0, sizeof(*it));
	it->type = SHELF_ITEM_PHYSICAL;
	it->slot = CARD_SLOTB;
	strlcpy(it->label, "Slot B", sizeof(it->label));
	probe = CARD_ProbeEx(CARD_SLOTB, NULL, NULL);
	it->present = (probe == CARD_ERROR_READY || probe == CARD_ERROR_BUSY);

	// VMC files
	vmc_file_entry vmcs[MAX_VMC_FILES];
	int num = vmc_scan_files(vmcs, MAX_VMC_FILES);
	for (int i = 0; i < num && shelf->count < SHELF_MAX_ITEMS - 1; i++) {
		if (vmcs[i].encoding != 0) continue;
		it = &shelf->items[shelf->count++];
		memset(it, 0, sizeof(*it));
		it->type = SHELF_ITEM_VMC;
		it->slot = -1;
		strlcpy(it->path, vmcs[i].path, PATHNAME_MAX);
		strlcpy(it->label, vmcs[i].label, sizeof(it->label));
		it->present = true;
		it->used_blocks = (vmcs[i].user_blocks > vmcs[i].free_blocks)
			? vmcs[i].user_blocks - vmcs[i].free_blocks : 0;
		it->total_blocks = vmcs[i].user_blocks;
	}

	// Create New...
	it = &shelf->items[shelf->count++];
	memset(it, 0, sizeof(*it));
	it->type = SHELF_ITEM_CREATE;
	strlcpy(it->label, "Create New...", sizeof(it->label));

	// Preserve cursor position
	if (saved_cursor < shelf->count) shelf->cursor = saved_cursor;
	else shelf->cursor = shelf->count > 0 ? shelf->count - 1 : 0;
	if (shelf->cursor < shelf->scroll)
		shelf->scroll = shelf->cursor;
	if (shelf->cursor >= shelf->scroll + SHELF_MAX_VIS)
		shelf->scroll = shelf->cursor - SHELF_MAX_VIS + 1;
}

static bool cm_shelf_apply(cm_shelf *shelf, cm_panel *detail) {
	shelf_item *item = &shelf->items[shelf->cursor];

	if (item->type == SHELF_ITEM_PHYSICAL) {
		if (detail->source == CM_SRC_PHYSICAL && detail->slot == item->slot)
			return false;
		detail->source = CM_SRC_PHYSICAL;
		detail->slot = item->slot;
		detail->vmc_path[0] = '\0';
		snprintf(detail->label, sizeof(detail->label), "Slot %c",
			item->slot == CARD_SLOTA ? 'A' : 'B');
		detail->needs_reload = true;
		return true;
	}
	else if (item->type == SHELF_ITEM_VMC) {
		if (detail->source == CM_SRC_VMC
			&& strcmp(detail->vmc_path, item->path) == 0)
			return false;
		detail->source = CM_SRC_VMC;
		strlcpy(detail->vmc_path, item->path, PATHNAME_MAX);
		strlcpy(detail->label, item->label, sizeof(detail->label));
		detail->needs_reload = true;
		return true;
	}
	return false;
}

// --- Context menu handling ---

static void cm_handle_context_menu(cm_panel *ap, bool *needs_redraw) {
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

			// Write to destination — check if detail panel matches dest for LED
			cm_panel *dest_panel = NULL;
			if (dest_src == CM_SRC_PHYSICAL
				&& ap->source == CM_SRC_PHYSICAL && ap->slot == dest_slot)
				dest_panel = ap;
			else if (dest_src == CM_SRC_VMC && dest_vmc_path
				&& ap->source == CM_SRC_VMC
				&& strcmp(ap->vmc_path, dest_vmc_path) == 0)
				dest_panel = ap;
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
				// Add entry to detail panel if it matches the destination
				if (ap->source == dest_src) {
					if (dest_src == CM_SRC_PHYSICAL && ap->slot == dest_slot)
						cm_panel_add_from_gci(ap, &gci, savedata, save_len);
					else if (dest_src == CM_SRC_VMC && dest_vmc_path
						&& strcmp(ap->vmc_path, dest_vmc_path) == 0)
						cm_panel_add_from_gci(ap, &gci, savedata, save_len);
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
			char *vmc_name = getRelativeName(ap->vmc_path);
			int assign_slot = (vmc_name && !strncasecmp(vmc_name, "MemoryCardB", 11)) ? 1 : 0;

			// Use the selected save's game code directly
			char game_label[48];
			if (sel->game_name[0])
				strlcpy(game_label, sel->game_name, sizeof(game_label));
			else
				strlcpy(game_label, sel->gamecode, sizeof(game_label));

			char msg[196];
			snprintf(msg, sizeof(msg),
				"%.40s will use this card\nas Slot %c.\n \nA: Confirm  |  B: Cancel",
				game_label, assign_slot ? 'B' : 'A');
			uiDrawObj_t *confirm = cm_draw_message(msg);
			DrawPublish(confirm);
			u16 btn = cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
			DrawDispose(confirm);

			if (btn & PAD_BUTTON_A) {
				if (cm_assign_vmc_to_game(vmc_name, sel->gamecode, assign_slot)) {
					uiDrawObj_t *ok = cm_draw_message("Assignment saved.");
					DrawPublish(ok);
					cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
					DrawDispose(ok);
				} else {
					cm_show_error("Failed to save assignment.");
				}
			}
			*needs_redraw = true;
			break;
		}

		default:
			*needs_redraw = true;
			break;
	}
}

// --- Main loop ---

void show_card_manager(void) {
	cm_panel *detail = calloc(1, sizeof(cm_panel));
	cm_shelf *shelf = calloc(1, sizeof(cm_shelf));
	if (!detail || !shelf) {
		free(detail);
		free(shelf);
		return;
	}

	cm_focus focus = CM_FOCUS_SHELF;
	int view_mode = 0;	// 0=cards, 1=library
	bool needs_redraw = false;
	int poll_counter = 0;
	int shelf_settle = 0;	// Debounce: frames until shelf selection applies
	u32 anim_tick = 0;
	uiDrawObj_t *page = NULL;

	lib_state *lib = calloc(1, sizeof(lib_state));

	cm_draw_init();

	// Populate shelf and load initial card
	cm_shelf_populate(shelf);
	detail->source = CM_SRC_PHYSICAL;
	detail->slot = CARD_SLOTA;
	detail->needs_reload = true;

	// Wait for A button release
	while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }

	while (1) {
		cm_retire_tick();
		bool toast_active = cm_toast_tick();

		// Shelf settle timer — debounce card loads while scrolling
		if (shelf_settle > 0) {
			shelf_settle--;
			if (shelf_settle == 0)
				cm_shelf_apply(shelf, detail);
		}

		// Reload detail panel if needed
		if (detail->needs_reload) {
			card_manager_free_graphics(detail->entries, detail->num_entries);
			detail->loading = true;
			detail->activity = CM_ACTIVITY_READ;
			cm_panel_load(detail);
			if (!detail->card_present || detail->num_entries == 0) {
				detail->loading = false;
				detail->activity = CM_ACTIVITY_IDLE;
				if (detail->_vmc_sysarea) {
					free(detail->_vmc_sysarea);
					detail->_vmc_sysarea = NULL;
				}
			}
			needs_redraw = true;
		}

		// Incremental detail loading — one save per frame
		if (detail->loading) {
			if (detail->load_cursor < detail->num_entries) {
				if (detail->source == CM_SRC_VMC) {
					vmc_read_save_detail(detail->vmc_path,
						&detail->entries[detail->load_cursor],
						detail->_vmc_sysarea, detail->_vmc_total_blocks);
					detail->load_cursor++;
				} else {
					if (CARD_ProbeEx(detail->slot, NULL, NULL) != CARD_ERROR_READY) {
						detail->load_cursor = detail->num_entries;
					} else {
						card_manager_read_save_detail(detail->slot,
							&detail->entries[detail->load_cursor]);
						detail->load_cursor++;
					}
				}
			}
			if (detail->load_cursor >= detail->num_entries) {
				detail->loading = false;
				detail->activity = CM_ACTIVITY_IDLE;
				if (detail->_vmc_sysarea) {
					free(detail->_vmc_sysarea);
					detail->_vmc_sysarea = NULL;
				}
				for (int i = 0; i < detail->num_entries; i++) {
					if (detail->entries[i].icon
						&& detail->entries[i].icon->num_frames > 1) {
						detail->has_animated_icons = true;
						break;
					}
				}
			}
			needs_redraw = true;
		}

		// Reload any library devices that need it
		if (lib->initialized && lib->needs_rebuild) {
			cm_toast("Updating library...", 90);
			lib_state_rebuild(lib);
			lib->needs_rebuild = false;
			needs_redraw = true;
		}

		// Draw current view
		bool has_anim = (view_mode == 0 &&
			(detail->has_animated_icons || detail->loading))
			|| view_mode == 1;
		if (needs_redraw || has_anim || toast_active) {
			uiDrawObj_t *newPage;
			if (view_mode == 0)
				newPage = card_manager_draw(shelf, detail, focus, anim_tick);
			else
				newPage = lib_draw_view(lib, anim_tick);

			cm_draw_toast(newPage);
			if (page) DrawDispose(page);
			page = newPage;
			DrawPublish(page);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();
		anim_tick++;

		// Poll physical slots for hotswap
		poll_counter++;
		if (poll_counter >= CARD_POLL_INTERVAL) {
			poll_counter = 0;
			bool slot_present[2];
			for (int slot = 0; slot < 2; slot++) {
				s32 probe = CARD_ProbeEx(slot, NULL, NULL);
				slot_present[slot] = (probe == CARD_ERROR_READY || probe == CARD_ERROR_BUSY);

				// Update shelf presence for physical slots
				if (slot < shelf->count && shelf->items[slot].type == SHELF_ITEM_PHYSICAL) {
					bool was_present = shelf->items[slot].present;
					shelf->items[slot].present = slot_present[slot];
					if (slot_present[slot] != was_present) {
						cm_toast(slot_present[slot]
							? (slot == 0 ? "Slot A detected" : "Slot B detected")
							: (slot == 0 ? "Slot A removed" : "Slot B removed"), 120);
					}
				}

				// Reload detail if showing this physical card and presence changed
				if (detail->source == CM_SRC_PHYSICAL
					&& detail->slot == slot
					&& detail->card_present != slot_present[slot]) {
					detail->needs_reload = true;
				}
			}
			// Library card polling
			if (lib->initialized && lib->devices) {
				for (int d = 0; d < lib->num_devices && d < LIB_NUM_PHYSICAL; d++) {
					bool present = slot_present[lib->devices[d].slot];
					if (present != lib->devices[d].present)
						lib->devices[d].needs_reload = true;
				}
				for (int d = 0; d < lib->num_devices; d++) {
					if (lib->devices[d].needs_reload) {
						lib->needs_rebuild = true;
						break;
					}
				}
			}

			if (detail->needs_reload)
				continue;
		}

		u16 btns = padsButtonsHeld();
		s8 stickX = padsStickX();
		s8 stickY = padsStickY();
		bool has_input = btns || stickY > 16 || stickY < -16 ||
			stickX > 16 || stickX < -16;
		if (!has_input) continue;

		// --- View toggle (X) — blocked during loading (CARD I/O conflict) ---
		if ((btns & PAD_BUTTON_X) && !detail->loading) {
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
			if (focus == CM_FOCUS_SHELF) {
				// Shelf navigation (up/down)
				u16 nav = 0;
				if ((btns & PAD_BUTTON_UP) || stickY > 16)   nav |= PAD_BUTTON_UP;
				if ((btns & PAD_BUTTON_DOWN) || stickY < -16) nav |= PAD_BUTTON_DOWN;

				if (nav) {
					int prev = shelf->cursor;
					if ((nav & PAD_BUTTON_UP) && shelf->cursor > 0)
						shelf->cursor--;
					if ((nav & PAD_BUTTON_DOWN) && shelf->cursor < shelf->count - 1)
						shelf->cursor++;

					if (shelf->cursor < shelf->scroll)
						shelf->scroll = shelf->cursor;
					if (shelf->cursor >= shelf->scroll + SHELF_MAX_VIS)
						shelf->scroll = shelf->cursor - SHELF_MAX_VIS + 1;

					if (shelf->cursor != prev) {
						shelf_settle = 10;
						if (detail->loading) {
							detail->loading = false;
							detail->activity = CM_ACTIVITY_IDLE;
							if (detail->_vmc_sysarea) {
								free(detail->_vmc_sysarea);
								detail->_vmc_sysarea = NULL;
							}
						}
					}
					needs_redraw = true;
				}

				// Enter detail or create VMC (A or Right)
				if (btns & (PAD_BUTTON_A | PAD_BUTTON_RIGHT)) {
					while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_RIGHT)) { VIDEO_WaitVSync(); }
					// Flush pending settle so the correct card loads
					if (shelf_settle > 0) {
						shelf_settle = 0;
						cm_shelf_apply(shelf, detail);
					}
					shelf_item *item = &shelf->items[shelf->cursor];
					if (item->type == SHELF_ITEM_CREATE && !detail->loading) {
						char new_filename[64];
						if (cm_vmc_create(1, new_filename, sizeof(new_filename))) {
							cm_shelf_populate(shelf);
							DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG]
								? devices[DEVICE_CONFIG]
								: devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
							if (dev) {
								char new_path[PATHNAME_MAX];
								concatf_path(new_path, dev->initial->name,
									"swiss/saves/%s", new_filename);
								for (int i = 0; i < shelf->count; i++) {
									if (shelf->items[i].type == SHELF_ITEM_VMC
										&& strcmp(shelf->items[i].path, new_path) == 0) {
										shelf->cursor = i;
										break;
									}
								}
							}
							cm_shelf_apply(shelf, detail);
						}
						needs_redraw = true;
					} else if (detail->card_present && detail->num_entries > 0
						&& !detail->loading) {
						focus = CM_FOCUS_DETAIL;
						detail->anim_start = anim_tick;
						needs_redraw = true;
					}
					goto debounce;
				}

				// Delete VMC (Y) — only on VMC shelf items
				if ((btns & PAD_BUTTON_Y) && !detail->loading && shelf_settle == 0) {
					shelf_item *item = &shelf->items[shelf->cursor];
					if (item->type == SHELF_ITEM_VMC) {
						while (padsButtonsHeld() & PAD_BUTTON_Y) { VIDEO_WaitVSync(); }

						// Check if any games reference this VMC
						char *vmc_name = getRelativeName(item->path);
						cm_game_entry assigned[MAX_GAME_ENTRIES];
						int num_assigned = cm_vmc_find_assignments(
							vmc_name, assigned, MAX_GAME_ENTRIES);

						char msg[256];
						if (num_assigned > 0) {
							char game_list[128] = "";
							for (int i = 0; i < num_assigned && i < 3; i++) {
								if (i > 0) strlcat(game_list, ", ", sizeof(game_list));
								strlcat(game_list, assigned[i].game_name, sizeof(game_list));
							}
							if (num_assigned > 3)
								strlcat(game_list, ", ...", sizeof(game_list));
							snprintf(msg, sizeof(msg),
								"Delete %s?\nUsed by: %s\nAll saves will be lost.\n \nHold L + A to delete  |  B: Cancel",
								item->label, game_list);
						} else {
							snprintf(msg, sizeof(msg),
								"Delete %s?\nAll saves on this card will be lost.\n \nHold L + A to delete  |  B: Cancel",
								item->label);
						}

						uiDrawObj_t *confirm = cm_draw_message(msg);
						DrawPublish(confirm);
						u16 btn = cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
						DrawDispose(confirm);
						if ((btn & PAD_BUTTON_A)
							&& (padsButtonsHeld() & PAD_TRIGGER_L)) {
							// Clear game config references
							if (num_assigned > 0)
								cm_vmc_clear_assignments(vmc_name);

							DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG]
								? devices[DEVICE_CONFIG]
								: devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
							if (dev && dev->deleteFile) {
								file_handle *f = calloc(1, sizeof(file_handle));
								if (f) {
									strlcpy(f->name, item->path, PATHNAME_MAX);
									dev->deleteFile(f);
									free(f);
								}
							}
							// If detail was showing this VMC, switch to slot A
							if (detail->source == CM_SRC_VMC
								&& strcmp(detail->vmc_path, item->path) == 0) {
								detail->source = CM_SRC_PHYSICAL;
								detail->slot = CARD_SLOTA;
								detail->vmc_path[0] = '\0';
								detail->needs_reload = true;
								shelf->cursor = 0;
							}
							cm_shelf_populate(shelf);
							cm_shelf_apply(shelf, detail);
							if (lib->initialized) lib->needs_rebuild = true;
						}
						needs_redraw = true;
						goto debounce;
					}
				}

				if (btns & PAD_BUTTON_B)
					break;

			} else {
				// --- CM_FOCUS_DETAIL ---

				// Check for return to shelf: Left at column 0 or B
				bool left_pressed = (btns & PAD_BUTTON_LEFT) || stickX < -16;
				bool at_left_edge = (detail->cursor % DETAIL_GRID_COLS == 0);
				if (left_pressed && at_left_edge) {
					focus = CM_FOCUS_SHELF;
					needs_redraw = true;
					goto debounce;
				}
				if (btns & PAD_BUTTON_B) {
					while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
					focus = CM_FOCUS_SHELF;
					needs_redraw = true;
					goto debounce;
				}

				// Grid navigation (d-pad + analog stick)
				u16 nav = btns & (PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT);
				if (stickY > 16)  nav |= PAD_BUTTON_UP;
				if (stickY < -16) nav |= PAD_BUTTON_DOWN;
				if (stickX < -16) nav |= PAD_BUTTON_LEFT;
				if (stickX > 16)  nav |= PAD_BUTTON_RIGHT;
				if (nav) {
					cm_panel_navigate(detail, nav, DETAIL_GRID_COLS);
					if (detail->cursor != detail->prev_cursor) {
						detail->anim_start = anim_tick;
						detail->prev_cursor = detail->cursor;
					}
					needs_redraw = true;
				}

				// Context menu (A)
				if ((btns & PAD_BUTTON_A) && detail->card_present
					&& detail->num_entries > 0 && !detail->loading) {
					while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
					cm_handle_context_menu(detail, &needs_redraw);
					needs_redraw = true;
					if (lib->initialized)
						lib->needs_rebuild = true;
					cm_shelf_populate(shelf);
					goto debounce;
				}
			}
		}

		// --- Library view input ---
		if (view_mode == 1) {
			cm_panel *lib_panels[2] = { detail, detail };
			int result = lib_handle_input(lib, lib_panels, btns, stickY);
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
	if (detail->_vmc_sysarea) free(detail->_vmc_sysarea);
	card_manager_free_graphics(detail->entries, detail->num_entries);
	free(detail);
	free(shelf);
	cm_retire_flush();
	cm_activity_hide();
	while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
}
