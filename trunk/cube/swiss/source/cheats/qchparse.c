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
#include "qchparse.h"
#include "main.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"

#define SHIFT_RIGHT(v,a) (v >> a)

#define CHTS_PER_PAGE 6
/* local vars */
char *cheatsFileInMem = NULL;
char *cheatGameNamesp[1024];
char  cheatGameNames[1024][128];
int   cheatFileSize = 0;
int   cheatNumGames = 0;
file_handle* cheatFile;

#define CHEATS_SIZE 16384
static u32 cheats_ar_format[CHEATS_SIZE];
static u32 cheats_wiird_format[CHEATS_SIZE];
static u32 cheats_wiird_format_size = 0;

u32 *getARToWiirdCheats() {
	return &cheats_wiird_format[0];
}

u32 getARToWiirdCheatsSize() {
	return cheats_wiird_format_size * sizeof(u32);
}

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

// Check if we can convert these AR/GCARS/Qoob cheats to WiiRD format. 1 = true, 0 = false
int can_convert_to_wiird(u32 *codes_list) {
	return 1;
}
//bin: |0  |1  |2  |3   |4  |5  |6  |7
//str: |0  |2  |4  |6   |9  |11 |13 |15
//raw: |00 |00 |00 |00  |00 |00 |00 |00

// Converts unencrypted AR/GCARS/Qoob cheats to WiiRD format
// num_codes is per 32bit value, so E2000001 80008000 = 1
void __convert_ar_to_wiird(u32 *codes_list, int num_codes) {
	int x = 0;
	for(x = 0; x < num_codes; x+=2) {
		print_gecko("Original Codes: %08X %08X\r\n",codes_list[x], codes_list[x+1]);
	}
	u8* codes_list_u8 = (u8*)&codes_list[0];
	int code_count = -1, end_conditional = -1;
	int i = 0, num_wiird = 0;
	cheats_wiird_format[num_wiird++] = 0x00D0C0DE;
	cheats_wiird_format[num_wiird++] = 0x00D0C0DE;
	for(i = 0; i < num_codes; i+=2) {
		code_count++;
		
		if(code_count==end_conditional) {
			cheats_wiird_format[num_wiird++] = 0xE2000001;
			cheats_wiird_format[num_wiird++] = 0x80008000;
		}
				
		// Master code/IO register write/unused subtype 3 - skip these
		print_gecko("codes_list_u8[i*4]==%02X\r\n",codes_list_u8[i*4]);
		if((codes_list_u8[i*4]==0xC4 || codes_list_u8[i*4]==0xC5 || codes_list_u8[i*4]==0xC6 || codes_list_u8[i*4]==0xC0 || codes_list_u8[i*4]==0xC2)) {
			continue;
		}
		// RAM write fill, RAM fill slide, z's, if = next line
		if(codes_list_u8[i*4]>>4==0) {
			if(codes_list[i] == 0x00000000 && codes_list[i+1] == 0x40000000) {	//z2
				cheats_wiird_format[num_wiird++] = 0xE0000000;
				cheats_wiird_format[num_wiird++] = 0x80008000;
			}
			else if(codes_list[i] == 0x00000000 && codes_list[i+1] == 0x00000000) {	//z0 - end code list
				cheats_wiird_format[num_wiird++] = 0xF0000000;
				cheats_wiird_format[num_wiird++] = 0x00000000;
			}
			else if(codes_list[i] == 0x00000000 && codes_list[i+1] == 0x60000000) {	//z3 - riggorus code handling
				// "no convert for rigorous code handling" wtf - needed?
			}
			else if(codes_list[i] == 0x00000000 && ((codes_list[i+1]>>28) == 0x8)) {	//ram fill and slide
				if((codes_list_u8[(i*4)+4]&0x0F) == 0 || (codes_list_u8[(i*4)+4]&0x0F) == 2 || (codes_list_u8[(i*4)+4]&0x0F) == 4) {
					cheats_wiird_format[num_wiird++] = 0x08000000 | (codes_list[i+1] & 0xFFFFFF);	//address and ct set (address < 81000000)
				}
				else {
					cheats_wiird_format[num_wiird++] = 0x09000000 | (codes_list[i+1] & 0xFFFFFF);	//address and ct set (address >= 81000000)
				}
				cheats_wiird_format[num_wiird++] = codes_list[i+2];	//initial value
				
				if((codes_list_u8[(i*4)+4]&0xF) == 0 || (codes_list_u8[(i*4)+4]&0xF) == 1) {		//8bit
					cheats_wiird_format[num_wiird++] = 0x00000000 | (codes_list[i+3] & 0xFFFFFF);
				}
				else if((codes_list_u8[(i*4)+4]&0xF) == 2 || (codes_list_u8[(i*4)+4]&0xF) == 3) {	//16bit
					cheats_wiird_format[num_wiird++] = 0x10000000 | (codes_list[i+3] & 0xFFFFFF);
				}
				else if((codes_list_u8[(i*4)+4]&0xF) == 4 || (codes_list_u8[(i*4)+4]&0xF) == 5) {	//32bit
					cheats_wiird_format[num_wiird++] = 0x20000000 | (codes_list[i+3] & 0xFFFFFF);
				}
				cheats_wiird_format[num_wiird++] = 0x00000000 | (codes_list[i+3] >> 24);
				i+=2;			// As we inspected the next line already
				code_count+=1;
				continue;
			}
			else if((codes_list_u8[i*4]&0x0F) == 0 || (codes_list_u8[i*4]&0x0F) == 1) {	//8bit ram write and fill
				cheats_wiird_format[num_wiird++] = codes_list[i];
				cheats_wiird_format[num_wiird++] = (codes_list[i+1]<<8 & 0xFFFF0000) | (codes_list[i+1]&0xFF);
			}
			else if((codes_list_u8[i*4]&0x0F) == 2 
					|| (codes_list_u8[i*4]&0x0F) == 3 
					|| (codes_list_u8[i*4]&0x0F) == 4 
					|| (codes_list_u8[i*4]&0x0F) == 5) {	//16 & 32 bit ram write and fill
				cheats_wiird_format[num_wiird++] = codes_list[i];
				cheats_wiird_format[num_wiird++] = codes_list[i+1];
			}
			else if((codes_list_u8[i*4]&0x0F) == 0xA) {	//16bit if equal next line address < 81
				end_conditional=code_count+2;
				cheats_wiird_format[num_wiird++] = 0x28000000 | (codes_list[i]&0xFFFFFF);
				cheats_wiird_format[num_wiird++] = codes_list[i+1];
			}
			else if((codes_list_u8[i*4]&0x0F) == 0xB) {	//16bit if equal next line address >= 81
				end_conditional=code_count+2;
				cheats_wiird_format[num_wiird++] = 0x29000000 | (codes_list[i]&0xFFFFFF);
				cheats_wiird_format[num_wiird++] = codes_list[i+1];
			}
			else if((codes_list_u8[i*4]&0x0F) == 0xC) {	//32bit if equal next line address < 81
				end_conditional=code_count+2;
				cheats_wiird_format[num_wiird++] = 0x20000000 | (codes_list[i]&0xFFFFFF);
				cheats_wiird_format[num_wiird++] = codes_list[i+1];
			}
			else if((codes_list_u8[i*4]&0x0F) == 0xD) {	//32bit if equal next line address >= 81
				end_conditional=code_count+2;
				cheats_wiird_format[num_wiird++] = 0x21000000 | (codes_list[i]&0xFFFFFF);
				cheats_wiird_format[num_wiird++] = codes_list[i+1];
			}
			else if((codes_list_u8[i*4]&0x0F) == 8 || (codes_list_u8[i*4]&0x0F) == 9) {	//8bit if equal next line
				end_conditional=code_count+2;
				cheats_wiird_format[num_wiird++] = 0x20000000 | (codes_list[i]&0x0FFFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));
				cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1] & 0xFF);
			}
		}
		// Not equal, signed lower
		else if(codes_list_u8[i*4]>>4==1 || codes_list_u8[i*4]>>4==5 || codes_list_u8[i*4]>>4==9 || codes_list_u8[i*4]>>4==0xD) {	
			if(((codes_list_u8[i*4]&0xF) == 2) || ((codes_list_u8[i*4]&0xF) == 3)	
			  || ((codes_list_u8[i*4]&0xF) == 4) || ((codes_list_u8[i*4]&0xF) == 5) 
			  || ((codes_list_u8[i*4]&0xF) == 0) || ((codes_list_u8[i*4]&0xF) == 1)
			  || ((codes_list_u8[i*4]&0xF) == 0xC) || ((codes_list_u8[i*4]&0xF) == 0xD)) {
				if((codes_list_u8[i*4]&0xF) == 1) {
					end_conditional=code_count+2;
				}
				else if((codes_list_u8[i*4]&0xF) == 5) {
					end_conditional=code_count+3;
				}
				if((codes_list_u8[i*4]&0xF) == 2) {
					cheats_wiird_format[num_wiird++] = 0x2A000000 | (codes_list[i]&0xFFFFFF);	//16bit if not equal next line address < 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF) == 3) {
					cheats_wiird_format[num_wiird++] = 0x2B000000 | (codes_list[i]&0xFFFFFF);	//16bit if not equal next line address >= 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF) == 4) {
					cheats_wiird_format[num_wiird++] = 0x22000000 | (codes_list[i]&0xFFFFFF);	//32bit if not equal next line address < 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF) == 5) {
					cheats_wiird_format[num_wiird++] = 0x23000000 | (codes_list[i]&0xFFFFFF);	//32bit if not equal next line address >= 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				//8bit if not equal next line address < 81
				else if((codes_list_u8[i*4]&0xF) == 0) {
					cheats_wiird_format[num_wiird++] = 0x2A000000 | (codes_list[i]&0x00FFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));
					cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1] & 0xFF);
				}
				//8bit if not equal next line address >= 81
				else if((codes_list_u8[i*4]&0xF) == 1) {
					cheats_wiird_format[num_wiird++] = 0x2B000000 | (codes_list[i]&0x00FFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));
					cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1] & 0xFF);
				}
				else if((codes_list_u8[i*4]&0xF) == 0xC) {
					cheats_wiird_format[num_wiird++] = 0x26000000 | (codes_list[i]&0xFFFFFF);	//32bit if signed less next line address < 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF) == 0xD) {
					cheats_wiird_format[num_wiird++] = 0x27000000 | (codes_list[i]&0xFFFFFF);	//32bit if signed less next line address >= 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}

			}
			else if((codes_list_u8[i*4]&0xF)==0xA || (codes_list_u8[i*4]&0xF)==0xB) {	//16bit if signed less next line
				//"No WiiRD CodeType for GCN AR CodeType- 16bit signed lower next line."
			}
			else if((codes_list_u8[i*4]&0xF)==8 || (codes_list_u8[i*4]&0xF)==9) //8bit if signed less next line address >= 81
			{
				//"No WiiRD CodeType for GCN AR CodeType- 8bit signed lower next line."
			}
		}
		// If signed greater, unsigned less
		else if(codes_list_u8[i*4]>>4==2 || codes_list_u8[i*4]>>4==6 || codes_list_u8[i*4]>>4==0xA || codes_list_u8[i*4]>>4==0xE) {
			if((codes_list_u8[i*4]&0xF)==2 || (codes_list_u8[i*4]&0xF)==3) {	//16bit if signed greater next line
				//"No WiiRD CodeType for GCN AR CodeType- 16bit signed greater next line."
			}
			else if((codes_list_u8[i*4]&0xF)==4 || (codes_list_u8[i*4]&0xF)==5 || (codes_list_u8[i*4]&0xF)==0xA || (codes_list_u8[i*4]&0xF)==0xB || (codes_list_u8[i*4]&0xF)==8 || (codes_list_u8[i*4]&0xF)==9) {
				if(codes_list_u8[i*4]>>4==2)
					end_conditional=code_count+2;
				else if(codes_list_u8[i*4]>>4==6)
					end_conditional=code_count+3;
				
				if((codes_list_u8[i*4]&0xF)==4 || (codes_list_u8[i*4]&0xF)==5) {
					cheats_wiird_format[num_wiird++] = codes_list[i];	//32bit if signed greater next line
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF)==0xA) {
					cheats_wiird_format[num_wiird++] = 0x2E000000 | (codes_list[i]&0xFFFFFF);	//16bit if unsigned less next line address < 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF)==0xB) {
					cheats_wiird_format[num_wiird++] = 0x2F000000 | (codes_list[i]&0xFFFFFF);	//16bit if unsigned less next line address >= 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				//8bit if unsigned less next line address < 81
				else if((codes_list_u8[i*4]&0xF)==8) {
					cheats_wiird_format[num_wiird++] = 0x2E000000 | (codes_list[i]&0x00FFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));
					cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1] & 0xFF);
				}
				//8bit if unsigned less next line address >= 81
				else if((codes_list_u8[i*4]&0xF)==9) {
					cheats_wiird_format[num_wiird++] = 0x2F000000 | (codes_list[i]&0x00FFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));
					cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1] & 0xFF);
				}
			}
			else if((codes_list_u8[i*4]&0xF)==0 || (codes_list_u8[i*4]&0xF)==1) {	//8bit if signed greater next line
				//"No WiiRD CodeType for GCN AR CodeType- 8bit signed greater next line.\r\n"
			}
			else if((codes_list_u8[i*4]&0xF)==4 || (codes_list_u8[i*4]&0xF)==5) {	//32bit if unsigned less next line
				//"No WiiRD CodeType for GCN AR CodeType- 32bit unsigned lower next line."
			}
		}
		else if(codes_list_u8[i*4]>>4==3 || codes_list_u8[i*4]>>4==7 || codes_list_u8[i*4]>>4==0xB || codes_list_u8[i*4]>>4==0xF) {	//if unsigned greater, bitwise
			if((codes_list_u8[i*4]&0xF)==2 || (codes_list_u8[i*4]&0xF)==3 || (codes_list_u8[i*4]&0xF)==0 || 
				(codes_list_u8[i*4]&0xF)==1 || (codes_list_u8[i*4]&0xF)==0xA || (codes_list_u8[i*4]&0xF)==0xB || (codes_list_u8[i*4]&0xF)==8 || (codes_list_u8[i*4]&0xF)==9) {	
				if(codes_list_u8[i*4]>>4==3)
					end_conditional=code_count+2;
				else if(codes_list_u8[i*4]>>4==7)
					end_conditional=code_count+3;

				if((codes_list_u8[i*4]&0xF)==2) {
					cheats_wiird_format[num_wiird++] = 0x2C000000 | (codes_list[i]&0xFFFFFF);	//16bit if unsigned greater next line address < 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF)==3) {
					cheats_wiird_format[num_wiird++] = 0x2D000000 | (codes_list[i]&0xFFFFFF);	//16bit if unsigned greater next line address >= 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				//8bit if unsigned greater next line address < 81
				else if((codes_list_u8[i*4]&0xF)==0) {
					cheats_wiird_format[num_wiird++] = 0x2C000000 | (codes_list[i]&0xFFFFFF);
					cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1]&0xFF);
				}
				//8bit if unsigned greater next line address >= 81
				else if ((codes_list_u8[i*4]&0xF)==1) {
					cheats_wiird_format[num_wiird++] = 0x2D000000 | (codes_list[i]&0xFFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));;
					cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1]&0xFF);
				}
				//16bit if equal bitwise next line address < 81
				else if ((codes_list_u8[i*4]&0xF)==0xA) {
					cheats_wiird_format[num_wiird++] = 0x28000000 | (codes_list[i]&0xFFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));;
					cheats_wiird_format[num_wiird++] = (((~(codes_list[i+1] & 0xFFFF))<<16 )& 0xFFFF0000) | (codes_list[i+1] & 0xFFFF);
				}
				//16bit if equal bitwise next line address >= 81
				else if ((codes_list_u8[i*4]&0xF)==0xB) {
					cheats_wiird_format[num_wiird++] = 0x29000000 | (codes_list[i]&0xFFFFFF);
					cheats_wiird_format[num_wiird++] = (((~(codes_list[i+1] & 0xFFFF))<<16 )& 0xFFFF0000) | (codes_list[i+1] & 0xFFFF);
				}
				//8bit if equal bitwise next line
				else if ((codes_list_u8[i*4]&0xF)==8 || (codes_list_u8[i*4]&0xF)==9) {
					cheats_wiird_format[num_wiird++] = 0x20000000 | (codes_list[i]&0x0FFFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));
					cheats_wiird_format[num_wiird++] = (((~(codes_list[i+1] & 0x00FF))<<16 )& 0xFFFF0000) | (codes_list[i+1] & 0xFF);
				}
			}
			else if((codes_list_u8[i*4]&0xF)==4 || (codes_list_u8[i*4]&0xF)==5) {
				//"No WiiRD CodeType for GCN AR CodeType- 32bit unsigned greater next line."	//32bit if unsigned greater next line
			}
			else if((codes_list_u8[i*4]&0xF)==0xC || (codes_list_u8[i*4]&0xF)==0xD) {
				//"No WiiRD CodeType for GCN AR CodeType- 32bit equal bitwise next line."	//32bit if equal bitwise next line
			}
		}
		// Pointer, equal 2 lines
		else if(codes_list_u8[i*4]>>4==4 || codes_list_u8[i*4]>>4==8 || codes_list_u8[i*4]>>4==0xC) {
			if(codes_list_u8[i*4] == 0x40 || codes_list_u8[i*4] == 0x41) {
				cheats_wiird_format[num_wiird++] = 0x48000000;
				cheats_wiird_format[num_wiird++] = 0x80000000 | (codes_list[i]&0xFFFFFFF);
				cheats_wiird_format[num_wiird++] = 0xDE000000;
				cheats_wiird_format[num_wiird++] = 0x80008180;
				cheats_wiird_format[num_wiird++] = 0x10000000 | (codes_list[i+1]&0x00FFFFFF);
				cheats_wiird_format[num_wiird++] = 0x00000000 | (codes_list[i+1]&0xFF);
				if(end_conditional==code_count+1) {
					end_conditional=code_count;
					cheats_wiird_format[num_wiird++] = 0xE2000002;
					cheats_wiird_format[num_wiird++] = 0x80008000;
				}
				else {
					cheats_wiird_format[num_wiird++] = 0xE2000001;
					cheats_wiird_format[num_wiird++] = 0x80008000;
				}
			}
			else if(codes_list_u8[i*4] >= 0x80 && codes_list_u8[i*4] <= 0x87) {
				//"No WiiRD CodeType for GCN AR CodeType- Increment."
			}
			else if((codes_list_u8[i*4]&0xF)==2 || (codes_list_u8[i*4]&0xF)==3) {
				if((codes_list_u8[i*4]&0xF)==2) {
					cheats_wiird_format[num_wiird++] = 0x48000000;
					cheats_wiird_format[num_wiird++] = 0x80000000 | (codes_list[i]&0xFFFFFF);
				}
				else {
					cheats_wiird_format[num_wiird++] = 0x48000000;
					cheats_wiird_format[num_wiird++] = 0x81000000 | (codes_list[i]&0xFFFFFF);
				}
				cheats_wiird_format[num_wiird++] = 0xDE000000;
				cheats_wiird_format[num_wiird++] = 0x80008180;
				cheats_wiird_format[num_wiird++] = 0x12000000 | (((codes_list[i+1]>>16) * 2) & 0xFFFFFF);
				cheats_wiird_format[num_wiird++] = 0x00000000 | (codes_list[i+1]&0xFFFF);
				if(end_conditional==code_count+1) {
					end_conditional=code_count;
					cheats_wiird_format[num_wiird++] = 0xE2000002;
					cheats_wiird_format[num_wiird++] = 0x80008000;
				}
				else {
					cheats_wiird_format[num_wiird++] = 0xE2000001;
					cheats_wiird_format[num_wiird++] = 0x80008000;
				}
			}
			else if((codes_list_u8[i*4]&0xF)==4 || (codes_list_u8[i*4]&0xF)==5) {
				cheats_wiird_format[num_wiird++] = 0x48000000;
				if((codes_list_u8[i*4]&0xF)==5) {
					cheats_wiird_format[num_wiird++] = 0x81000000 | (codes_list[i]&0xFFFFFFF);
				}
				else {
					cheats_wiird_format[num_wiird++] = 0x80000000 | (codes_list[i]&0xFFFFFFF);
				}
				cheats_wiird_format[num_wiird++] = 0xDE000000;
				cheats_wiird_format[num_wiird++] = 0x80008180;
				cheats_wiird_format[num_wiird++] = 0x14000000;
				cheats_wiird_format[num_wiird++] = codes_list[i+1];
				if(end_conditional==code_count+1) {
					end_conditional=code_count;
					cheats_wiird_format[num_wiird++] = 0xE2000002;
					cheats_wiird_format[num_wiird++] = 0x80008000;
				}
				else {
					cheats_wiird_format[num_wiird++] = 0xE2000001;
					cheats_wiird_format[num_wiird++] = 0x80008000;
				}
			}
			else if(((codes_list_u8[i*4]&0xF)==0xA) || ((codes_list_u8[i*4]&0xF)==0xB) || ((codes_list_u8[i*4]&0xF)==0xC) || ((codes_list_u8[i*4]&0xF)==0xD) 
																					|| ((codes_list_u8[i*4]&0xF)==8) || ((codes_list_u8[i*4]&0xF)==9)) {
				if(codes_list_u8[i*4]>>4==4) {
					end_conditional=code_count+3;
				}
				if((codes_list_u8[i*4]&0xF)==0xA) {
					cheats_wiird_format[num_wiird++] = 0x28000000 | (codes_list[i]&0xFFFFFF);	//16bit if equal address < 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF)==0xB) {
					cheats_wiird_format[num_wiird++] = 0x29000000 | (codes_list[i]&0xFFFFFF);		//16bit if equal next 2 line address >= 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF)==0xC) {
					cheats_wiird_format[num_wiird++] = 0x20000000 | (codes_list[i]&0xFFFFFF);		//32bit if equal next 2 line address < 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				else if((codes_list_u8[i*4]&0xF)==0xD) {
					cheats_wiird_format[num_wiird++] = 0x21000000 | (codes_list[i]&0xFFFFFF);		//32bit if equal next 2 line address >= 81
					cheats_wiird_format[num_wiird++] = codes_list[i+1];
				}
				//8bit if equal next 2 line
				else if((codes_list_u8[i*4]&0xF)==8 || (codes_list_u8[i*4]&0xF)==9) {
					cheats_wiird_format[num_wiird++] = 0x20000000 | (codes_list[i]&0x0FFFFFF0) | ((codes_list_u8[(i*4)+3]&0x0F & 1) ? ((codes_list_u8[(i*4)+3]&0x0F)-1) : (codes_list_u8[(i*4)+3]&0x0F));
					cheats_wiird_format[num_wiird++] = 0xFF000000 | (codes_list[i+1]&0xFF);
				}
			}
		}
	}
	if(end_conditional==code_count) {
		cheats_wiird_format[num_wiird++] = 0xE2000001;
		cheats_wiird_format[num_wiird++] = 0x80008000;
	}
	cheats_wiird_format[num_wiird++] = 0xFF000000;	// Terminates cheats list
	cheats_wiird_format_size = num_wiird;
	for(x = 0; x < num_wiird; x+=2) {
		print_gecko("Wiird Codes: %08X %08X\r\n",cheats_wiird_format[x], cheats_wiird_format[x+1]);
	}
}

void QCH_SetCheatsFile(file_handle* cheatFile) {
	
	if(cheatsFileInMem) {
		free(cheatsFileInMem);
	}
	cheatsFileInMem = (char*)memalign(32,cheatFile->size);
	
	if(deviceHandler_readFile(cheatFile,cheatsFileInMem,cheatFile->size) == cheatFile->size) {
		cheatFileSize = cheatFile->size;
	}
}

/* Call this in the menu to set things up */
int QCH_Init()
{
	memset(cheatGameNamesp,0,sizeof(cheatGameNamesp));
	memset(cheatGameNames,0,sizeof(cheatGameNames));

	return 1;
}

/* Call this when we launch a game to free up resources */
void QCH_DeInit()
{
	//free cheats data
	free(cheatsFileInMem);
	cheatsFileInMem = NULL;
}

/* returns number of games in QCH file and fills out everything */
int QCH_Parse(curGameCheats *chts)
{
  int i = HEADER_SIZE, found = 0,cheatnum=0,codenum=0,codemast=0;

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
  while(PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
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
		WriteFontStyled(640/2, 364, "(A) Select - (B) Return", 1.0f, true, defaultColor);
		DrawFrameFinish();
		while ( !(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && 
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) &&
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B))
		{ VIDEO_WaitVSync (); }
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
		while((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || (PAD_ButtonsHeld(0) & PAD_BUTTON_UP)) { VIDEO_WaitVSync (); }
	}
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
	
  int apply = 0;
	int res = QCH_Parse(chts);
	chts->numCheats = res;
	curChtSelection=0;
	while(1) {
  	DrawFrameStart();
		DrawEmptyBox(25,120, vmode->fbWidth-28, 400, COLOR_BLACK);
		WriteFontStyled(640/2, 130, &chts->GameName[0], 1.0f, true, defaultColor);
		
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
		WriteFontStyled(640/2, 364, "(Start) Apply - (A) Toggle - (B) Abort", 1.0f, true, defaultColor);
		DrawFrameFinish();
		while ( !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)  && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) &&
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) &&
		        !(PAD_ButtonsHeld(0) & PAD_BUTTON_START))
			{ VIDEO_WaitVSync (); }
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) curChtSelection = (--curChtSelection < 0) ? chts->numCheats-1 : curChtSelection;
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN)curChtSelection = (curChtSelection + 1) % chts->numCheats;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_A)) { chts->cheats[curChtSelection].enabled^=1; while((PAD_ButtonsHeld(0) & PAD_BUTTON_A)); }
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_B)) break;
		if((PAD_ButtonsHeld(0) & PAD_BUTTON_START)) {apply=1; break;}
	  while((PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) || (PAD_ButtonsHeld(0) & PAD_BUTTON_UP)) { VIDEO_WaitVSync (); }
	}
	if(apply) {
  	DrawFrameStart();
  	DrawEmptyBox(25,120, vmode->fbWidth-28, 400, COLOR_BLACK);
	WriteFontStyled(640/2, 130, &chts->GameName[0], 1.0f, true, defaultColor);
  	
	u32 *codez = &cheats_ar_format[0];
  	memset(codez,0,CHEATS_SIZE);
	
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
	
	__convert_ar_to_wiird(codez,j*2);
	
  	sprintf(txtbuffer,"%d codes are enabled",j);
	WriteFontStyled(640/2, 190, txtbuffer, 1.0f, true, defaultColor);
	WriteFontStyled(640/2, 370, "Press A to return", 1.0f, true, defaultColor);
  	DrawFrameFinish();
  	wait_press_A();
	}
  return 1; //ok
}
