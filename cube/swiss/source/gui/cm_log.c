/* cm_log.c
	- Card Manager logging subsystem
	by Swiss contributors
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "cm_internal.h"

// Uses Swiss device handler API (not fopen) since FAT devoptab isn't available

#define CM_LOG_MAX 16384
static char cm_log_buf[CM_LOG_MAX];
static int cm_log_len = 0;
static DEVICEHANDLER_INTERFACE *cm_log_dev = NULL;

void cm_log_open(void) {
	cm_log_len = 0;
	cm_log_dev = devices[DEVICE_CONFIG] ? devices[DEVICE_CONFIG] :
		devices[DEVICE_CUR] ? devices[DEVICE_CUR] : NULL;
	if (cm_log_dev) {
		cm_log_len += snprintf(cm_log_buf + cm_log_len, CM_LOG_MAX - cm_log_len,
			"\n--- Card Manager session started ---\n");
	}
}

void cm_log(const char *fmt, ...) {
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

void cm_log_close(void) {
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

void cm_log_card_info(int slot, s32 mem_size, s32 sector_size, int num_entries) {
	cm_log("Slot %c: mem_size=%d, sector_size=%d, entries=%d",
		slot == CARD_SLOTA ? 'A' : 'B', (int)mem_size, (int)sector_size, num_entries);
}

void cm_log_entry(int idx, card_entry *entry) {
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
	cm_log("       icon_addr=0x%x icon_fmt=0x%04x icon_speed=0x%04x banner_fmt=0x%02x",
		entry->icon_addr, entry->icon_fmt, entry->icon_speed, entry->banner_fmt);
	if (entry->icon) {
		cm_log("       anim: %d frames, type=%s",
			entry->icon->num_frames,
			(entry->icon->anim_type & CARD_ANIM_BOUNCE) ? "BOUNCE" : "LOOP");
		for (int f = 0; f < entry->icon->num_frames; f++) {
			cm_log("         frame[%d]: fmt=%d speed=%d data=%s size=%d",
				f, entry->icon->frames[f].fmt, entry->icon->frames[f].speed,
				entry->icon->frames[f].data ? "yes" : "no",
				entry->icon->frames[f].data_size);
		}
	}
}
