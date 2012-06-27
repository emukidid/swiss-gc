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
#include "patcher.h"

static u8 cheats_buffer[0x1800];
static int isDebug = 0;

// Installs the GeckoOS (kenobiGC) cheats engine to 0x80001800 and sets up variables/copies cheats
void kenobi_install_engine() {
	// If high memory is in use, we'll use low, otherwise high.
	u8 *ptr = isDebug ? kenobigc_dbg_bin : kenobigc_bin;
	u32 size = isDebug ? kenobigc_dbg_bin_size : kenobigc_bin_size;
	
	print_gecko("Copying kenobi%s to %08X\r\n", (isDebug?"_dbg":""),(u32)CHEATS_ENGINE);
	memcpy(CHEATS_ENGINE, ptr, size);
	memcpy(CHEATS_GAMEID, (void*)0x80000000, CHEATS_GAMEID_LEN);
	if(!isDebug) {
		CHEATS_ENABLE_CHEATS = CHEATS_TRUE;
	}
	CHEATS_START_PAUSED = isDebug ? CHEATS_TRUE : CHEATS_FALSE;
	memset(CHEATS_LOCATION(size), 0, kenobi_get_maxsize());
	print_gecko("Copying %08X of cheats to %08X\r\n", kenobi_get_maxsize(),(u32)CHEATS_LOCATION(size));
	memcpy(CHEATS_LOCATION(size), cheats_buffer, kenobi_get_maxsize());
}

// Copy the cheats to somewhere we know about
void kenobi_set_cheats(u8 *buffer, u32 size) {
	memset(&cheats_buffer[0], 0, 0x1800);
	memcpy(&cheats_buffer[0], buffer, size);
}

void kenobi_set_debug(int useDebug) {
	isDebug = useDebug;
}

int kenobi_get_maxsize() {
	return CHEATS_MAX_SIZE((isDebug ? kenobigc_dbg_bin_size : kenobigc_bin_size));
}
