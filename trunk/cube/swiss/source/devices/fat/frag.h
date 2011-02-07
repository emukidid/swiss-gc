#ifndef FRAG_H_
#define FRAG_H_

#define MAX_FRAG 20000	//20000*12b = 240kb

typedef struct
{
        u32 offset; // file offset, in sectors unit
        u32 sector;
        u32 count;
} Fragment;

typedef struct
{
        u32 size; // num sectors
        u32 num;  // num fragments
        u32 maxnum;
        Fragment frag[MAX_FRAG];
} FragList;

extern FragList *frag_list;

void frag_init(FragList *ff, int maxnum);
void get_frag_list(char *path);
#endif
