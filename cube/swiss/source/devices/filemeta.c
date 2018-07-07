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
#include "swiss.h"
#include "deviceHandler.h"
#include "FrameBufferMagic.h"

//this is the blank banner that will be shown if no banner is found on a disc
extern char blankbanner[];
// Banner is 96 cols * 32 lines in RGB5A3 fmt
#define BannerSize (96*32*2)

#define NUM_META_MAX (512)
#define META_CACHE_SIZE (sizeof(file_meta) * NUM_META_MAX)

static heap_cntrl* meta_cache = NULL;

file_meta* create_basic_meta(const u8* img, const u32 img_size) {
	file_meta* _meta = (file_meta*)memalign(32,sizeof(file_meta));
	memset(_meta, 0, sizeof(file_meta));
	TPLFile *tplFile = (TPLFile*) memalign(32,sizeof(TPLFile));
	memset(tplFile,0,sizeof(TPLFile));				
	_meta->banner = memalign(32,img_size);
	memcpy(_meta->banner,(void *)img, img_size);
	DCFlushRange(_meta->banner, img_size);
	TPL_OpenTPLFromMemory(tplFile, (void *)_meta->banner, img_size);
	TPL_GetTexture(tplFile,0,&_meta->bannerTexObj);
	free(tplFile);
	return _meta;
}

void meta_free(void* ptr) {
	if(meta_cache && ptr) {
		__lwp_heap_free(meta_cache, ptr);
	}
}

void* meta_alloc(unsigned int size){
	if(!meta_cache){
		meta_cache = memalign(32,sizeof(heap_cntrl));
		__lwp_heap_init(meta_cache, memalign(32,META_CACHE_SIZE), META_CACHE_SIZE, 32);
	}

	void* ptr = __lwp_heap_allocate(meta_cache, size);
	// While there's no room to allocate, call release
	while(!ptr) {
		int i = 0;
		for (i = 0; i < files; i++) {
			if(!(i >= current_view_start && i <= current_view_end)) {
				if(allFiles[i].meta) {
					meta_free(allFiles[i].meta);
					allFiles[i].meta = NULL;
					break;
				}
			}
		}
		ptr = __lwp_heap_allocate(meta_cache, size);
	}
	
	return ptr;
}

void populate_meta(file_handle *f) {
	// If the meta hasn't been created, lets read it.
	
	if(!f->meta) {
		
		// File detection (GCM, DOL, MP3 etc)
		if(f->fileAttrib==IS_FILE) {
			//print_gecko("Creating Meta for FILE %s\r\n", f->name);
			
			// If it's a GCM or ISO or DVD Disc
			if(endsWith(f->name,".iso") || endsWith(f->name,".gcm")) {
				
				if(devices[DEVICE_CUR] == &__device_wode) {
					f->meta = (file_meta*)meta_alloc(sizeof(file_meta));
					memset(f->meta, 0, sizeof(file_meta));
					// Assign GCM region texture
					ISOInfo_t* isoInfo = (ISOInfo_t*)&f->other;
					char region = wodeRegionToChar(isoInfo->iso_region);
					if(region == 'E')
						f->meta->regionTexId = TEX_NTSCU;
					else if(region == 'J')
						f->meta->regionTexId = TEX_NTSCJ;
					else if(region == 'P')
						f->meta->regionTexId = TEX_PAL;
					else if(region == 'E')
						f->meta->regionTexId = TEX_PAL;
				}
				else {
					f->meta = (file_meta*)meta_alloc(sizeof(file_meta));
					memset(f->meta, 0, sizeof(file_meta));
					DiskHeader *header = memalign(32, sizeof(DiskHeader));
					devices[DEVICE_CUR]->seekFile(f, 0, DEVICE_HANDLER_SEEK_SET);
					devices[DEVICE_CUR]->readFile(f, header, sizeof(DiskHeader));
					
					if(header->DVDMagicWord == DVD_MAGIC) {
						//print_gecko("FILE identifed as valid GCM\r\n");
						unsigned int bannerOffset = getBannerOffset(f);
						f->meta->banner = memalign(32,BannerSize);
						if(!bannerOffset) {
							//print_gecko("Banner not found\r\n");
							memcpy(f->meta->banner,blankbanner+0x20,BannerSize);
						}
						else
						{
							devices[DEVICE_CUR]->seekFile(f,bannerOffset+0x20,DEVICE_HANDLER_SEEK_SET);
							if(devices[DEVICE_CUR]->readFile(f,f->meta->banner,BannerSize)!=BannerSize) {
								memcpy(f->meta->banner,blankbanner+0x20,BannerSize);
							}
							//print_gecko("Read banner from %08X+0x20\r\n", bannerOffset);
							
							char bnrType[8];
							// If this is a BNR2 banner, show the proper description for the language the console is set to
							devices[DEVICE_CUR]->seekFile(f,bannerOffset,DEVICE_HANDLER_SEEK_SET);
							devices[DEVICE_CUR]->readFile(f,&bnrType[0],4);
							if(!strncmp(bnrType, "BNR2", 4)) {
								devices[DEVICE_CUR]->seekFile(f,bannerOffset+(0x18e0+(swissSettings.sramLanguage*0x0140)),DEVICE_HANDLER_SEEK_SET);
							}
							else {
								devices[DEVICE_CUR]->seekFile(f,bannerOffset+0x18e0,DEVICE_HANDLER_SEEK_SET);
							}
							devices[DEVICE_CUR]->readFile(f,&f->meta->description[0],0x80);
							//print_gecko("Meta Description: [%s]\r\n",&f->meta->description[0]);
						}
						DCFlushRange(f->meta->banner,BannerSize);
						GX_InitTexObj(&f->meta->bannerTexObj,f->meta->banner,96,32,GX_TF_RGB5A3,GX_CLAMP,GX_CLAMP,GX_FALSE);
						//print_gecko("Meta Gathering complete\r\n\r\n");
						// Assign GCM region texture
						if(header->CountryCode == 'E')
							f->meta->regionTexId = TEX_NTSCU;
						else if(header->CountryCode == 'J')
							f->meta->regionTexId = TEX_NTSCJ;
						else if(header->CountryCode == 'P')
							f->meta->regionTexId = TEX_PAL;
						else if(header->CountryCode == 'U')
							f->meta->regionTexId = TEX_PAL;
							
						// TODO GCM file type fileTypeTexId
					}
					if(header) free(header);
				}
			}
			else if(endsWith(f->name,".mp3")) {	//MP3
				f->meta = create_basic_meta(mp3img_tpl, mp3img_tpl_size);
			}
			else if(endsWith(f->name,".dol")) {	//DOL
				f->meta = create_basic_meta(dolimg_tpl, dolimg_tpl_size);
			}
			else if(endsWith(f->name,".dol+cli")) {	//DOL+CLI
				f->meta = create_basic_meta(dolcliimg_tpl, dolcliimg_tpl_size);
			}
			else {
				f->meta = create_basic_meta(fileimg_tpl, fileimg_tpl_size);
			}
		}
		else if (f->fileAttrib == IS_DIR) {
			f->meta = create_basic_meta(dirimg_tpl, dirimg_tpl_size);
		}
	}
}

