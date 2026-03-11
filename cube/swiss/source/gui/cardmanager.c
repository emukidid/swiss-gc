/* cardmanager.c
	- Memory Card Manager UI
	by Swiss contributors
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <ogc/card.h>
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "devices/memcard/deviceHandler-CARD.h"

#define SAVES_PER_PAGE 10
#define CARD_MANAGER_LIST_X 30
#define CARD_MANAGER_LIST_Y 100
#define CARD_MANAGER_ROW_HEIGHT 28
#define CARD_POLL_INTERVAL 30

typedef struct {
	char filename[CARD_FILENAMELEN + 1];
	char gamecode[5];
	char company[3];
	u32 filesize;
	u16 blocks;
	u32 icon_fmt;
	u32 banner_fmt;
	u32 comment_addr;
} card_entry;

// --- Logging ---

static FILE *logfile = NULL;

static void cm_log_open(void) {
	if (logfile) return;
	if (devices[DEVICE_CONFIG] == NULL) return;
	char logpath[PATHNAME_MAX];
	concatf_path(logpath, devices[DEVICE_CONFIG]->initial->name, "cardmanager.log");
	logfile = fopen(logpath, "a");
	if (logfile) {
		fprintf(logfile, "\n--- Card Manager session started ---\n");
		fflush(logfile);
	}
}

static void cm_log(const char *fmt, ...) {
	if (!logfile) return;
	va_list args;
	va_start(args, fmt);
	vfprintf(logfile, fmt, args);
	va_end(args);
	fprintf(logfile, "\n");
	fflush(logfile);
}

static void cm_log_close(void) {
	if (!logfile) return;
	fprintf(logfile, "--- Card Manager session ended ---\n");
	fclose(logfile);
	logfile = NULL;
}

static void cm_log_card_info(int slot, s32 mem_size, s32 sector_size, int num_entries) {
	cm_log("Slot %c: mem_size=%d, sector_size=%d, entries=%d",
		slot == CARD_SLOTA ? 'A' : 'B', (int)mem_size, (int)sector_size, num_entries);
}

static void cm_log_entry(int idx, card_entry *entry) {
	cm_log("  [%d] \"%s\" code=%s company=%s blocks=%d size=%dK",
		idx, entry->filename, entry->gamecode, entry->company,
		entry->blocks, entry->filesize / 1024);
}

// --- Card reading ---

static int card_manager_read_saves(int slot, card_entry *entries, int max_entries) {
	int count = 0;
	card_dir carddir;

	CARD_SetGamecode("SWIS");
	CARD_SetCompany("S0");

	memset(&carddir, 0, sizeof(card_dir));
	s32 ret = CARD_FindFirst(slot, &carddir, true);
	while (ret != CARD_ERROR_NOFILE && count < max_entries) {
		memset(&entries[count], 0, sizeof(card_entry));
		snprintf(entries[count].filename, CARD_FILENAMELEN + 1, "%.*s",
			CARD_FILENAMELEN, carddir.filename);
		memcpy(entries[count].gamecode, carddir.gamecode, 4);
		entries[count].gamecode[4] = '\0';
		memcpy(entries[count].company, carddir.company, 2);
		entries[count].company[2] = '\0';
		entries[count].filesize = carddir.filelen;
		entries[count].blocks = carddir.filelen / 8192;
		count++;
		ret = CARD_FindNext(&carddir);
	}
	return count;
}

// --- Drawing ---
// Uses a single fixed-size container for all states to avoid pop-in/out

static uiDrawObj_t *card_manager_draw_page(int slot, card_entry *entries,
	int num_entries, int cursor, int scroll_offset,
	s32 mem_size, s32 sector_size, bool card_present) {

	uiDrawObj_t *container = DrawEmptyBox(20, 60, getVideoMode()->fbWidth - 20, 420);

	// Title bar
	char title[64];
	if (card_present) {
		int total_blocks = (mem_size << 20 >> 3) / sector_size - 5;
		int used_blocks = 0;
		for (int i = 0; i < num_entries; i++) {
			used_blocks += entries[i].blocks;
		}
		int free_blocks = total_blocks - used_blocks;
		sprintf(title, "Memory Card  Slot %c  -  %d/%d blocks free",
			slot == CARD_SLOTA ? 'A' : 'B', free_blocks, total_blocks);
	}
	else {
		sprintf(title, "Memory Card  Slot %c", slot == CARD_SLOTA ? 'A' : 'B');
	}
	DrawAddChild(container, DrawStyledLabel(640 / 2, 68, title, 0.7f, ALIGN_CENTER, defaultColor));

	if (!card_present) {
		DrawAddChild(container, DrawStyledLabel(640 / 2, 240, "Insert a memory card...", 0.75f, ALIGN_CENTER, defaultColor));
	}
	else if (num_entries == 0) {
		DrawAddChild(container, DrawStyledLabel(640 / 2, 240, "No saves found", 0.75f, ALIGN_CENTER, defaultColor));
	}
	else {
		// Column headers
		GXColor headerColor = (GXColor){180, 180, 180, 255};
		DrawAddChild(container, DrawStyledLabel(CARD_MANAGER_LIST_X, 88, "Game", 0.55f, ALIGN_LEFT, headerColor));
		DrawAddChild(container, DrawStyledLabel(350, 88, "Code", 0.55f, ALIGN_LEFT, headerColor));
		DrawAddChild(container, DrawStyledLabel(440, 88, "Blocks", 0.55f, ALIGN_LEFT, headerColor));
		DrawAddChild(container, DrawStyledLabel(530, 88, "Size", 0.55f, ALIGN_LEFT, headerColor));

		int visible = num_entries - scroll_offset;
		if (visible > SAVES_PER_PAGE) visible = SAVES_PER_PAGE;

		for (int i = 0; i < visible; i++) {
			int entry_idx = scroll_offset + i;
			int row_y = CARD_MANAGER_LIST_Y + (i * CARD_MANAGER_ROW_HEIGHT);
			// Font renderer centers text vertically on the Y coordinate,
			// so place at the row's vertical midpoint
			int text_y = row_y + CARD_MANAGER_ROW_HEIGHT / 2;
			GXColor color = (entry_idx == cursor) ?
				(GXColor){96, 107, 164, 255} : defaultColor;

			if (entry_idx == cursor) {
				DrawAddChild(container, DrawEmptyColouredBox(
					CARD_MANAGER_LIST_X - 4, row_y,
					getVideoMode()->fbWidth - 44, row_y + CARD_MANAGER_ROW_HEIGHT,
					(GXColor){96, 107, 164, 80}));
			}

			DrawAddChild(container, DrawStyledLabel(
				CARD_MANAGER_LIST_X, text_y,
				entries[entry_idx].filename,
				0.6f, ALIGN_LEFT, color));

			DrawAddChild(container, DrawStyledLabel(
				350, text_y,
				entries[entry_idx].gamecode,
				0.6f, ALIGN_LEFT, color));

			char blocks_str[16];
			sprintf(blocks_str, "%d", entries[entry_idx].blocks);
			DrawAddChild(container, DrawStyledLabel(
				440, text_y, blocks_str,
				0.6f, ALIGN_LEFT, color));

			char size_str[16];
			sprintf(size_str, "%dK", entries[entry_idx].filesize / 1024);
			DrawAddChild(container, DrawStyledLabel(
				530, text_y, size_str,
				0.6f, ALIGN_LEFT, color));
		}

		if (num_entries > SAVES_PER_PAGE) {
			float scroll_pct = (float)scroll_offset / (float)(num_entries - SAVES_PER_PAGE);
			DrawAddChild(container, DrawVertScrollBar(
				getVideoMode()->fbWidth - 35, CARD_MANAGER_LIST_Y,
				10, SAVES_PER_PAGE * CARD_MANAGER_ROW_HEIGHT,
				scroll_pct, 30));
		}
	}

	DrawAddChild(container, DrawStyledLabel(640 / 2, 400,
		"D-Pad: Navigate  |  L/R: Switch Slot  |  B: Back",
		0.55f, ALIGN_CENTER, defaultColor));

	return container;
}

// --- Card status polling ---

static bool card_manager_check_status(int slot, bool was_present) {
	s32 ret = CARD_ProbeEx(slot, NULL, NULL);
	bool is_present = (ret == CARD_ERROR_READY || ret == CARD_ERROR_BUSY);
	return is_present != was_present;
}

// --- Main loop ---

void show_card_manager(void) {
	int slot = CARD_SLOTA;
	int cursor = 0;
	int scroll_offset = 0;
	card_entry entries[128];
	int num_entries = 0;
	s32 mem_size = 0, sector_size = 0;
	bool needs_reload = true;
	bool needs_redraw = false;
	bool card_present = false;
	int poll_counter = 0;
	uiDrawObj_t *pagePanel = NULL;

	cm_log_open();
	cm_log("Opened card manager, starting on Slot A");

	// Wait for A button release
	while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }

	while (1) {
		if (needs_reload) {
			num_entries = 0;
			cursor = 0;
			scroll_offset = 0;
			mem_size = 0;
			sector_size = 0;
			card_present = false;

			cm_log("Loading Slot %c...", slot == CARD_SLOTA ? 'A' : 'B');

			s32 ret = initialize_card(slot);
			if (ret == CARD_ERROR_READY) {
				CARD_ProbeEx(slot, &mem_size, &sector_size);
				num_entries = card_manager_read_saves(slot, entries, 128);
				card_present = true;
				cm_log_card_info(slot, mem_size, sector_size, num_entries);
				for (int i = 0; i < num_entries; i++) {
					cm_log_entry(i, &entries[i]);
				}
			}
			else {
				cm_log("No card detected (error=%d)", (int)ret);
			}
			needs_reload = false;
			needs_redraw = true;
			poll_counter = 0;
		}

		if (needs_redraw) {
			uiDrawObj_t *newPanel = card_manager_draw_page(slot, entries,
				num_entries, cursor, scroll_offset,
				mem_size, sector_size, card_present);

			if (pagePanel) DrawDispose(pagePanel);
			pagePanel = newPanel;
			DrawPublish(pagePanel);
			needs_redraw = false;
		}

		VIDEO_WaitVSync();

		// Poll card status periodically
		poll_counter++;
		if (poll_counter >= CARD_POLL_INTERVAL) {
			poll_counter = 0;
			if (card_manager_check_status(slot, card_present)) {
				cm_log("Card %s in Slot %c",
					card_present ? "removed" : "inserted",
					slot == CARD_SLOTA ? 'A' : 'B');
				needs_reload = true;
				continue;
			}
		}

		u16 btns = padsButtonsHeld();
		if (!btns) continue;

		if ((btns & PAD_BUTTON_UP) && cursor > 0) {
			cursor--;
			if (cursor < scroll_offset)
				scroll_offset = cursor;
			needs_redraw = true;
		}
		if ((btns & PAD_BUTTON_DOWN) && cursor < num_entries - 1) {
			cursor++;
			if (cursor >= scroll_offset + SAVES_PER_PAGE)
				scroll_offset = cursor - SAVES_PER_PAGE + 1;
			needs_redraw = true;
		}

		if ((btns & PAD_TRIGGER_L) && slot != CARD_SLOTA) {
			slot = CARD_SLOTA;
			cm_log("Switched to Slot A");
			needs_reload = true;
		}
		if ((btns & PAD_TRIGGER_R) && slot != CARD_SLOTB) {
			slot = CARD_SLOTB;
			cm_log("Switched to Slot B");
			needs_reload = true;
		}

		if (btns & PAD_BUTTON_B)
			break;

		while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN |
			PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT |
			PAD_BUTTON_A | PAD_BUTTON_B |
			PAD_TRIGGER_L | PAD_TRIGGER_R)) {
			VIDEO_WaitVSync();
		}
	}

	cm_log("Closed card manager");
	cm_log_close();
	if (pagePanel) DrawDispose(pagePanel);
	while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
}
