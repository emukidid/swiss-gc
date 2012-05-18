/***************************************************************************
* Memory Card emulation code for GC/Wii
* emu_kidid 2012
*
***************************************************************************/

// we have 0x1800 bytes to play with at 0x80001800 (code+data), or use above Arena Hi
// This code is placed either at 0x80001800 or Above Arena Hi (depending on the game)
// memory map for our variables that sit at the top 0x100 of memory
#define VAR_AREA 			(0x81800000)		// Base location of our variables
#define VAR_AREA_SIZE		(0x100)				// Size of our variables block
#define VAR_DISC_1_LBA 		(VAR_AREA-0x100)	// is the base file sector for disk 1
#define VAR_DISC_2_LBA 		(VAR_AREA-0xFC)		// is the base file sector for disk 2
#define VAR_CUR_DISC_LBA 	(VAR_AREA-0xF8)		// is the currently selected disk sector
#define VAR_EXI_BUS_SPD 	(VAR_AREA-0xF4)		// is the EXI bus speed (16mhz vs 32mhz)
#define VAR_SD_TYPE 		(VAR_AREA-0xF0)		// is the Card Type (SDHC=0, SD=1)
#define VAR_EXI_FREQ 		(VAR_AREA-0xDC)		// is the EXI frequency (4 = 16mhz, 5 = 32mhz)
#define VAR_EXI_SLOT 		(VAR_AREA-0xD8)		// is the EXI slot (0 = slot a, 1 = slot b)
#define VAR_TMP1  			(VAR_AREA-0xD4)		// space for a variable if required
#define VAR_TMP2  			(VAR_AREA-0xD0)		// space for a variable if required
#define VAR_TMP3  			(VAR_AREA-0xCC)		// space for a variable if required
#define VAR_TMP4  			(VAR_AREA-0xC8)		// space for a variable if required
#define VAR_CB_ADDR			(VAR_AREA-0xC4)		// high level read callback addr
#define VAR_CB_ARG1			(VAR_AREA-0xC0)		// high level read callback r3
#define VAR_CB_ARG2			(VAR_AREA-0xBC)		// high level read callback r4
#define VAR_PROG_MODE		(VAR_AREA-0xB8)		// data/code to overwrite GXRMode obj with for 480p forcing
#define VAR_MUTE_AUDIO		(VAR_AREA-0x20)		// does the user want audio muted during reads?
#define VAR_ASPECT_FLOAT	(VAR_AREA-0x1C)		// Aspect ratio multiply float (8 bytes)
#define VAR_MEMCARD_LBA		(VAR_AREA-0x14)		// Memory card file base on SD
#define VAR_MEMCARD_WORK	(VAR_AREA-0x10)		// Memory card work area 40960 bytes big

#define CARD_RESULT_READY           0
#define CARD_RESULT_NOFILE          -4
#define CARD_RESULT_EXIST           -7

#define CARD_FILENAME_MAX   32
#define CARD_ICON_MAX       8

#define NUM_STAT_BLOCK_ENTRIES 64
#define NUM_STAT_ENTRIES_PER_BLOCK 3

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
	u8 _padding;
	CARDFileInfo info;
	u32 rawoffset;
} CARDStatAndInfo;	// 146 bytes

typedef struct SwissStatBlock
{
	CARDStatAndInfo statandinfo[NUM_STAT_ENTRIES_PER_BLOCK];
	u8 _padding[86];
} SwissStatBlock;	// 512 bytes

typedef struct SwissMemcardHeader
{
	SwissStatBlock statblock[NUM_STAT_BLOCK_ENTRIES];
} SwissMemcardHeader;	// 32768 bytes - 192 save files allowed per game :P

extern void do_read(void *dst, u32 size, u32 offset);
extern void do_write(void *src, u32 size, u32 offset);

void read_wrapper(void *dst, u32 size, u32 offset) {
	u32 old = *(u32*)(VAR_CUR_DISC_LBA);
	*(u32*)(VAR_CUR_DISC_LBA) = *(u32*)(VAR_MEMCARD_LBA);
	do_read(dst, size, offset);
	*(u32*)(VAR_CUR_DISC_LBA) = old;
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
inline u8 *workArea() {
	return (u8*)(*(u32*)VAR_MEMCARD_WORK);	// 32kb of the 40kb
}

// Sync the file table to SD card
void sync_filetable() {
	SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
	do_write(header, sizeof(SwissMemcardHeader), 0);
}

// Find if a file we want exists or not from our memcard image
// Returns the (block << 16 | entry_num) in the block
s32 find_file(s32 fileNo, char* fileName) {
	SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
	// Read our entire header into the work area.
	read_wrapper(header, sizeof(header), 0);	// Read 32kb
	
	int i,j;
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		// Each 512 bytes contains 3 file stat/info entries
		SwissStatBlock *statBlock = (SwissStatBlock*)&(header->statblock[i]);
		for(j = 0; j < NUM_STAT_ENTRIES_PER_BLOCK; j++) {
			if(fileNo && statBlock->statandinfo[j].info.fileNo == fileNo) {
				// we found it, return block and block idx
				return ((i<<16) | (j & 0xF));
			}
			else if(fileName && statBlock->statandinfo[j].stat.fileName[0]) {
				int k, diff = 0;
				for(k = 0; k < _strlen(fileName); k++) {
					if(fileName[k] != statBlock->statandinfo[j].stat.fileName[k]) {
						diff = 1;
					}
				}
				if(!diff) {
					// we found it, return block and block idx
					return ((i<<16) | (j & 0xF));
				}
			}
		}
	}
	
	return CARD_RESULT_NOFILE;
}

s32 card_genericopen(s32 fileNo, char* fileName, CARDFileInfo* fileInfo) {
	// Try to find the file based on fileNo or fileName, whichever is passed in
	s32 statIdx = find_file(fileNo, fileName);
	if(statIdx >= 0) {
		// Populate fileInfo
		SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
		CARDFileInfo *info = (CARDFileInfo*)&(header->statblock[statIdx>>16].statandinfo[statIdx&0xF].info);
		_memcpy(fileInfo, info, sizeof(CARDFileInfo));
		return CARD_RESULT_READY;
	}
	else {
		return CARD_RESULT_NOFILE;		
	}
}

s32 card_open(char* fileName, CARDFileInfo* fileInfo) {
	return card_genericopen(0, fileName, fileInfo);
}

s32 card_fastopen(s32 fileNo, CARDFileInfo* fileInfo) {
	return card_genericopen(fileNo, 0, fileInfo);
}

void card_close(CARDFileInfo* fileInfo) {
	// Update something?
}

s32 find_free_block() {
	int i,j;
	SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		// Each 512 bytes contains 3 file stat/info entries
		SwissStatBlock *statBlock = (SwissStatBlock*)&(header->statblock[i]);
		for(j = 0; j < NUM_STAT_ENTRIES_PER_BLOCK; j++) {
			if(!statBlock->statandinfo[j].info.fileNo) {
				// Found an empty block in our stats collection
				return ((i<<16) | (j & 0xF));
			}
		}
	}
	return 0;
}

s32 find_next_fileno() {
	int i,j,lowest=1;
	SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		// Each 512 bytes contains 3 file stat/info entries
		SwissStatBlock *statBlock = (SwissStatBlock*)&(header->statblock[i]);
		for(j = 0; j < NUM_STAT_ENTRIES_PER_BLOCK; j++) {
			if(statBlock->statandinfo[j].info.fileNo && (statBlock->statandinfo[j].info.fileNo <= lowest)) {
				lowest = statBlock->statandinfo[j].info.fileNo + 1;
			}
		}
	}
	return lowest;
}

s32 find_next_rawoffset(s32 size) {
	int i,j,lowest=32768;
	SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
	for(i = 0; i < NUM_STAT_BLOCK_ENTRIES; i++) {
		// Each 512 bytes contains 3 file stat/info entries
		SwissStatBlock *statBlock = (SwissStatBlock*)&(header->statblock[i]);
		for(j = 0; j < NUM_STAT_ENTRIES_PER_BLOCK; j++) {
			// Can we fit before all entries?
			if(statBlock->statandinfo[j].info.fileNo && ((statBlock->statandinfo[j].rawoffset) <= (lowest+size))) {
				lowest = (statBlock->statandinfo[j].rawoffset + statBlock->statandinfo[j].info.length) + 512;
			}
		}
	}
	return lowest;
}

s32 card_create(char* fileName, u32 size, CARDFileInfo* fileInfo) {
	// Try to find the file based on fileName
	s32 statIdx = find_file(0, fileName);
	if(statIdx >= 0) {
		return CARD_RESULT_EXIST;
	}
	else {
		// find the next free fileNo and rawoffset to store it in our memcard file by iterating through our header
		statIdx = find_free_block();
		s32 fileNo = find_next_fileno();
		s32 rawoffset = find_next_rawoffset(size);
		// Create the stat and info and write it out to our stat file
		SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
		CARDStatAndInfo *info = (CARDStatAndInfo*)&(header->statblock[statIdx>>16].statandinfo[statIdx&0xF]);
		// Internal raw offset
		info->rawoffset = rawoffset;
		// CardFileInfo
		info->info.fileNo = fileNo;
		info->info.chan = 0;
		info->info.offset = 0;
		info->info.length = size;
		info->info.iBlock = 0;
		// CardStat
		_memset(&info->stat, 0, sizeof(CARDStat));
		_memcpy(info->stat.fileName, fileName, _strlen(fileName) );
		info->stat.length = size;
		info->stat.time   = 0x15745a1a;
		_memcpy( info->stat.gameName, (void*)0x80000000, 4 );
		_memcpy( info->stat.company, (void*)0x80000004, 2 );
		info->stat.iconAddr			= -1;
		info->stat.commentAddr		= -1;
		info->stat.offsetBanner		= -1;
		info->stat.offsetBannerTlut	= -1;
		int i;
		for( i=0; i < 8; ++i )
			info->stat.offsetIcon[i] = -1;
		info->stat.offsetIconTlut	= -1;
		sync_filetable();
		// Update the fileInfo passed in with what we've got
		fileInfo->fileNo = fileNo;
		fileInfo->chan = 0;
		fileInfo->offset = 0;
		fileInfo->length = size;
		fileInfo->iBlock = 0;
		return CARD_RESULT_READY;
	}
}

s32 card_delete(char* fileName) {
	s32 statIdx = find_file(0, fileName);
	if(statIdx >= 0) {
		// Delete it and sync the FS to SD
		SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
		CARDStatAndInfo *info = (CARDStatAndInfo*)&(header->statblock[statIdx>>16].statandinfo[statIdx&0xF]);
		_memset(info, 0, sizeof(CARDStatAndInfo));
		sync_filetable();
		return CARD_RESULT_READY;
	}
	else {
		return CARD_RESULT_NOFILE;
	}
}

void card_readwrite(s32 statIdx, void* buf, s32 length, s32 offset, s32 read) {
	SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
	CARDStatAndInfo *info = (CARDStatAndInfo*)&(header->statblock[statIdx>>16].statandinfo[statIdx&0xF].info);
	if(read) {
		do_read(buf, length, offset + info->rawoffset);
	}
	else {
		do_write(buf, length, offset + info->rawoffset);
	}
}

s32 card_read(CARDFileInfo* fileInfo, void* buf, s32 length, s32 offset) {
	// Try to find the file based on fileInfo->fileNo
	s32 statIdx = find_file(fileInfo->fileNo, 0);
	if(statIdx >= 0) {
		// Read to buf from offset for length bytes
		card_readwrite(statIdx, buf, length, offset, 1);
		// TODO Update fileInfo->offset?
		return CARD_RESULT_READY;
	}
	else {
		return CARD_RESULT_NOFILE;	
	}
}

s32 card_write(CARDFileInfo* fileInfo, void* buf, s32 length, s32 offset) {
	// Try to find the file based on fileInfo->fileNo
	s32 statIdx = find_file(fileInfo->fileNo, 0);
	if(statIdx >= 0) {
		// Write buf to offset for length bytes
		card_readwrite(statIdx, buf, length, offset, 0);
		// TODO Update fileInfo->offset?
		return CARD_RESULT_READY;
	}
	else {
		return CARD_RESULT_NOFILE;	
	}
}

s32 card_getstatus(s32 fileNo, CARDStat* stat) {
	// Try to find the file based on fileNo
	s32 statIdx = find_file(fileNo, 0);
	if(statIdx >= 0) {
		// Populate stat
		SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
		CARDStat *cardstat = (CARDStat*)&(header->statblock[statIdx>>16].statandinfo[statIdx&0xF].stat);
		_memcpy(stat, cardstat, sizeof(CARDStat));
		return CARD_RESULT_READY;
	}
	else {
		return CARD_RESULT_NOFILE;		
	}
}

s32 card_setstatus(s32 fileNo, CARDStat* stat) {
	// Try to find the file based on fileNo
	s32 statIdx = find_file(fileNo, 0);
	if(statIdx >= 0) {
		// Update our stat
		SwissMemcardHeader *header = (SwissMemcardHeader*)(workArea());
		CARDStat *cardstat = (CARDStat*)&(header->statblock[statIdx>>16].statandinfo[statIdx&0xF].stat);
		_memcpy(cardstat, stat, sizeof(CARDStat));
		sync_filetable();
		return CARD_RESULT_READY;
	}
	else {
		return CARD_RESULT_NOFILE;		
	}
}

