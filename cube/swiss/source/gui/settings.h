#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include "config.h"
#include "deviceHandler.h"

extern char *uiVModeStr[];
extern char *softProgressiveStr[];
extern char *forceWidescreenStr[];
void show_settings(file_handle *file, ConfigEntry *config);
void refreshSRAM();

#endif