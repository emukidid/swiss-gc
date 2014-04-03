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
#include "banner.h"
#include "dvd.h"
#include "swiss.h"
#include "deviceHandler.h"
#include "mp3img_tpl.h"
#include "dolimg_tpl.h"


// Banner is 96 cols * 32 lines in RGB5A3 fmt
#define BannerSize (96*32*2)

void populate_meta(file_handle *f) {
	// If the meta hasn't been created, lets read it.
	
	if(!f->meta) {
		
		// File detection (GCM, DOL, MP3 etc)
		if(f->fileAttrib==IS_FILE) {
			//print_gecko("Creating Meta for FILE %s\r\n", f->name);
			
			// If it's a GCM or ISO or DVD Disc
			if((strstr(f->name,".iso")!=NULL) || (strstr(f->name,".gcm")!=NULL) 
				|| (strstr(f->name,".ISO")!=NULL) || (strstr(f->name,".GCM")!=NULL)) {
				
				f->meta = (file_meta*)memalign(32,sizeof(file_meta));
				memset(f->meta, 0, sizeof(file_meta));
				DiskHeader *header = memalign(32, sizeof(DiskHeader));
				deviceHandler_seekFile(f, 0, DEVICE_HANDLER_SEEK_SET);
				deviceHandler_readFile(f, header, sizeof(DiskHeader));
				
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
						deviceHandler_seekFile(f,bannerOffset+0x20,DEVICE_HANDLER_SEEK_SET);
						if(deviceHandler_readFile(f,f->meta->banner,BannerSize)!=BannerSize) {
							memcpy(f->meta->banner,blankbanner+0x20,BannerSize);
						}
						//print_gecko("Read banner from %08X+0x20\r\n", bannerOffset);
						
						char bnrType[8];
						// If this is a BNR2 banner, show the proper description for the language the console is set to
						deviceHandler_seekFile(f,bannerOffset,DEVICE_HANDLER_SEEK_SET);
						deviceHandler_readFile(f,&bnrType[0],4);
						if(!strncmp(bnrType, "BNR2", 4)) {
							deviceHandler_seekFile(f,bannerOffset+(0x18e0+(swissSettings.sramLanguage*0x0140)),DEVICE_HANDLER_SEEK_SET);
						}
						else {
							deviceHandler_seekFile(f,bannerOffset+0x18e0,DEVICE_HANDLER_SEEK_SET);
						}
						deviceHandler_readFile(f,&f->meta->description[0],0x80);
						//print_gecko("Meta Description: [%s]\r\n",&f->meta->description[0]);
					}
					DCFlushRange(f->meta->banner,BannerSize);
					GX_InitTexObj(&f->meta->bannerTexObj,f->meta->banner,96,32,GX_TF_RGB5A3,GX_CLAMP,GX_CLAMP,GX_FALSE);
					print_gecko("Meta Gathering complete\r\n\r\n");
					// TODO region regionTextureId
					// TODO GCM file type fileTypeTexId
				}
				if(header) free(header);
			}
			else if((strstr(f->name,".MP3")!=NULL) || (strstr(f->name,".mp3")!=NULL)) {	//MP3
				f->meta = (file_meta*)memalign(32,sizeof(file_meta));
				memset(f->meta, 0, sizeof(file_meta));
				TPLFile *tplFile = (TPLFile*) memalign(32,sizeof(TPLFile));
				memset(tplFile,0,sizeof(TPLFile));				
				f->meta->banner = memalign(32,mp3img_tpl_size);
				memcpy(f->meta->banner,(void *)mp3img_tpl, mp3img_tpl_size);
				DCFlushRange(f->meta->banner, mp3img_tpl_size);
				TPL_OpenTPLFromMemory(tplFile, (void *)f->meta->banner, mp3img_tpl_size);
				TPL_GetTexture(tplFile,0,&f->meta->bannerTexObj);
				free(tplFile);
			}
			else if((strstr(f->name,".DOL")!=NULL) || (strstr(f->name,".dol")!=NULL)) {	//DOL
				f->meta = (file_meta*)memalign(32,sizeof(file_meta));
				memset(f->meta, 0, sizeof(file_meta));
				TPLFile *tplFile = (TPLFile*) memalign(32,sizeof(TPLFile));
				memset(tplFile,0,sizeof(TPLFile));				
				f->meta->banner = memalign(32,dolimg_tpl_size);
				memcpy(f->meta->banner,(void *)dolimg_tpl, dolimg_tpl_size);
				DCFlushRange(f->meta->banner, dolimg_tpl_size);
				TPL_OpenTPLFromMemory(tplFile, (void *)f->meta->banner, dolimg_tpl_size);
				TPL_GetTexture(tplFile,0,&f->meta->bannerTexObj);
				free(tplFile);
			}
			else {
				//Unknown, no meta.
				if(f->meta) {
					free(f->meta);
					f->meta = 0;
				}
			}
		}
	}
}
