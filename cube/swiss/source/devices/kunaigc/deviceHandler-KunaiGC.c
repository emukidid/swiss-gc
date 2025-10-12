/* deviceHandler-KunaiGC.c
    - device implementation for KunaiGC Flash
    by bbsan2k
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
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
static lfs_t *lfs;

// configuration of the filesystem is provided by this struct
static struct lfs_config cfg = {
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
    .block_cycles = 500,
    .cache_size = W25Q80BV_PAGE_SIZE*8,
    .lookahead_size = 16,
    .disk_version = 0x00020000,
};

file_handle initial_KunaiGC = {
    .name     = "kunai:/",
    .fileType = IS_DIR,
    .device   = &__device_kunaigc,
};

static device_info initial_KunaiGC_info;

device_info* deviceHandler_KunaiGC_info(file_handle* file) {
    device_info* info = NULL;
    lfs_ssize_t size = lfs_fs_size(lfs);
    if (size >= 0) {
        info = &initial_KunaiGC_info;
        info->freeSpace = ((lfs->block_count - size) * lfs->cfg->block_size);
        info->totalSpace = (lfs->block_count * lfs->cfg->block_size);
        info->metric = false;
    }
    return info;
}

s32 deviceHandler_KunaiGC_readDir(file_handle* ffile, file_handle** dir, u32 type) {

    lfs_dir_t *dp = malloc(sizeof(lfs_dir_t));
    if (lfs_dir_open(lfs, dp, getDevicePath(ffile->name)) != LFS_ERR_OK) return -1;
    struct lfs_info entry;
    
    // Set everything up to read
    int num_entries = 1, i = 1;
    *dir = calloc(num_entries, sizeof(file_handle));
    concat_path((*dir)[0].name, ffile->name, "..");
    (*dir)[0].fileType = IS_SPECIAL;
    (*dir)[0].device   = ffile->device;
    
    // Read each entry of the directory
    while (lfs_dir_read(lfs, dp, &entry) == true) {
        if(!strcmp(entry.name, ".") || !strcmp(entry.name, "..")) {
            continue;
        }
        // Do we want this one?
        if((type == -1 || ((entry.type == LFS_TYPE_DIR) ? (type==IS_DIR) : (type==IS_FILE)))) {
            // Make sure we have room for this one
            if(i == num_entries){
                ++num_entries;
                *dir = reallocarray(*dir, num_entries, sizeof(file_handle));
            }
            memset(&(*dir)[i], 0, sizeof(file_handle));
            if(concat_path((*dir)[i].name, ffile->name, entry.name) < PATHNAME_MAX) {
                (*dir)[i].size      = entry.size;
                (*dir)[i].fileType  = (entry.type == LFS_TYPE_DIR) ? IS_DIR : IS_FILE;
                (*dir)[i].blockSize = lfs->cfg->block_size;
                (*dir)[i].device    = ffile->device;
                ++i;
            }
        }
    }
    
    lfs_dir_close(lfs, dp);
    free(dp);
    return i;
}

s32 deviceHandler_KunaiGC_statFile(file_handle* file) {
    struct lfs_info entry;
    int ret = lfs_stat(lfs, getDevicePath(file->name), &entry);
    if (ret == LFS_ERR_OK) {
        file->size     = entry.size;
        file->fileType = (entry.type == LFS_TYPE_DIR) ? IS_DIR : IS_FILE;
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
        file->fp = malloc(sizeof(lfs_file_t));
        if (lfs_file_open(lfs, file->fp, getDevicePath(file->name), LFS_O_RDONLY) != LFS_ERR_OK) {
            free(file->fp);
            file->fp = NULL;
            return -1;
        }
        file->fileType = IS_FILE;
    }
    
    lfs_file_seek(lfs, file->fp, file->offset, LFS_SEEK_SET);
    lfs_ssize_t bytes_read = lfs_file_read(lfs, file->fp, buffer, length);
    file->offset = lfs_file_tell(lfs, file->fp);
    file->size = lfs_file_size(lfs, file->fp);
    return bytes_read;
}

s32 deviceHandler_KunaiGC_writeFile(file_handle* file, const void* buffer, u32 length) {
    if (!file->fp) {
        file->fp = malloc(sizeof(lfs_file_t));
        if (lfs_file_open(lfs, file->fp, getDevicePath(file->name), LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
            free(file->fp);
            file->fp = NULL;
            return -1;
        }
        file->fileType = IS_FILE;
    }
    
    lfs_file_seek(lfs, file->fp, file->offset, LFS_SEEK_SET);
    lfs_ssize_t bytes_written = lfs_file_write(lfs, file->fp, buffer, length);
    file->offset = lfs_file_tell(lfs, file->fp);
    file->size = lfs_file_size(lfs, file->fp);
    return bytes_written;
}

bool deviceHandler_KunaiGC_test() {
    u32 jedec_id = kunai_get_jedecID();
    cfg.block_count = (((1 << (jedec_id & 0xFF)) - KUNAI_OFFS) / cfg.block_size);

    return (jedec_id != 0) && (jedec_id != 0xFFFFFFFF);
}

s32 deviceHandler_KunaiGC_init(file_handle* file) {
    if (!deviceHandler_KunaiGC_test()) return ENODEV;

    lfs = malloc(sizeof(lfs_t));
    if (lfs_mount(lfs, &cfg) != LFS_ERR_OK) {
        free(lfs);
        lfs = NULL;
        return EIO;
    }
    return 0;
}

s32 deviceHandler_KunaiGC_closeFile(file_handle* file) {
    int ret = 0;
    if (file && file->fp) {
        ret = lfs_file_close(lfs, file->fp);
        free(file->fp);
        file->fp = NULL;
    }
    return ret;
}

s32 deviceHandler_KunaiGC_deinit(file_handle* file) {
    deviceHandler_KunaiGC_closeFile(file);
    if (lfs) {
        lfs_unmount(lfs);
        free(lfs);
        lfs = NULL;
    }
    return 0;
}

s32 deviceHandler_KunaiGC_deleteFile(file_handle* file) {
    deviceHandler_KunaiGC_closeFile(file);
    return lfs_remove(lfs, getDevicePath(file->name));
}

s32 deviceHandler_KunaiGC_renameFile(file_handle* file, char* name) {
    deviceHandler_KunaiGC_closeFile(file);
    int ret = lfs_rename(lfs, getDevicePath(file->name), getDevicePath(name));
    if (ret == LFS_ERR_OK)
        strcpy(file->name, name);
    return ret;
}

s32 deviceHandler_KunaiGC_makeDir(file_handle* file) {
    return lfs_mkdir(lfs, getDevicePath(file->name));
}

char* deviceHandler_KunaiGC_status(file_handle* file) {
    return NULL;
}

DEVICEHANDLER_INTERFACE __device_kunaigc = {
    .deviceUniqueId = DEVICE_ID_L,
    .hwName = "KunaiGC",
    .deviceName = "KunaiGC",
    .deviceDescription = "KunaiGC Flash File System",
    .deviceTexture = {TEX_KUNAIGC, 115, 72, 120, 80},
    .features = FEAT_READ|FEAT_WRITE|FEAT_BOOT_DEVICE|FEAT_CONFIG_DEVICE|FEAT_AUTOLOAD_DOL,
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
