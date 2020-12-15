#include <stdarg.h>
#include <stdio.h>
#include "swiss.h"
#include "main.h"
#include "util.h"


/* File name helper functions */
char *knownExtensions[] = {".dol", ".iso", ".gcm", ".mp3", ".fzn", ".gci", ".dol+cli", ".elf"};

int endsWith(char *str, char *end) {
	size_t len_str = strlen(str);
	size_t len_end = strlen(end);
	if(len_str < len_end)
		return 0;
	return !strcasecmp(str + len_str - len_end, end);
}

bool checkExtension(char *filename) {
	if(!swissSettings.hideUnknownFileTypes)
		return true;
	int i;
	for(i = 0; i < sizeof(knownExtensions)/sizeof(char*); i++) {
		if(endsWith(filename, knownExtensions[i])) {
			return true;
		}
	}
	return false;
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

