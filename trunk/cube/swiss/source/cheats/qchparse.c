/* qchparser.c 
	- Qoob cheat loading functions
	by emu_kidid
 */

#include <string.h>
#include <stdio.h>
#include <gccore.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/dir.h>
#include "font.h"
#include "qchparse.h"
#include "main.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "cheats/cheats.h"

#define CHTS_PER_PAGE 6
/* local vars */
char *cheatsFileInMem = NULL;
char *cheatGameNamesp[1024];
char  cheatGameNames[1024][128];
int   cheatFileSize = 0;
int   cheatNumGames = 0;
int   useInbuilt    = 1;
file_handle* cheatFile;

int cmpstringp(const void *p1, const void *p2)
{
    /* The actual arguments to this function are "pointers to
       pointers to char", but strcmp() arguments are "pointers
       to char", hence the following cast plus dereference */

   return strcasecmp(* (char * const *) p1, * (char * const *) p2);
}

int getBestMatch(char *gamename) {
  int i = 0,match = 0,lastmatch=0,bestmatchnum=-1;
  char *pch;
  
  strlwr(gamename); //make game name lowercase
  if((gamename[0]=='t')&&(gamename[1]=='h')&&(gamename[2]=='e')&&(gamename[3]==' ')) {
    gamename+=4;  //fix games that start with "the "
  }
  for(i = 0; i<cheatNumGames; i++)  //iterate through them all
  {
    char tempCName[255];
    memcpy(&tempCName[0],cheatGameNamesp[i],128);
    strlwr(&tempCName[0]);  //copy and lowercase it too
    
    lastmatch=match;        //copy previous best match
    match=0;                //reset current match count
    pch = strtok (&tempCName[0]," ,.-:"); //tokenize
    while (pch != NULL)
    {
      if(strlen(pch)>3) { //ignore things like "the", "of", etc
        if(strstr(gamename,pch)) {  //if token match
          match++;                //update match count
        }
      }
      pch = strtok (NULL, " ,.-:");
    }
    
    if((match>lastmatch) && (tempCName[0]==gamename[0])) {
      bestmatchnum=i;   //the best match
    }
  }
  if(bestmatchnum!=-1) {
    return bestmatchnum;
  }
  return 0;
}

void QCH_SetCheatsFile(file_handle* cheatFile) {
	
	if(cheatsFileInMem) {
		free(cheatsFileInMem);
	}
	cheatsFileInMem = (char*)memalign(32,cheatFile->size);
	
	if(deviceHandler_readFile(cheatFile,cheatsFileInMem,cheatFile->size) == cheatFile->size) {
		useInbuilt = 0;
		cheatFileSize = cheatFile->size;
	}
	else {
		useInbuilt = 1;
	}
}

/* Call this in the menu to set things up */
int QCH_Init()
{
	memset(cheatGameNamesp,0,sizeof(cheatGameNamesp));
	memset(cheatGameNames,0,sizeof(cheatGameNames));

	if(useInbuilt) {
		cheatsFileInMem = (char*)&cheatsEmbedded[0];
		cheatFileSize = cheatsEmbedded_size;
 	}
	return 1;
}

/* Call this when we launch a game to free up resources */
void QCH_DeInit()
{
	if(!useInbuilt) {
		//free cheats data
		free(cheatsFileInMem);
	}
	useInbuilt = 1;
	cheatsFileInMem = NULL;
}

/* returns number of games in QCH file and fills out everything */
int QCH_Parse(curGameCheats *chts)
{
  int i = HEADER_SIZE, found = 0,cheatnum=0,codenum=0,codemast=0;
  //u32 tmpMasterA,tmpMasterB,tmpCheatA,tmpCheatB;
  //char *tmpCheatName = NULL;
  cheatNumGames = 0;
  while(i < cheatFileSize)
  {
    u16 token = *(u16*)&cheatsFileInMem[i];
    i+=2;
    switch((token&0xFF00))
    {
      case MASK_GAMENAME:
        if(found) found=0;
        if(chts) {
          if(!memcmp(&chts->GameName[0],(char*)&cheatsFileInMem[i],token&0xff)) {
            found = 1;
          }
        }
        else {
          memcpy(&cheatGameNames[cheatNumGames],(char*)&cheatsFileInMem[i],token&0xff);
          cheatGameNamesp[cheatNumGames] = &cheatGameNames[cheatNumGames][0];
          cheatNumGames++;
        }
        i+=(token&0x00ff);
        break;
      case MASK_MASTER:
        if((found)&&(chts)) {
          strcpy(&chts->cheats[0].cht_name[0],"Master Code");
          chts->cheats[0].codes[codemast] = *(u32*)&cheatsFileInMem[i];
          chts->cheats[0].codes[codemast+1] = *(u32*)&cheatsFileInMem[(i+4)];
          codemast+=2;
          chts->cheats[0].enabled = 1;
          chts->cheats[0].num_codes=codemast;
        }
        i+=8;
        break;
      case MASK_CHTNAME:
        if((found)&&(chts)) {
          if(cheatnum) {
            chts->cheats[cheatnum].num_codes=codenum;
          }
          codenum=0;
          cheatnum++;
          memcpy(&chts->cheats[cheatnum].cht_name[0],(char*)&cheatsFileInMem[i],token&0xff);
          chts->cheats[cheatnum].enabled = 0;
        }
        i+=(token&0xff);
        break;
      case MASK_NORMAL:
        if((found)&&(chts)) {
          chts->cheats[cheatnum].codes[codenum] = *(u32*)&cheatsFileInMem[i];
          chts->cheats[cheatnum].codes[codenum+1] = *(u32*)&cheatsFileInMem[(i+4)];
          codenum+=2;
        }
        i+=8;
        break;
      default:
        //printf("Unknown token %04X at %08X\n",token,i);
        return -1;
        break;
    }
  }
  if(chts) {
    return cheatnum;
  }
  qsort(&cheatGameNamesp[0], cheatNumGames, sizeof(char *), cmpstringp);
  return cheatNumGames;
}

int QCH_Find(char *gamename, curGameCheats *chts)
{ 
  int i = 0,j=0,k,base,max,curChtSelection=0;
	memset(txtbuffer,0,sizeof(txtbuffer));
  if(!cheatNumGames) {
    return 0;
  }
  while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
  curChtSelection=getBestMatch(gamename);
	while(1){
  	DrawFrameStart();
  	DrawEmptyBox(25,120, vmode->fbWidth-28, 400, COLOR_BLACK);
		i = MIN(MAX(0,curChtSelection-(CHTS_PER_PAGE/2)),MAX(0,cheatNumGames-CHTS_PER_PAGE));
		max = MIN(cheatNumGames, MAX(curChtSelection+(CHTS_PER_PAGE/2),CHTS_PER_PAGE));
		WriteFont(30,130, "Select your game:");
		for(j = 0; i < max; ++i, ++j) {
			DrawSelectableButton(30,170+(j*32), vmode->fbWidth-33, 170+(j*32)+30, cheatGameNamesp[i], (i == curChtSelection) ? B_SELECTED:B_NOSELECT, -1);
		}
		WriteCentre(364, "(A) Select - (B) Return");
		DrawFrameFinish();
		while ( !(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && 
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) &&
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) curChtSelection = (--curChtSelection < 0) ? cheatNumGames-1 : curChtSelection;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)curChtSelection = (curChtSelection + 1) % cheatNumGames;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) curChtSelection = ((curChtSelection-=(CHTS_PER_PAGE-1)) < 0) ? cheatNumGames-1 : curChtSelection;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT))curChtSelection = (curChtSelection + CHTS_PER_PAGE-1) % cheatNumGames;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A))	{
			memcpy(&chts->GameName[0],cheatGameNamesp[curChtSelection],MAX_GAMENAME);
      break;
		}
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_B)) { 
  		return 1;
		}
		while((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || (PAD_ButtonsHeld(0) & PAD_BUTTON_UP));
	}
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A);
	
  int apply = 0;
	int res = QCH_Parse(chts);
	chts->numCheats = res;
	curChtSelection=0;
	while(1) {
  	DrawFrameStart();
		DrawEmptyBox(25,120, vmode->fbWidth-28, 400, COLOR_BLACK);
		WriteCentre(130,&chts->GameName[0]);
		
		i = MIN(MAX(0,curChtSelection-(CHTS_PER_PAGE/2)),MAX(0,chts->numCheats-CHTS_PER_PAGE));
		max = MIN(chts->numCheats, MAX(curChtSelection+(CHTS_PER_PAGE/2),CHTS_PER_PAGE));
		for(j = 0; i<max; ++i,++j)
		{
  		if(chts->cheats[i].num_codes) //it's a code
  		  sprintf(txtbuffer,"[%s] %s",chts->cheats[i].enabled ? "*":" ",&chts->cheats[i].cht_name[0]);
  		else  //it's a title/comment
  		  sprintf(txtbuffer,"%s",&chts->cheats[i].cht_name[0]);
		  DrawSelectableButton(30,170+(j*32), vmode->fbWidth-33-45, 170+(j*32)+30, txtbuffer, (i == curChtSelection) ?B_SELECTED:B_NOSELECT, -1);
		}
	  WriteCentre(364,"(Start) Apply - (A) Toggle - (B) Abort");
		DrawFrameFinish();
		while ( !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)  && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) &&
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) &&
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_START));
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) curChtSelection = (--curChtSelection < 0) ? chts->numCheats-1 : curChtSelection;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)curChtSelection = (curChtSelection + 1) % chts->numCheats;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A)) { chts->cheats[curChtSelection].enabled^=1; while((PAD_ButtonsHeld(0) & PAD_BUTTON_A)); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_B)) break;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_START)) {apply=1; break;}
	  while((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || (PAD_ButtonsHeld(0) & PAD_BUTTON_UP));
	}
	if(apply) {
  	DrawFrameStart();
  	DrawEmptyBox(25,120, vmode->fbWidth-28, 400, COLOR_BLACK);
  	WriteCentre(130,&chts->GameName[0]);
  	u32 *codez = getCodeBasePtr();
  	memset(codez,0,getCodeBaseSize());
  
  	for(i=j=base=0; i < chts->numCheats; i++)
  	{
    	if(chts->cheats[i].enabled){
    	  j+=((chts->cheats[i].num_codes)/2);
    	  for(k=0; k<chts->cheats[i].num_codes; k++) {
    	    codez[base+k]=chts->cheats[i].codes[k];
  	    }
    	  base+=k;
  	  }
  	}
  	sprintf(txtbuffer,"%d codes are enabled",j);
  	WriteCentre(190,txtbuffer);
  	WriteCentre(370,"Press A to return");
  	DrawFrameFinish();
  	wait_press_A();
	}
  return 1; //ok
}


  

