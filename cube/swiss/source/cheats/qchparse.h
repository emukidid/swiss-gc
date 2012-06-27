/* qchparser.h 
	- Qoob cheat loading functions
	by emu_kidid
 */

#ifndef QCH_H
#define QCH_H

#include "devices/deviceHandler.h"

#define HEADER_SIZE   0x100
#define MASK_GAMENAME 0x0000
#define MASK_CHTNAME  0x0200
#define MASK_MASTER   0x0300
#define MASK_NORMAL   0x0400
#define MAX_GAMENAME  96
#define MAX_CHEATNAME 128
#define MAX_CHEATS    512     //per game
#define MAX_CODES     192     //per cheat

typedef struct {
  char cht_name[MAX_CHEATNAME];
  u32  enabled;
  u32  num_codes;
  u32  codes[MAX_CODES];
} cheat __attribute__((aligned(32)));

typedef struct {
  char GameName[MAX_GAMENAME];
  int  numCheats;
  cheat cheats[MAX_CHEATS];
} curGameCheats __attribute__((aligned(32)));

int cmpstringp(const void *p1, const void *p2);
void wait_press_A();
void QCH_SetCheatsFile(file_handle* cheatFile);
int  QCH_Init();
void QCH_DeInit();
int  QCH_Parse();
int  QCH_Find(char *game, curGameCheats *chts);

u32 *getARToWiirdCheats();
u32 getARToWiirdCheatsSize();
#endif
