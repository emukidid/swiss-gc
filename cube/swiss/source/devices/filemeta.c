/* filemeta.c
	- file meta gathering
	by emu_kidid
 */

#include <stdio.h>
#include <ogcsys.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <malloc.h>
#include <gcm.h>
#include <main.h>
#include <ogc/lwp_heap.h>
#include "dvd.h"
#include "filemeta.h"
#include "nkit.h"
#include "swiss.h"
#include "deviceHandler.h"
#include "FrameBufferMagic.h"

//this is the blank banner that will be shown if no banner is found on a disc
extern char blankbanner[];

#define NUM_META_MAX (512)
#define META_CACHE_SIZE (sizeof(file_meta) * NUM_META_MAX)

static heap_cntrl* meta_cache = NULL;

void meta_free(file_meta* meta) {
	if(meta_cache && meta) {
		if(meta->banner) {
			free(meta->banner);
			meta->banner = NULL;
		}
		__lwp_heap_free(meta_cache, meta);
	}
}

file_meta* meta_alloc() {
	if(!meta_cache){
		meta_cache = memalign(32, sizeof(heap_cntrl));
		__lwp_heap_init(meta_cache, memalign(32,META_CACHE_SIZE), META_CACHE_SIZE, 32);
	}

	file_meta* meta = __lwp_heap_allocate(meta_cache, sizeof(file_meta));
	// While there's no room to allocate, call release
	file_handle* dirEntries = getCurrentDirEntries();
	while(!meta) {
		int i = 0;
		for (i = 0; i < getCurrentDirEntryCount(); i++) {
			if(!(i >= current_view_start && i <= current_view_end)) {
				if(dirEntries[i].meta) {
					meta_free(dirEntries[i].meta);
					dirEntries[i].meta = NULL;
					break;
				}
			}
		}
		meta = __lwp_heap_allocate(meta_cache, sizeof(file_meta));
	}
	memset(meta, 0, sizeof(file_meta));
	return meta;
}

void meta_create_direct_texture(file_meta* meta) {
	DCFlushRange(meta->banner, meta->bannerSize);
	GX_InitTexObj(&meta->bannerTexObj, meta->banner, 96, 32, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjFilterMode(&meta->bannerTexObj, GX_LINEAR, GX_NEAR);
}

void meta_create_direct_texture_ci(file_meta* meta) {
	DCFlushRange(meta->banner, meta->bannerSize);
	GX_InitTlutObj(&meta->bannerTlutObj, meta->banner + 96 * 32, GX_TL_RGB5A3, 256);
	GX_InitTexObjCI(&meta->bannerTexObj, meta->banner, 96, 32, GX_TF_CI8, GX_CLAMP, GX_CLAMP, GX_FALSE, GX_TLUT0);
	GX_InitTexObjFilterMode(&meta->bannerTexObj, GX_LINEAR, GX_NEAR);
	GX_InitTexObjUserData(&meta->bannerTexObj, &meta->bannerTlutObj);
}

void populate_save_meta(file_handle *f, u8 bannerFormat, u32 bannerOffset, u32 commentOffset) {
	if(bannerOffset != -1) {
		switch(bannerFormat & CARD_BANNER_MASK) {
			case CARD_BANNER_CI:
				f->meta->bannerSize = CARD_BANNER_W * CARD_BANNER_H + 256 * 2;
				f->meta->banner = memalign(32, f->meta->bannerSize);
				devices[DEVICE_CUR]->seekFile(f, bannerOffset, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(f, f->meta->banner, f->meta->bannerSize) == f->meta->bannerSize) {
					meta_create_direct_texture_ci(f->meta);
				}
				else {
					free(f->meta->banner);
					f->meta->banner = NULL;
				}
				break;
			case CARD_BANNER_RGB:
				f->meta->bannerSize = CARD_BANNER_W * CARD_BANNER_H * 2;
				f->meta->banner = memalign(32, f->meta->bannerSize);
				devices[DEVICE_CUR]->seekFile(f, bannerOffset, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(f, f->meta->banner, f->meta->bannerSize) == f->meta->bannerSize) {
					meta_create_direct_texture(f->meta);
				}
				else {
					free(f->meta->banner);
					f->meta->banner = NULL;
				}
				break;
		}
	}
	if(commentOffset != -1) {
		char comment[64];
		devices[DEVICE_CUR]->seekFile(f, commentOffset, DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(f, comment, 64) == 64) {
			strncat(f->meta->bannerDesc.description, &comment[0], 32);
			strlcat(f->meta->bannerDesc.description, "\r\n", BNR_DESC_LEN);
			strncat(f->meta->bannerDesc.description, &comment[32], 32);
		}
	}
}

void populate_game_meta(file_handle *f, u32 bannerOffset, u32 bannerSize) {
	f->meta->bannerSize = BNR_PIXELDATA_LEN;
	f->meta->banner = memalign(32,BNR_PIXELDATA_LEN);
	memcpy(f->meta->banner,blankbanner+0x20,BNR_PIXELDATA_LEN);
	if(!bannerOffset || bannerOffset > f->size) {
		print_gecko("Banner not found or out of range\r\n");
	}
	else {
		BNR *banner = memalign(32, bannerSize);
		devices[DEVICE_CUR]->seekFile(f, bannerOffset, DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(f, banner, bannerSize) != bannerSize) {
			print_gecko("Banner read failed %i from offset %08X\r\n", bannerSize, bannerOffset);
		}
		else {
			if(!memcmp(banner->magic, "BNR1", 4)) {
				memcpy(f->meta->banner, banner->pixelData, BNR_PIXELDATA_LEN);
				memcpy(&f->meta->bannerDesc, &banner->desc[0], sizeof(BNRDesc));
			}
			else if(!memcmp(banner->magic, "BNR2", 4)) {
				memcpy(f->meta->banner, banner->pixelData, BNR_PIXELDATA_LEN);
				memcpy(&f->meta->bannerDesc, &banner->desc[swissSettings.sramLanguage], sizeof(BNRDesc));
			}
			if(strlen(f->meta->bannerDesc.description)) {
				// Some banners only have empty spaces as padding until they hit a new line in the IPL
				char *desc_ptr = f->meta->bannerDesc.description;
				if((desc_ptr = strstr(desc_ptr, "  "))) {
					desc_ptr[0] = '\r';
					desc_ptr[1] = '\n';
				}
				// ...and some banners have no CR/LF and we'd like a sane wrap point
				desc_ptr = f->meta->bannerDesc.description;
				if(!strstr(desc_ptr, "\r") && !strstr(desc_ptr, "\n") && strlen(desc_ptr) > 50) {
					desc_ptr+=(strlen(desc_ptr) / 2);
					if((desc_ptr = strstr(desc_ptr, " "))) {
						desc_ptr[0] = '\r';
					}
				}
			}
		}
		free(banner);
	}
	meta_create_direct_texture(f->meta);
}

void populate_meta(file_handle *f) {
	// If the meta hasn't been created, lets read it.
	if(!f->meta) {
		f->meta = meta_alloc();
		// File detection (GCM, DOL, MP3 etc)
		if(f->fileAttrib==IS_FILE) {
			if(devices[DEVICE_CUR] == &__device_wode) {
				f->meta->bannerSize = BNR_PIXELDATA_LEN;
				f->meta->banner = memalign(32,BNR_PIXELDATA_LEN);
				memcpy(f->meta->banner,blankbanner+0x20,BNR_PIXELDATA_LEN);
				meta_create_direct_texture(f->meta);
				// Assign GCM region texture
				ISOInfo_t* isoInfo = (ISOInfo_t*)&f->other;
				char region = wodeRegionToChar(isoInfo->iso_region);
				if(region == 'J')
					f->meta->regionTexObj = &ntscjTexObj;
				else if(region == 'E')
					f->meta->regionTexObj = &ntscuTexObj;
				else if(region == 'P')
					f->meta->regionTexObj = &palTexObj;
			}
			else if(devices[DEVICE_CUR] == &__device_card_a || devices[DEVICE_CUR] == &__device_card_b) {
				card_dir* dir = (card_dir*)&f->other;
				card_stat stat;
				if(CARD_GetStatus(dir->chn, dir->fileno, &stat) == CARD_ERROR_READY) {
					populate_save_meta(f, stat.banner_fmt, stat.icon_addr, stat.comment_addr);
					char region = getGCIRegion((const char*)stat.gamecode);
					if(region == 'J')
						f->meta->regionTexObj = &ntscjTexObj;
					else if(region == 'E')
						f->meta->regionTexObj = &ntscuTexObj;
					else if(region == 'P')
						f->meta->regionTexObj = &palTexObj;
				}
			}
			else if(endsWith(f->name,".gci")) {
				GCI gci;
				devices[DEVICE_CUR]->seekFile(f, 0, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(f, &gci, sizeof(GCI)) == sizeof(GCI)) {
					if(gci.icon_addr != -1) gci.icon_addr += sizeof(GCI);
					if(gci.comment_addr != -1) gci.comment_addr += sizeof(GCI);
					populate_save_meta(f, gci.banner_fmt, gci.icon_addr, gci.comment_addr);
					char region = getGCIRegion((const char*)gci.gamecode);
					if(region == 'J')
						f->meta->regionTexObj = &ntscjTexObj;
					else if(region == 'E')
						f->meta->regionTexObj = &ntscuTexObj;
					else if(region == 'P')
						f->meta->regionTexObj = &palTexObj;
				}
			}
			else if(endsWith(f->name,".gcm") || endsWith(f->name,".iso")) {
				DiskHeader *header = memalign(32, sizeof(DiskHeader));
				devices[DEVICE_CUR]->seekFile(f, 0, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(f, header, sizeof(DiskHeader)) == sizeof(DiskHeader) && valid_gcm_magic(header)) {
					u32 bannerOffset = 0, bannerSize = f->size;
					if(!get_gcm_banner_fast(header, &bannerOffset, &bannerSize))
						get_gcm_banner(f, &bannerOffset, &bannerSize);
					populate_game_meta(f, bannerOffset, bannerSize);
					// Assign GCM region texture
					char region = wodeRegionToChar(header->RegionCode);
					if(region == 'J')
						f->meta->regionTexObj = &ntscjTexObj;
					else if(region == 'E')
						f->meta->regionTexObj = &ntscuTexObj;
					else if(region == 'P')
						f->meta->regionTexObj = &palTexObj;
					memcpy(&f->meta->diskId, header, sizeof(dvddiskid));
				}
				free(header);
			}
			else if(endsWith(f->name,".tgc")) {
				TGCHeader tgcHeader;
				devices[DEVICE_CUR]->seekFile(f, 0, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(f, &tgcHeader, sizeof(TGCHeader)) == sizeof(TGCHeader) && tgcHeader.magic == TGC_MAGIC) {
					populate_game_meta(f, tgcHeader.bannerStart, tgcHeader.bannerLength);
				}
			}
			if(endsWith(f->name,".dol"))
				f->meta->fileTypeTexObj = &dolimgTexObj;
			else if(endsWith(f->name,".dol+cli"))
				f->meta->fileTypeTexObj = &dolcliimgTexObj;
			else if(endsWith(f->name,".elf"))
				f->meta->fileTypeTexObj = &elfimgTexObj;
			else if(endsWith(f->name,".mp3"))
				f->meta->fileTypeTexObj = &mp3imgTexObj;
			else
				f->meta->fileTypeTexObj = &fileimgTexObj;
		}
		else if (f->fileAttrib == IS_DIR) {
			f->meta->fileTypeTexObj = &dirimgTexObj;
		}
	}
}

file_handle* meta_find_disc2(file_handle *f) {
	file_handle* disc2File = NULL;
	if(f->meta && is_multi_disc(&f->meta->diskId)) {
		file_handle* dirEntries = getCurrentDirEntries();
		for(int i = 0; i < getCurrentDirEntryCount(); i++) {
			if(!dirEntries[i].meta) {
				populate_meta(&dirEntries[i]);
			}
			if(dirEntries[i].meta) {
				if(strncmp((const char*)dirEntries[i].meta->diskId.gamename, (const char*)f->meta->diskId.gamename, 4)) {
					continue;
				}
				if(strncmp((const char*)dirEntries[i].meta->diskId.company, (const char*)f->meta->diskId.company, 2)) {
					continue;
				}
				if(dirEntries[i].meta->diskId.disknum == f->meta->diskId.disknum) {
					continue;
				}
				if(dirEntries[i].meta->diskId.gamever != f->meta->diskId.gamever) {
					continue;
				}
				if(strcasecmp(dirEntries[i].name, f->name) != dirEntries[i].meta->diskId.disknum - f->meta->diskId.disknum) {
					disc2File = &dirEntries[i];
					continue;
				}
				return &dirEntries[i];
			}
		}
	}
	return disc2File;
}
