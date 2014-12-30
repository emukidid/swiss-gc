/*
 *  Copyright (C) 2008 dhewg, #wiidev efnet
 *
 *  this file is part of geckoloader
 *  http://wiibrew.org/index.php?title=Geckoloader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "gecko.h"

#define SWISSSERVER_VERSION "v0.1"

#ifndef O_BINARY
#define O_BINARY 0
#endif

typedef struct {
	char name[1024]; // File or Folder, absolute path goes here
	uint64_t fileBase;   // Raw sector on device
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size of the file
	int fileAttrib;        // IS_FILE or IS_DIR
	int status;            // is the device ok
	FILE *fp;				// file pointer
} file_handle;

typedef struct {
	unsigned int offset;    // Offset in the file
	unsigned int size;      // size to read
} usb_data_req;

FILE *served_file_fp;
char served_file[1024];		// The file we're currently serving to the GC

unsigned char ASK_READY = 0x15;
unsigned char ASK_OPENPATH = 0x16;
unsigned char ASK_GETENTRY = 0x17;
unsigned char ASK_SERVEFILE = 0x18;
unsigned char ASK_LOCKFILE = 0x19;
unsigned char ANS_READY = 0x25;

unsigned char cmd_send = 0x14;
const char *envvar = "USBGECKODEVICE";

char *curPath = NULL;
file_handle cached_files[2048];
int cached_files_num = 0;
int next_serve_num = 0;

//File type
enum fileTypes
{
  IS_FILE=0,
  IS_DIR,
  IS_SPECIAL
};

#ifndef __WIN32__
#ifdef __APPLE__
char *default_tty = "/dev/tty.usbserial-GECKUSB0";
#else
char *default_tty = "/dev/ttyUSB0";
#endif
#endif

void wait_for_ack () {
        unsigned char ack;
        if (gecko_read (&ack, 1))
                fprintf (stderr, "\nerror receiving the ack\n");
        else if (ack != 0xaa)
                fprintf (stderr, "\nunknown ack (0x%02x)\n", ack);
}

char *setCurPath(char *path) {
	if(curPath == NULL) {
		curPath = malloc(4096);
	}
	memset(curPath, 0, 4096);
	strcpy(curPath, path);
	
	return curPath;
}

char *getCurPath() {
	if(curPath == NULL) {
		curPath = malloc(4096);
		memset(curPath, 0, 4096);
		strcpy(curPath, ".");
	}
	return curPath;
}

// Cache a paths contents
void cache_path() {
	DIR *dir = opendir (getCurPath());
	cached_files_num = 0;
	next_serve_num = 0;
	if (dir != NULL) {
		struct dirent *ent;
		struct stat fstat;
		printf("Current DIR: %s\n",curPath);
		while ((ent = readdir (dir)) != NULL) {
			// Skip parent links
			if(((strlen(ent->d_name)==2) && !strncmp(ent->d_name, "..", 2)) ||
				((strlen(ent->d_name)==1) && !strncmp(ent->d_name, ".", 1))) {
				continue;
			}
			char *path = malloc(4096);
			memset(path,0,4096);
			sprintf(path, "%s/%s",curPath,ent->d_name);
			stat(path, &fstat);
			printf ("%s %i [%s]\n", ent->d_name, fstat.st_size, S_ISDIR(fstat.st_mode) ? "DIR":"FILE");
			memset(&cached_files[cached_files_num],0,sizeof(file_handle));
			sprintf(&cached_files[cached_files_num].name[0],"%s",path);
			cached_files[cached_files_num].size = fstat.st_size;
			cached_files[cached_files_num].fileAttrib = S_ISDIR(fstat.st_mode) ? IS_DIR:IS_FILE;
			cached_files_num++;
			free(path);
		}
		closedir (dir);
	} 
	else {
		fprintf (stderr, "Could not read directory [%s]\n",getCurPath());
		exit(EXIT_FAILURE);
	}
}

void send_file_data(usb_data_req *req) {
	printf("Read File: offset %08X, size %i          \r", req->offset, req->size);
	if(served_file_fp) {
		fseek(served_file_fp, req->offset, SEEK_SET);
		// Read and Send
		void *buffer = (void*)malloc(req->size);
		fread(buffer, 1, req->size, served_file_fp);
		gecko_write(buffer, req->size);
		free(buffer);
	}
	
}

int main (int argc, char **argv) {
       
        printf ("swissserver " SWISSSERVER_VERSION "\n"
                "coded by emu_kidid for Swiss + USB Gecko\n\n");

        char *tty = NULL;
#ifndef __WIN32__
        struct stat st;
        tty = getenv (envvar);
        if (!tty)
                tty = default_tty;

        if (tty && stat (tty, &st))
                tty = NULL;

        if (!tty) {
                fprintf (stderr, "Please set the environment variable %s to "
                         "your usbgecko "
                         "tty device (eg \"/dev/ttyUSB0\")"
                         "\n", envvar);
                exit (EXIT_FAILURE);
        }

        printf ("Using: %s\n\n", tty);	
#endif
        if (gecko_open (tty)) {
                fprintf (stderr, "unable to open the device\n");
                exit (EXIT_FAILURE);
        }

		unsigned char resp = 0;
		while(1) {
			resp = gecko_read_byte();
			if(resp == ASK_READY) {
				printf("Got Ready? (%02X) request from GC\n",resp);
				gecko_send_byte(&ANS_READY);
			}
			else if(resp == ASK_OPENPATH) {
				printf("Got Open Path (%02X) request from GC\n",resp);
				gecko_send_byte(&ANS_READY);
				// Read path requested by GC and set it
				gecko_read(getCurPath(), 4096);
				printf("Got path: %s\n",getCurPath());
				cache_path();
			}
			else if(resp == ASK_GETENTRY) {
				//printf("Got Get Entry (%02X) request from GC\n",resp);
				// Serve file info out to the GC from the directory the GC asked for
				gecko_send_byte(&ANS_READY);
				if(next_serve_num < cached_files_num) {
					printf("Sending entry to GC: %s\n",cached_files[next_serve_num].name);
					gecko_write(&cached_files[next_serve_num], sizeof(file_handle));
					next_serve_num++;
				}
				else {
					printf("Sending NULL entry to GC\n");
					memset(&cached_files[0],0, sizeof(file_handle));
					gecko_write(&cached_files[0], sizeof(file_handle));
				}
			}
			else if(resp == ASK_SERVEFILE) {
				printf("Got Serve File (%02X) request from GC\n",resp);
				gecko_send_byte(&ANS_READY);
				// Read file, offset and size requested by GC
				char filename[1024];
				memset(&filename[0], 0, 1024);
				gecko_read(&filename[0], 1024);
				// compare, open file.
				if(strcmp((const char*)&served_file, &filename[0])) {
					if(served_file_fp) {
						printf("Closed File %s\n",served_file);
						fclose(served_file_fp);
						served_file_fp = NULL;
					}	
					strcpy((char*)served_file, &filename[0]);
					served_file_fp = fopen(&served_file[0], "rb");
					printf("Opened File %s\n",served_file);
				}
			}
			else if(resp == ASK_LOCKFILE) {
				printf("Got Lock File (%02X) request from GC\n",resp);
				gecko_send_byte(&ANS_READY);
				while(1) {
					// Read file, offset and size requested by GC
					usb_data_req req;
					memset(&req, 0, sizeof(usb_data_req));
					gecko_read(&req, sizeof(usb_data_req));
					if(req.size != 0) {
						send_file_data(&req);
					}
					else {
						printf("Got Unlock File (%02X) request from GC\n",resp);
						break;	// end locked file mode
					}
				}
			}
			else if(!resp) {
				gecko_read_byte();
				printf("Unlock called when already unlocked, this is ok.\n");
			}
			else {
				printf("Unknown reply from GC!\n");
				exit(1);
			}
		}

        printf ("done.\n");

        gecko_close ();

        return 0;
}

