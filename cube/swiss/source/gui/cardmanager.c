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
#include <ogc/timesupp.h>
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
	char game_name[33];
	char game_desc[33];
	u32 filesize;
	u16 blocks;
	u32 fileno;
	u8 permissions;
} card_entry;

// --- Logging ---
// Uses Swiss device handler API (not fopen) since FAT devoptab isn't available

#define CM_LOG_MAX 16384
static char cm_log_buf[CM_LOG_MAX];
static int cm_log_len = 0;
static DEVICEHANDLER_INTERFACE *cm_log_dev = NULL;

static void cm_log_open(void) {
	cm_log_len = 0;
	cm_log_dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (cm_log_dev) {
		cm_log_len += snprintf(cm_log_buf + cm_log_len, CM_LOG_MAX - cm_log_len,
			"\n--- Card Manager session started ---\n");
	}
}

static void cm_log(const char *fmt, ...) {
	if (!cm_log_dev || cm_log_len >= CM_LOG_MAX - 1) return;
	va_list args;
	va_start(args, fmt);
	cm_log_len += vsnprintf(cm_log_buf + cm_log_len, CM_LOG_MAX - cm_log_len, fmt, args);
	va_end(args);
	if (cm_log_len < CM_LOG_MAX - 1) {
		cm_log_buf[cm_log_len++] = '\n';
		cm_log_buf[cm_log_len] = '\0';
	}
}

static void cm_log_close(void) {
	if (!cm_log_dev || cm_log_len == 0) return;
	cm_log_len += snprintf(cm_log_buf + cm_log_len, CM_LOG_MAX - cm_log_len,
		"--- Card Manager session ended ---\n");

	file_handle *logFile = calloc(1, sizeof(file_handle));
	if (logFile) {
		concat_path(logFile->name, cm_log_dev->initial->name, "cardmanager.log");
		// Read existing content if present
		char *existing = NULL;
		u32 existing_len = 0;
		if (!cm_log_dev->statFile(logFile) && logFile->size) {
			existing = calloc(1, logFile->size);
			if (existing) {
				cm_log_dev->readFile(logFile, existing, logFile->size);
				existing_len = logFile->size;
			}
			cm_log_dev->closeFile(logFile);
		}
		// Combine existing + new into single buffer and write once
		u32 total_len = existing_len + cm_log_len;
		char *combined = calloc(1, total_len);
		if (combined) {
			if (existing && existing_len)
				memcpy(combined, existing, existing_len);
			memcpy(combined + existing_len, cm_log_buf, cm_log_len);
			cm_log_dev->deleteFile(logFile);
			cm_log_dev->writeFile(logFile, combined, total_len);
			cm_log_dev->closeFile(logFile);
			free(combined);
		}
		free(existing);
		free(logFile);
	}
	cm_log_dev = NULL;
	cm_log_len = 0;
}

static void cm_log_card_info(int slot, s32 mem_size, s32 sector_size, int num_entries) {
	cm_log("Slot %c: mem_size=%d, sector_size=%d, entries=%d",
		slot == CARD_SLOTA ? 'A' : 'B', (int)mem_size, (int)sector_size, num_entries);
}

static void cm_log_entry(int idx, card_entry *entry) {
	if (entry->game_name[0]) {
		cm_log("  [%d] \"%s\" (%s) code=%s company=%s blocks=%d size=%dK",
			idx, entry->filename, entry->game_name, entry->gamecode,
			entry->company, entry->blocks, entry->filesize / 1024);
	}
	else {
		cm_log("  [%d] \"%s\" code=%s company=%s blocks=%d size=%dK",
			idx, entry->filename, entry->gamecode, entry->company,
			entry->blocks, entry->filesize / 1024);
	}
}

// --- Card reading ---

static void card_manager_read_comment(int slot, card_entry *entry) {
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);

	card_file cardfile;
	if (CARD_Open(slot, entry->filename, &cardfile) != CARD_ERROR_READY)
		return;

	card_stat cardstat;
	if (CARD_GetStatus(slot, cardfile.filenum, &cardstat) == CARD_ERROR_READY
		&& cardstat.comment_addr != (u32)-1) {
		// Read 64-byte comment block: first 32 = game name, next 32 = description
		// CARD_Read requires sector-aligned offsets (8192 bytes)
		u32 sector_offset = cardstat.comment_addr & ~(8192 - 1);
		u8 *buf = (u8 *)memalign(32, 8192);
		if (buf) {
			if (CARD_Read(&cardfile, buf, 8192, sector_offset) == CARD_ERROR_READY) {
				u32 off_in_sector = cardstat.comment_addr - sector_offset;
				char *comment = (char *)(buf + off_in_sector);
				snprintf(entry->game_name, sizeof(entry->game_name), "%.32s", comment);
				snprintf(entry->game_desc, sizeof(entry->game_desc), "%.32s", comment + 32);
			}
			free(buf);
		}
	}
	CARD_Close(&cardfile);
}

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
		entries[count].fileno = carddir.fileno;
		entries[count].permissions = carddir.permissions;
		count++;
		ret = CARD_FindNext(&carddir);
	}

	// Read comment blocks for game names
	for (int i = 0; i < count; i++) {
		card_manager_read_comment(slot, &entries[i]);
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
		DrawAddChild(container, DrawStyledLabel(380, 88, "Code", 0.55f, ALIGN_LEFT, headerColor));
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

			// Show game name if available, otherwise filename
			const char *display_name = entries[entry_idx].game_name[0] ?
				entries[entry_idx].game_name : entries[entry_idx].filename;
			DrawAddChild(container, DrawStyledLabel(
				CARD_MANAGER_LIST_X, text_y,
				display_name,
				0.6f, ALIGN_LEFT, color));

			DrawAddChild(container, DrawStyledLabel(
				380, text_y,
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

	if (card_present && num_entries > 0) {
		DrawAddChild(container, DrawStyledLabel(640 / 2, 395,
			"A: Export  |  X: Import  |  Z: Delete  |  L/R: Slot  |  B: Back",
			0.45f, ALIGN_CENTER, defaultColor));
	}
	else if (card_present) {
		DrawAddChild(container, DrawStyledLabel(640 / 2, 400,
			"X: Import  |  L/R: Switch Slot  |  B: Back",
			0.5f, ALIGN_CENTER, defaultColor));
	}
	else {
		DrawAddChild(container, DrawStyledLabel(640 / 2, 400,
			"L/R: Switch Slot  |  B: Back",
			0.55f, ALIGN_CENTER, defaultColor));
	}

	return container;
}

// --- Delete ---

static bool card_manager_confirm_delete(const char *filename) {
	char msg[256];
	sprintf(msg, "Delete \"%s\"?\n \nPress L + A to confirm, or B to cancel.", filename);
	uiDrawObj_t *msgBox = DrawMessageBox(D_WARN, msg);
	DrawPublish(msgBox);

	bool confirmed = false;
	while (1) {
		u16 btns = padsButtonsHeld();
		if ((btns & (PAD_BUTTON_A | PAD_TRIGGER_L)) == (PAD_BUTTON_A | PAD_TRIGGER_L)) {
			confirmed = true;
			break;
		}
		if (btns & PAD_BUTTON_B) break;
		VIDEO_WaitVSync();
	}
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_TRIGGER_L | PAD_BUTTON_B)) {
		VIDEO_WaitVSync();
	}
	DrawDispose(msgBox);
	return confirmed;
}

static bool card_manager_delete_save(int slot, card_entry *entry) {
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	s32 ret = CARD_Delete(slot, entry->filename);
	if (ret != CARD_ERROR_READY) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "Delete failed.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & PAD_BUTTON_A)) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}
	return true;
}

// --- Export (card → GCI on SD) ---

static bool card_manager_export_save(int slot, card_entry *entry, s32 sector_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_FAIL, "No storage device available.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Show progress
	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Exporting save...");
	DrawPublish(msgBox);

	// Open card file
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	card_file cardfile;
	s32 ret = CARD_Open(slot, entry->filename, &cardfile);
	if (ret != CARD_ERROR_READY) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to open card file.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Build GCI header from the raw directory entry in sys_area.
	// CARD_GetStatus transforms some fields (strips banner animation bits),
	// so we read directly from sys_area for a faithful copy.
	GCI gci;
	memset(&gci, 0, sizeof(GCI));
	gci.reserved01 = 0xFF;
	gci.reserved02 = 0xFFFF;
	gci.filesize8 = entry->filesize / 8192;

	bool got_raw = false;
	unsigned char *sysarea = get_card_sys_area(slot);
	if (sysarea) {
		// Find raw directory entry at the specific fileno position
		u32 entry_off = cardfile.filenum * 64;
		GCI *raw = NULL;
		for (int blk = 0; blk < 5; blk++) {
			GCI *candidate = (GCI *)(sysarea + blk * 8192 + entry_off);
			if (memcmp(candidate->gamecode, entry->gamecode, 4) == 0
				&& memcmp(candidate->company, entry->company, 2) == 0
				&& memcmp(candidate->filename, entry->filename, CARD_FILENAMELEN) == 0) {
				raw = candidate;
			}
		}
		if (raw) {
			memcpy(gci.gamecode, raw->gamecode, 4);
			memcpy(gci.company, raw->company, 2);
			memcpy(gci.filename, raw->filename, CARD_FILENAMELEN);
			gci.banner_fmt = raw->banner_fmt;
			gci.time = raw->time;
			gci.icon_addr = raw->icon_addr;
			gci.icon_fmt = raw->icon_fmt;
			gci.icon_speed = raw->icon_speed;
			gci.permission = raw->permission;
			gci.copy_counter = raw->copy_counter;
			gci.comment_addr = raw->comment_addr;
			got_raw = true;
			cm_log("  Read raw direntry from sys_area");
		}
	}
	if (!got_raw) {
		// Fallback to CARD_GetStatus (may lose banner animation bits)
		card_stat cardstat;
		CARD_GetStatus(slot, cardfile.filenum, &cardstat);
		u8 file_attr = 0;
		CARD_GetAttributes(slot, cardfile.filenum, &file_attr);
		memcpy(gci.gamecode, cardstat.gamecode, 4);
		memcpy(gci.company, cardstat.company, 2);
		memcpy(gci.filename, cardstat.filename, CARD_FILENAMELEN);
		gci.banner_fmt = cardstat.banner_fmt;
		gci.time = cardstat.time;
		gci.icon_addr = cardstat.icon_addr;
		gci.icon_fmt = cardstat.icon_fmt;
		gci.icon_speed = cardstat.icon_speed;
		gci.permission = file_attr;
		gci.comment_addr = cardstat.comment_addr;
		cm_log("  WARNING: raw direntry not found, fell back to CARD_GetStatus");
	}

	cm_log("  GCI header: filesize8=%d banner_fmt=0x%02x icon_addr=0x%x icon_fmt=0x%04x icon_speed=0x%04x comment_addr=0x%x perms=0x%02x copy=%d",
		gci.filesize8, gci.banner_fmt, gci.icon_addr, gci.icon_fmt, gci.icon_speed, gci.comment_addr, gci.permission, gci.copy_counter);

	// Read save data from card
	u8 *savedata = (u8 *)memalign(32, entry->filesize);
	if (!savedata) {
		CARD_Close(&cardfile);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Out of memory.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	u8 *read_buf = (u8 *)memalign(32, sector_size);
	bool read_ok = true;
	u32 offset = 0;
	while (offset < entry->filesize) {
		ret = CARD_Read(&cardfile, read_buf, sector_size, offset);
		if (ret != CARD_ERROR_READY) {
			// Retry once
			usleep(2000);
			ret = CARD_Read(&cardfile, read_buf, sector_size, offset);
		}
		cm_log("  CARD_Read offset=%u len=%d ret=%d", offset, sector_size, (int)ret);
		if (ret != CARD_ERROR_READY) {
			read_ok = false;
			break;
		}
		u32 copy_len = entry->filesize - offset;
		if (copy_len > (u32)sector_size) copy_len = sector_size;
		memcpy(savedata + offset, read_buf, copy_len);
		offset += sector_size;
	}
	free(read_buf);
	CARD_Close(&cardfile);

	// Log data fingerprint for verification
	if (read_ok && entry->filesize >= 16) {
		cm_log("  Export data: first4=%02x%02x%02x%02x last4=%02x%02x%02x%02x total=%u",
			savedata[0], savedata[1], savedata[2], savedata[3],
			savedata[entry->filesize-4], savedata[entry->filesize-3],
			savedata[entry->filesize-2], savedata[entry->filesize-1],
			entry->filesize);
	}

	if (!read_ok) {
		free(savedata);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to read card data.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Write GCI file to SD: swiss/saves/<gamecode>-<filename>.gci
	file_handle *outFile = calloc(1, sizeof(file_handle));
	if (!outFile) {
		free(savedata);
		DrawDispose(msgBox);
		return false;
	}

	// Ensure swiss/saves/ directory exists
	file_handle *savesDir = calloc(1, sizeof(file_handle));
	if (savesDir) {
		concat_path(savesDir->name, dev->initial->name, "swiss/saves");
		dev->makeDir(savesDir);
		free(savesDir);
	}

	char gci_name[64];
	snprintf(gci_name, sizeof(gci_name), "swiss/saves/%.4s-%.32s.gci",
		entry->gamecode, entry->filename);
	concat_path(outFile->name, dev->initial->name, gci_name);

	// Check if file already exists
	if (!dev->statFile(outFile)) {
		char overwrite_msg[128];
		snprintf(overwrite_msg, sizeof(overwrite_msg),
			"%.4s-%.32s.gci already exists.\n \nA: Overwrite  |  X: Keep Both  |  B: Cancel",
			entry->gamecode, entry->filename);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_WARN, overwrite_msg);
		DrawPublish(msgBox);
		int choice = 0; // 0=pending, 1=overwrite, 2=keep both, 3=cancel
		while (!choice) {
			u16 btn = padsButtonsHeld();
			if (btn & PAD_BUTTON_A) choice = 1;
			else if (btn & PAD_BUTTON_X) choice = 2;
			else if (btn & PAD_BUTTON_B) choice = 3;
			VIDEO_WaitVSync();
		}
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_X | PAD_BUTTON_B)) {
			VIDEO_WaitVSync();
		}
		DrawDispose(msgBox);
		if (choice == 3) {
			free(savedata);
			free(outFile);
			return false;
		}
		if (choice == 2) {
			// Find next available numbered filename
			for (int n = 2; n <= 99; n++) {
				snprintf(gci_name, sizeof(gci_name), "swiss/saves/%.4s-%.32s (%d).gci",
					entry->gamecode, entry->filename, n);
				concat_path(outFile->name, dev->initial->name, gci_name);
				if (dev->statFile(outFile)) break; // doesn't exist, use it
			}
		}
		else {
			// Overwrite: delete existing
			dev->deleteFile(outFile);
		}
		msgBox = DrawMessageBox(D_INFO, "Exporting save...");
		DrawPublish(msgBox);
	}

	// Combine GCI header + save data into single buffer for one atomic write
	u32 gci_total = sizeof(GCI) + entry->filesize;
	u8 *gcibuf = (u8 *)memalign(32, gci_total);
	if (!gcibuf) {
		free(savedata);
		free(outFile);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Out of memory.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}
	memcpy(gcibuf, &gci, sizeof(GCI));
	memcpy(gcibuf + sizeof(GCI), savedata, entry->filesize);
	free(savedata);

	s32 write_ret = dev->writeFile(outFile, gcibuf, gci_total);
	dev->closeFile(outFile);
	cm_log("  Wrote GCI: %u bytes (header=%u + data=%u), writeFile ret=%d",
		gci_total, (u32)sizeof(GCI), entry->filesize, (int)write_ret);
	free(gcibuf);
	free(outFile);

	DrawDispose(msgBox);
	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Exported %.4s-%.32s.gci",
		entry->gamecode, entry->filename);
	msgBox = DrawMessageBox(D_INFO, done_msg);
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
	DrawDispose(msgBox);
	return true;
}

// --- Import (GCI from SD → card) ---

#define MAX_GCI_FILES 64

typedef struct {
	char path[PATHNAME_MAX];
	char display[48];
	u32 filesize;
} gci_file_entry;

static int card_manager_scan_gci_files(gci_file_entry *gci_files, int max_files) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return 0;

	file_handle *dir = calloc(1, sizeof(file_handle));
	if (!dir) return 0;
	concat_path(dir->name, dev->initial->name, "swiss/saves");

	if (dev->statFile(dir)) {
		free(dir);
		return 0;
	}

	// Read directory
	file_handle *dirEntries = NULL;
	int count = 0;

	s32 num_dir = dev->readDir(dir, &dirEntries, -1);
	if (num_dir > 0 && dirEntries) {
		for (int i = 0; i < num_dir && count < max_files; i++) {
			char *name = getRelativeName(dirEntries[i].name);
			int len = strlen(name);
			if (len > 4 && !strcasecmp(name + len - 4, ".gci")) {
				strlcpy(gci_files[count].path, dirEntries[i].name, PATHNAME_MAX);
				snprintf(gci_files[count].display, sizeof(gci_files[count].display),
					"%.47s", name);
				gci_files[count].filesize = dirEntries[i].size;
				count++;
			}
		}
		free(dirEntries);
	}
	free(dir);
	return count;
}

static int card_manager_pick_gci(gci_file_entry *files, int num_files) {
	if (num_files == 0) {
		uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "No .gci files found in swiss/saves/");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return -1;
	}

	int cursor = 0;
	int scroll = 0;
	int max_visible = 8;

	while (1) {
		uiDrawObj_t *container = DrawEmptyBox(40, 80, 600, 400);
		DrawAddChild(container, DrawStyledLabel(640 / 2, 90,
			"Select GCI to import", 0.7f, ALIGN_CENTER, defaultColor));

		int visible = num_files - scroll;
		if (visible > max_visible) visible = max_visible;

		for (int i = 0; i < visible; i++) {
			int idx = scroll + i;
			int y = 120 + i * 30 + 15;
			GXColor color = (idx == cursor) ?
				(GXColor){96, 107, 164, 255} : defaultColor;
			if (idx == cursor) {
				DrawAddChild(container, DrawEmptyColouredBox(
					50, 120 + i * 30, 590, 120 + (i + 1) * 30,
					(GXColor){96, 107, 164, 80}));
			}
			DrawAddChild(container, DrawStyledLabel(60, y,
				files[idx].display, 0.55f, ALIGN_LEFT, color));
		}

		DrawAddChild(container, DrawStyledLabel(640 / 2, 385,
			"A: Import  |  B: Cancel", 0.55f, ALIGN_CENTER, defaultColor));
		DrawPublish(container);

		while (1) {
			VIDEO_WaitVSync();
			u16 btns = padsButtonsHeld();
			if (!btns) continue;

			bool redraw = false;
			if ((btns & PAD_BUTTON_UP) && cursor > 0) {
				cursor--;
				if (cursor < scroll) scroll = cursor;
				redraw = true;
			}
			if ((btns & PAD_BUTTON_DOWN) && cursor < num_files - 1) {
				cursor++;
				if (cursor >= scroll + max_visible) scroll = cursor - max_visible + 1;
				redraw = true;
			}
			if (btns & PAD_BUTTON_A) {
				while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
				DrawDispose(container);
				return cursor;
			}
			if (btns & PAD_BUTTON_B) {
				while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
				DrawDispose(container);
				return -1;
			}
			if (redraw) {
				while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN)) {
					VIDEO_WaitVSync();
				}
				break;
			}
		}
		DrawDispose(container);
	}
}

static bool card_manager_import_gci(int slot, gci_file_entry *gci_entry, s32 sector_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Importing save...");
	DrawPublish(msgBox);

	// Read GCI file
	file_handle *inFile = calloc(1, sizeof(file_handle));
	if (!inFile) {
		DrawDispose(msgBox);
		return false;
	}
	strlcpy(inFile->name, gci_entry->path, PATHNAME_MAX);

	if (dev->statFile(inFile)) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "File not found.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		free(inFile);
		return false;
	}

	u32 total_size = inFile->size;
	if (total_size <= sizeof(GCI)) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Invalid GCI file (too small).");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		free(inFile);
		return false;
	}

	u8 *filebuf = (u8 *)memalign(32, total_size);
	if (!filebuf) {
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Out of memory.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		free(inFile);
		return false;
	}

	s32 read_ret = dev->readFile(inFile, filebuf, total_size);
	dev->closeFile(inFile);
	free(inFile);
	cm_log("  Read GCI: requested=%u, readFile ret=%d", total_size, (int)read_ret);

	// Parse GCI header
	GCI *gci = (GCI *)filebuf;
	u8 *savedata = filebuf + sizeof(GCI);
	u32 save_len = total_size - sizeof(GCI);
	u32 expected_len = gci->filesize8 * 8192;

	cm_log("  GCI header: filesize8=%d banner_fmt=0x%02x icon_addr=0x%x icon_fmt=0x%04x icon_speed=0x%04x comment_addr=0x%x perms=0x%02x",
		gci->filesize8, gci->banner_fmt, gci->icon_addr, gci->icon_fmt, gci->icon_speed, gci->comment_addr, gci->permission);

	if (expected_len >= 16) {
		cm_log("  Import data: first4=%02x%02x%02x%02x last4=%02x%02x%02x%02x total=%u",
			savedata[0], savedata[1], savedata[2], savedata[3],
			savedata[expected_len-4], savedata[expected_len-3],
			savedata[expected_len-2], savedata[expected_len-1],
			expected_len);
	}

	if (save_len != expected_len) {
		free(filebuf);
		DrawDispose(msgBox);
		char errmsg[128];
		snprintf(errmsg, sizeof(errmsg), "GCI size mismatch: got %dK, expected %dK",
			(int)(save_len / 1024), (int)(expected_len / 1024));
		msgBox = DrawMessageBox(D_FAIL, errmsg);
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Set gamecode/company from GCI
	char gamecode[8], company[4];
	memset(gamecode, 0, sizeof(gamecode));
	memset(company, 0, sizeof(company));
	memcpy(gamecode, gci->gamecode, 4);
	memcpy(company, gci->company, 2);
	CARD_SetGamecode(gamecode);
	CARD_SetCompany(company);

	// Get filename from GCI
	char filename[CARD_FILENAMELEN + 1];
	memset(filename, 0, sizeof(filename));
	memcpy(filename, gci->filename, CARD_FILENAMELEN);

	// Open or create file on card
	card_file cardfile;
	bool is_overwrite = false;
	s32 ret = CARD_Open(slot, filename, &cardfile);
	if (ret == CARD_ERROR_NOFILE) {
		ret = CARD_Create(slot, filename, expected_len, &cardfile);
		if (ret != CARD_ERROR_READY) {
			free(filebuf);
			DrawDispose(msgBox);
			msgBox = DrawMessageBox(D_FAIL, "Failed to create file on card.\nCard may be full.");
			DrawPublish(msgBox);
			while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
			while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
			DrawDispose(msgBox);
			return false;
		}
		ret = CARD_Open(slot, filename, &cardfile);
		cm_log("  Created new file on card");
	}
	else if (ret == CARD_ERROR_READY) {
		// File exists — confirm overwrite
		CARD_Close(&cardfile);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_WARN, "Save already exists on card.\n \nA: Overwrite  |  B: Cancel");
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
		if (choice == 2) {
			free(filebuf);
			return false;
		}
		msgBox = DrawMessageBox(D_INFO, "Importing save...");
		DrawPublish(msgBox);
		// Re-open for writing
		ret = CARD_Open(slot, filename, &cardfile);
		is_overwrite = true;
		cm_log("  Overwriting existing file on card");
	}

	if (ret != CARD_ERROR_READY) {
		free(filebuf);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to open card file.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Write data sector by sector
	u8 *write_buf = (u8 *)memalign(32, sector_size);
	bool write_ok = true;
	u32 offset = 0;
	while (offset < expected_len) {
		u32 chunk = expected_len - offset;
		if (chunk > (u32)sector_size) chunk = sector_size;
		memset(write_buf, 0, sector_size);
		memcpy(write_buf, savedata + offset, chunk);
		ret = CARD_Write(&cardfile, write_buf, sector_size, offset);
		cm_log("  CARD_Write offset=%u len=%d ret=%d", offset, sector_size, (int)ret);
		if (ret != CARD_ERROR_READY) {
			write_ok = false;
			break;
		}
		offset += sector_size;
	}
	free(write_buf);

	if (!write_ok) {
		CARD_Close(&cardfile);
		free(filebuf);
		DrawDispose(msgBox);
		msgBox = DrawMessageBox(D_FAIL, "Failed to write data to card.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}

	// Verify write by reading back and comparing
	u8 *verify_buf = (u8 *)memalign(32, sector_size);
	bool verify_ok = true;
	if (verify_buf) {
		offset = 0;
		while (offset < expected_len) {
			ret = CARD_Read(&cardfile, verify_buf, sector_size, offset);
			if (ret != CARD_ERROR_READY) {
				cm_log("  VERIFY: CARD_Read failed at offset=%u ret=%d", offset, (int)ret);
				verify_ok = false;
				break;
			}
			u32 chunk = expected_len - offset;
			if (chunk > (u32)sector_size) chunk = sector_size;
			if (memcmp(verify_buf, savedata + offset, chunk) != 0) {
				cm_log("  VERIFY: DATA MISMATCH at offset=%u!", offset);
				// Log first differing byte
				for (u32 j = 0; j < chunk; j++) {
					if (verify_buf[j] != savedata[offset + j]) {
						cm_log("    byte %u: wrote=0x%02x read=0x%02x",
							offset + j, savedata[offset + j], verify_buf[j]);
						break;
					}
				}
				verify_ok = false;
				break;
			}
			offset += sector_size;
		}
		free(verify_buf);
		if (verify_ok) {
			cm_log("  VERIFY: all %u bytes match", expected_len);
		}
	}

	// Patch raw directory entry in sys_area to restore ALL GCI fields.
	// CARD_SetStatus has bugs: it rejects icon_addr >= 512, overwrites
	// timestamp with current time, and strips banner animation bits.
	// Instead, we find the raw 64-byte directory entry in the card's
	// system area and patch it directly, then flush via CARD_SetAttributes.
	if (!is_overwrite) {
		unsigned char *sysarea = get_card_sys_area(slot);
		int patches = 0;
		if (sysarea) {
			// Patch ALL matching directory entries in sys_area.
			// There are two directory blocks (at offsets 0x2000 and 0x4000).
			// We must patch BOTH because we don't know which one curr_dir
			// points to — if we only patch the backup, __card_updatedir
			// would read unpatched values from curr_dir and discard our fix.
			u32 entry_off = cardfile.filenum * 64;
			for (int blk = 0; blk < 5; blk++) {
				GCI *candidate = (GCI *)(sysarea + blk * 8192 + entry_off);
				if (memcmp(candidate->gamecode, gci->gamecode, 4) == 0
					&& memcmp(candidate->company, gci->company, 2) == 0
					&& memcmp(candidate->filename, gci->filename, CARD_FILENAMELEN) == 0) {
					if (patches == 0) {
						cm_log("  Before: banner_fmt=0x%02x time=0x%08x icon_addr=0x%08x icon_fmt=0x%04x icon_speed=0x%04x perm=0x%02x copy=%d comment=0x%08x",
							candidate->banner_fmt, candidate->time, candidate->icon_addr,
							candidate->icon_fmt, candidate->icon_speed, candidate->permission,
							candidate->copy_counter, candidate->comment_addr);
					}
					candidate->banner_fmt = gci->banner_fmt;
					candidate->time = gci->time;
					candidate->icon_addr = gci->icon_addr;
					candidate->icon_fmt = gci->icon_fmt;
					candidate->icon_speed = gci->icon_speed;
					candidate->comment_addr = gci->comment_addr;
					candidate->permission = gci->permission;
					candidate->copy_counter = gci->copy_counter;
					patches++;
				}
			}
		}
		if (patches > 0) {
			cm_log("  Patched %d direntry copies in sys_area", patches);
			cm_log("  After:  banner_fmt=0x%02x time=0x%08x icon_addr=0x%08x icon_fmt=0x%04x icon_speed=0x%04x perm=0x%02x copy=%d comment=0x%08x",
				gci->banner_fmt, gci->time, gci->icon_addr,
				gci->icon_fmt, gci->icon_speed, gci->permission,
				gci->copy_counter, gci->comment_addr);
			// Flush directory to card via CARD_SetAttributes (triggers __card_updatedir)
			s32 attr_ret = CARD_SetAttributes(slot, cardfile.filenum, gci->permission);
			cm_log("  CARD_SetAttributes (flush) ret=%d", (int)attr_ret);
		} else {
			cm_log("  WARNING: Could not find direntry in sys_area, falling back to CARD_SetStatus");
			card_stat cardstat;
			CARD_GetStatus(slot, cardfile.filenum, &cardstat);
			cardstat.banner_fmt = gci->banner_fmt;
			cardstat.icon_addr = gci->icon_addr;
			cardstat.icon_fmt = gci->icon_fmt;
			cardstat.icon_speed = gci->icon_speed;
			cardstat.comment_addr = gci->comment_addr;
			u64 real_time = gettime();
			settime(secs_to_ticks(gci->time));
			s32 status_ret = CARD_SetStatus(slot, cardfile.filenum, &cardstat);
			settime(real_time);
			cm_log("  CARD_SetStatus ret=%d", (int)status_ret);
			CARD_SetAttributes(slot, cardfile.filenum, gci->permission);
		}
	} else {
		cm_log("  Skipping directory update (overwrite mode)");
	}

	CARD_Close(&cardfile);
	free(filebuf);

	DrawDispose(msgBox);
	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Imported \"%s\" successfully.", filename);
	msgBox = DrawMessageBox(D_INFO, done_msg);
	DrawPublish(msgBox);
	while (!(padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B))) { VIDEO_WaitVSync(); }
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_BUTTON_B)) { VIDEO_WaitVSync(); }
	DrawDispose(msgBox);
	return true;
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

		if ((btns & PAD_BUTTON_A) && card_present && num_entries > 0) {
			while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
			cm_log("Exporting [%d] \"%s\" from Slot %c",
				cursor, entries[cursor].filename,
				slot == CARD_SLOTA ? 'A' : 'B');
			if (card_manager_export_save(slot, &entries[cursor], sector_size)) {
				cm_log("Export successful");
			}
			else {
				cm_log("Export failed or cancelled");
			}
			needs_redraw = true;
			continue;
		}

		if ((btns & PAD_BUTTON_X) && card_present) {
			while (padsButtonsHeld() & PAD_BUTTON_X) { VIDEO_WaitVSync(); }
			gci_file_entry *gci_files = calloc(MAX_GCI_FILES, sizeof(gci_file_entry));
			if (gci_files) {
				int num_gci = card_manager_scan_gci_files(gci_files, MAX_GCI_FILES);
				cm_log("Found %d GCI files for import", num_gci);
				int pick = card_manager_pick_gci(gci_files, num_gci);
				if (pick >= 0) {
					cm_log("Importing \"%s\" to Slot %c",
						gci_files[pick].display,
						slot == CARD_SLOTA ? 'A' : 'B');
					if (card_manager_import_gci(slot, &gci_files[pick], sector_size)) {
						cm_log("Import successful");
						needs_reload = true;
					}
					else {
						cm_log("Import failed or cancelled");
						needs_redraw = true;
					}
				}
				else {
					needs_redraw = true;
				}
				free(gci_files);
			}
			continue;
		}

		if ((btns & PAD_TRIGGER_Z) && card_present && num_entries > 0) {
			while (padsButtonsHeld() & PAD_TRIGGER_Z) { VIDEO_WaitVSync(); }
			if (card_manager_confirm_delete(entries[cursor].filename)) {
				cm_log("Deleting [%d] \"%s\" from Slot %c",
					cursor, entries[cursor].filename,
					slot == CARD_SLOTA ? 'A' : 'B');
				if (card_manager_delete_save(slot, &entries[cursor])) {
					cm_log("Delete successful");
					needs_reload = true;
				}
				else {
					cm_log("Delete failed");
				}
			}
			else {
				cm_log("Delete cancelled for \"%s\"", entries[cursor].filename);
				needs_redraw = true;
			}
			continue;
		}

		if (btns & PAD_BUTTON_B)
			break;

		while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN |
			PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT |
			PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_X |
			PAD_TRIGGER_L | PAD_TRIGGER_R | PAD_TRIGGER_Z)) {
			VIDEO_WaitVSync();
		}
	}

	cm_log("Closed card manager");
	cm_log_close();
	if (pagePanel) DrawDispose(pagePanel);
	while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
}
