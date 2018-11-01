/* filemeta.h
	- file meta gathering
	by emu_kidid
 */

#ifndef FILEMETA_H
#define FILEMETA_H

#include <stdint.h>
#include <stdio.h>
#include "deviceHandler.h"

// Banner is 96 cols * 32 lines in RGB5A3 fmt
#define BannerSize (96*32*2)

void populate_meta(file_handle *f);
void meta_free(void* ptr);
#endif

