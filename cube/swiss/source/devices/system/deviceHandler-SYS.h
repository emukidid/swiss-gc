/* deviceHandler-SYS.h
        - virtual device interface for dumping ROMs
        by Streetwalrus
 */

#ifndef DEVICE_HANDLER_SYS_H
#define DEVICE_HANDLER_SYS_H

#include "../deviceHandler.h"

extern file_handle initial_SYS;
extern device_info* deviceHandler_SYS_info();

int deviceHandler_SYS_init(file_handle* file);
int deviceHandler_SYS_readDir(file_handle* ffile, file_handle** dir, unsigned int type);
int deviceHandler_SYS_readFile(file_handle* file, void* buffer, unsigned int length);
int deviceHandler_SYS_writeFile(file_handle* file, void* buffer, unsigned int length);
int deviceHandler_SYS_deleteFile(file_handle* file);
int deviceHandler_SYS_seekFile(file_handle* file, unsigned int where, unsigned int type);
int deviceHandler_SYS_setupFile(file_handle* file, file_handle* file2);
int deviceHandler_SYS_closeFile(file_handle* file);
int deviceHandler_SYS_deinit();

int read_rom_ipl(unsigned int offset, void* buffer, unsigned int length);
int read_rom_ipl_clear(unsigned int offset, void* buffer, unsigned int length);
int read_rom_sram(unsigned int offset, void* buffer, unsigned int length);
int read_rom_stub(unsigned int offset, void* buffer, unsigned int length);

#endif
