#ifndef FILES_H
#define FILES_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "deviceHandler.h"

file_handle** sortFiles(file_handle* dir, int num_files);
void freeFiles();
void scanFiles();
file_handle** getSortedDirEntries();
file_handle* getCurrentDirEntries();
#define getSortedDirEntryCount getCurrentDirEntryCount
int getCurrentDirEntryCount();
void concat_path(char *pathName, const char *dirName, const char *baseName);
void concatf_path(char *pathName, const char *dirName, const char *baseName, ...);
void ensure_path(int deviceSlot, char *path, char *oldPath);

#endif 
