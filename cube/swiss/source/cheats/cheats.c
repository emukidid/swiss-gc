/**
*
* Gecko OS/WiiRD cheat engine (kenobigc)
* 
* Adapted to Swiss by emu_kidid 2012
*
*/

#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "swiss.h"
#include "main.h"
#include "cheats.h"

static u8 cheats_buffer[0x8000];

// Installs the GeckoOS (kenobiGC) cheats engine to 0x80001800 and sets up variables/copies cheats
void kenobi_install_engine() {
	memcpy(CHEATS_ENGINE, kenobigc_bin, kenobigc_bin_size);
	memcpy(CHEATS_GAMEID, (void*)0x80000000, CHEATS_GAMEID_LEN);
	CHEATS_ENABLE_CHEATS = CHEATS_TRUE;
	CHEATS_START_PAUSED = CHEATS_FALSE;
	memset(CHEATS_LOCATION, 0, CHEATS_MAX_SIZE);
	memcpy(CHEATS_LOCATION, cheats_buffer, CHEATS_MAX_SIZE);
}

// Copy the cheats to somewhere we know about
void kenobi_set_cheats(u8 *buffer, u32 size) {
	memset(&cheats_buffer[0], 0, CHEATS_MAX_SIZE);
	memcpy(&cheats_buffer[0], buffer, size);
}

int kenobi_get_maxsize() {
	return CHEATS_MAX_SIZE;
}
