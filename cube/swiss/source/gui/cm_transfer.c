/* cm_transfer.c
	- Card Manager export, import, and delete operations
	by Swiss contributors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <ogc/timesupp.h>
#include "cm_internal.h"
#include "IPLFontWrite.h"
#include "swiss.h"

// --- Delete ---

bool card_manager_confirm_delete(const char *filename) {
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

bool card_manager_delete_save(int slot, card_entry *entry) {
	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Deleting save...\nDo not remove the Memory Card.");
	DrawPublish(msgBox);
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	s32 ret = CARD_Delete(slot, entry->filename);
	DrawDispose(msgBox);
	if (ret != CARD_ERROR_READY) {
		msgBox = DrawMessageBox(D_FAIL, "Delete failed.");
		DrawPublish(msgBox);
		while (!(padsButtonsHeld() & PAD_BUTTON_A)) { VIDEO_WaitVSync(); }
		while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
		DrawDispose(msgBox);
		return false;
	}
	return true;
}

// --- Export (card → GCI on SD) ---

bool card_manager_export_save(int slot, card_entry *entry, s32 sector_size) {
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

	// Show progress (§6.3: indicate when accessing Memory Card)
	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Exporting save...\nDo not remove the Memory Card.");
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

int card_manager_scan_gci_files(gci_file_entry *gci_files, int max_files) {
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

int card_manager_pick_gci(gci_file_entry *files, int num_files) {
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

bool card_manager_import_gci(int slot, gci_file_entry *gci_entry, s32 sector_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	uiDrawObj_t *msgBox = DrawMessageBox(D_INFO, "Importing save...\nDo not remove the Memory Card.");
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
		msgBox = DrawMessageBox(D_INFO, "Importing save...\nDo not remove the Memory Card.");
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
