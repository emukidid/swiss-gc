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
extern BNR blankbanner;

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

void fixBannerDesc(char *str, int len) {
	const char *end = str + len;
	for (char *s1 = str; s1 < end && *s1; s1++) {
		if (*s1 == ' ') {
			for (char *s2 = s1;;) {
				if (++s2 == end || *s2 == '\0') {
					memset(s1, '\0', s2 - s1);
					return;
				} else if (*s2 != ' ') {
					if (s2 - s1 > 2) {
						if (*s2 != '\n')
							*s1++ = '\n';
						s2 = mempcpy(s1, s2, end - s2);
						memset(s2, '\0', end - s2);
					}
					break;
				}
			}
		}
	}
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
			snprintf(f->meta->bannerDesc.description, BNR_DESC_LEN, "%.32s\n%.32s", &comment[0], &comment[32]);
			fixBannerDesc(f->meta->bannerDesc.description, BNR_DESC_LEN);
		}
	}
}

void populate_game_meta(file_handle *f, u32 bannerOffset, u32 bannerSize) {
	f->meta->bannerSum = 0xFFFF;
	f->meta->bannerSize = BNR_PIXELDATA_LEN;
	f->meta->banner = memalign(32,BNR_PIXELDATA_LEN);
	memcpy(f->meta->banner,blankbanner.pixelData,BNR_PIXELDATA_LEN);
	if(bannerOffset == -1 || bannerOffset + bannerSize > f->size) {
		print_gecko("Banner not found or out of range\r\n");
	}
	else if(bannerSize) {
		BNR *banner = memalign(32, bannerSize);
		devices[DEVICE_CUR]->seekFile(f, bannerOffset, DEVICE_HANDLER_SEEK_SET);
		if(devices[DEVICE_CUR]->readFile(f, banner, bannerSize) != bannerSize) {
			print_gecko("Banner read failed %i from offset %08X\r\n", bannerSize, bannerOffset);
		}
		else {
			if(!memcmp(banner->magic, "BNR1", 4)) {
				f->meta->bannerSum = fletcher16(banner, bannerSize);
				memcpy(f->meta->banner, banner->pixelData, f->meta->bannerSize);
				memcpy(&f->meta->bannerDesc, &banner->desc[0], sizeof(f->meta->bannerDesc));
			}
			else if(!memcmp(banner->magic, "BNR2", 4)) {
				f->meta->bannerSum = fletcher16(banner, bannerSize);
				memcpy(f->meta->banner, banner->pixelData, f->meta->bannerSize);
				memcpy(&f->meta->bannerDesc, &banner->desc[swissSettings.sramLanguage], sizeof(f->meta->bannerDesc));
			}
			fixBannerDesc(f->meta->bannerDesc.gameName, BNR_SHORT_TEXT_LEN);
			if(strnlen(f->meta->bannerDesc.gameName, BNR_SHORT_TEXT_LEN))
				f->meta->displayName = f->meta->bannerDesc.gameName;
			fixBannerDesc(f->meta->bannerDesc.company, BNR_SHORT_TEXT_LEN);
			fixBannerDesc(f->meta->bannerDesc.fullGameName, BNR_FULL_TEXT_LEN);
			if(strnlen(f->meta->bannerDesc.fullGameName, BNR_FULL_TEXT_LEN))
				f->meta->displayName = f->meta->bannerDesc.fullGameName;
			fixBannerDesc(f->meta->bannerDesc.fullCompany, BNR_FULL_TEXT_LEN);
			// Some banners only have empty spaces as padding until they hit a new line in the IPL
			fixBannerDesc(f->meta->bannerDesc.description, BNR_DESC_LEN);
			// ...and some banners have no CR/LF and we'd like a sane wrap point
			if(strlen(f->meta->bannerDesc.description) > 50) {
				char *desc_ptr = f->meta->bannerDesc.description;
				if(!strchr(desc_ptr, '\n')) {
					desc_ptr+=(strlen(desc_ptr) / 2);
					if((desc_ptr = strchr(desc_ptr, ' '))) {
						*desc_ptr = '\n';
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
				f->meta->bannerSum = 0xFFFF;
				f->meta->bannerSize = BNR_PIXELDATA_LEN;
				f->meta->banner = memalign(32,BNR_PIXELDATA_LEN);
				memcpy(f->meta->banner,blankbanner.pixelData,BNR_PIXELDATA_LEN);
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
				f->meta->displayName = strncpy(f->meta->bannerDesc.fullGameName, isoInfo->name, BNR_FULL_TEXT_LEN);
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
					f->meta->displayName = strncpy(f->meta->bannerDesc.gameName, stat.filename, BNR_SHORT_TEXT_LEN);
				}
			}
			else if(endsWith(f->name,".gci") || endsWith(f->name,".gcs") || endsWith(f->name,".sav")) {
				GCI gci;
				devices[DEVICE_CUR]->seekFile(f, 0, DEVICE_HANDLER_SEEK_SET);
				if(devices[DEVICE_CUR]->readFile(f, &gci, sizeof(GCI)) == sizeof(GCI)) {
					if(!memcmp(&gci, "DATELGC_SAVE", 12)) {
						devices[DEVICE_CUR]->seekFile(f, 0x80, DEVICE_HANDLER_SEEK_SET);
						devices[DEVICE_CUR]->readFile(f, &gci, sizeof(GCI));
						#pragma GCC diagnostic push
						#pragma GCC diagnostic ignored "-Wrestrict"
						swab(&gci.reserved01, &gci.reserved01, 2);
						swab(&gci.icon_addr,  &gci.icon_addr, 20);
						#pragma GCC diagnostic pop
					}
					else if(!memcmp(&gci, "GCSAVE", 6)) {
						devices[DEVICE_CUR]->seekFile(f, 0x110, DEVICE_HANDLER_SEEK_SET);
						devices[DEVICE_CUR]->readFile(f, &gci, sizeof(GCI));
					}
					if(f->size - f->offset == gci.filesize8 * 8192) {
						if(gci.icon_addr != -1) gci.icon_addr += f->offset;
						if(gci.comment_addr != -1) gci.comment_addr += f->offset;
						populate_save_meta(f, gci.banner_fmt, gci.icon_addr, gci.comment_addr);
						char region = getGCIRegion((const char*)gci.gamecode);
						if(region == 'J')
							f->meta->regionTexObj = &ntscjTexObj;
						else if(region == 'E')
							f->meta->regionTexObj = &ntscuTexObj;
						else if(region == 'P')
							f->meta->regionTexObj = &palTexObj;
						f->meta->displayName = strncpy(f->meta->bannerDesc.gameName, gci.filename, BNR_SHORT_TEXT_LEN);
					}
				}
			}
			else if(endsWith(f->name,".gcm") || endsWith(f->name,".iso")) {
				DiskHeader *diskHeader = get_gcm_header(f);
				if(diskHeader) {
					u32 bannerOffset = 0, bannerSize = f->size;
					if(!get_gcm_banner_fast(diskHeader, &bannerOffset, &bannerSize))
						get_gcm_banner(f, &bannerOffset, &bannerSize);
					populate_game_meta(f, bannerOffset, bannerSize);
					get_gcm_title(diskHeader, f->meta);
					// Assign GCM region texture
					char region = wodeRegionToChar(diskHeader->RegionCode);
					if(region == 'J')
						f->meta->regionTexObj = &ntscjTexObj;
					else if(region == 'E')
						f->meta->regionTexObj = &ntscuTexObj;
					else if(region == 'P')
						f->meta->regionTexObj = &palTexObj;
					memcpy(&f->meta->diskId, diskHeader, sizeof(dvddiskid));
					free(diskHeader);
				}
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
			
			file_handle *bannerFile = calloc(1, sizeof(file_handle));
			concat_path(bannerFile->name, f->name, "opening.bnr");
			bannerFile->meta = f->meta;
			
			if (devices[DEVICE_CUR]->readFile(bannerFile, NULL, 0) == 0 && bannerFile->size) {
				populate_game_meta(bannerFile, 0, bannerFile->size);
				devices[DEVICE_CUR]->closeFile(bannerFile);
				
				file_handle *bootFile = calloc(1, sizeof(file_handle));
				concat_path(bootFile->name, f->name, "default.dol");
				bootFile->meta = f->meta;
				
				if (devices[DEVICE_CUR]->readFile(bootFile, NULL, 0) == 0 && bootFile->size) {
					devices[DEVICE_CUR]->closeFile(bootFile);
					
					f = memcpy(f, bootFile, sizeof(file_handle));
					f->meta->fileTypeTexObj = &dolimgTexObj;
				}
				free(bootFile);
			}
			free(bannerFile);
		}
	}
}

file_handle* meta_find_disc2(file_handle *f) {
	file_handle* disc2File = NULL;
	if(is_multi_disc(f->meta)) {
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
