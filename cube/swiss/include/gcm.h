#ifndef __GCM_H
#define __GCM_H

#include "devices/deviceHandler.h"

/* 	The Retail Game DVD Disk Header 
	Info source: YAGCD
	Size of Struct: 0x2440	*/
typedef struct {
	u8  ConsoleID;		//G = Gamecube, R = Wii.
	u8  GamecodeA;		//2 Ascii letters to indicate the GameID.
	u8  GamecodeB;		//2 Ascii letters to indicate the GameID.
	u8	CountryCode;	//J=JAP . P=PAL . E=USA . D=OoT MasterQuest
	u8	MakerCodeA;		//Eg 08 = Sega etc.
	u8  MakerCodeB;
	u8	DiscID;
	u8	Version;
	u8	AudioStreaming; //01 = Enable it. 00 = Don't
	u8 	StreamBufSize;	//For the AudioEnable. (always 00?)
	u8	unused_1[18];	
	u32 DVDMagicWord;	//0xC2339F3D
	char GameName[64];	//String Name of Game, rarely > 32 chars
	u8  unused_2[416];
	u32 NKitMagicWord;
	u32 NKitVersion;
	u32 ImageCRC;
	u32 ForceCRC;
	u32 ImageSize;
	u32 JunkID;
	u8  unused_3[488];
	u32 ApploaderSize;
	u32 ApploaderFunc1;
	u32 ApploaderFunc2;
	u32 ApploaderFunc3;
	u8  unused_4[16];
	u32 DOLOffset;		//offset of main executable DOL (bootfile)
	u32 FSTOffset;		//offset of the FST ("fst.bin")
	u32	FSTSize;		//size of FST
	u32	MaxFSTSize;		//maximum size of FST (usually same as FSTSize)*
	u32 FSTAddress;
	u32 UserPos;		//user position(?)
	u32 UserLength;		//user length(?)
	u32 unused_5;
	u32 DebugMonSize;
	u32 SimMemSize;
	u32 ArgOffset;
	u32 DebugFlag;
	u32 TRKLocation;
	u32 TRKSize;
	u32 RegionCode;
	u32 TotalDisc;
	u32 LongFileName;
	u32 PADSpec;
	u32 DOLLimit;
	u8  unused_6[8148];
	// *Multiple DVDs must use it, to properly reside all FSTs.
} DiskHeader __attribute__((aligned(32)));

typedef struct {
	char date[16];
	u32 entry;
	u32 size;
	u32 rebootSize;
	u32 reserved;
} ApploaderHeader __attribute__((aligned(32)));

typedef struct {
	u32 magic;
	u32 version;
	u32 headerStart;
	u32 headerLength;
	u32 fstStart;
	u32 fstLength;
	u32 fstMaxLength;
	u32 dolStart;
	u32 dolLength;
	u32 userStart;
	u32 userLength;
	u32 bannerStart;
	u32 bannerLength;
	u32 gcmUserStart;
	u32 apploaderOffset;
	u32 reserved;
} TGCHeader __attribute__((aligned(32)));

typedef struct {
	u32 offset;
	u32 size;
	char name[256];
	u32 type;
	u32 tgcFstOffset;
	u32 tgcFstSize;
	u32 tgcBase;
	u32 tgcFileStartArea;
	u32 tgcFakeOffset;
} ExecutableFile;

int parse_gcm(file_handle *file, ExecutableFile *filesToPatch);
void adjust_tgc_fst(char* FST, u32 tgc_base, u32 fileAreaStart, u32 fakeAmount);
int parse_tgc(file_handle *file, ExecutableFile *filesToPatch, u32 tgc_base, char* tgcname);
int patch_gcm(file_handle *file, ExecutableFile *filesToPatch, int numToPatch);
void parse_gcm_add(file_handle *file, ExecutableFile *filesToPatch, u32 *numToPatch, char *fileName);
int parse_gcm_for_ext(file_handle *file, char *ext, bool find32k);
int read_fst(file_handle *file, file_handle** dir, u64 *usedSpace);
void get_gcm_banner(file_handle *file, u32 *file_offset, u32 *file_size);
#endif
