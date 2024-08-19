#ifndef DVD_H
#define DVD_H

#include "common.h"

#define DVDRoundUp32KB(x)   (((u32)(x) + (32768 - 1)) & ~(32768 - 1))
#define DVDRoundDown32KB(x) ((u32)(x) & ~(32768 - 1))

typedef struct DVDCommandBlock DVDCommandBlock;

typedef void (*DVDCBCallback)(s32 result, DVDCommandBlock *block);

typedef struct DVDDiskID DVDDiskID;

struct DVDCommandBlock {
	DVDCommandBlock *next;
	DVDCommandBlock *prev;
	u32 command;
	s32 state;
	u32 offset;
	u32 length;
	void *addr;
	u32 currTransferSize;
	u32 transferredSize;
	DVDDiskID *id;
	DVDCBCallback callback;
	void *userData;
};

struct DVDDiskID {
	char gameName[4];
	char company[2];
	u8 diskNumber;
	u8 gameVersion;
	u8 streaming;
	u8 streamingBufSize;
	u8 padding[18];
	u32 magic;
};

typedef struct DVDDriveInfo {
	u16 revisionLevel;
	u16 deviceCode;
	u32 releaseDate;
	u8 padding[24];
} DVDDriveInfo;

typedef struct DVDBB2 {
	u32 bootFilePosition;
	u32 FSTPosition;
	u32 FSTLength;
	u32 FSTMaxLength;
	void *FSTAddress;
	u32 userPosition;
	u32 userLength;
	u32 padding;
} DVDBB2;

#endif /* DVD_H */
