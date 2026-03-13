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

	// Write only the current session's log (overwrite, not append).
	// This avoids reading a potentially large old log from SD on every exit.
	file_handle *logFile = calloc(1, sizeof(file_handle));
	if (logFile) {
		concat_path(logFile->name, cm_log_dev->initial->name, "cardmanager.log");
		if (!cm_log_dev->statFile(logFile))
			cm_log_dev->deleteFile(logFile);
		cm_log_dev->writeFile(logFile, cm_log_buf, cm_log_len);
		cm_log_dev->closeFile(logFile);
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
}
