/* filemeta.h
	- file meta gathering
	by emu_kidid
 */

#ifndef FILEMETA_H
#define FILEMETA_H

#include <stdint.h>
#include <stdio.h>
#include "deviceHandler.h"

void populate_meta(file_handle *f);
file_handle* meta_find_disc2(file_handle *f);
void meta_free(file_meta* meta);
#endif

