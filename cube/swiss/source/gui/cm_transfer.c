/* cm_transfer.c
	- Card Manager export, import, and delete operations
	by Swiss contributors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <gccore.h>
#include <ogc/timesupp.h>
#include "cm_internal.h"
#include "IPLFontWrite.h"
#include "swiss.h"

// --- Delete ---

bool card_manager_confirm_delete(const char *filename) {
	char msg[256];
	sprintf(msg, "Delete \"%s\"?\n \nPress L + A to confirm, or B to cancel.", filename);
	uiDrawObj_t *msgBox = cm_draw_message(msg);
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
	uiDrawObj_t *msgBox = cm_draw_message("Deleting save...\nDo not remove the Memory Card.");
	DrawPublish(msgBox);
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	s32 ret = CARD_Delete(slot, entry->filename);
	DrawDispose(msgBox);
	if (ret == CARD_ERROR_READY)
		cm_gfx_cache_invalidate(entry);
	if (ret != CARD_ERROR_READY) {
		msgBox = cm_draw_message("Delete failed.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A);
		DrawDispose(msgBox);
		return false;
	}
	msgBox = cm_draw_message("Save deleted.");
	DrawPublish(msgBox);
	cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
	DrawDispose(msgBox);
	return true;
}

// --- Shared GCI-to-SD writer ---
// Writes a GCI header + save data to swiss/saves/ on SD.
// Handles overwrite/keep-both dialog and success message.

bool cm_write_gci_to_sd(GCI *gci, u8 *savedata, u32 save_len) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) {
		uiDrawObj_t *msgBox = cm_draw_message("No storage device available.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}

	char gamecode[5], filename[CARD_FILENAMELEN + 1];
	memcpy(gamecode, gci->gamecode, 4); gamecode[4] = '\0';
	memset(filename, 0, sizeof(filename));
	memcpy(filename, gci->filename, CARD_FILENAMELEN);

	file_handle *outFile = calloc(1, sizeof(file_handle));
	if (!outFile) return false;

	// Ensure swiss/saves/ directory exists
	file_handle *savesDir = calloc(1, sizeof(file_handle));
	if (savesDir) {
		concat_path(savesDir->name, dev->initial->name, "swiss/saves");
		dev->makeDir(savesDir);
		free(savesDir);
	}

	char gci_name[64];
	snprintf(gci_name, sizeof(gci_name), "swiss/saves/%.4s-%.32s.gci",
		gamecode, filename);
	concat_path(outFile->name, dev->initial->name, gci_name);

	// Check if file already exists
	if (!dev->statFile(outFile)) {
		char overwrite_msg[128];
		snprintf(overwrite_msg, sizeof(overwrite_msg),
			"%.4s-%.32s.gci already exists.\n \nA: Overwrite  |  X: Keep Both  |  B: Cancel",
			gamecode, filename);
		uiDrawObj_t *msgBox = cm_draw_message(overwrite_msg);
		DrawPublish(msgBox);
		u16 btn = cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_X | PAD_BUTTON_B);
		DrawDispose(msgBox);
		if (btn & PAD_BUTTON_B) {
			free(outFile);
			return false;
		}
		if (btn & PAD_BUTTON_X) {
			for (int n = 2; n <= 99; n++) {
				snprintf(gci_name, sizeof(gci_name), "swiss/saves/%.4s-%.32s (%d).gci",
					gamecode, filename, n);
				concat_path(outFile->name, dev->initial->name, gci_name);
				if (dev->statFile(outFile)) break;
			}
		}
	}

	uiDrawObj_t *msgBox = cm_draw_message("Writing file...");
	DrawPublish(msgBox);

	u32 gci_total = sizeof(GCI) + save_len;
	u8 *gcibuf = (u8 *)memalign(32, gci_total);
	if (!gcibuf) {
		free(outFile);
		DrawDispose(msgBox);
		msgBox = cm_draw_message("Out of memory.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}
	memcpy(gcibuf, gci, sizeof(GCI));
	memcpy(gcibuf + sizeof(GCI), savedata, save_len);

	s32 write_ret = dev->writeFile(outFile, gcibuf, gci_total);
	dev->closeFile(outFile);
	free(gcibuf);
	free(outFile);

	DrawDispose(msgBox);
	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Exported %.4s-%.32s.gci",
		gamecode, filename);
	msgBox = cm_draw_message(done_msg);
	DrawPublish(msgBox);
	cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
	DrawDispose(msgBox);
	return true;
}

// --- Export (card → GCI on SD) ---

bool card_manager_export_save(int slot, card_entry *entry, s32 sector_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	int total_sectors = (entry->filesize + sector_size - 1) / sector_size;
	cm_led_progress("Exporting save...\nDo not remove the Memory Card.", 0, total_sectors);

	// Open card file
	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	card_file cardfile;
	s32 ret = CARD_Open(slot, entry->filename, &cardfile);
	if (ret != CARD_ERROR_READY) {
		cm_led_hide();
		uiDrawObj_t *msgBox = cm_draw_message("Failed to open card file.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
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
	}

	// Read save data from card
	u8 *savedata = (u8 *)memalign(32, entry->filesize);
	if (!savedata) {
		CARD_Close(&cardfile);
		cm_led_hide();
		uiDrawObj_t *msgBox = cm_draw_message("Out of memory.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}

	u8 *read_buf = (u8 *)memalign(32, sector_size);
	bool read_ok = true;
	u32 offset = 0;
	int cur_sector = 0;
	while (offset < entry->filesize) {
		cm_led_progress("Exporting save...\nDo not remove the Memory Card.",
			cur_sector, total_sectors);
		ret = CARD_Read(&cardfile, read_buf, sector_size, offset);
		if (ret != CARD_ERROR_READY) {
			read_ok = false;
			break;
		}
		u32 copy_len = entry->filesize - offset;
		if (copy_len > (u32)sector_size) copy_len = sector_size;
		memcpy(savedata + offset, read_buf, copy_len);
		offset += sector_size;
		cur_sector++;
	}
	free(read_buf);
	CARD_Close(&cardfile);
	cm_led_hide();

	if (!read_ok) {
		free(savedata);
		uiDrawObj_t *errBox = cm_draw_message("Failed to read card data.");
		DrawPublish(errBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(errBox);
		return false;
	}

	bool result = cm_write_gci_to_sd(&gci, savedata, entry->filesize);
	free(savedata);
	return result;
}

// --- Import (GCI from SD → card) ---

// Parse a single GCI file's header, comment, and first-frame icon for display
static void cm_parse_gci_file(gci_file_entry *gci, DEVICEHANDLER_INTERFACE *dev) {
	file_handle *f = calloc(1, sizeof(file_handle));
	if (!f) return;
	strlcpy(f->name, gci->path, PATHNAME_MAX);

	if (dev->statFile(f) || f->size <= sizeof(GCI)) {
		free(f);
		return;
	}

	u32 total = f->size;
	u8 *buf = (u8 *)memalign(32, total);
	if (!buf) { free(f); return; }

	dev->readFile(f, buf, total);
	dev->closeFile(f);
	free(f);

	GCI *hdr = (GCI *)buf;
	u8 *savedata = buf + sizeof(GCI);
	u32 save_len = total - sizeof(GCI);

	// Populate entry metadata from GCI header
	card_entry *e = &gci->entry;
	memcpy(e->gamecode, hdr->gamecode, 4); e->gamecode[4] = '\0';
	memcpy(e->company, hdr->company, 2); e->company[2] = '\0';
	snprintf(e->filename, sizeof(e->filename), "%.*s", CARD_FILENAMELEN, hdr->filename);
	e->filesize = hdr->filesize8 * 8192;
	e->blocks = hdr->filesize8;
	e->permissions = hdr->permission;
	e->time = hdr->time;
	e->banner_fmt = hdr->banner_fmt;
	e->icon_addr = hdr->icon_addr;
	e->icon_fmt = hdr->icon_fmt;
	e->icon_speed = hdr->icon_speed;

	// Parse comment block from save data
	if (hdr->comment_addr != (u32)-1)
		cm_parse_comment(savedata, save_len, hdr->comment_addr, e);

	// Parse graphics (banner + icons) from save data at icon_addr
	if (hdr->icon_addr != (u32)-1 && hdr->icon_addr < save_len) {
		u8 *gfx = savedata + hdr->icon_addr;
		u32 gfx_len = save_len - hdr->icon_addr;
		cm_parse_save_graphics(gfx, gfx_len,
			hdr->banner_fmt, hdr->icon_fmt, hdr->icon_speed, e);
	}

	free(buf);
}

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

	file_handle *dirEntries = NULL;
	int count = 0;

	s32 num_dir = dev->readDir(dir, &dirEntries, -1);
	if (num_dir > 0 && dirEntries) {
		for (int i = 0; i < num_dir && count < max_files; i++) {
			char *name = getRelativeName(dirEntries[i].name);
			int len = strlen(name);
			if (len > 4 && !strcasecmp(name + len - 4, ".gci")) {
				memset(&gci_files[count], 0, sizeof(gci_file_entry));
				strlcpy(gci_files[count].path, dirEntries[i].name, PATHNAME_MAX);
				gci_files[count].filesize = dirEntries[i].size;
				// Parse header, comment, and graphics for rich display
				cm_parse_gci_file(&gci_files[count], dev);
				count++;
			}
		}
		free(dirEntries);
	}
	free(dir);
	return count;
}

void card_manager_free_gci_files(gci_file_entry *gci_files, int count) {
	for (int i = 0; i < count; i++) {
		card_manager_free_graphics(&gci_files[i].entry, 1);
	}
}

// --- GCI picker ---
#define GCI_PICK_ROW_H 36
#define GCI_PICK_ICON_SZ 32
#define GCI_PICK_MAX_VIS 7

// GC epoch is 2000-01-01 00:00:00 UTC
#define GC_EPOCH_OFFSET 946684800ULL

static void cm_format_gc_date(u32 gc_time, char *buf, int buflen, bool with_time) {
	if (gc_time == 0 || gc_time == (u32)-1) {
		snprintf(buf, buflen, "----");
		return;
	}
	time_t unix_time = (time_t)((u64)gc_time + GC_EPOCH_OFFSET);
	struct tm *t = gmtime(&unix_time);
	if (!t) { snprintf(buf, buflen, "----"); return; }
	if (with_time)
		snprintf(buf, buflen, "%04d-%02d-%02d %02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min);
	else
		snprintf(buf, buflen, "%04d-%02d-%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

static int cm_gci_sort_date_desc(const void *a, const void *b) {
	const gci_file_entry *ga = (const gci_file_entry *)a;
	const gci_file_entry *gb = (const gci_file_entry *)b;
	if (gb->entry.time > ga->entry.time) return 1;
	if (gb->entry.time < ga->entry.time) return -1;
	return 0;
}

// Shared list input handler: returns action
// -2 = no action, -1 = B pressed, >=0 = A pressed
// Updates *cursor and *scroll, signals *redraw
static int cm_list_input(int *cursor, int *scroll, int count, int max_vis) {
	u16 btns = padsButtonsHeld();
	s8 stickY = padsStickY();
	bool has_input = btns || stickY > 16 || stickY < -16;
	if (!has_input) return -2;

	bool moved = false;
	if (((btns & PAD_BUTTON_UP) || stickY > 16) && *cursor > 0) {
		(*cursor)--;
		if (*cursor < *scroll) *scroll = *cursor;
		moved = true;
	}
	if (((btns & PAD_BUTTON_DOWN) || stickY < -16) && *cursor < count - 1) {
		(*cursor)++;
		if (*cursor >= *scroll + max_vis) *scroll = *cursor - max_vis + 1;
		moved = true;
	}
	if (btns & (PAD_BUTTON_LEFT | PAD_TRIGGER_L)) {
		*cursor -= max_vis;
		if (*cursor < 0) *cursor = 0;
		if (*cursor < *scroll) *scroll = *cursor;
		moved = true;
	}
	if (btns & (PAD_BUTTON_RIGHT | PAD_TRIGGER_R)) {
		*cursor += max_vis;
		if (*cursor >= count) *cursor = count - 1;
		if (*cursor >= *scroll + max_vis) *scroll = *cursor - max_vis + 1;
		moved = true;
	}
	if (btns & PAD_BUTTON_A) {
		while (padsButtonsHeld() & PAD_BUTTON_A) { VIDEO_WaitVSync(); }
		return *cursor;
	}
	if (btns & PAD_BUTTON_B) {
		while (padsButtonsHeld() & PAD_BUTTON_B) { VIDEO_WaitVSync(); }
		return -1;
	}
	if (moved) {
		if (stickY > 16 || stickY < -16) {
			int wait = abs(stickY) > 64 ? 2 : 5;
			for (int w = 0; w < wait; w++) VIDEO_WaitVSync();
		} else {
			while (padsButtonsHeld() & (PAD_BUTTON_UP | PAD_BUTTON_DOWN |
				PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT |
				PAD_TRIGGER_L | PAD_TRIGGER_R)) {
				VIDEO_WaitVSync();
			}
		}
	}
	return moved ? -3 : -2;  // -3 = redraw needed
}

// Game group: groups GCI files by gamecode
typedef struct {
	char gamecode[5];
	char game_name[33];
	int first_idx;		// Index into the files array
	int count;			// Number of saves for this game
	card_entry *rep;	// Representative entry (for icon)
} gci_game_group;

#define MAX_GAME_GROUPS 64

// Build game groups from sorted file list
static int cm_build_game_groups(gci_file_entry *files, int num_files,
	gci_game_group *groups, int max_groups) {
	int ng = 0;
	for (int i = 0; i < num_files && ng < max_groups; i++) {
		// Check if this gamecode already has a group
		int found = -1;
		for (int g = 0; g < ng; g++) {
			if (strcmp(groups[g].gamecode, files[i].entry.gamecode) == 0) {
				found = g;
				break;
			}
		}
		if (found >= 0) {
			groups[found].count++;
			// Use the most recent save's name as representative
			if (files[i].entry.time > files[groups[found].first_idx].entry.time) {
				groups[found].rep = &files[i].entry;
				if (files[i].entry.game_name[0])
					strlcpy(groups[found].game_name, files[i].entry.game_name,
						sizeof(groups[found].game_name));
			}
		} else {
			strlcpy(groups[ng].gamecode, files[i].entry.gamecode, 5);
			strlcpy(groups[ng].game_name,
				files[i].entry.game_name[0] ? files[i].entry.game_name : files[i].entry.filename,
				sizeof(groups[ng].game_name));
			groups[ng].first_idx = i;
			groups[ng].count = 1;
			groups[ng].rep = &files[i].entry;
			ng++;
		}
	}
	return ng;
}

// --- Backup file operations ---

enum {
	BK_ACT_IMPORT,
	BK_ACT_RENAME,
	BK_ACT_DUPLICATE,
	BK_ACT_DELETE,
	BK_ACT_COUNT
};

static DEVICEHANDLER_INTERFACE *cm_get_sd_dev(void) {
	return devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
}

bool cm_backup_rename(gci_file_entry *gci) {
	DEVICEHANDLER_INTERFACE *dev = cm_get_sd_dev();
	if (!dev) return false;

	// Extract current basename (without path and .gci extension)
	char *rel = getRelativeName(gci->path);
	char newname[128];
	strlcpy(newname, rel, sizeof(newname));
	int len = strlen(newname);
	if (len > 4 && !strcasecmp(newname + len - 4, ".gci"))
		newname[len - 4] = '\0';

	DrawGetTextEntry(ENTRYMODE_ALPHA | ENTRYMODE_FILE, "Rename backup", newname, sizeof(newname) - 5);

	// If empty or unchanged, skip
	if (!newname[0]) return false;
	char full_new[PATHNAME_MAX];
	char gci_name[160];
	snprintf(gci_name, sizeof(gci_name), "swiss/saves/%s.gci", newname);
	concat_path(full_new, dev->initial->name, gci_name);
	if (strcmp(full_new, gci->path) == 0) return false;

	// Check if target exists
	file_handle *check = calloc(1, sizeof(file_handle));
	if (check) {
		strlcpy(check->name, full_new, PATHNAME_MAX);
		if (!dev->statFile(check)) {
			free(check);
			uiDrawObj_t *msgBox = cm_draw_message("A file with that name already exists.");
			DrawPublish(msgBox);
			cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
			DrawDispose(msgBox);
			return false;
		}
		free(check);
	}

	// Rename = read old, write new, delete old
	file_handle *src = calloc(1, sizeof(file_handle));
	if (!src) return false;
	strlcpy(src->name, gci->path, PATHNAME_MAX);
	if (dev->statFile(src)) { free(src); return false; }

	u32 total = src->size;
	u8 *buf = (u8 *)memalign(32, total);
	if (!buf) { free(src); return false; }
	dev->readFile(src, buf, total);
	dev->closeFile(src);

	file_handle *dst = calloc(1, sizeof(file_handle));
	if (!dst) { free(buf); free(src); return false; }
	strlcpy(dst->name, full_new, PATHNAME_MAX);
	dev->writeFile(dst, buf, total);
	dev->closeFile(dst);
	free(buf);
	free(dst);

	dev->deleteFile(src);
	free(src);

	strlcpy(gci->path, full_new, PATHNAME_MAX);
	return true;
}

bool cm_backup_delete(gci_file_entry *gci) {
	DEVICEHANDLER_INTERFACE *dev = cm_get_sd_dev();
	if (!dev) return false;

	char *rel = getRelativeName(gci->path);
	char msg[256];
	snprintf(msg, sizeof(msg), "Delete backup \"%s\"?\n \nPress L + A to confirm, or B to cancel.", rel);
	uiDrawObj_t *msgBox = cm_draw_message(msg);
	DrawPublish(msgBox);

	bool confirmed = false;
	while (1) {
		u16 btns = padsButtonsHeld();
		if ((btns & (PAD_BUTTON_A | PAD_TRIGGER_L)) == (PAD_BUTTON_A | PAD_TRIGGER_L)) {
			confirmed = true; break;
		}
		if (btns & PAD_BUTTON_B) break;
		VIDEO_WaitVSync();
	}
	while (padsButtonsHeld() & (PAD_BUTTON_A | PAD_TRIGGER_L | PAD_BUTTON_B)) {
		VIDEO_WaitVSync();
	}
	DrawDispose(msgBox);

	if (!confirmed) return false;

	file_handle *f = calloc(1, sizeof(file_handle));
	if (!f) return false;
	strlcpy(f->name, gci->path, PATHNAME_MAX);
	dev->deleteFile(f);
	free(f);
	return true;
}

static bool cm_backup_duplicate(gci_file_entry *gci) {
	DEVICEHANDLER_INTERFACE *dev = cm_get_sd_dev();
	if (!dev) return false;

	file_handle *src = calloc(1, sizeof(file_handle));
	if (!src) return false;
	strlcpy(src->name, gci->path, PATHNAME_MAX);
	if (dev->statFile(src)) { free(src); return false; }

	u32 total = src->size;
	u8 *buf = (u8 *)memalign(32, total);
	if (!buf) { free(src); return false; }
	dev->readFile(src, buf, total);
	dev->closeFile(src);
	free(src);

	// Find next available numbered name
	char *rel = getRelativeName(gci->path);
	char base[128];
	strlcpy(base, rel, sizeof(base));
	int len = strlen(base);
	if (len > 4 && !strcasecmp(base + len - 4, ".gci"))
		base[len - 4] = '\0';

	file_handle *dst = calloc(1, sizeof(file_handle));
	if (!dst) { free(buf); return false; }
	char gci_name[160];
	for (int n = 2; n <= 99; n++) {
		snprintf(gci_name, sizeof(gci_name), "swiss/saves/%s (%d).gci", base, n);
		concat_path(dst->name, dev->initial->name, gci_name);
		if (dev->statFile(dst)) break;  // doesn't exist, use it
	}

	dev->writeFile(dst, buf, total);
	dev->closeFile(dst);
	free(dst);
	free(buf);
	return true;
}

// --- Backup manager save list (level 2) ---
// Returns: 1 = card was modified (import), 0 = files changed on SD, -1 = back (no changes)
static int cm_backup_save_list(gci_file_entry **pfiles, int *pnum,
	gci_game_group *group, cm_panel *panel) {

	int cursor = 0, scroll = 0;
	int box_x1 = 40, box_y1 = 80, box_x2 = 600, box_y2 = 400;
	int list_top = 110;
	int list_w = box_x2 - box_x1 - 20;
	bool card_modified = false;
	bool files_changed = false;

	while (1) {
		// After file operations, rescan GCI files in place
		if (files_changed) {
			card_manager_free_gci_files(*pfiles, *pnum);
			*pnum = card_manager_scan_gci_files(*pfiles, MAX_GCI_FILES);
			qsort(*pfiles, *pnum, sizeof(gci_file_entry), cm_gci_sort_date_desc);
			files_changed = false;
		}

		// Rebuild indices for this game (may change after delete/rename)
		gci_file_entry *files = *pfiles;
		int num_files = *pnum;
		int indices[MAX_GCI_FILES];
		int count = 0;
		for (int i = 0; i < num_files && count < MAX_GCI_FILES; i++) {
			if (strcmp(files[i].entry.gamecode, group->gamecode) == 0)
				indices[count++] = i;
		}
		if (count == 0) return card_modified ? 1 : 0;
		if (cursor >= count) cursor = count - 1;
		if (cursor < 0) cursor = 0;

		uiDrawObj_t *container = DrawEmptyBox(box_x1, box_y1, box_x2, box_y2);
		DrawAddChild(container, DrawStyledLabel(640 / 2, 90,
			group->game_name, 0.65f, ALIGN_CENTER, defaultColor));

		int visible = count - scroll;
		if (visible > GCI_PICK_MAX_VIS) visible = GCI_PICK_MAX_VIS;

		for (int i = 0; i < visible; i++) {
			int vi = scroll + i;
			int fi = indices[vi];
			int ry = list_top + i * GCI_PICK_ROW_H;
			card_entry *e = &files[fi].entry;

			if (vi == cursor)
				DrawAddChild(container, cm_draw_highlight(
					box_x1 + 10, ry, box_x2 - box_x1 - 20, GCI_PICK_ROW_H));

			int ix = box_x1 + 14;
			int iy = ry + (GCI_PICK_ROW_H - GCI_PICK_ICON_SZ) / 2;
			if (e->icon && e->icon->num_frames > 0 && e->icon->frames[0].snap) {
				DrawAddChild(container, DrawTexObj(&e->icon->frames[0].snap->tex,
					ix, iy, GCI_PICK_ICON_SZ, GCI_PICK_ICON_SZ,
					0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
			} else if (e->banner_snap) {
				DrawAddChild(container, DrawTexObj(&e->banner_snap->tex,
					ix, iy, GCI_PICK_ICON_SZ, GCI_PICK_ICON_SZ,
					0, 0.33f, 0.67f, 0.0f, 1.0f, 0));
			}

			int tx = ix + GCI_PICK_ICON_SZ + 8;
			GXColor name_color = (vi == cursor)
				? (GXColor){96, 107, 164, 255} : defaultColor;

			// Show GCI filename (from SD path, not internal card filename)
			char *dispname = getRelativeName(files[fi].path);
			float ns = GetTextScaleToFitInWidthWithMax(dispname,
				list_w - GCI_PICK_ICON_SZ - 24, 0.55f);
			DrawAddChild(container, DrawStyledLabel(tx, ry + 10,
				dispname, ns, ALIGN_LEFT, name_color));

			char datebuf[24];
			cm_format_gc_date(e->time, datebuf, sizeof(datebuf), true);
			char stats[64];
			snprintf(stats, sizeof(stats), "%d blk  %s", e->blocks, datebuf);
			DrawAddChild(container, DrawStyledLabel(tx, ry + 26,
				stats, 0.45f, ALIGN_LEFT, (GXColor){140, 140, 140, 255}));
		}

		if (count > GCI_PICK_MAX_VIS) {
			float pct = (float)scroll / (float)(count - GCI_PICK_MAX_VIS);
			DrawAddChild(container, DrawVertScrollBar(
				box_x2 - 14, list_top, 6,
				GCI_PICK_MAX_VIS * GCI_PICK_ROW_H, pct, 16));
		}

		DrawAddChild(container, DrawStyledLabel(640 / 2, 385,
			"A: Actions  |  B: Back", 0.55f, ALIGN_CENTER, defaultColor));
		DrawPublish(container);

		while (1) {
			VIDEO_WaitVSync();
			int result = cm_list_input(&cursor, &scroll, count, GCI_PICK_MAX_VIS);
			if (result >= 0) {
				DrawDispose(container);
				int fi = indices[result];

				// Show context menu for this backup
				const char *items[BK_ACT_COUNT];
				bool enabled[BK_ACT_COUNT];
				items[BK_ACT_IMPORT] = panel->source == CM_SRC_VMC
					? "Import to VMC" : "Import to card";
				enabled[BK_ACT_IMPORT] = panel->card_present;
				items[BK_ACT_RENAME] = "Rename";
				enabled[BK_ACT_RENAME] = true;
				items[BK_ACT_DUPLICATE] = "Duplicate";
				enabled[BK_ACT_DUPLICATE] = true;
				items[BK_ACT_DELETE] = "Delete backup";
				enabled[BK_ACT_DELETE] = true;

				int act = cm_context_menu(items, enabled, BK_ACT_COUNT);
				switch (act) {
					case BK_ACT_IMPORT:
						if (panel->source == CM_SRC_VMC) {
							if (vmc_import_save(panel->vmc_path, &files[fi]))
								card_modified = true;
						} else {
							if (card_manager_import_gci(panel->slot, &files[fi], panel->sector_size))
								card_modified = true;
						}
						break;
					case BK_ACT_RENAME:
						if (cm_backup_rename(&files[fi]))
							files_changed = true;
						break;
					case BK_ACT_DUPLICATE:
						if (cm_backup_duplicate(&files[fi]))
							files_changed = true;
						break;
					case BK_ACT_DELETE:
						if (cm_backup_delete(&files[fi]))
							files_changed = true;
						break;
				}
				break;  // redraw list
			}
			if (result == -1) {
				DrawDispose(container);
				if (card_modified) return 1;
				if (files_changed) return 0;
				return -1;  // back, no changes
			}
			if (result == -3) break;  // redraw
		}
		DrawDispose(container);
	}
}

// --- Backup manager (game list → save list → actions) ---

bool card_manager_backups(cm_panel *panel) {
	bool card_modified = false;

	gci_file_entry *gci_files = calloc(MAX_GCI_FILES, sizeof(gci_file_entry));
	if (!gci_files) return false;
	int num_gci = card_manager_scan_gci_files(gci_files, MAX_GCI_FILES);
	if (num_gci == 0) {
		uiDrawObj_t *msgBox = cm_draw_message("No backups found in swiss/saves/");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		free(gci_files);
		return card_modified;
	}

	// Sort by date descending
	qsort(gci_files, num_gci, sizeof(gci_file_entry), cm_gci_sort_date_desc);

	// Build game groups
	gci_game_group groups[MAX_GAME_GROUPS];
	int num_groups = cm_build_game_groups(gci_files, num_gci, groups, MAX_GAME_GROUPS);

	int cursor = 0, scroll = 0;
	int box_x1 = 40, box_y1 = 80, box_x2 = 600, box_y2 = 400;
	int list_top = 110;
	int list_w = box_x2 - box_x1 - 20;
	bool rebuild_groups = false;

	// If only one game, go straight to save list
	if (num_groups == 1) {
		int r = cm_backup_save_list(&gci_files, &num_gci, &groups[0], panel);
		if (r == 1) card_modified = true;
		if (r == 0 || r == 1) rebuild_groups = true;
		else {
			card_manager_free_gci_files(gci_files, num_gci);
			free(gci_files);
			return card_modified;
		}
	}

	while (1) {
		// Rebuild groups after file changes (delete/duplicate/rename/import)
		if (rebuild_groups) {
			num_groups = cm_build_game_groups(gci_files, num_gci, groups, MAX_GAME_GROUPS);
			if (num_groups == 0) break;
			if (cursor >= num_groups) cursor = num_groups - 1;
			rebuild_groups = false;
		}

		uiDrawObj_t *container = DrawEmptyBox(box_x1, box_y1, box_x2, box_y2);
		DrawAddChild(container, DrawStyledLabel(640 / 2, 90,
			"Backups", 0.7f, ALIGN_CENTER, defaultColor));

		int visible = num_groups - scroll;
		if (visible > GCI_PICK_MAX_VIS) visible = GCI_PICK_MAX_VIS;

		for (int i = 0; i < visible; i++) {
			int idx = scroll + i;
			int ry = list_top + i * GCI_PICK_ROW_H;
			gci_game_group *g = &groups[idx];
			card_entry *e = g->rep;

			if (idx == cursor)
				DrawAddChild(container, cm_draw_highlight(
					box_x1 + 10, ry, box_x2 - box_x1 - 20, GCI_PICK_ROW_H));

			int ix = box_x1 + 14;
			int iy = ry + (GCI_PICK_ROW_H - GCI_PICK_ICON_SZ) / 2;
			if (e->icon && e->icon->num_frames > 0 && e->icon->frames[0].snap) {
				DrawAddChild(container, DrawTexObj(&e->icon->frames[0].snap->tex,
					ix, iy, GCI_PICK_ICON_SZ, GCI_PICK_ICON_SZ,
					0, 0.0f, 1.0f, 0.0f, 1.0f, 0));
			} else if (e->banner_snap) {
				DrawAddChild(container, DrawTexObj(&e->banner_snap->tex,
					ix, iy, GCI_PICK_ICON_SZ, GCI_PICK_ICON_SZ,
					0, 0.33f, 0.67f, 0.0f, 1.0f, 0));
			}

			int tx = ix + GCI_PICK_ICON_SZ + 8;
			GXColor name_color = (idx == cursor)
				? (GXColor){96, 107, 164, 255} : defaultColor;
			float ns = GetTextScaleToFitInWidthWithMax(g->game_name,
				list_w - GCI_PICK_ICON_SZ - 24, 0.55f);
			DrawAddChild(container, DrawStyledLabel(tx, ry + 10,
				g->game_name, ns, ALIGN_LEFT, name_color));

			char stats[48];
			snprintf(stats, sizeof(stats), "%s  %d save%s",
				g->gamecode, g->count, g->count > 1 ? "s" : "");
			DrawAddChild(container, DrawStyledLabel(tx, ry + 26,
				stats, 0.45f, ALIGN_LEFT, (GXColor){140, 140, 140, 255}));
		}

		if (num_groups > GCI_PICK_MAX_VIS) {
			float pct = (float)scroll / (float)(num_groups - GCI_PICK_MAX_VIS);
			DrawAddChild(container, DrawVertScrollBar(
				box_x2 - 14, list_top, 6,
				GCI_PICK_MAX_VIS * GCI_PICK_ROW_H, pct, 16));
		}

		DrawAddChild(container, DrawStyledLabel(640 / 2, 385,
			"A: Open  |  B: Back", 0.55f, ALIGN_CENTER, defaultColor));
		DrawPublish(container);

		while (1) {
			VIDEO_WaitVSync();
			int result = cm_list_input(&cursor, &scroll, num_groups, GCI_PICK_MAX_VIS);
			if (result >= 0) {
				DrawDispose(container);
				gci_game_group *sel = &groups[result];
				int r = cm_backup_save_list(&gci_files, &num_gci, sel, panel);
				if (r == 1) card_modified = true;
				if (r == 0 || r == 1) rebuild_groups = true;
				// r == -1: back to game list, no rebuild needed
				break;
			}
			if (result == -1) {
				DrawDispose(container);
				card_manager_free_gci_files(gci_files, num_gci);
				free(gci_files);
				return card_modified;
			}
			if (result == -3) break;
		}
		DrawDispose(container);
	}

	card_manager_free_gci_files(gci_files, num_gci);
	free(gci_files);
	return card_modified;
}

// --- Read save from physical card as in-memory GCI ---

bool cm_read_physical_save(int slot, card_entry *entry, s32 sector_size,
	GCI *out_gci, u8 **out_data, u32 *out_len) {

	int total_sectors = (entry->filesize + sector_size - 1) / sector_size;
	cm_led_progress("Reading save...\nDo not remove the Memory Card.", 0, total_sectors);

	CARD_SetGamecode(entry->gamecode);
	CARD_SetCompany(entry->company);
	card_file cardfile;
	s32 ret = CARD_Open(slot, entry->filename, &cardfile);
	if (ret != CARD_ERROR_READY) {
		cm_led_hide();
		uiDrawObj_t *msgBox = cm_draw_message("Failed to open card file.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}

	// Build GCI header from raw sys_area directory entry
	GCI gci;
	memset(&gci, 0, sizeof(GCI));
	gci.reserved01 = 0xFF;
	gci.reserved02 = 0xFFFF;
	gci.filesize8 = entry->filesize / 8192;

	bool got_raw = false;
	unsigned char *sysarea = get_card_sys_area(slot);
	if (sysarea) {
		u32 entry_off = cardfile.filenum * 64;
		for (int blk = 0; blk < 5; blk++) {
			GCI *candidate = (GCI *)(sysarea + blk * 8192 + entry_off);
			if (memcmp(candidate->gamecode, entry->gamecode, 4) == 0
				&& memcmp(candidate->company, entry->company, 2) == 0
				&& memcmp(candidate->filename, entry->filename, CARD_FILENAMELEN) == 0) {
				memcpy(gci.gamecode, candidate->gamecode, 4);
				memcpy(gci.company, candidate->company, 2);
				memcpy(gci.filename, candidate->filename, CARD_FILENAMELEN);
				gci.banner_fmt = candidate->banner_fmt;
				gci.time = candidate->time;
				gci.icon_addr = candidate->icon_addr;
				gci.icon_fmt = candidate->icon_fmt;
				gci.icon_speed = candidate->icon_speed;
				gci.permission = candidate->permission;
				gci.copy_counter = candidate->copy_counter;
				gci.comment_addr = candidate->comment_addr;
				got_raw = true;
				break;
			}
		}
	}
	if (!got_raw) {
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
	}

	u8 *savedata = (u8 *)memalign(32, entry->filesize);
	if (!savedata) {
		CARD_Close(&cardfile);
		cm_led_hide();
		uiDrawObj_t *msgBox = cm_draw_message("Out of memory.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}

	u8 *read_buf = (u8 *)memalign(32, sector_size);
	bool read_ok = true;
	u32 offset = 0;
	int cur_sector = 0;
	while (offset < entry->filesize) {
		cm_led_progress("Reading save...\nDo not remove the Memory Card.",
			cur_sector, total_sectors);
		ret = CARD_Read(&cardfile, read_buf, sector_size, offset);
		if (ret != CARD_ERROR_READY) {
			usleep(2000);
			ret = CARD_Read(&cardfile, read_buf, sector_size, offset);
		}
		if (ret != CARD_ERROR_READY) { read_ok = false; break; }
		u32 copy_len = entry->filesize - offset;
		if (copy_len > (u32)sector_size) copy_len = sector_size;
		memcpy(savedata + offset, read_buf, copy_len);
		offset += sector_size;
		cur_sector++;
	}
	free(read_buf);
	CARD_Close(&cardfile);
	cm_led_hide();

	if (!read_ok) {
		free(savedata);
		uiDrawObj_t *errBox = cm_draw_message("Failed to read card data.");
		DrawPublish(errBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(errBox);
		return false;
	}

	*out_gci = gci;
	*out_data = savedata;
	*out_len = entry->filesize;
	return true;
}

// --- Import GCI buffer to physical card ---

bool card_manager_import_gci_buf(int slot, GCI *gci, u8 *savedata,
	u32 save_len, s32 sector_size) {

	uiDrawObj_t *msgBox = cm_draw_message("Writing save...\nDo not remove the Memory Card.");
	DrawPublish(msgBox);

	u32 expected_len = gci->filesize8 * 8192;
	if (save_len != expected_len) {
		DrawDispose(msgBox);
		char errmsg[128];
		snprintf(errmsg, sizeof(errmsg), "GCI size mismatch: got %dK, expected %dK",
			(int)(save_len / 1024), (int)(expected_len / 1024));
		msgBox = cm_draw_message(errmsg);
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
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
			DrawDispose(msgBox);
			msgBox = cm_draw_message("Failed to create file on card.\nCard may be full.");
			DrawPublish(msgBox);
			cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
			DrawDispose(msgBox);
			return false;
		}
		ret = CARD_Open(slot, filename, &cardfile);
	}
	else if (ret == CARD_ERROR_READY) {
		// File exists — confirm overwrite
		CARD_Close(&cardfile);
		DrawDispose(msgBox);
		msgBox = cm_draw_message("Save already exists on card.\n \nA: Overwrite  |  B: Cancel");
		DrawPublish(msgBox);
		u16 btn = cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		if (btn & PAD_BUTTON_B) {
			return false;
		}
		msgBox = cm_draw_message("Importing save...\nDo not remove the Memory Card.");
		DrawPublish(msgBox);
		// Re-open for writing
		ret = CARD_Open(slot, filename, &cardfile);
		is_overwrite = true;
	}

	if (ret != CARD_ERROR_READY) {
		DrawDispose(msgBox);
		uiDrawObj_t *errBox = cm_draw_message("Failed to open card file.");
		DrawPublish(errBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(errBox);
		return false;
	}

	// Switch to progress bar display for the write/verify phase
	DrawDispose(msgBox);
	int total_sectors = (expected_len + sector_size - 1) / sector_size;
	cm_led_progress("Writing save...\nDo not remove the Memory Card.", 0, total_sectors);

	// Write data sector by sector
	u8 *write_buf = (u8 *)memalign(32, sector_size);
	bool write_ok = true;
	u32 offset = 0;
	int cur_sector = 0;
	while (offset < expected_len) {
		cm_led_progress("Writing save...\nDo not remove the Memory Card.",
			cur_sector, total_sectors);
		u32 chunk = expected_len - offset;
		if (chunk > (u32)sector_size) chunk = sector_size;
		memset(write_buf, 0, sector_size);
		memcpy(write_buf, savedata + offset, chunk);
		ret = CARD_Write(&cardfile, write_buf, sector_size, offset);
		if (ret != CARD_ERROR_READY) {
			write_ok = false;
			break;
		}
		offset += sector_size;
		cur_sector++;
	}
	free(write_buf);

	if (!write_ok) {
		cm_led_hide();
		CARD_Close(&cardfile);
		uiDrawObj_t *errBox = cm_draw_message("Failed to write data to card.");
		DrawPublish(errBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(errBox);
		return false;
	}

	// Verify write by reading back and comparing
	u8 *verify_buf = (u8 *)memalign(32, sector_size);
	bool verify_ok = true;
	if (verify_buf) {
		offset = 0;
		cur_sector = 0;
		while (offset < expected_len) {
			cm_led_progress("Verifying save...\nDo not remove the Memory Card.",
				cur_sector, total_sectors);
			ret = CARD_Read(&cardfile, verify_buf, sector_size, offset);
			if (ret != CARD_ERROR_READY) {
				verify_ok = false;
				break;
			}
			u32 chunk = expected_len - offset;
			if (chunk > (u32)sector_size) chunk = sector_size;
			if (memcmp(verify_buf, savedata + offset, chunk) != 0) {
				verify_ok = false;
				break;
			}
			offset += sector_size;
			cur_sector++;
		}
		free(verify_buf);
	}
	cm_led_hide();

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
			// Flush directory to card via CARD_SetAttributes (triggers __card_updatedir)
			CARD_SetAttributes(slot, cardfile.filenum, gci->permission);
		} else {
			card_stat cardstat;
			CARD_GetStatus(slot, cardfile.filenum, &cardstat);
			cardstat.banner_fmt = gci->banner_fmt;
			cardstat.icon_addr = gci->icon_addr;
			cardstat.icon_fmt = gci->icon_fmt;
			cardstat.icon_speed = gci->icon_speed;
			cardstat.comment_addr = gci->comment_addr;
			u64 real_time = gettime();
			settime(secs_to_ticks(gci->time));
			CARD_SetStatus(slot, cardfile.filenum, &cardstat);
			settime(real_time);
			CARD_SetAttributes(slot, cardfile.filenum, gci->permission);
		}
	}

	CARD_Close(&cardfile);
	cm_led_hide();

	// Invalidate graphics cache — the save's graphics may have changed
	card_entry tmp;
	memset(&tmp, 0, sizeof(tmp));
	memcpy(tmp.gamecode, gci->gamecode, 4);
	memcpy(tmp.company, gci->company, 2);
	memcpy(tmp.filename, gci->filename, CARD_FILENAMELEN);
	cm_gfx_cache_invalidate(&tmp);

	char done_msg[128];
	snprintf(done_msg, sizeof(done_msg), "Imported \"%s\" successfully.", filename);
	uiDrawObj_t *doneBox = cm_draw_message(done_msg);
	DrawPublish(doneBox);
	cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
	DrawDispose(doneBox);
	return true;
}

// --- Import GCI file from SD to physical card ---

bool card_manager_import_gci(int slot, gci_file_entry *gci_entry, s32 sector_size) {
	DEVICEHANDLER_INTERFACE *dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (!dev) return false;

	// Read GCI file from SD
	file_handle *inFile = calloc(1, sizeof(file_handle));
	if (!inFile) return false;
	strlcpy(inFile->name, gci_entry->path, PATHNAME_MAX);

	if (dev->statFile(inFile)) {
		free(inFile);
		uiDrawObj_t *msgBox = cm_draw_message("File not found.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}

	u32 total_size = inFile->size;
	if (total_size <= sizeof(GCI)) {
		free(inFile);
		uiDrawObj_t *msgBox = cm_draw_message("Invalid GCI file (too small).");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}

	u8 *filebuf = (u8 *)memalign(32, total_size);
	if (!filebuf) {
		free(inFile);
		uiDrawObj_t *msgBox = cm_draw_message("Out of memory.");
		DrawPublish(msgBox);
		cm_wait_buttons(PAD_BUTTON_A | PAD_BUTTON_B);
		DrawDispose(msgBox);
		return false;
	}

	dev->readFile(inFile, filebuf, total_size);
	dev->closeFile(inFile);
	free(inFile);

	GCI *gci = (GCI *)filebuf;
	u8 *savedata = filebuf + sizeof(GCI);
	u32 save_len = total_size - sizeof(GCI);

	bool result = card_manager_import_gci_buf(slot, gci, savedata, save_len, sector_size);
	free(filebuf);
	return result;
}
