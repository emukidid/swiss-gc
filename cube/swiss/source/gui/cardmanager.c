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
	uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, msg);
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
			uiDrawObj_t *msgBox = DrawMessageBox(D_WARN, msg);
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
				msgBox = DrawMessageBox(D_INFO, "Formatting Memory Card...\nDo not remove the Memory Card.");
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
			uiDrawObj_t *msgBox = DrawMessageBox(D_WARN, msg);
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
				msgBox = DrawMessageBox(D_INFO, "Formatting Memory Card...\nDo not remove the Memory Card.");
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
	// Future: CM_SRC_VMC handling here

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
	if (cy >= panel->scroll_row + GRID_ROWS_VISIBLE_MAX)
		panel->scroll_row = cy - GRID_ROWS_VISIBLE_MAX + 1;
}

// --- Context menu handling ---

static void cm_handle_context_menu(cm_panel *ap, cm_panel *other, bool *needs_redraw) {
	card_entry *sel = &ap->entries[ap->cursor];

	// Build context menu items
	char copy_label[48];
	snprintf(copy_label, sizeof(copy_label), "Copy to %s", other->label);

	const char *items[CM_ACT_COUNT];
	bool enabled[CM_ACT_COUNT];

	items[CM_ACT_COPY] = copy_label;
	enabled[CM_ACT_COPY] = other->card_present;

	items[CM_ACT_EXPORT] = "Export to SD (.gci)";
	enabled[CM_ACT_EXPORT] = true;

	items[CM_ACT_IMPORT] = "Import from SD (.gci)";
	enabled[CM_ACT_IMPORT] = ap->card_present;

	items[CM_ACT_DELETE] = "Delete";
	enabled[CM_ACT_DELETE] = true;

	int choice = cm_context_menu(items, enabled, CM_ACT_COUNT);

	switch (choice) {
		case CM_ACT_COPY:
			// TODO: implement cross-panel copy
			cm_log("Copy requested: \"%s\" → %s (not yet implemented)",
				sel->filename, other->label);
			break;

		case CM_ACT_EXPORT:
			cm_log("Exporting [%d] \"%s\" from %s",
				ap->cursor, sel->filename, ap->label);
			if (card_manager_export_save(ap->slot, sel, ap->sector_size)) {
				cm_log("Export successful");
			} else {
				cm_log("Export failed or cancelled");
			}
			*needs_redraw = true;
			break;

		case CM_ACT_IMPORT:
			if (card_manager_backups(ap->slot, ap->sector_size)) {
				ap->needs_reload = true;
			} else {
				*needs_redraw = true;
			}
			break;

		case CM_ACT_DELETE:
			if (card_manager_confirm_delete(sel->filename)) {
				cm_log("Deleting [%d] \"%s\" from %s",
					ap->cursor, sel->filename, ap->label);
				if (card_manager_delete_save(ap->slot, sel)) {
					cm_log("Delete successful");
					ap->needs_reload = true;
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

	int active = 0;
	bool needs_redraw = false;
	int poll_counter = 0;
	u32 anim_tick = 0;
	uiDrawObj_t *pagePanel = NULL;

	cm_draw_init();
	cm_log_open();
	cm_log("Opened card manager (dual-panel)");

	// Wait for A button release
	while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }

	while (1) {
		// Reload any panel that needs it
		for (int p = 0; p < 2; p++) {
			if (panels[p]->needs_reload) {
				card_manager_free_graphics(panels[p]->entries, panels[p]->num_entries);
				panels[p]->num_entries = 0;
				panels[p]->loading = true;
				uiDrawObj_t *loadPanel = card_manager_draw(panels[0], panels[1], active, anim_tick);
				if (pagePanel) DrawDispose(pagePanel);
				pagePanel = loadPanel;
				DrawPublish(pagePanel);
				cm_panel_load(panels[p]);
				panels[p]->loading = false;
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

		// Context menu (A) — requires a save selected
		if ((btns & PAD_BUTTON_A) && ap->card_present && ap->num_entries > 0) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			cm_handle_context_menu(ap, other, &needs_redraw);
			goto debounce;
		}

		// Backups (X) — browse/manage GCI backups on SD
		if (btns & PAD_BUTTON_X) {
			while (padsButtonsHeld() & PAD_BUTTON_X) { VIDEO_WaitVSync(); }
			if (card_manager_backups(ap->slot, ap->sector_size)) {
				ap->needs_reload = true;
			} else {
				needs_redraw = true;
			}
			goto debounce;
		}

		if (btns & PAD_BUTTON_B)
			break;

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
