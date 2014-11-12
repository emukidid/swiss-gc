/**
 * Swiss - deviceHandler-SMB.h (originally from WiiSX)
 * Copyright (C) 2010-2014 emu_kidid
 * 
 * fileBrowser module for Samba based shares
 *
 * Swiss homepage: https://code.google.com/p/swiss-gc/
 * email address:  emukidid@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/


#ifndef FILE_BROWSER_SMB_H
#define FILE_BROWSER_SMB_H

#include "deviceHandler.h"

// error codes
#define SMB_NETINITERR -110
#define SMB_SMBCFGERR  -111
#define SMB_SMBERR -112

extern file_handle initial_SMB;

extern device_info* deviceHandler_SMB_info();

int deviceHandler_SMB_readDir(file_handle*, file_handle**, unsigned int type);
int deviceHandler_SMB_readFile(file_handle*, void*, unsigned int);
int deviceHandler_SMB_seekFile(file_handle*, unsigned int, unsigned int);
int deviceHandler_SMB_init(file_handle* file);
int deviceHandler_SMB_deinit(file_handle* file);


#endif

