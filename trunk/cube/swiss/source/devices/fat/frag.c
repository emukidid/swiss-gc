/**
 * Frag.c
 *
 * Borrowed from cfg-loader, thanks guys.
 * Adapted for swiss :)
**/

#include <ogcsys.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>

#include <fat.h>
#include "frag.h"

#include "swiss.h"
#include "main.h"
#include "gui/IPLFontWrite.h"

FragList *frag_list = NULL;

typedef int (*_frag_append_t)(void *ff, u32 offset, u32 sector, u32 count);

//extern from modified libfat
int _FAT_get_fragments (const char *path, _frag_append_t append_fragment, void *callback_data);

void frag_init(FragList *ff, int maxnum)
{
        memset(ff, 0, sizeof(Fragment) * (maxnum+1));
        ff->maxnum = maxnum;
}

void frag_print(FragList *ff)
{
	int i;
	print_gecko("frag list: %d %d %x\r\n", ff->num, ff->size, ff->size);

	for (i=0; i<ff->num; i++) {
		if (i>10) {
			print_gecko("...\n");
			break;
		}
		print_gecko(" %d : %8x %8x %8x (%i bytes)\r\n", i,ff->frag[i].offset,ff->frag[i].count,ff->frag[i].sector, ff->frag[i].count*512);
	}
}

int frag_append(FragList *ff, u32 offset, u32 sector, u32 count)
{
        int n;
        if (count) {
                n = ff->num - 1;
                if (ff->num > 0
                        && ff->frag[n].offset + ff->frag[n].count == offset
                        && ff->frag[n].sector + ff->frag[n].count == sector)
                {
                        // merge
                        ff->frag[n].count += count;
                }
                else
                {
                        // add
                        if (ff->num >= ff->maxnum) {
                                // too many fragments
                                return -500;
                        }
                        n = ff->num;
                        ff->frag[n].offset = offset;
                        ff->frag[n].sector = sector;
                        ff->frag[n].count  = count;
                        ff->num++;
                }
        }
        ff->size = offset + count;
        return 0;
}

int _frag_append(void *ff, u32 offset, u32 sector, u32 count)
{
        return frag_append(ff, offset, sector, count);
}

void get_frag_list(char *path)
{
	FragList *fs = NULL;
	
	fs = malloc(sizeof(FragList));
	frag_init(fs, MAX_FRAG);
	_FAT_get_fragments(path, &_frag_append, fs);
	
	if(frag_list) free(frag_list);
	frag_list = malloc(sizeof(FragList));
	frag_init(frag_list, MAX_FRAG);
	memcpy(frag_list, fs, sizeof(FragList));
	
	free(fs);
//	frag_print(frag_list);
}
