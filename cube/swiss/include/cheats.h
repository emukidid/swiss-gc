#ifndef __CHEATS_H
#define __CHEATS_H

extern int kenobigc_bin_size;
extern u8 kenobigc_bin[];
extern int kenobigc_dbg_bin_size;
extern u8 kenobigc_dbg_bin[];

#define CHEATS_MAX_SIZE(size)	(WIIRD_ENGINE_SPACE-size-8)
#define CHEATS_LOCATION(size)	((void*)(WIIRD_ENGINE + size - 8))
#define CHEATS_ENGINE			((void*)WIIRD_ENGINE)
#define CHEATS_GAMEID			((void*)WIIRD_ENGINE)
#define CHEATS_GAMEID_LEN		4
#define CHEATS_ENABLE_CHEATS	(*(u32*)(WIIRD_ENGINE+0x04))
#define CHEATS_START_PAUSED		(*(u32*)(WIIRD_ENGINE+0x08))
#define CHEATS_TRUE				1
#define CHEATS_FALSE			0
#define CHEATS_ENGINE_START		((void*)(WIIRD_ENGINE+0xA8))

#define CHEATS_NAME_LEN			128
#define CHEATS_MAX_FOR_GAME		400
#define SINGLE_CHEAT_MAX_LINES	512

// Example:
/*
[G9TD52] Shark Tale (PAL German)


Enable 60Hz Mode [Ralf]
04256540 00000014
042565F4 00000014
0432FD48 00000000

Infinite Health [Ralf]
04275CAC FFFFFFFF
*/

typedef struct {
	char name[CHEATS_NAME_LEN];
	u32 (*codes)[2];
	int num_codes;
	int enabled;
} CheatEntry;

typedef struct {
	CheatEntry cheat[CHEATS_MAX_FOR_GAME];
	int num_cheats;
	int enabled;
} CheatEntries;

// Installs the GeckoOS (kenobiGC) cheats engine and sets up variables/copies cheats
void kenobi_install_engine();
// Copy the cheats to somewhere we know about
void kenobi_set_cheats(u8 *buffer, u32 size);
int kenobi_get_maxsize();
void parseCheats(char *filecontents);
int getEnabledCheatsSize(void);
int getEnabledCheatsCount(void);
CheatEntries* getCheats();
int findCheats(bool silent);
int applyAllCheats();
#endif
