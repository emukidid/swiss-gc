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

#ifndef __WIN32__
#include <termios.h>
#else
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "gecko.h"

#define FTDI_PACKET_SIZE 3968

// TODO
// this whole approach blocks, how do i implement a timeout with tty's?

#ifndef __WIN32__
static int fd_gecko = -1;
#else
static HANDLE handle_gecko = NULL;
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
#else
        COMMTIMEOUTS timeouts;
		DCB config;
        handle_gecko = CreateFile (dev, GENERIC_READ | GENERIC_WRITE, 0, 0,
                                   OPEN_EXISTING, 0, 0);

								   config.DCBlength = sizeof(config);


		if((GetCommState(handle_gecko, &config) == 0))
		{
			fprintf(stderr,"Get configuration port has a problem.");
			return 1;
		}

		config.BaudRate = 9600;
		config.StopBits = ONESTOPBIT;
		config.Parity = NOPARITY; 
		config.ByteSize = DATABITS_8;
		config.fDtrControl = 0;
		config.fRtsControl = 0;

		if (!SetCommState(handle_gecko, &config))
		{
			fprintf(stderr, "Failed to Set Comm State Reason: %d\n",(unsigned int)GetLastError());
			return 1;
		}

		printf("Current Settings\n Baud Rate %d Parity %d Byte Size %d Stop Bits %d\n", (unsigned int)config.BaudRate, 
        (unsigned int)config.Parity, config.ByteSize, config.StopBits);

								   
        GetCommTimeouts (handle_gecko, &timeouts);

        timeouts.ReadIntervalTimeout = MAXDWORD; 
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = 0;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts (handle_gecko, &timeouts)) {
                fprintf (stderr, "error setting timeouts on port\n");
                return 1;
        }

        if (!SetCommMask (handle_gecko, 0)) {
                fprintf (stderr, "error setting communications event mask\n");
                return 1;
        }
#endif

        gecko_flush ();

        return 0;
}

void gecko_close () {
#ifndef __WIN32__
        if (fd_gecko > 0)
                close (fd_gecko);
#else
        CloseHandle (handle_gecko);
#endif
}

void gecko_flush () {
#ifndef __WIN32__
        // TODO doesnt seem to work with ftdi-sio
        // i need a way to check if data is actually available
        tcflush (fd_gecko, TCIOFLUSH);
#else
        PurgeComm (handle_gecko, PURGE_RXCLEAR | PURGE_TXCLEAR |
                   PURGE_TXABORT | PURGE_RXABORT);

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
        size_t left, chunk;
#ifndef __WIN32__
        size_t res;
#else
        DWORD res;
#endif

        left = count;
        while (left) {
                chunk = left;
                if (chunk > FTDI_PACKET_SIZE)
                        chunk = FTDI_PACKET_SIZE;

#ifndef __WIN32__
                res = read (fd_gecko, buf, chunk);
                if (res < 1) {
                        perror ("gecko_read");
                        return 1;
                }
#else
                if (!ReadFile (handle_gecko, buf, chunk, &res, NULL)) {
                        fprintf (stderr, "gecko_read\n");
                        return 1;
                }
#endif

                left -= res;
                buf += res;
				usleep(100);
        }

        return 0;
}

int gecko_write (const void *buf, size_t count) {
        size_t left, chunk;
#ifndef __WIN32__
        size_t res;
#else
        DWORD res;
#endif

        left = count;

        while (left) {
                chunk = left;
                if (chunk > FTDI_PACKET_SIZE)
                        chunk = FTDI_PACKET_SIZE;

#ifndef __WIN32__
                res = write (fd_gecko, buf, count);
                if (res < 1) {
                        perror ("gecko_write");
                        return 1;
                }
#else
                if (!WriteFile (handle_gecko, buf, chunk, &res, NULL)) {
                        fprintf (stderr, "gecko_write\n");
                        return 1;
                }
#endif

                left -= res;
                buf += res;

#ifndef __WIN32__
                // does this work with ftdi-sio?
                if (tcdrain (fd_gecko)) {
                        perror ("gecko_drain");
                        return 1;
                }
#endif
			usleep(100);
        }

        return 0;
}

