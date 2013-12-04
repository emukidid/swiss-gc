/* deviceHandler-wiikeyfusion.c
	- device implementation for Wiikey Fusion (FAT filesystem)
	by emu_kidid
 */

#include <fat.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ogc/dvd.h>
#include <sys/dir.h>
#include <sys/statvfs.h>
#include <ogc/machine/processor.h>
#include <sdcard/gcsd.h>
#include "deviceHandler.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "wkf.h"
#include "frag.h"
#include "patcher.h"

const DISC_INTERFACE* wkf = &__io_wkf;
int wkfFragSetupReq = 0;

file_handle initial_WKF =
	{ "wkf:/",       // directory
	  0ULL,     // fileBase (u64)
	  0,        // offset
	  0,        // size
	  IS_DIR,
	  0,
	  0
	};


device_info initial_WKF_info = {
	TEX_WIIKEY,
	0,
	0
};
	
device_info* deviceHandler_WKF_info() {
	return &initial_WKF_info;
}

int deviceHandler_WKF_readDir(file_handle* ffile, file_handle** dir, unsigned int type){	

	DIR* dp = opendir( ffile->name );
	if(!dp) return -1;
	struct dirent *entry;
	struct stat fstat;
	
	// Set everything up to read
	int num_entries = 1, i = 1;
	char file_name[1024];
	*dir = malloc( num_entries * sizeof(file_handle) );
	memset(*dir,0,sizeof(file_handle) * num_entries);
	(*dir)[0].fileAttrib = IS_SPECIAL;
	strcpy((*dir)[0].name, "..");
	
	// Read each entry of the directory
	while( (entry = readdir(dp)) != NULL ){
		if(strlen(entry->d_name) <= 2  && (entry->d_name[0] == '.' || entry->d_name[1] == '.')) {
			continue;
		}
		memset(&file_name[0],0,1024);
		sprintf(&file_name[0], "%s/%s", ffile->name, entry->d_name);
		stat(&file_name[0],&fstat);
		// Do we want this one?
		if(type == -1 || ((fstat.st_mode & S_IFDIR) ? (type==IS_DIR) : (type==IS_FILE))) {
			// Make sure we have room for this one
			if(i == num_entries){
				++num_entries;
				*dir = realloc( *dir, num_entries * sizeof(file_handle) ); 
			}
			sprintf((*dir)[i].name, "%s/%s", ffile->name, entry->d_name);
			(*dir)[i].offset = 0;
			(*dir)[i].size     = fstat.st_size;
			(*dir)[i].fileAttrib   = (fstat.st_mode & S_IFDIR) ? IS_DIR : IS_FILE;
			(*dir)[i].fp = 0;
			if((*dir)[i].fileAttrib == IS_FILE) {
				get_frag_list((*dir)[i].name);
				u32 file_base = frag_list->num > 1 ? -1 : frag_list->frag[0].sector;
				(*dir)[i].fileBase = file_base;
			}
			else {
				(*dir)[i].fileBase = 0;
			}		
			++i;
		}
	}
	
	closedir(dp);
	return num_entries;
}


int deviceHandler_WKF_seekFile(file_handle* file, unsigned int where, unsigned int type){
	if(type == DEVICE_HANDLER_SEEK_SET) file->offset = where;
	else if(type == DEVICE_HANDLER_SEEK_CUR) file->offset += where;
	return file->offset;
}


int deviceHandler_WKF_readFile(file_handle* file, void* buffer, unsigned int length){
	if(!file->fp) {
		file->fp = fopen( file->name, "rb" );
	}
	if(!file->fp) return -1;
	
	fseek(file->fp, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, file->fp);
	if(bytes_read > 0) file->offset += bytes_read;
	return bytes_read;
}


int deviceHandler_WKF_writeFile(file_handle* file, void* buffer, unsigned int length){
	return -1;
}


int deviceHandler_WKF_setupFile(file_handle* file, file_handle* file2) {
	// If there are 2 discs, we only allow 5 fragments per disc.
	int maxFrags = file2 ? ((VAR_FRAG_SIZE/12)/2) : (VAR_FRAG_SIZE/12), i = 0, frags = 0;
	u32 *fragList = (u32*)VAR_FRAG_LIST;
	
	memset((void*)VAR_FRAG_LIST, 0, VAR_FRAG_SIZE);
	
	// If disc 1 is fragmented, make a note of the fragments and their sizes
	get_frag_list(file->name);
	if(frag_list->num < maxFrags) {
		for(i = 0; i < frag_list->num; i++) {
			fragList[frags*3] = frag_list->frag[i].offset*512;
			fragList[(frags*3)+1] = frag_list->frag[i].count*512;
			fragList[(frags*3)+2] = frag_list->frag[i].sector;
			//print_gecko("Wrote Frag: ofs: %08X count: %08X sector: %08X\r\n",
			//			fragList[frags*3],fragList[(frags*3)+1],fragList[(frags*3)+2]);
			frags++;
		}
	}
	else {
		// file is too fragmented - go defrag it!
		return 0;
	}
		
	// If there is a disc 2 and it's fragmented, make a note of the fragments and their sizes
	if(file2) {
		// No fragment room left for the second disc, fail.
		if(frags+1 == maxFrags) {
			return 0;
		}
		get_frag_list(file2->name);
		if(frag_list->num < maxFrags) {
			for(i = 0; i < frag_list->num; i++) {
				fragList[(frags*3) + (maxFrags*3)] = frag_list->frag[i].offset*512;
				fragList[((frags*3) + 1) + (maxFrags*3)]  = frag_list->frag[i].count*512;
				fragList[((frags*3) + 2) + (maxFrags*3)] = frag_list->frag[i].sector;
				//print_gecko("Wrote Frag: ofs: %08X count: %08X sector: %08X\r\n",
				//		fragList[frags*3],fragList[(frags*3)+1],fragList[(frags*3)+2]);
				frags++;
			}
		}
		else {
			// file is too fragmented - go defrag it!
			return 0;
		}
	}
	
	// Disk 1 base sector
	*(volatile unsigned int*)VAR_DISC_1_LBA = fragList[2];
	// Disk 2 base sector
	*(volatile unsigned int*)VAR_DISC_2_LBA = file2 ? fragList[2 + (maxFrags*3)]:fragList[2];
	// Currently selected disk base sector
	*(volatile unsigned int*)VAR_CUR_DISC_LBA = fragList[2];
	
	wkfFragSetupReq = (file2 && frags > 2) ? 1 : frags>1;
	
	return 1;
}

int deviceHandler_WKF_init(file_handle* file){
	struct statvfs buf;
	
	DrawFrameStart();
	DrawMessageBox(D_INFO,"Init Wiikey Fusion");
	DrawFrameFinish();
	int ret = fatMountSimple ("wkf", wkf) ? 1 : 0;
	if(ret)
		statvfs("wkf", &buf);

	initial_WKF_info.freeSpaceInKB = ret ? (u32)((uint64_t)(buf.f_bsize*buf.f_bfree)/1024):0;
	initial_WKF_info.totalSpaceInKB = ret ? (u32)((uint64_t)(buf.f_bsize*buf.f_blocks)/1024):0;
	return ret;
}

int deviceHandler_WKF_deinit(file_handle* file) {
	if(file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
	return 0;
}

int deviceHandler_WKF_deleteFile(file_handle* file) {
	return -1;
}

