#ifndef UTIL_H
#define UTIL_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "deviceHandler.h"

#define RECENT_ERR_ENT_MISSING 1
#define RECENT_ERR_DEV_MISSING 2

char *endsWith(char *str, char *end);
bool canLoadFileType(char *filename, char **extraExtensions);
bool checkExtension(char *filename, char **extraExtensions);
char *getRelativeName(char *path);
char *getRelativePath(char *path, char *parentPath);
bool getParentPath(char *path, char *parentPath);
char *getDevicePath(char *path);
char *getExternalPath(char *path);
char *stripInvalidChars(char *str);
void load_auto_dol(int argc, char *argv[]);
void print_debug(const char *fmt, ...);
bool update_recent();
bool is_recent_entry(char *entry);
int find_existing_entry(char *entry, bool load);
bool deleteFileOrDir(file_handle* entry);
int formatBytes(char *string, off_t count, blksize_t blocksize, bool metric);

#endif 
