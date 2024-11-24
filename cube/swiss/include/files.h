#ifndef FILES_H
#define FILES_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "deviceHandler.h"

int sortFiles(file_handle* dir, int numFiles, file_handle*** sortedDir);
void freeFiles();
void scanFiles();
file_handle** getSortedDirEntries();
file_handle* getCurrentDirEntries();
int getSortedDirEntryCount();
int getCurrentDirEntryCount();
u64 getCurrentDirSize();
size_t concat_path(char *pathName, const char *dirName, const char *baseName);
size_t concatf_path(char *pathName, const char *dirName, const char *baseName, ...);
void ensure_path(int deviceSlot, char *path, char *oldPath);

#endif 
