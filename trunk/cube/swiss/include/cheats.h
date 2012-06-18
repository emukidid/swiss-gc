#ifndef __CHEATS_H
#define __CHEATS_H

extern int kenobigc_bin_size;
extern u8 kenobigc_bin[];

#define CHEATS_MAX_SIZE			(0x2F00 - (0x1800+kenobigc_bin_size-8))
#define CHEATS_LOCATION			((void*)(0x80001800 + kenobigc_bin_size - 8))
#define CHEATS_ENGINE			((void*)0x80001800)
#define CHEATS_GAMEID			((void*)0x80001800)
#define CHEATS_GAMEID_LEN		4
#define CHEATS_ENABLE_CHEATS	(*(u32*)0x80001804)
#define CHEATS_START_PAUSED		(*(u32*)0x80001808)
#define CHEATS_TRUE				1
#define CHEATS_FALSE			0

// Installs the GeckoOS (kenobiGC) cheats engine to 0x80001800 and sets up variables/copies cheats
void kenobi_install_engine();
// Copy the cheats to somewhere we know about
void kenobi_set_cheats(u8 *buffer, u32 size);
int kenobi_get_maxsize();
#endif
