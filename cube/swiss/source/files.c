#include <fnmatch.h>
#include <gccore.h>
#include <stdarg.h>
#include <stdlib.h>
#include "swiss.h"
#include "main.h"
#include "dvd.h"
#include "files.h"
#include "devices/filemeta.h"

static file_handle** sortedDirEntries;
static file_handle* curDirEntries;  // all the files in the current dir
static int sortedDirEntryCount;
static int curDirEntryCount;		// count of files in the current dir

int fileComparator(const void *a1, const void *b1)
{
	const file_handle* a = *(const file_handle **)a1;
	const file_handle* b = *(const file_handle **)b1;
	
	if((devices[DEVICE_CUR] == &__device_dvd) && ((dvdDiscTypeInt == ISO9660_GAMECUBE_DISC) || (dvdDiscTypeInt == GAMECUBE_DISC) || (dvdDiscTypeInt == MULTIDISC_DISC)))
	{
		if(a->size == DISC_SIZE && a->fileBase == 0)
			return -1;
		if(b->size == DISC_SIZE && b->fileBase == 0)
			return 1;
	}
	
	if(a->fileType != b->fileType)
		return b->fileType - a->fileType;

	return strcasecmp(a->name, b->name);
}

int sortFiles(file_handle* dir, int numFiles, file_handle*** sortedDir)
{
	int i = 0;
	*sortedDir = calloc(numFiles, sizeof(file_handle*));
	if(*sortedDir) {
		for(int j = 0; j < numFiles; j++) {
			if((dir[j].fileAttrib & ATTRIB_HIDDEN) && !swissSettings.showHiddenFiles) {
				continue;
			}
			if(dir[j].fileType == IS_FILE && !checkExtension(dir[j].name, devices[DEVICE_CUR]->extraExtensions)) {
				continue;
			}
			(*sortedDir)[i++] = &dir[j];
		}
		qsort(*sortedDir, i, sizeof(file_handle*), fileComparator);
	}
	return i;
}

void freeFiles() {
	free(sortedDirEntries);
	sortedDirEntries = NULL;
	if(curDirEntries) {
		for(int i = 0; i < curDirEntryCount; i++) {
			if(curDirEntries[i].meta) {
				meta_free(curDirEntries[i].meta);
				curDirEntries[i].meta = NULL;
			}
			devices[DEVICE_CUR]->closeFile(&curDirEntries[i]);
		}
		free(curDirEntries);
		curDirEntries = NULL;
		curDirEntryCount = 0;
	}
}

void scanFiles() {
	freeFiles();
	// Read the directory/device TOC
	print_gecko("Reading directory: %s\r\n",curDir.name);
	curDirEntryCount = devices[DEVICE_CUR]->readDir(&curDir, &curDirEntries, -1);
	if(!fnmatch(swissSettings.flattenDir, curDir.name, FNM_PATHNAME | FNM_CASEFOLD)) {
		for(int i = 0; i < curDirEntryCount; i++) {
			if(curDirEntries[i].fileType == IS_DIR) {
				file_handle* dirEntries = NULL;
				int dirEntryCount = devices[DEVICE_CUR]->readDir(&curDirEntries[i], &dirEntries, -1);
				if(dirEntryCount > 1) {
					curDirEntryCount--; dirEntryCount--;
					curDirEntries = reallocarray(curDirEntries, curDirEntryCount + dirEntryCount, sizeof(file_handle));
					memmove(&curDirEntries[i + dirEntryCount], &curDirEntries[i + 1], (curDirEntryCount - i) * sizeof(file_handle));
					memmove(&curDirEntries[i], &dirEntries[1], dirEntryCount * sizeof(file_handle));
					curDirEntryCount += dirEntryCount; i--;
				}
				free(dirEntries);
			}
		}
	}
	print_gecko("Found %i entries\r\n",curDirEntryCount);
	sortedDirEntryCount = sortFiles(curDirEntries, curDirEntryCount, &sortedDirEntries);
	for(int i = 0; i < sortedDirEntryCount; i++) {
		if(!strcmp(sortedDirEntries[i]->name, curFile.name)) {
			curSelection = i;
			break;
		}
	}
	memcpy(&curFile, &curDir, sizeof(file_handle));
}

file_handle** getSortedDirEntries() {
	return sortedDirEntries;
}

file_handle* getCurrentDirEntries() {
	return curDirEntries;
}

int getSortedDirEntryCount() {
	return sortedDirEntryCount;
}

int getCurrentDirEntryCount() {
	return curDirEntryCount;
}

u64 getCurrentDirSize() {
	u64 curDirSize = 0LL;
	for(int i = 0; i < curDirEntryCount; i++) {
		curDirSize += curDirEntries[i].size;
	}
	return curDirSize;
}

size_t concat_path(char *pathName, const char *dirName, const char *baseName)
{
	size_t len;

	if (pathName == dirName)
		len = strlen(pathName);
	else
		len = strlcpy(pathName, dirName, PATHNAME_MAX);

	if (len >= PATHNAME_MAX)
		return len;

	if (len) {
		if (pathName[len - 1] != '/' && baseName[0] != '/') {
			if (len + 1 >= PATHNAME_MAX)
				return len + 1 + strlen(baseName);

			pathName[len++] = '/';
			pathName[len] = '\0';
		} else if (pathName[len - 1] == '/' && baseName[0] == '/')
			baseName++;
	}

	return strlcat(pathName, baseName, PATHNAME_MAX);
}

size_t concatf_path(char *pathName, const char *dirName, const char *baseName, ...)
{
	size_t len;

	if (pathName == dirName)
		len = strlen(pathName);
	else
		len = strlcpy(pathName, dirName, PATHNAME_MAX);

	if (len >= PATHNAME_MAX)
		return len;

	if (len) {
		if (pathName[len - 1] != '/' && baseName[0] != '/') {
			if (len + 1 >= PATHNAME_MAX) {
				va_list args;
				va_start(args, baseName);
				len += vsnprintf(NULL, 0, baseName, args);
				va_end(args);
				return len + 1;
			}

			pathName[len++] = '/';
			pathName[len] = '\0';
		} else if (pathName[len - 1] == '/' && baseName[0] == '/')
			baseName++;
	}

	va_list args;
	va_start(args, baseName);
	len += vsnprintf(pathName + len, PATHNAME_MAX - len, baseName, args);
	va_end(args);
	return len;
}

// Either renames a path to a new one, or creates one.
void ensure_path(int deviceSlot, char *path, char *oldPath, bool hidden) {
	file_handle fhFullPath = { .fileType = IS_DIR };
	concat_path(fhFullPath.name, devices[deviceSlot]->initial->name, path);
	if(oldPath) {
		file_handle fhOldFullPath = { .fileType = IS_DIR };
		concat_path(fhOldFullPath.name, devices[deviceSlot]->initial->name, oldPath);
		if(devices[deviceSlot]->renameFile) {
			if(devices[deviceSlot]->renameFile(&fhOldFullPath, fhFullPath.name)) {
				if(devices[deviceSlot]->makeDir) {
					devices[deviceSlot]->makeDir(&fhFullPath);
				}
			}
		}
		else if(devices[deviceSlot]->makeDir) {
			devices[deviceSlot]->makeDir(&fhFullPath);
		}
	}
	else if(devices[deviceSlot]->makeDir) {
		devices[deviceSlot]->makeDir(&fhFullPath);
	}
	if(devices[deviceSlot]->hideFile) {
		devices[deviceSlot]->hideFile(&fhFullPath, hidden);
	}
}
