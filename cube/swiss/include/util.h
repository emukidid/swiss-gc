#ifndef UTIL_H
#define UTIL_H
#include <gccore.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

int endsWith(char *str, char *end);
bool checkExtension(char *filename);
char *getRelativeName(char *str);
char *stripInvalidChars(char *str);
void load_auto_dol();
void print_gecko(const char* fmt, ...);
extern void __libogc_exit(int status);

#endif 
