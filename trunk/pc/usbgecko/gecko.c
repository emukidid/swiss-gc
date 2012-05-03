/*
 *  Copyright (C) 2008 dhewg, #wiidev efnet
 *
 *  this file is part of wiifuse
 *  http://wiibrew.org/index.php?title=Wiifuse
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "gecko.h"

#ifndef __WIN32__
#include <termios.h>
#define FTDI_PACKET_SIZE 3968
#else
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "FTD2XX.H"
#define FTDI_PACKET_SIZE 0xF7D8
#endif

#ifndef __WIN32__
static int fd_gecko = -1;
#else
FT_HANDLE fthandle;	// Handle of the device to be opened and used for all functions
FT_STATUS status;	// Variable needed for FTDI Library functions
DWORD TxSent;
DWORD RxSent;
int returnvalue;
#endif

int gecko_open (const char *dev) {
#ifndef __WIN32__
	struct termios newtio;

	fd_gecko = open (dev, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);

	if (fd_gecko == -1) {
			perror ("gecko_open");
			return 1;
	}

	if (fcntl (fd_gecko, F_SETFL, 0)) {
			perror ("F_SETFL on serial port");
			return 1;
	}

	if (tcgetattr(fd_gecko, &newtio)) {
			perror ("tcgetattr");
			return 1;
	}

	cfmakeraw (&newtio);

	newtio.c_cflag |= CRTSCTS | CS8 | CLOCAL | CREAD;

	if (tcsetattr (fd_gecko, TCSANOW, &newtio)) {
			perror ("tcsetattr");
			return 1;
	}
	gecko_flush ();
#else
	// Open by Serial Number
	status = FT_OpenEx("GECKUSB0", FT_OPEN_BY_SERIAL_NUMBER, &fthandle);
	if(status != FT_OK) {
		printf("Error: Couldn't connect to USB Gecko. Please check Installation\n");
		exit(0);
	}
	// Reset device			
	status = FT_ResetDevice(fthandle);
	if(status != FT_OK) {
		printf("Error: Couldnt Reset Device %d\n",status);
		FT_Close(fthandle);
		exit(0);
	}

	status = FT_SetTimeouts(fthandle,0,0);	// 0 Second Timeout
	if(status != FT_OK) {
		printf("Error: Timeouts failed to set %d\n",status);
		FT_Close(fthandle);
		exit(0);
	}	
	// Purge buffers		
	status = FT_Purge(fthandle,FT_PURGE_RX);
	if(status != FT_OK)	{
		printf("Error: Problem clearing buffers %d\n",status);
		FT_Close(fthandle);
		exit(0);
	}
	status = FT_Purge(fthandle,FT_PURGE_TX);
	if(status != FT_OK) {
		printf("Error: Problem clearing buffers %d\n",status);
		FT_Close(fthandle);
		exit(0);
	}
	status = FT_SetUSBParameters(fthandle,65536,0);	// Set to 64K packet size (USB 2.0 Max)
	if(status != FT_OK)	{
		printf("Error: Couldnt Set USB Parameters %d\n",status);
		FT_Close(fthandle);
		exit(0);
	}
	Sleep(150);
#endif
	return 0;
}

void gecko_close () {
#ifndef __WIN32__
	if (fd_gecko > 0)
		close (fd_gecko);
#else
	FT_Close(fthandle);
#endif
}

void gecko_flush () {
#ifndef __WIN32__
	// TODO doesnt seem to work with ftdi-sio
	// i need a way to check if data is actually available
	tcflush (fd_gecko, TCIOFLUSH);
#endif
}
static char returnbyte = 0;
unsigned char gecko_read_byte() {
	gecko_read (&returnbyte, 1);
	return returnbyte;
}

void gecko_send_byte(unsigned char *byte) {
	gecko_write(byte, 1);
}

int gecko_read (void *buf, size_t count) {
#ifndef __WIN32__
	size_t left, chunk;
	size_t res;

	left = count;
	while (left) {
		chunk = left;
		if (chunk > FTDI_PACKET_SIZE)
			chunk = FTDI_PACKET_SIZE;

		res = read (fd_gecko, buf, chunk);
		if (res < 1) {
			perror ("gecko_read");
			return 1;
		}
		left -= res;
		buf += res;
		usleep(100);
	}
#else

	size_t left, chunk;
	left = count;
	while (left) {
		chunk = left;
		if (chunk > FTDI_PACKET_SIZE)
			chunk = FTDI_PACKET_SIZE;

		status = FT_Read(fthandle, buf, chunk, &RxSent);	// Read in the data
	
		if (status != FT_OK) { // Check read ok
			printf("Error: Read Error. Closing\n");
			FT_Close(fthandle);	// Close device as fatal error
			exit(-1);
		}
		left -= chunk;
		buf += chunk;
	}
#endif
	return 0;
}

int gecko_write (void *buf, size_t count) {
#ifndef __WIN32__
	size_t left, chunk;
	size_t res;
	left = count;

	while (left) {
			chunk = left;
			if (chunk > FTDI_PACKET_SIZE)
					chunk = FTDI_PACKET_SIZE;

			res = write (fd_gecko, buf, chunk);
			if (res < 1) {
					perror ("gecko_write");
					return 1;
			}

			left -= res;
			buf += res;

			// does this work with ftdi-sio?
			if (tcdrain (fd_gecko)) {
					perror ("gecko_drain");
					return 1;
			}
		usleep(100);
	}
#else
	size_t left, chunk;
	left = count;

	while (left) {
		chunk = left;
		if (chunk > FTDI_PACKET_SIZE)
				chunk = FTDI_PACKET_SIZE;

		status = FT_Write(fthandle, buf, chunk, &TxSent);	// Read in the data

		if (status != FT_OK) {	// Check read ok
			printf("Error: Write Error. Closing.\n");
			FT_Close(fthandle);	// Close device as fatal error
			exit(-1);
		}
		left -= chunk;
		buf += chunk;
	}
	
#endif
	return 0;
}

