/**
 * Wii64 - boxartpacker.c
 * Copyright (C) 2008, 2013 emu_kidid
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: emukidid@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
// back:  491w x 680h
// spine: 48w x 680h
// cover: 485w x 680h
/* function prototype(s) */
int do_ls(char []);

/* define(s) */
#define OUTPUT_NAME "boxart_hires.bin"
#define SWAP32(x) ((((x)>>24) & 0xfful) | (((x)>>8) & 0xff00ul) | (((x)<<8) & 0xff0000ul) | (((x)<<24) & 0xff000000ul))

/* 
	This program assumes that boxart is stored as such in RGB888.bmp format:
	(left rotated basically)
	[Front Cover]
	[spine]
	[Back Cover]
*/
// Sizes
#define BOXART_BACK_HEIGHT 202
#define BOXART_SPINE_HEIGHT 22
#define BOXART_FRONT_HEIGHT 200
#define BOXART_HEIGHT (BOXART_BACK_HEIGHT+BOXART_SPINE_HEIGHT+BOXART_FRONT_HEIGHT)
#define BOXART_WIDTH 280

#define BPP 2
#define RGB565_SIZE ((BOXART_WIDTH * BOXART_HEIGHT) * BPP)	/*1392640*/
#define TEX_SIZE ((BOXART_WIDTH * BOXART_HEIGHT) * BPP)		/*1392640*/ //237440
#define RGB888_SIZE ((BOXART_WIDTH * BOXART_HEIGHT) * 3)	/*2088960*/
#define FIRST_TEX_LOCATION 0x4000

/* global variable(s) */
FILE *fout;				//boxart.bin file
int GameIDCount = 0;
int filesCount = 0;
char filesArray[4096][255];
int Current_Tex_location = FIRST_TEX_LOCATION;

/*	string to hex functions 
	- used for filename->u32 conversion	*/
char xtod(char c) 
{
	if (c>='0' && c<='9') return c-'0';
 	if (c>='A' && c<='F') return c-'A'+10;
 	if (c>='a' && c<='f') return c-'a'+10;
	return c=0;        // not Hex digit
}

int HextoDec(char *hex, int l)
{
    if (*hex==0) return(l);
    return HextoDec(hex+1, l*16+xtod(*hex)); // hex+1?
}

int xstrtoi(char *hex)    // hex string to integer
{
    return SWAP32(HextoDec(hex,0));
}

/* converts filename to crc and adds it to the header */
void process_file(char *filename)
{
	char temp_filename[255];
	unsigned int temp_location=0;
	
	strncpy(temp_filename,filename,6);
	printf("File/GameID: %s\n",temp_filename);

	fwrite(&temp_filename, 1, 6, fout);
	
	//pre-swap things to speed things up on PPC
	temp_location=SWAP32(Current_Tex_location);
	fwrite(&temp_location, 1, 4, fout);
	GameIDCount++;
	
	printf(" at %08X (%i)\n",Current_Tex_location, TEX_SIZE);
	//update tex location in file..
	Current_Tex_location+=TEX_SIZE;
	printf(" next %08X\n",Current_Tex_location);
}

/* converts from rgb888 to rgb565->4x4 tile and adds it */
void add_file(char *filename, int num)
{
//	if(strlen(filename)<12)	//eliminate thumbs.db and other crap..
//		return;
	printf("Adding: %s %i at %08X\n",filename, num, ftell(fout));
	int i,j,k,m,n,y;
	unsigned char  *rgb_888data;
	FILE  *rgb_888file = fopen(filename, "rb");
	
    if (!rgb_888file) {
        printf("---Can't open '%s'\n", filename);
        exit(-1);
    }

    //malloc the original bitmap (rgb888) data
    rgb_888data = (unsigned char*)malloc(RGB888_SIZE);
    
	//skip .bmp header
	fseek(rgb_888file, 0x36, SEEK_SET);
	//read rgb888 data
	fread(rgb_888data, 1, RGB888_SIZE, rgb_888file);
	//close the file
	fclose(rgb_888file);
	
	//RGB888 r[8] g[8] b[8]
	//RGB565 r[5] g[6] b[5]
	for(m = 0; m < BOXART_HEIGHT; m+=4)
	{
		for(n = 0; n < BOXART_WIDTH; n+=4)
		{
			for(j = 0; j < 4; j++)
			{
				for(k = 0; k < 4; k++)
				{
					y = BOXART_HEIGHT-1-(m+j); //TEX is vertically flipped from BMP image
					i = 3*( BOXART_WIDTH*y + (n+k) );
					unsigned short rgb565_temp = ((rgb_888data[i+2])>>3)<<11 | ((rgb_888data[i+1])>>2)<<5 | ((rgb_888data[i])>>3);
					//fwrite(&rgb565_temp,1,2,fout);	//for PC
					// when writing for GC/Wii use this..
					fputc(rgb565_temp>>8,fout);	//flip one u8
					fputc(rgb565_temp,fout);	//flip the other u8
					fflush(fout);
					//printf("Converted: %02X%02X%02X -> %04X\n",rgb_888data[i],rgb_888data[i+1],rgb_888data[i+2],rgb565_temp);
				}
			}
		}
	}
	free(rgb_888data);
	printf("Added: %s %i\n",filename, num);
}

int main()
{
	fout = fopen(OUTPUT_NAME, "wb");
    if (!fout) {
        printf("---Can't open '%s'\n", OUTPUT_NAME);
        exit(-1);
    }
    
	do_ls(".");
	int i = 0;
	//add the files to the header
	for(i=0;i<filesCount;i++)
		process_file(filesArray[i]);
	printf("Found %i GameID's\n",GameIDCount);
	
	//fill up to the first texture up
	fseek(fout, 0, SEEK_END);
	int buffer_amount = FIRST_TEX_LOCATION-ftell(fout);
	printf("Padding header with %i bytes\n",buffer_amount);
	while(buffer_amount>0)
	{
		fputc(0xFF, fout);
		buffer_amount--;
	}
	fflush(fout);
	
	fseek(fout,FIRST_TEX_LOCATION,SEEK_SET);
	//actually convert and add them now..
	for(i=0;i<filesCount;i++){
		add_file(filesArray[i], i);
	}
	
    fclose(fout);
}

int do_ls( char dirname[] )
{
	struct stat fileInfo;			/* to test the file */
	DIR		*dir_ptr;				/* the directory */
	struct dirent	*direntp;		/* each entry	 */
	char tempStr[255];				/* for recursion */
	char tempFilename[255];			/* temporary filename */
	
	/* If we fail to open it as a Directory, then exit */
	if ( ( dir_ptr = opendir( dirname ) ) == NULL )
		exit(-1);
	
	/* We recursively go through each directory (if any) and add it's contents */
	while ( ( direntp = readdir( dir_ptr ) ) != NULL )
	{
		stat(direntp->d_name, &fileInfo);
		//if we don't have a directory, add the filename to the array
		if(!S_ISDIR(fileInfo.st_mode)) {
			if(!strcmp(&direntp->d_name[strlen(direntp->d_name)-4], ".bmp")) 
			{
				strcpy(filesArray[filesCount],direntp->d_name);
				filesCount++;
			}
		}
	}
	closedir(dir_ptr); //close it
	return 0;
}
