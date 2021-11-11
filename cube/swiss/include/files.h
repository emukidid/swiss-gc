#ifndef FILES_H
#define FILES_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "deviceHandler.h"

void sortFiles(file_handle* dir, int num_files);
void freeFiles();
void scanFiles();
file_handle* getCurrentDirEntries();
int getCurrentDirEntryCount();
void ensure_path(int deviceSlot, char *path, char *oldPath);

#endif 
