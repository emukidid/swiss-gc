/* gcars.c 
	- GCARS cheat engine
	by courtesy of Fuzziqer Software for GCOS
 */

#include <gccore.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ogc/lwp_threads.h>
#include "main.h"
#include "hook.bin.h"
#include "gcars.h"
#include "sidestep.h"

#define HOOK_SIZE 0x3A44

extern unsigned int load_app(int mode);
extern unsigned int load_appDVD(int mode);

#define CHEATS_SIZE 16384
static u32 codez[CHEATS_SIZE];

void GCARSStartGame(u32* codelist)
{
    u32 x,codesactive,*codes;
	
    memset((void*)GCARS_MEMORY_START,0,0x8000); //clear first to avoid problems!
	
    // how many codes are active? 
    for (codesactive = 0; (codelist[codesactive] | codelist[codesactive + 1]) != 0; codesactive += 2);
    codesactive /= 2;
	
    // reserve hook memory 
    *(volatile u32*)0x80000028 = (u32)GCARS_MEMORY_START & 0x01FFFFFF;
    *(volatile u32*)0x8000002C = (u32)0x00000003;
    *(volatile u32*)0x80000034 = 0;
    *(volatile u32*)0x80000038 = 0;
    *(volatile u32*)0x800000EC = (u32)GCARS_MEMORY_START;
    *(volatile u32*)0x800000F0 = (u32)GCARS_MEMORY_START & 0x01FFFFFF;
    *(volatile u32*)0x800000F4 = 0;

    DCInvalidateRange((void*)ENTRYPOINT,HOOK_SIZE);
    ICInvalidateRange((void*)ENTRYPOINT,HOOK_SIZE);
    memcpy((void*)ENTRYPOINT,&hook,HOOK_SIZE); // copy hook 
    DCFlushRange((void*)ENTRYPOINT,HOOK_SIZE); // flush data cache 
    memcpy((void*)GCARS_CODELIST,codelist,codesactive * 8); // copy code list
        
    GCARS_PAUSE = 0; // game starts unpaused 
    GCARS_ENABLE_CS = 0; // disable the Control Simulator 
    GCARS_CONDITIONAL = 0; // initialize the conditional 
    
	  
    // run the game's apploader and return the game's entrypoint 
	u8* main_dol = (u8*)load_app(CHEATS);
      
    //this fixes file-swapping codes to allow them to work with GCARS 
    codes = (u32*)GCARS_CODELIST;

    for (x = 0; x < codesactive; x++)
    {
        if (((codes[x * 2] & 0x01FFFFFF) | 0x80000000) > *(volatile u32*)0x80000038) codes[x * 2] -= 0x8000;
    }

    ENTRYPOINT(); // call the GCARS executor to hook the game before starting it 
    DOLtoARAM(main_dol);

    *(volatile u32*)0xCC003024 = 0; // if the game returns, reset the GC (should never happen) 
}

u32 *getCodeBasePtr() {
  return &codez[0];
}

u32 getCodeBaseSize() {
  return CHEATS_SIZE;
}
