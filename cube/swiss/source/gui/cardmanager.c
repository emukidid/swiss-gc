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

// --- Card status polling ---

static bool card_manager_check_status(int slot, bool was_present) {
	s32 ret = CARD_ProbeEx(slot, NULL, NULL);
	bool is_present = (ret == CARD_ERROR_READY || ret == CARD_ERROR_BUSY);
	return is_present != was_present;
}

// --- Panel helpers ---

static void cm_show_error(const char *msg) {
	uiDrawObj_t *msgBox = cm_draw_message(msg);
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
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

	char load_msg[64];
	snprintf(load_msg, sizeof(load_msg), "Reading %s...", panel->label);
	cm_led_show(load_msg);

	if (panel->source == CM_SRC_PHYSICAL) {
		char slot_ch = panel->slot == CARD_SLOTA ? 'A' : 'B';
		snprintf(panel->label, sizeof(panel->label), "Slot %c", slot_ch);
		cm_log("Loading Slot %c...", slot_ch);

		s32 ret = initialize_card(panel->slot);

		// Handle mount errors per Nintendo Memory Card Guidelines §3.6-3.8
		if (ret == CARD_ERROR_WRONGDEVICE) {
			char msg[128];
			sprintf(msg, "The device in Slot %c is not a\nMemory Card.", slot_ch);
			cm_log("Slot %c: wrong device", slot_ch);
			cm_show_error(msg);
		}
		else if (ret == CARD_ERROR_ENCODING) {
			char msg[128];
			sprintf(msg, "Memory Card in Slot %c uses an\nincompatible format.\n \nA: Format  |  B: Cancel", slot_ch);
			cm_log("Slot %c: encoding mismatch (foreign card)", slot_ch);
			uiDrawObj_t *msgBox = cm_draw_message(msg);
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
			if (choice == 1) {
				msgBox = cm_draw_message("Formatting Memory Card...\nDo not remove the Memory Card.");
				DrawPublish(msgBox);
				ret = CARD_Format(panel->slot);
				DrawDispose(msgBox);
				if (ret == CARD_ERROR_READY) {
					cm_log("Slot %c: format successful, reloading", slot_ch);
					ret = initialize_card(panel->slot);
				} else {
					char fmsg[128];
					sprintf(fmsg, "Memory Card in Slot %c is damaged\nand cannot be used.", slot_ch);
					cm_log("Slot %c: format failed (error=%d)", slot_ch, (int)ret);
					cm_show_error(fmsg);
				}
			}
		}
		else if (ret == CARD_ERROR_BROKEN) {
			char msg[128];
			sprintf(msg, "Memory Card in Slot %c has corrupted\ndata. Format the card?\n \nA: Format  |  B: Cancel", slot_ch);
			cm_log("Slot %c: card broken/corrupted", slot_ch);
			uiDrawObj_t *msgBox = cm_draw_message(msg);
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
			if (choice == 1) {
				msgBox = cm_draw_message("Formatting Memory Card...\nDo not remove the Memory Card.");
				DrawPublish(msgBox);
				ret = CARD_Format(panel->slot);
				DrawDispose(msgBox);
				if (ret == CARD_ERROR_READY) {
					cm_log("Slot %c: format successful, reloading", slot_ch);
					ret = initialize_card(panel->slot);
				} else {
					char fmsg[128];
					sprintf(fmsg, "Memory Card in Slot %c is damaged\nand cannot be used.", slot_ch);
					cm_log("Slot %c: format failed (error=%d)", slot_ch, (int)ret);
					cm_show_error(fmsg);
				}
			}
		}
		else if (ret != CARD_ERROR_READY && ret != CARD_ERROR_NOCARD) {
			cm_log("Slot %c: error=%d", slot_ch, (int)ret);
		}

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
		else if (ret != CARD_ERROR_NOCARD) {
			cm_log("No card detected in Slot %c (error=%d)", slot_ch, (int)ret);
		}
	}
	else if (panel->source == CM_SRC_VMC) {
		cm_log("Loading VMC: %s", panel->vmc_path);
		panel->num_entries = vmc_read_saves(panel->vmc_path, panel->entries, 128,
			&panel->mem_size, &panel->sector_size);
		if (panel->num_entries >= 0) {
			panel->card_present = true;
			cm_log("VMC \"%s\": %d KB, %d entries",
				panel->label, panel->mem_size / 1024, panel->num_entries);
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
	}

	panel->needs_reload = false;
	cm_led_hide();
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
	if (cy >= panel->scroll_row + GRID_ROWS_VISIBLE_MAX)
		panel->scroll_row = cy - GRID_ROWS_VISIBLE_MAX + 1;
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
				while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
				while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
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
			cm_log("Copy \"%s\" from %s → %s",
				sel->filename, ap->label,
				dest_src == CM_SRC_VMC ? dest_vmc_path : (dest_slot == 0 ? "Slot A" : "Slot B"));

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
					while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
					while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
					DrawDispose(msg);
					write_ok = false;
				} else {
					write_ok = card_manager_import_gci_buf(dest_slot, &gci, savedata,
						save_len, dest_sector);
				}
			}
			if (dest_panel) dest_panel->activity = CM_ACTIVITY_IDLE;

			if (write_ok) {
				cm_log("Copy successful");
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
			cm_log("Exporting [%d] \"%s\" from %s",
				ap->cursor, sel->filename, ap->label);
			ap->activity = CM_ACTIVITY_READ;
			if (ap->source == CM_SRC_VMC) {
				if (vmc_export_save(ap->vmc_path, sel))
					cm_log("VMC export successful");
				else
					cm_log("VMC export failed or cancelled");
			} else {
				if (card_manager_export_save(ap->slot, sel, ap->sector_size))
					cm_log("Export successful");
				else
					cm_log("Export failed or cancelled");
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
				cm_log("Deleting [%d] \"%s\" from %s",
					ap->cursor, sel->filename, ap->label);
				ap->activity = CM_ACTIVITY_WRITE;
				bool deleted;
				if (ap->source == CM_SRC_VMC)
					deleted = vmc_delete_save(ap->vmc_path, sel);
				else
					deleted = card_manager_delete_save(ap->slot, sel);
				ap->activity = CM_ACTIVITY_IDLE;
				if (deleted) {
					cm_log("Delete successful");
					cm_panel_remove_entry(ap, ap->cursor);
					*needs_redraw = true;
				} else {
					cm_log("Delete failed");
				}
			} else {
				cm_log("Delete cancelled for \"%s\"", sel->filename);
				*needs_redraw = true;
			}
			break;

		default:
			*needs_redraw = true;
			break;
	}
}

// --- Main loop ---

// --- VMC picker ---
// Shows a list of .raw VMC files and lets the user pick one to mount on the panel.
// Returns true if panel was changed, false if cancelled.
static bool cm_pick_vmc(cm_panel *panel) {
	vmc_file_entry *vmcs = calloc(MAX_VMC_FILES, sizeof(vmc_file_entry));
	if (!vmcs) return false;

	int num = vmc_scan_files(vmcs, MAX_VMC_FILES);
	if (num == 0) {
		uiDrawObj_t *msg = cm_draw_message("No virtual card files found\nin swiss/saves/");
		DrawPublish(msg);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msg);
		free(vmcs);
		return false;
	}

	// Add "Physical card" option at the top
	const char *items[MAX_VMC_FILES + 1];
	bool enabled[MAX_VMC_FILES + 1];
	char phys_label[48];
	snprintf(phys_label, sizeof(phys_label), "Physical Slot %c",
		panel->slot == CARD_SLOTA ? 'A' : 'B');
	items[0] = phys_label;
	enabled[0] = true;
	for (int i = 0; i < num; i++) {
		items[i + 1] = vmcs[i].label;
		enabled[i + 1] = true;
	}

	int choice = cm_context_menu(items, enabled, num + 1);

	if (choice == 0) {
		// Switch back to physical card
		if (panel->source != CM_SRC_PHYSICAL) {
			panel->source = CM_SRC_PHYSICAL;
			panel->vmc_path[0] = '\0';
			panel->needs_reload = true;
			cm_log("Switched panel to Physical Slot %c",
				panel->slot == CARD_SLOTA ? 'A' : 'B');
		}
		free(vmcs);
		return panel->needs_reload;
	}
	else if (choice >= 1 && choice <= num) {
		int vi = choice - 1;
		panel->source = CM_SRC_VMC;
		strlcpy(panel->vmc_path, vmcs[vi].path, PATHNAME_MAX);
		strlcpy(panel->label, vmcs[vi].label, sizeof(panel->label));
		panel->needs_reload = true;
		cm_log("Switched panel to VMC: %s", vmcs[vi].label);
		free(vmcs);
		return true;
	}

	free(vmcs);
	return false;
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
	cm_log_open();
	cm_log("Opened card manager");

	// Wait for A button release
	while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }

	while (1) {
		// Reload any panel that needs it
		bool any_reload = false;
		for (int p = 0; p < 2; p++) {
			if (panels[p]->needs_reload) {
				card_manager_free_graphics(panels[p]->entries, panels[p]->num_entries);
				panels[p]->num_entries = 0;
				panels[p]->loading = true;
				panels[p]->activity = CM_ACTIVITY_READ;
				any_reload = true;
			}
		}
		if (any_reload) {
			// Show loading state in both views (LEDs go orange)
			uiDrawObj_t *loadPage;
			if (view_mode == 0)
				loadPage = card_manager_draw(panels[0], panels[1], active, anim_tick);
			else
				loadPage = lib_draw_view(lib, anim_tick);
			cm_draw_status_leds(loadPage, panels);
			if (page) DrawDispose(page);
			page = loadPage;
			DrawPublish(page);
		}
		for (int p = 0; p < 2; p++) {
			if (panels[p]->loading) {
				cm_panel_load(panels[p]);
				panels[p]->loading = false;
				panels[p]->activity = CM_ACTIVITY_IDLE;
				needs_redraw = true;
			}
		}

		// Rebuild library index after panel reloads
		if (lib->initialized && lib->needs_rebuild)
			lib_state_rebuild(lib, panels);

		// Draw current view
		bool has_anim = (view_mode == 0 &&
			(panels[0]->has_animated_icons || panels[1]->has_animated_icons))
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

		// Poll both slots for hotswap (physical only)
		poll_counter++;
		if (poll_counter >= CARD_POLL_INTERVAL) {
			poll_counter = 0;
			for (int p = 0; p < 2; p++) {
				if (panels[p]->source != CM_SRC_PHYSICAL) continue;
				if (card_manager_check_status(panels[p]->slot, panels[p]->card_present)) {
					cm_log("Card %s in Slot %c",
						panels[p]->card_present ? "removed" : "inserted",
						panels[p]->slot == CARD_SLOTA ? 'A' : 'B');
					panels[p]->needs_reload = true;
					if (lib->initialized) lib->needs_rebuild = true;
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

		// --- View toggle (X) ---
		if (btns & PAD_BUTTON_X) {
			while (padsButtonsHeld() & PAD_BUTTON_X) { VIDEO_WaitVSync(); }
			if (view_mode == 0) {
				if (!lib->initialized)
					lib_state_init(lib, panels);
				else if (lib->needs_rebuild)
					lib_state_rebuild(lib, panels);
				view_mode = 1;
				cm_log("Switched to library view");
			} else {
				view_mode = 0;
				cm_log("Switched to cards view");
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

			// Context menu (A) — requires a save selected
			if ((btns & PAD_BUTTON_A) && ap->card_present && ap->num_entries > 0) {
				while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
				cm_handle_context_menu(ap, other, &needs_redraw);
				if (lib->initialized) lib->needs_rebuild = true;
				goto debounce;
			}

			// VMC picker (Z) — switch active panel between physical card and VMC
			if (btns & PAD_TRIGGER_Z) {
				while (padsButtonsHeld() & PAD_TRIGGER_Z) { VIDEO_WaitVSync(); }
				cm_pick_vmc(ap);
				if (lib->initialized) lib->needs_rebuild = true;
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
				cm_log("Library → cards (back)");
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
		card_manager_free_graphics(panels[p]->entries, panels[p]->num_entries);
		free(panels[p]);
	}
	cm_led_end();
	cm_log("Closed card manager");
	cm_log_close();
	while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
}
