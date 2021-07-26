#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "swiss.h"
#include "main.h"
#include "util.h"
#include "dvd.h"
#include "devices/filemeta.h"


/* File name helper functions */
char *knownExtensions[] = {".dol", ".iso", ".gcm", ".mp3", ".fzn", ".dol+cli", ".elf", ".tgc"};

int endsWith(char *str, char *end) {
	size_t len_str = strlen(str);
	size_t len_end = strlen(end);
	if(len_str < len_end)
		return 0;
	return !strcasecmp(str + len_str - len_end, end);
}

bool canLoadFileType(char *filename) {
	int i;
	for(i = 0; i < sizeof(knownExtensions)/sizeof(char*); i++) {
		if(endsWith(filename, knownExtensions[i])) {
			return true;
		}
	}
	return false;
}

bool checkExtension(char *filename) {
	if(!swissSettings.hideUnknownFileTypes)
		return true;
	return canLoadFileType(filename);
}

char *getRelativeName(char *str) {
	int i;
	for(i=strlen(str);i>0;i--){
		if(str[i]=='/') {
			return str+i+1;
		}
	}
	return str;
}

void getParentPath(char *src, char *dst) {
	int i;
	for(i=strlen(src);i>0;i--){
		if(src[i]=='/') {
			strncpy(dst, src, i);
		}
	}
}

char stripbuffer[PATHNAME_MAX];
char *stripInvalidChars(char *str) {
	strcpy(stripbuffer, str);
	int i = 0;
	for(i = 0; i < strlen(stripbuffer); i++) {
		if(str[i] == '\\' || str[i] == '/' || str[i] == ':'|| str[i] == '*'
		|| str[i] == '?'|| str[i] == '"'|| str[i] == '<'|| str[i] == '>'|| str[i] == '|') {
			stripbuffer[i] = '_';
		}
	}
	return &stripbuffer[0];
}

/* Autoboot DOL from the current device, from the current autoboot_dols list */
char *autoboot_dols[] = { "/boot.dol", "/boot2.dol" }; // Keep this list sorted
void load_auto_dol() {
	u8 rev_buf[sizeof(GITREVISION) - 1]; // Don't include the NUL termination in the comparison

	memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
	scanFiles();
	file_handle* curDirEntries = getCurrentDirEntries();
	for (int i = 0; i < getCurrentDirEntryCount(); i++) {
		for (int f = 0; f < (sizeof(autoboot_dols) / sizeof(char *)); f++) {
			if (endsWith(curDirEntries[i].name, autoboot_dols[f])) {
				// Official Swiss releases have the short commit hash appended to
				// the end of the DOL, compare it to our own to make sure we don't
				// bootloop the same version
				devices[DEVICE_CUR]->seekFile(&curDirEntries[i],
						curDirEntries[i].size - sizeof(rev_buf),
						DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_CUR]->readFile(&curDirEntries[i], rev_buf, sizeof(rev_buf));
				if (memcmp(GITREVISION, rev_buf, sizeof(rev_buf)) != 0) {
					// Emulate some of the menu's behavior to satisfy boot_dol
					curSelection = i;
					memcpy(&curFile, &curDirEntries[i], sizeof(file_handle));
					boot_dol();
				}

				// If we've made it this far, we've already found an autoboot DOL,
				// the first one (boot.dol) is not cancellable, but the rest of the
				// list is
				if (PAD_ButtonsHeld(0) & PAD_BUTTON_Y) {
					return;
				}
			}
		}
	}
}

/* Print over USB Gecko if enabled */
void print_gecko(const char* fmt, ...)
{
	if(swissSettings.debugUSB && usb_isgeckoalive(1)) {
		char tempstr[2048];
		va_list arglist;
		va_start(arglist, fmt);
		vsprintf(tempstr, fmt, arglist);
		va_end(arglist);
		// write out over usb gecko ;)
		usb_sendbuffer_safe(1,tempstr,strlen(tempstr));
	}
}

/* Update recent list with a new entry. */
bool update_recent() {
	// Off
	if(swissSettings.recentListLevel == 2) return false;
	
	int i, found_idx = -1, max_idx = RECENT_MAX-1;
	// See if this entry already exists in the recent list, if so, we'll move it to the top.
	for(i = 0; i < RECENT_MAX; i++) {
		if(swissSettings.recent[i][0] == 0) {
			max_idx = i;
			break;
		}
		if(!strcmp(&swissSettings.recent[i][0], &curFile.name[0])) {
			found_idx = i;
		}
	}
	
	// It's already at the top (or the recent list is set to "lazy" update mode), we have nothing to update, return.
	if(found_idx == 0 || (found_idx != -1 && swissSettings.recentListLevel == 1)) {
		return false;
	}
		
	// Move everything down
	max_idx = found_idx != -1 ? found_idx : max_idx;
	for(i = max_idx; i > 0; i--) {
		memset(&swissSettings.recent[i][0], 0, PATHNAME_MAX);
		strcpy(&swissSettings.recent[i][0], &swissSettings.recent[i-1][0]);
	}
	
	// Put our new entry at the top
	strcpy(&swissSettings.recent[0][0], &curFile.name[0]);
	return true;
}

int load_existing_entry(char *entry) {
	// get the device handler for it
	DEVICEHANDLER_INTERFACE *entryDevice = getDeviceFromPath(entry);
	if(entryDevice) {
		print_gecko("Device required for entry [%s]\r\n", entryDevice->deviceName);
		
		DEVICEHANDLER_INTERFACE *oldDevice = devices[DEVICE_CUR];
		devices[DEVICE_CUR] = entryDevice;
		// Init the device if it isn't one we were about to browse anyway
		int found = 0;
		if(devices[DEVICE_CUR] == oldDevice || devices[DEVICE_CUR]->init(devices[DEVICE_CUR]->initial)) {
			// Save the old current path in case if we fail to find the recent list entry.
			file_handle *oldPath = calloc(1, sizeof(file_handle));
			memcpy(oldPath, &curFile, sizeof(file_handle));
			
			// Attempt to read the directory the recent file lives in (required for 2 disc games)
			memset(&curFile, 0, sizeof(file_handle));
			getParentPath(entry, &curFile.name[0]);
			scanFiles();
			
			// Finally, read the actual file
			memset(&curFile, 0, sizeof(file_handle));
			strcpy(&curFile.name[0], entry);
			if(devices[DEVICE_CUR] == &__device_dvd) curFile.size = DISC_SIZE;
			if(devices[DEVICE_CUR]->readFile(&curFile, NULL, 0) == 0) {
				found = 1;
				if(endsWith(entry,".dol") || endsWith(entry,".dol+cli") || endsWith(entry,".elf")) {
					boot_dol();
				}
				else {
					print_gecko("Entry exists, reading meta.\r\n");
					populate_meta(&curFile);
					load_file();
				}
			}
			// User cancelled, clean things up
			memcpy(&curFile, oldPath, sizeof(file_handle));
			free(oldPath);
			scanFiles();
		}
		devices[DEVICE_CUR] = oldDevice;
		return found ? 0 : RECENT_ERR_ENT_MISSING;
	}
	print_gecko("Device was not found\r\n");
	return RECENT_ERR_DEV_MISSING;
}

bool deleteFileOrDir(file_handle* entry) {
	if(entry->fileAttrib == IS_DIR) {
		print_gecko("Entering dir for deletion: %s\r\n", entry);
		file_handle* dirEntries = NULL;
		int dirEntryCount = devices[DEVICE_CUR]->readDir(entry, &dirEntries, -1);
		int i;
		for(i = 0; i < dirEntryCount; i++) {
			if(!deleteFileOrDir(&dirEntries[i])) {
				return false;
			}
		}
		if(dirEntries) free(dirEntries);
		print_gecko("Finally, deleting empty directory: %s\r\n", entry);
		return !devices[DEVICE_CUR]->deleteFile(entry);
	}
	if(entry->fileAttrib == IS_FILE) {
		print_gecko("Deleting file: %s\r\n", entry);
		return !devices[DEVICE_CUR]->deleteFile(entry);
	}
	return true;	// IS_SPECIAL can be ignored.
}
