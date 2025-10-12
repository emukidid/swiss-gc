#include <fnmatch.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "swiss.h"
#include "main.h"
#include "util.h"
#include "dvd.h"
#include "aram/sidestep.h"
#include "devices/filemeta.h"


/* File name helper functions */
char *knownExtensions[] = {".bin", ".dol", ".dol+cli", ".elf", ".fpkg", ".fzn", ".gcm", ".gcz", ".iso", ".mp3", ".rvz", ".tgc", NULL};

char *endsWith(char *str, char *end) {
	size_t len_str = strlen(str);
	size_t len_end = strlen(end);
	if(len_str < len_end)
		return NULL;
	str += len_str - len_end;
	return !strcasecmp(str, end) ? str : NULL;
}

bool canLoadFileType(char *filename, char **extraExtensions) {
	char **ext;
	if(extraExtensions) {
		for(ext = extraExtensions; *ext; ext++) {
			if(endsWith(filename, *ext)) {
				return !is_rom_name(filename);
			}
		}
	}
	for(ext = knownExtensions; *ext; ext++) {
		if(endsWith(filename, *ext)) {
			return !is_rom_name(filename);
		}
	}
	return false;
}

bool checkExtension(char *filename, char **extraExtensions) {
	if(!swissSettings.hideUnknownFileTypes)
		return true;
	return canLoadFileType(filename, extraExtensions);
}

char *getRelativeName(char *path)
{
	char *chr = strrchr(path, '/');
	if (!chr) chr = strchr(path, ':');
	return chr ? chr + 1 : path;
}

char *getRelativePath(char *path, char *parentPath)
{
	size_t len = strlen(parentPath);

	if (!strncmp(path, parentPath, len)) {
		if (parentPath[len - 1] == '/')
			return path + len;
		else if (path[len] == '/')
			return path + len + 1;
	}

	return path;
}

bool getParentPath(char *path, char *parentPath)
{
	if (path != parentPath)
		strcpy(parentPath, path);

	char *first = strchr(parentPath, '/');
	char *last = strrchr(parentPath, '/');

	if (!first || first[1] == '\0')
		return true;
	if (first == last)
		first[1] = '\0';
	else last[0] = '\0';
	return false;
}

char *getDevicePath(char *path)
{
	char *chr = strchr(path, ':');
	return chr ? chr + 1 : path;
}

char *getExternalPath(char *path)
{
	static const struct {
		const char *internal;
		const char *external;
	} devices[] = {
		{ "ataa:/",  "carda:" },
		{ "atab:/",  "cardb:" },
		{ "atac:/",  "fat:"   },
		{ "fdffs:/", "flash:" },
		{ "fldrv:/", "dvd:"   },
		{ "gcldr:/", "dvd:"   },
		{ "kunai:/", "flash:" },
		{ "sda:/",   "carda:" },
		{ "sdb:/",   "cardb:" },
		{ "sdc:/",   "sd:"    },
		{ "wkf:/",   "dvd:"   },
		{ NULL }
	};

	char *devicePath = getDevicePath(path);

	if (devicePath == path)
		return strdup(path);

	for (int i = 0; devices[i].internal; i++) {
		if (!strncmp(path, devices[i].internal, devicePath - path)) {
			char *newPath = malloc(strlen(devices[i].external) + strlen(devicePath) + 1);
			return strcat(strcpy(newPath, devices[i].external), devicePath);
		}
	}

	return strdup(path);
}

char stripbuffer[PATHNAME_MAX];
char *stripInvalidChars(char *str)
{
	char *dst = stripbuffer;

	for (char *src = str; *src; src++) {
		switch (*src) {
			case 0x00 ... 0x1F:
			case 0x7F:
				*dst++ = ' ';
				break;
			case '"':
			case '*':
			case '<':
			case '>':
				*dst++ = '\'';
				break;
			case '/':
			case ':':
			case '\\':
			case '|':
				if (src > str) {
					if (src[-1] != ' ' && src[1] == ' ')
						*dst++ = ' ';
					*dst++ = '-';
					if (src[-1] == ' ' && src[1] != ' ')
						*dst++ = ' ';
				}
				break;
			case '?':
				break;
			default:
				*dst++ = *src;
				break;
		}
	}

	*dst++ = '\0';
	return stripbuffer;
}

static const char git_tags[][sizeof(GIT_COMMIT)] = {
#include "tags.h"
};
/* Autoboot DOL from the current device, from the current autoboot_dols list */
static const char *autoboot_dols[] = {
	"*/swiss_r[1-9]*.dol",
	"atac:/[abxyz].dol",
	"atac:/start.dol",
	"atac:/ipl.dol",
	"sd[abc]:/[abxyz].dol",
	"sd[abc]:/start.dol",
	"sd[abc]:/ipl.dol",
	"sd[ab]:/AUTOEXEC.DOL",
	"*/boot.dol",
	"*/boot2.dol"
};
void load_auto_dol(int argc, char *argv[]) {
	char trailer[sizeof(GIT_COMMIT) - 1]; // Don't include the NUL termination in the comparison
	int trailer_size;

	memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
	scanFiles();
	file_handle *dirEntries = getCurrentDirEntries();
	int dirEntryCount = getCurrentDirEntryCount();
	for (int f = 0; f < sizeof(autoboot_dols) / sizeof(*autoboot_dols); f++) {
		for (int i = 0; i < dirEntryCount; i++) {
			if (!fnmatch(autoboot_dols[f], dirEntries[i].name, FNM_PATHNAME | FNM_CASEFOLD)) {
				DOLHEADER dolhdr;
				devices[DEVICE_CUR]->seekFile(&dirEntries[i], 0, DEVICE_HANDLER_SEEK_SET);
				if (devices[DEVICE_CUR]->readFile(&dirEntries[i], &dolhdr, DOLHDRLENGTH) == DOLHDRLENGTH) {
					// Official Swiss releases have the short commit hash appended to
					// the end of the DOL, compare it to our own to make sure we don't
					// bootloop the same version
					devices[DEVICE_CUR]->seekFile(&dirEntries[i], DOLSize(&dolhdr), DEVICE_HANDLER_SEEK_SET);
					trailer_size = devices[DEVICE_CUR]->readFile(&dirEntries[i], trailer, sizeof(trailer));
				} else {
					trailer_size = 0;
				}
				if ((*autoboot_dols[f] == '*' && trailer_size < 7) || (trailer_size >= 7 &&
					memcmp(GIT_COMMIT, trailer, trailer_size) != 0 &&
					memmem(git_tags, sizeof(git_tags), trailer, trailer_size) == NULL)) {
					boot_dol(&dirEntries[i], argc, argv);
				}
				devices[DEVICE_CUR]->closeFile(&dirEntries[i]);

				// If we've made it this far, we've already found an autoboot DOL,
				// the first one (boot.dol) is not cancellable, but the rest of the
				// list is
				if (padsButtonsHeld() & PAD_BUTTON_Y) {
					return;
				}
			}
		}
		if (swissSettings.cubebootInvoked) {
			return;
		}
	}
}

/* Print over USB Gecko if enabled */
void print_debug(const char *fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
	SYS_EnableGecko(swissSettings.enableUSBGecko - USBGECKO_MEMCARD_SLOT_A, swissSettings.waitForUSBGecko);
	SYS_Reportv(fmt, arglist);
	va_end(arglist);
}

/* Update recent list with a new entry. */
bool update_recent() {
	// Off
	if(swissSettings.recentListLevel == 0) return false;
	
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

int find_existing_entry(char *entry, bool load) {
	// get the device handler for it
	DEVICEHANDLER_INTERFACE *entryDevice = getDeviceFromPath(entry);
	if(entryDevice) {
		print_debug("Device required for entry [%s]\n", entryDevice->deviceName);
		
		// Init the device if it isn't one we were about to browse anyway
		if(devices[DEVICE_CUR] == entryDevice || !entryDevice->init(entryDevice->initial)) {
			freeFiles();
			if(devices[DEVICE_CUR] && devices[DEVICE_CUR] != entryDevice) {
				devices[DEVICE_PREV] = devices[DEVICE_CUR];
				devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
			}
			// Attempt to read the directory the recent file lives in (required for 2 disc games)
			devices[DEVICE_CUR] = entryDevice;
			memcpy(&curDir, devices[DEVICE_CUR]->initial, sizeof(file_handle));
			getParentPath(entry, curDir.name);
			while(!fnmatch(swissSettings.flattenDir, curDir.name, FNM_PATHNAME | FNM_CASEFOLD | FNM_LEADING_DIR)
				&& fnmatch(swissSettings.flattenDir, curDir.name, FNM_PATHNAME | FNM_CASEFOLD) == FNM_NOMATCH)
				getParentPath(curDir.name, curDir.name);
			scanFiles();
			
			curMenuLocation = ON_FILLIST;
			curMenuSelection = 0;
			curSelection = 0;
			needsDeviceChange = 0;
			needsRefresh = 0;
			
			// Finally, read the actual file
			file_handle *dirEntries = getCurrentDirEntries();
			int dirEntryCount = getCurrentDirEntryCount();
			for(int i = 0; i < dirEntryCount; i++) {
				if(!strcmp(entry, dirEntries[i].name)
				|| !fnmatch(entry, dirEntries[i].name, FNM_PATHNAME)) {
					curSelection = getSortedDirEntryIndex(&dirEntries[i]);
					if(dirEntries[i].fileType == IS_FILE && load) {
						populate_meta(&dirEntries[i]);
						memcpy(&curFile, &dirEntries[i], sizeof(file_handle));
						load_file();
						memcpy(&dirEntries[i], &curFile, sizeof(file_handle));
					}
					else if(dirEntries[i].fileType == IS_DIR) {
						memcpy(&curDir, &dirEntries[i], sizeof(file_handle));
						needsRefresh = 1;
					}
					return 0;
				}
			}
			return RECENT_ERR_ENT_MISSING;
		}
	}
	print_debug("Device was not found\n");
	return RECENT_ERR_DEV_MISSING;
}

bool deleteFileOrDir(file_handle* entry) {
	if(entry->fileType == IS_DIR) {
		print_debug("Entering dir for deletion: %s\n", entry);
		file_handle* dirEntries = NULL;
		int dirEntryCount = entry->device->readDir(entry, &dirEntries, -1);
		int i;
		for(i = 0; i < dirEntryCount; i++) {
			if(!deleteFileOrDir(&dirEntries[i])) {
				return false;
			}
		}
		if(dirEntries) free(dirEntries);
		print_debug("Finally, deleting empty directory: %s\n", entry);
		return !entry->device->deleteFile(entry);
	}
	if(entry->fileType == IS_FILE) {
		print_debug("Deleting file: %s\n", entry);
		return !entry->device->deleteFile(entry);
	}
	return true;	// IS_SPECIAL can be ignored.
}

int formatBytes(char *string, off_t count, blksize_t blocksize, bool metric)
{
	static const struct {
		const char *unit[2];
		double value[2];
	} units[] = {
		{{ "YiB", "YB" }, { 0x1p80, 1e24 }},
		{{ "ZiB", "ZB" }, { 0x1p70, 1e21 }},
		{{ "EiB", "EB" }, { 0x1p60, 1e18 }},
		{{ "PiB", "PB" }, { 0x1p50, 1e15 }},
		{{ "TiB", "TB" }, { 0x1p40, 1e12 }},
		{{ "GiB", "GB" }, { 0x1p30, 1e9 }},
		{{ "MiB", "MB" }, { 0x1p20, 1e6 }},
		{{ "KiB", "kB" }, { 0x1p10, 1e3 }},
		{{ NULL }}
	};

	if (blocksize) {
		blkcnt_t blocks = (count + blocksize - 1) / blocksize;

		for (int i = 0; units[i].unit[metric]; i++)
			if (count >= units[i].value[metric])
				return sprintf(string, "%.3g %s (%jd blocks)", count / units[i].value[metric], units[i].unit[metric], (intmax_t)blocks);

		return sprintf(string, "%jd bytes (%jd blocks)", (intmax_t)count, (intmax_t)blocks);
	}

	for (int i = 0; units[i].unit[metric]; i++)
		if (count >= units[i].value[metric])
			return sprintf(string, "%.3g %s", count / units[i].value[metric], units[i].unit[metric]);

	return sprintf(string, "%jd bytes", (intmax_t)count);
}
