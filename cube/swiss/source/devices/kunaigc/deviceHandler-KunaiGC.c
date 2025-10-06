/* deviceHandler-KunaiGC.c
    - device implementation for KunaiGC Flash
    by bbsan2k
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler-KunaiGC.h"
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"

#include "lfs.h"
#include "kunaigc.h"
#include "spiflash.h"
#include "util.h"


// variables used by the filesystem
lfs_t lfs;

// configuration of the filesystem is provided by this struct
struct lfs_config cfg = {
    // block device operations
    .read  = kunai_read,
    .prog  = kunai_write,
    .erase = kunai_erase,
    .sync  = kunai_sync,

    // block device configuration
    .read_size = 4,
    .prog_size = W25Q80BV_PAGE_SIZE,
    .block_size = 4096,
    .block_count = 4032,
    .cache_size = W25Q80BV_PAGE_SIZE*8,
    .lookahead_size = 16,
    .block_cycles = 500,
};



file_handle initial_KunaiGC =
{ 
    .name       = "kunai:/",       // directory
    .fileType   = IS_DIR,
    .device     = &__device_kunaigc
};
    
device_info initial_KunaiGC_info = {
    0x200000,
    0x200000,
    false
};

static void readLFSInfo(void) {
    initial_KunaiGC_info.totalSpace = (cfg.block_size * cfg.block_count);
    initial_KunaiGC_info.freeSpace = initial_KunaiGC_info.totalSpace - (lfs_fs_size(&lfs) * cfg.block_size);
}

device_info* deviceHandler_KunaiGC_info(file_handle* file) {
    return &initial_KunaiGC_info;
}

s32 deviceHandler_KunaiGC_makeDir(file_handle* file) {	
    return lfs_mkdir(&lfs, getDevicePath(file->name));
}

s32 deviceHandler_KunaiGC_readDir(file_handle* ffile, file_handle** dir, u32 type) {	
    lfs_dir_t lfsdir;
    struct lfs_info info;

    int err = lfs_dir_open(&lfs, &lfsdir, getDevicePath(ffile->name));
    
    if (err < 0) {
        return -1;
    }
    
    // Set everything up to read
    int num_entries = 1, i = 1;
    *dir = calloc(num_entries, sizeof(file_handle));
    concat_path((*dir)[0].name, ffile->name, "..");
    (*dir)[0].fileType = IS_SPECIAL;
    (*dir)[0].device = ffile->device;
    
    // Read each entry of the directory
    while (lfs_dir_read(&lfs, &lfsdir, &info) > 0) {
        errno = 0;		

        if(!strcmp(info.name, ".") || !strcmp(info.name, "..")) {
            continue;
        }
        
        // Do we want this one?
        if((type == -1 || ((info.type == LFS_TYPE_DIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
            struct lfs_info fstat;

            // Make sure we have room for this one
            if(i == num_entries){
                ++num_entries;
                *dir = reallocarray(*dir, num_entries, sizeof(file_handle));
            }
            memset(&(*dir)[i], 0, sizeof(file_handle));
            if(concat_path((*dir)[i].name, ffile->name, info.name) < PATHNAME_MAX
                && !lfs_stat(&lfs, getDevicePath((*dir)[i].name), &fstat) && fstat.size <= UINT32_MAX) {
                (*dir)[i].size       = fstat.size;
                (*dir)[i].fileAttrib = (fstat.type == LFS_TYPE_DIR) ? ATTRIB_DIRECTORY : 0x00;
                (*dir)[i].fileType   = (fstat.type == LFS_TYPE_DIR) ? IS_DIR : IS_FILE;
                (*dir)[i].device     = ffile->device;
                ++i;
            }
        }
    };
    
    lfs_dir_close(&lfs, &lfsdir);
    
    return i;
}

s32 deviceHandler_KunaiGC_statFile(file_handle* file) {
    struct lfs_info fstat;
    int ret = lfs_stat(&lfs, getDevicePath(file->name), &fstat);
    if (LFS_ERR_OK == ret) {
        file->size = fstat.size;
        file->fileType = IS_FILE;
    }
    return ret;
}

s64 deviceHandler_KunaiGC_seekFile(file_handle* file, s64 where, u32 type) {
    if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
    else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset = file->offset + where;
    else if(type == DEVICE_HANDLER_SEEK_END) file->offset = file->size + where;
    return file->offset;
}

s32 deviceHandler_KunaiGC_readFile(file_handle* file, void* buffer, u32 length) {
    if (!file->fp) {
        file->fp = memalign(32, sizeof(lfs_file_t));
        if (lfs_file_open(&lfs, file->fp, getDevicePath(file->name), LFS_O_RDONLY) < 0) {
            free(file->fp);
            file->fp = NULL;
            return -1;
        }
    }
    if(file->size <= 0) {
        lfs_file_seek(&lfs, (lfs_file_t*)file->fp, 0, LFS_SEEK_END);
        file->size = lfs_file_tell(&lfs, (lfs_file_t*)file->fp);
    }
    lfs_file_seek(&lfs, (lfs_file_t*)file->fp, file->offset, LFS_SEEK_SET);
    size_t bytes_read = lfs_file_read(&lfs, (lfs_file_t*)file->fp, buffer, length);
    file->offset = lfs_file_tell(&lfs, (lfs_file_t*)file->fp);
    return bytes_read;
}

// Assumes a single call to write a file.
s32 deviceHandler_KunaiGC_writeFile(file_handle* file, const void* buffer, u32 length) {
    s32 total_bytes_written = 0;
    if (!file->fp || !(((lfs_file_t*)file->fp)->flags & LFS_O_WRONLY)) {
        file->fp = memalign(32, sizeof(lfs_file_t));
        if (lfs_file_open(&lfs, file->fp, getDevicePath(file->name), LFS_O_RDWR | LFS_O_CREAT) < 0) {
            free(file->fp);
            file->fp = NULL;
            return -1;
        }
    }
    lfs_file_seek(&lfs, (lfs_file_t *) file->fp, file->offset, LFS_SEEK_SET);

    do {
        s32 bytes_written = 0;
        size_t chunkSize = length > cfg.prog_size ? cfg.prog_size : length;
        bytes_written += lfs_file_write(&lfs, (lfs_file_t *) file->fp, (buffer + total_bytes_written), chunkSize);
        length -= bytes_written; 
        total_bytes_written += bytes_written;
    } while (length > 0);

    file->offset = lfs_file_tell(&lfs, (lfs_file_t *) file->fp);
    readLFSInfo();
    return total_bytes_written;
}

s32 deviceHandler_KunaiGC_deleteFile(file_handle* file) {
    if (file->fp ) {
        if (file->fileAttrib == IS_DIR)
            lfs_dir_close(&lfs, (lfs_dir_t*)file->fp);
        else
            lfs_file_close(&lfs, (lfs_file_t*)file->fp);
    }
    
    lfs_remove(&lfs, getDevicePath(file->name));
    readLFSInfo();
    return 0;
}

s32 deviceHandler_KunaiGC_renameFile(file_handle* file, char* name) {
    return lfs_rename(&lfs, getDevicePath(file->name), getDevicePath(name));
}

bool deviceHandler_KunaiGC_test() {
    u32 jedec_id = kunai_get_jedecID();
    cfg.block_count = (((1 << (jedec_id & 0xFFUL)) - KUNAI_OFFS) / cfg.block_size);

    return (jedec_id != 0x0) && (jedec_id != UINT32_MAX);
}


s32 deviceHandler_KunaiGC_init(file_handle* file) {
    if(!deviceHandler_KunaiGC_test()) {
        return ENODEV;
    }

    if (lfs_mount(&lfs, &cfg) < 0) {
        file->status = E_CONNECTFAIL;
        return EFAULT;
    }

    readLFSInfo();


    return 0;
}

s32 deviceHandler_KunaiGC_closeFile(file_handle* file) {
    int ret = 0;
    if(file && file->fp) {
        ret = lfs_file_close(&lfs, (lfs_file_t*)file->fp);
        free(file->fp);
        file->fp = NULL;
		file->status = STATUS_NOT_MAPPED;
    }
    return ret;
}

s32 deviceHandler_KunaiGC_deinit(file_handle* file) {
    deviceHandler_KunaiGC_closeFile(file);
    initial_KunaiGC_info.freeSpace = 0LL;
    initial_KunaiGC_info.totalSpace = 0LL;

    return 0;
}

char* deviceHandler_KunaiGC_status(file_handle* file) {
    return NULL;
}

DEVICEHANDLER_INTERFACE __device_kunaigc = {
    .deviceUniqueId = DEVICE_ID_L,
    .hwName = "KunaiGC IPL",
    .deviceName = "KunaiGC",
    .deviceDescription = "KunaiGC File System",
    .deviceTexture = {TEX_KUNAIGC, 120, 80, 120, 80},
    .features = FEAT_READ|FEAT_WRITE|FEAT_CONFIG_DEVICE|FEAT_BOOT_DEVICE|FEAT_AUTOLOAD_DOL,
    .location = LOC_SYSTEM,
    .initial = &initial_KunaiGC,
    .test = deviceHandler_KunaiGC_test,
    .info = deviceHandler_KunaiGC_info,
    .init = deviceHandler_KunaiGC_init,
    .makeDir = deviceHandler_KunaiGC_makeDir,
    .readDir = deviceHandler_KunaiGC_readDir,
    .statFile = deviceHandler_KunaiGC_statFile,
    .seekFile = deviceHandler_KunaiGC_seekFile,
    .readFile = deviceHandler_KunaiGC_readFile,
    .writeFile = deviceHandler_KunaiGC_writeFile,
    .closeFile = deviceHandler_KunaiGC_closeFile,
    .deleteFile = deviceHandler_KunaiGC_deleteFile,
    .renameFile = deviceHandler_KunaiGC_renameFile,
    .deinit = deviceHandler_KunaiGC_deinit,
    .status = deviceHandler_KunaiGC_status,
};
