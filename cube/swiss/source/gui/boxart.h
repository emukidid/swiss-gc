/* boxart.h 
	- Boxart texture loading from a binary compilation
	by emu_kidid
 */

#ifndef BOXART_H
#define BOXART_H

// TODO: Japanese titles have a different ratio (wider spineart)
#define BOXART_BACK_WIDTH 202
#define BOXART_SPINE_WIDTH 20
#define BOXART_FRONT_WIDTH 200
#define BOXART_HEIGHT 280
#define BOXART_WIDTH (BOXART_BACK_WIDTH+BOXART_SPINE_WIDTH+BOXART_FRONT_WIDTH)

#define BOXART_TEX_BPP			2
#define BOXART_TEX_BACK_SIZE	(BOXART_BACK_WIDTH*BOXART_HEIGHT*BOXART_TEX_BPP)
#define BOXART_TEX_FRONT_SIZE	(BOXART_FRONT_WIDTH*BOXART_HEIGHT*BOXART_TEX_BPP)
#define BOXART_TEX_SPINE_SIZE	(BOXART_SPINE_WIDTH*BOXART_HEIGHT*BOXART_TEX_BPP)
#define BOXART_TEX_SIZE			(BOXART_WIDTH*BOXART_HEIGHT*BOXART_TEX_BPP)
#define BOXART_TEX_FMT 			GX_TF_RGB565

enum BOXART_TYPE {
	BOX_FRONT,
	BOX_SPINE,
	BOX_BACK,
	BOX_ALL
};

void BOXART_Init();
void BOXART_DeInit();
void BOXART_LoadTexture(char *gameID, char *buffer, int type);

#endif

