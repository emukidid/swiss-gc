#include <gccore.h>
#include <stdlib.h>
#include "swiss.h"
#include "main.h"
#include "dvd.h"
#include "files.h"
#include "devices/filemeta.h"

static file_handle* curDirEntries;  // all the files in the current dir
static int curDirEntryCount;		// count of files in the current dir

int fileComparator(const void *a1, const void *b1)
{
	const file_handle* a = a1;
	const file_handle* b = b1;
	
	if(!a && b) return 1;
	if(a && !b) return -1;
	if(!a && !b) return 0;
	
	if((devices[DEVICE_CUR] == &__device_dvd) && ((dvdDiscTypeInt == GAMECUBE_DISC) || (dvdDiscTypeInt == MULTIDISC_DISC)))
	{
		if(a->size == DISC_SIZE && a->fileBase == 0)
			return -1;
		if(b->size == DISC_SIZE && b->fileBase == 0)
			return 1;
	}
	
	if(a->fileAttrib == IS_DIR && b->fileAttrib == IS_FILE)
		return -1;
	if(a->fileAttrib == IS_FILE && b->fileAttrib == IS_DIR)
		return 1;

	return strcasecmp(a->name, b->name);
}

void sortFiles(file_handle* dir, int num_files)
{
	if(num_files > 0) {
		qsort(&dir[0],num_files,sizeof(file_handle), fileComparator);
	}
}

void freeFiles() {
	if(curDirEntries) {
		int i;
		for(i = 0; i < curDirEntryCount; i++) {
			if(curDirEntries[i].meta) {
				meta_free(curDirEntries[i].meta);
				curDirEntries[i].meta = NULL;
			}
		}
		free(curDirEntries);
		curDirEntries = NULL;
		curDirEntryCount = 0;
	}
}

void scanFiles() {
	freeFiles();
	// Read the directory/device TOC
	if(curDirEntries){ free(curDirEntries); curDirEntries = NULL; }
	print_gecko("Reading directory: %s\r\n",curFile.name);
	curDirEntryCount = devices[DEVICE_CUR]->readDir(&curFile, &curDirEntries, -1);
	memcpy(&curDir, &curFile, sizeof(file_handle));
	sortFiles(curDirEntries, curDirEntryCount);
	print_gecko("Found %i entries\r\n",curDirEntryCount);
}

file_handle* getCurrentDirEntries() {
	return curDirEntries;
}

int getCurrentDirEntryCount() {
	return curDirEntryCount;
}

// Either renames a path to a new one, or creates one.
void ensure_path(int deviceSlot, char *path, char *oldPath) {
	char *fullPath = calloc(1, PATHNAME_MAX);
	if(oldPath) {
		char *oldFullPath = calloc(1, PATHNAME_MAX);
		snprintf(oldFullPath, PATHNAME_MAX, "%s%s", devices[DEVICE_PATCHES]->initial->name, oldPath);
		snprintf(fullPath, PATHNAME_MAX, "%s%s", devices[DEVICE_PATCHES]->initial->name, path);
		f_rename(oldFullPath, fullPath);
		free(oldFullPath);
	}
	else {
		snprintf(fullPath, PATHNAME_MAX, "%s%s", devices[deviceSlot]->initial->name, path);
		f_mkdir(fullPath);
	}
	free(fullPath);
}
