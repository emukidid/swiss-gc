/***************************************************************************
* Memory Card emulation code for GC/Wii
* emu_kidid 2012
*
***************************************************************************/

#include "../../reservedarea.h"

#define CARD_FILENAME_MAX		32

#define CARD_RESULT_READY		0
#define CARD_RESULT_NOFILE		-4
#define CARD_RESULT_EXIST		-7

#define	CARD_ICON_MAX			8
#define	CARD_ICON_WIDTH			32
#define	CARD_ICON_HEIGHT		32
#define	CARD_BANNER_WIDTH		96
#define	CARD_BANNER_HEIGHT		32

#define	CARD_STAT_ICON_NONE		0
#define	CARD_STAT_ICON_C8		1
#define	CARD_STAT_ICON_RGB5A3	2

#define	CARD_STAT_BANNER_NONE	0
#define	CARD_STAT_BANNER_C8		1
#define	CARD_STAT_BANNER_RGB5A3	2

#define NUM_STAT_BLOCK_ENTRIES 240

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;


typedef struct CARDStat
{
    // read-only (Set by CARDGetStatus)
    char fileName[CARD_FILENAME_MAX];
    u32  length;
    u32  time;           // seconds since 01/01/2000 midnight
    u8   gameName[4];
    u8   company[2];

    // read/write (Set by CARDGetStatus/CARDSetStatus)
    u8   bannerFormat;
    u32  iconAddr;      // offset to the banner, bannerTlut, icon, iconTlut data set.
    u16  iconFormat;
    u16  iconSpeed;
    u32  commentAddr;   // offset to the pair of 32 byte character strings.

    // read-only (Set by CARDGetStatus)
    u32  offsetBanner;
    u32  offsetBannerTlut;
    u32  offsetIcon[CARD_ICON_MAX];
    u32  offsetIconTlut;
    u32  offsetData;
} CARDStat;	// 107 bytes

typedef struct CARDFileInfo
{
    s32 chan;
    s32 fileNo;

    s32 offset;
    s32 length;
    u16 iBlock;
} CARDFileInfo;	// 34 bytes

typedef struct CARDStatAndInfo
{
	CARDStat stat;
	CARDFileInfo info;
	u32 rawoffset;
} CARDStatAndInfo;	// 146 bytes

typedef struct SwissMemcardHeader
{
	CARDStatAndInfo statandinfo[NUM_STAT_BLOCK_ENTRIES];
} SwissMemcardHeader;	// 240 entries, near 32768


extern void do_read(void *dst, u32 size, u32 offset);
extern void do_write(void *src, u32 size, u32 offset);

void read_wrapper(void *dst, u32 size, u32 offset) {
	u32 old = *(volatile u32*)(VAR_CUR_DISC_LBA);
	*(volatile u32*)(VAR_CUR_DISC_LBA) = *(volatile u32*)(VAR_MEMCARD_LBA);
	do_read(dst, size, offset);
	*(volatile u32*)(VAR_CUR_DISC_LBA) = old;
}

void _memcpy(void* dest, const void* src, int count) {
	char* tmp = (char*)dest,* s = (char*)src;
	while (count--)
		*tmp++ = *s++;
}

void _memset(void* s, int c, int count) {
	char* xs = (char*)s;
	while (count--)
		*xs++ = c;
}

s32 _strlen(const char* s)
{
	const char* sc;
	for (sc = s; *sc != '\0'; ++sc);
	return sc - s;
}

// This is a ptr to the workarea provided by the game - it's 40*1024 bytes big
#define WORKAREA ((*(volatile u32*)VAR_MEMCARD_WORK))	// 32kb of the 40kb

// Sync the file table to SD card
void sync_filetable() {
	SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
	//usb_sendbuffer_safe("sync_filetable start\r\n",22);
	do_write(header, 32768, 0);
	//usb_sendbuffer_safe("sync_filetable end\r\n",20);
}

// Read our entire header into the work area.
void card_setup() {
	//usb_sendbuffer_safe("read header\r\n",11);
	SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
	read_wrapper(header, 32768, 0);	// Read 32kb
}

// Find if a file we want exists or not from our memcard image
// Returns the block number in the header
s32 find_file(s32 fileNo, char* fileName) {
	//usb_sendbuffer_safe("find file\r\n",11);
	SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
	
	int i;
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		//usb_sendbuffer_safe(".",1);
		CARDStatAndInfo *statandinfo = &header->statandinfo[i];
		if(fileNo && statandinfo->info.fileNo == fileNo) {
			// we found it, return block
			//usb_sendbuffer_safe("matched on fileNo\r\n",19);
			return i;
		}
		else if(fileName && statandinfo->stat.fileName[0]) {
			//usb_sendbuffer_safe("comparing fileName\r\n",20);
			int k, diff = 0;
			for(k = 0; k < _strlen(fileName); k++) {
				if(fileName[k] != statandinfo->stat.fileName[k]) {
					diff = 1;
				}
			}
			if(!diff) {
				// we found it, return block
				//usb_sendbuffer_safe("matched on fileName\r\n",21);
				return i;
			}
		}
	}
	//usb_sendbuffer_safe("didn't find match\r\n",19);
	return CARD_RESULT_NOFILE;
}

s32 card_genericopen(s32 chan, s32 fileNo, char* fileName, CARDFileInfo* fileInfo) {
	// Try to find the file based on fileNo or fileName, whichever is passed in
	s32 statIdx = find_file(fileNo, fileName);
	if(statIdx >= 0) {
		// Populate fileInfo
		SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
		CARDFileInfo *info = &header->statandinfo[statIdx].info;
		_memcpy(fileInfo, info, sizeof(CARDFileInfo));
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_READY;
		return CARD_RESULT_READY;
	}
	else {
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_NOFILE;
		return CARD_RESULT_NOFILE;		
	}
}

void card_updatestats( CARDStat *CStat )
{
	//usb_sendbuffer_safe("updatestats\r\n",13);
	u32 iconSize,bannerSize,format,offset,i,tLut=0;

	bannerSize	= CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT;
	iconSize	= CARD_ICON_WIDTH * CARD_ICON_HEIGHT;
	offset		= CStat->iconAddr;

	if( CStat->bannerFormat & CARD_STAT_BANNER_C8 )
	{
		CStat->offsetBanner		= offset;
		CStat->offsetBannerTlut	= offset + bannerSize;

		offset += bannerSize + 512;

	} else if( CStat->bannerFormat & CARD_STAT_BANNER_RGB5A3 ) {

		CStat->offsetBanner		=  offset;
		CStat->offsetBannerTlut	= -1;

		offset += bannerSize * 2;

	} else {
					
		CStat->offsetBanner		= -1;
		CStat->offsetBannerTlut	= -1;
	}
				
	for( i=0; i < CARD_ICON_MAX; ++i )
	{
		format = CStat->iconFormat >> ( i * 2 );

		if( format & CARD_STAT_ICON_C8 )
		{
			CStat->offsetIcon[i] = offset;
			offset += iconSize;
			tLut = 1;

		} else if ( format & CARD_STAT_ICON_RGB5A3 ) {
						
			CStat->offsetIcon[i] = offset;
			offset += iconSize * 2;

		} else {
			CStat->offsetIcon[i] = -1;
		}
	}

	if( tLut )
	{
		CStat->offsetIconTlut = offset;
		offset += 512;
	} else {
		CStat->offsetIconTlut = -1;
	}

	CStat->offsetData = offset;
}

s32 card_open(s32 chan, char* fileName, CARDFileInfo* fileInfo) {
	//usb_sendbuffer_safe("card_open\r\n",11);
	return card_genericopen(chan, 0, fileName, fileInfo);
}

s32 card_fastopen(s32 chan, s32 fileNo, CARDFileInfo* fileInfo) {
	//usb_sendbuffer_safe("card_fastopen\r\n",15);
	return card_genericopen(chan, fileNo, 0, fileInfo);
}

void card_close(CARDFileInfo* fileInfo) {
	// Update something?
}

// Find the first free stats block
s32 find_free_block() {
	int i;
	SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		if(!header->statandinfo[i].info.fileNo) {
			// Found an empty block in our stats collection
			return i;
		}
	}

	return 0;
}

// FileNo starts at 1, not 0.
s32 find_next_fileno() {
	s32 i,lowest=1;
	SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		if(header->statandinfo[i].info.fileNo && (header->statandinfo[i].info.fileNo <= lowest)) {
			lowest = header->statandinfo[i].info.fileNo + 1;
		}
	}
	return lowest;
}

// Lowest offset is just above our "stats" table, at offset 32768 in our file
s32 find_next_rawoffset(s32 size) {
	int i,lowest=32768;
	SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		// Can we fit before all entries?
		if(header->statandinfo[i].info.fileNo && ((header->statandinfo[i].rawoffset) <= (lowest+size))) {
			lowest = (header->statandinfo[i].rawoffset + header->statandinfo[i].info.length) + 512;
		}
	}
	return lowest;
}

s32 card_create(s32 chan, char* fileName, u32 size, CARDFileInfo* fileInfo) {
	//usb_sendbuffer_safe("card_create\r\n",13);
	// Try to find the file based on fileName
	s32 statIdx = find_file(0, fileName);
	if(statIdx >= 0) {
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_EXIST;
		return CARD_RESULT_EXIST;
	}
	else {
		// find the next free fileNo and rawoffset to store it in our memcard file by iterating through our header
		statIdx = find_free_block();
		s32 fileNo = find_next_fileno();
		s32 rawoffset = find_next_rawoffset(size);
		// Create the stat and info and write it out to our stat file
		SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
		CARDStatAndInfo *statandinfo = &header->statandinfo[statIdx];
		CARDFileInfo *info = &statandinfo->info;
		CARDStat *stat = &statandinfo->stat;
		// CardStat
		_memcpy(&stat->fileName[0], fileName, _strlen(fileName) );
		stat->length = size;
		stat->time   = 0x15745a1a;
		_memcpy(&stat->gameName[0], (void*)0x80000000, 6 );
		stat->iconAddr			= -1;
		stat->commentAddr		= -1;
		stat->offsetBanner		= -1;
		stat->offsetBannerTlut	= -1;
		_memset(&stat->offsetIcon[0], 0xFF, 32);
		stat->offsetIconTlut	= -1;
		// CardFileInfo
		info->fileNo = fileNo;
		info->chan = 0;
		info->offset = 0;
		info->length = size;
		info->iBlock = 0;
		// Internal raw offset
		statandinfo->rawoffset = rawoffset;
		sync_filetable();
		// Update the fileInfo passed in with what we've got
		fileInfo->fileNo = fileNo;
		fileInfo->chan = 0;
		fileInfo->offset = 0;
		fileInfo->length = size;
		fileInfo->iBlock = 0;
		//usb_sendbuffer_safe("card_create return\r\n",20);
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_READY;
		return CARD_RESULT_READY;
	}
}

s32 card_delete(s32 chan, char* fileName) {
	//usb_sendbuffer_safe("card_delete\r\n",13);
	s32 statIdx = find_file(0, fileName);
	if(statIdx >= 0) {
		// Delete it and sync the FS to SD
		SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
		CARDStatAndInfo *info = &header->statandinfo[statIdx];
		_memset(info, 0, sizeof(CARDStatAndInfo));
		sync_filetable();
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_READY;
		return CARD_RESULT_READY;
	}
	else {
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_NOFILE;
		return CARD_RESULT_NOFILE;
	}
}

void card_readwrite(s32 statIdx, void* buf, s32 length, s32 offset, s32 read) {
	SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
	CARDStatAndInfo *info = &header->statandinfo[statIdx];
	if(read) {
		read_wrapper(buf, length, offset + info->rawoffset);
	}
	else {
		do_write(buf, length, offset + info->rawoffset);
	}
}

s32 card_read(CARDFileInfo* fileInfo, void* buf, s32 length, s32 offset) {
	//usb_sendbuffer_safe("card_read\r\n",11);
	// Try to find the file based on fileInfo->fileNo
	s32 statIdx = find_file(fileInfo->fileNo, 0);
	if(statIdx >= 0) {
		// Read to buf from offset for length bytes
		card_readwrite(statIdx, buf, length, offset, 1);
		// TODO Update fileInfo->offset?
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_READY;
		return CARD_RESULT_READY;
	}
	else {
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_NOFILE;
		return CARD_RESULT_NOFILE;	
	}
}

s32 card_write(CARDFileInfo* fileInfo, void* buf, s32 length, s32 offset) {
	//usb_sendbuffer_safe("card_write\r\n",12);
	// Try to find the file based on fileInfo->fileNo
	s32 statIdx = find_file(fileInfo->fileNo, 0);
	if(statIdx >= 0) {
		// Write buf to offset for length bytes
		card_readwrite(statIdx, buf, length, offset, 0);
		// TODO Update fileInfo->offset?
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_READY;
		return CARD_RESULT_READY;
	}
	else {
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_NOFILE;
		return CARD_RESULT_NOFILE;	
	}
}

s32 card_getstatus(s32 chan, s32 fileNo, CARDStat* stat) {
	//usb_sendbuffer_safe("card_getstatus\r\n",16);
	// Try to find the file based on fileNo
	s32 statIdx = find_file(fileNo, 0);
	if(statIdx >= 0) {
		// Populate stat
		SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
		CARDStat *cardstat = &header->statandinfo[statIdx].stat;
		card_updatestats(cardstat);
		_memcpy(stat, cardstat, sizeof(CARDStat));
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_READY;
		return CARD_RESULT_READY;
	}
	else {
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_NOFILE;
		return CARD_RESULT_NOFILE;		
	}
}

s32 card_setstatus(s32 chan, s32 fileNo, CARDStat* stat) {
	//usb_sendbuffer_safe("card_setstatus\r\n",16);
	// Try to find the file based on fileNo
	s32 statIdx = find_file(fileNo, 0);
	if(statIdx >= 0) {
		// Update our stat
		SwissMemcardHeader *header = (SwissMemcardHeader*)(WORKAREA);
		CARDStat *cardstat = &header->statandinfo[statIdx].stat;
		cardstat->bannerFormat	= stat->bannerFormat;
		cardstat->iconAddr		= stat->iconAddr;
		cardstat->iconFormat	= stat->iconFormat;
		cardstat->iconSpeed		= stat->iconSpeed;
		cardstat->commentAddr	= stat->commentAddr;
		// update stats on cardstat
		card_updatestats(cardstat);
		sync_filetable();
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_READY;
		return CARD_RESULT_READY;
	}
	else {
		*(volatile u32*)(VAR_MEMCARD_RESULT) = CARD_RESULT_NOFILE;
		return CARD_RESULT_NOFILE;		
	}
}

