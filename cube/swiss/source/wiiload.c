/* 
 * Copyright (c) 2014-2026, Extrems <extrems@extremscorner.org>
 * 
 * This file is part of Swiss.
 * 
 * Swiss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Swiss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * with Swiss.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gccore.h>
#include <malloc.h>
#include <network.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "aram/sidestep.h"
#include "bba.h"
#include "elf.h"
#include "gui/FrameBufferMagic.h"
#include "patcher.h"
#include "swiss.h"
#include "wiiload.h"

typedef struct {
	uint32_t magic;
	uint16_t version;
	uint16_t args_size;
	uint32_t deflate_size;
	uint32_t inflate_size;
} ATTRIBUTE_PACKED wiiload_header_t;

typedef int wiiload_read_function_t(int, void *, size_t);

static lwp_t main_thread = LWP_THREAD_NULL;
static lwp_t tcp_thread = LWP_THREAD_NULL;
static lwp_t usb_thread = LWP_THREAD_NULL;

static struct {
	struct {
		int sd;
		union {
			struct sockaddr sa;
			struct sockaddr_in sin;
		};
		struct timeval tv;
	} sv;

	struct {
		int sd;
		union {
			struct sockaddr sa;
			struct sockaddr_in sin;
		};
	} cl;
} wiiload = {
	.sv.sd = INVALID_SOCKET,

	.sv.sin.sin_family = AF_INET,
	.sv.sin.sin_port = 4299,
	.sv.sin.sin_addr.s_addr = INADDR_ANY,

	.sv.tv.tv_sec = 1,
	.sv.tv.tv_usec = 0,

	.cl.sd = INVALID_SOCKET,
};

static int tcp_read(int sd, void *buf, size_t size)
{
	int ret, pos = 0;

	while (pos < size) {
		ret = net_read(sd, buf + pos, size - pos);
		if (ret < 1) return ret < 0 ? ret : pos;
		else pos += ret;
	}

	return pos;
}

static int usb_read(int chn, void *buf, size_t size)
{
	return usb_recvbuffer_safe_ex(chn, buf, size, 65536);
}

static void *wiiload_read(int sd, size_t insize, size_t outsize, wiiload_read_function_t read, uiDrawObj_t *progress)
{
	Byte inbuf[4096];
	z_stream zstream = {0};
	int ret, pos = 0;

	void *outbuf = malloc(outsize);

	if (!outbuf)
		goto fail;
	if (inflateInit(&zstream) < 0)
		goto fail;

	zstream.next_out  = outbuf;
	zstream.avail_out = outsize;

	while (pos < insize) {
		ret = read(sd, inbuf, MIN(insize - pos, sizeof(inbuf)));
		if (ret < 0) goto fail;
		else pos += ret;

		zstream.next_in  = inbuf;
		zstream.avail_in = ret;
		ret = inflate(&zstream, Z_NO_FLUSH);
		if (ret < 0) goto fail;

		DrawUpdateProgressBar(progress, pos * 100 / insize);
	}

	inflateEnd(&zstream);
	return outbuf;

fail:
	inflateEnd(&zstream);
	free(outbuf);
	return NULL;
}

static void *wiiload_read_args(int sd, size_t size, wiiload_read_function_t read)
{
	void *args = malloc(size);

	if (!args)
		goto fail;
	if (read(sd, args, size) < size)
		goto fail;

	return args;

fail:
	free(args);
	return NULL;
}

static void wiiload_handler(int sd, wiiload_read_function_t read, const char *message)
{
	wiiload_header_t header;

	if (read(sd, &header, sizeof(header)) < sizeof(header))
		return;
	if (memcmp(&header.magic, "HAXX", 4))
		return;
	if (header.version != 5)
		return;

	int priority = LWP_SetThreadPriority(LWP_THREAD_NULL, LWP_PRIO_NORMAL);
	LWP_SuspendThread(main_thread);

	uiDrawObj_t *progress = DrawPublish(DrawProgressBar(false, 0, message));
	void *buffer = wiiload_read(sd, header.deflate_size, header.inflate_size, read, progress);
	void *args = wiiload_read_args(sd, header.args_size, read);

	if (buffer) {
		if (!memcmp(buffer, ELFMAG, SELFMAG))
			ELFtoARAM(buffer, args, header.args_size);
		else if (!strncasecmp(args, "SDLOADER.BIN", header.args_size))
			BINtoARAM(buffer, header.inflate_size, 0x81700000, 0x81700000);
		else if (branchResolve(buffer, PATCH_BIN, 0))
			BINtoARAM(buffer, header.inflate_size, 0x80003100, 0x80003100);
		else
			DOLtoARAM(buffer, args, header.args_size);
	}

	free(args);
	free(buffer);
	DrawDispose(progress);

	LWP_ResumeThread(main_thread);
	LWP_SetThreadPriority(LWP_THREAD_NULL, priority);
}

static void *tcp_thread_func(void *arg)
{
	wiiload.sv.sd = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (wiiload.sv.sd == INVALID_SOCKET)
		goto fail;
	if (net_bind(wiiload.sv.sd, &wiiload.sv.sa, sizeof(wiiload.sv.sin)) < 0)
		goto fail;
	if (net_listen(wiiload.sv.sd, 0) < 0)
		goto fail;

	while (tcp_thread == LWP_GetSelf()) {
		fd_set readset;
		FD_ZERO(&readset);
		FD_SET(wiiload.sv.sd, &readset);
		net_select(FD_SETSIZE, &readset, NULL, NULL, &wiiload.sv.tv);

		if (FD_ISSET(wiiload.sv.sd, &readset)) {
			socklen_t addrlen = sizeof(wiiload.cl.sin);
			wiiload.cl.sd = net_accept(wiiload.sv.sd, &wiiload.cl.sa, &addrlen);

			if (wiiload.cl.sd != INVALID_SOCKET) {
				char addrstr[16];
				char *message = NULL;
				asprintf(&message, "Receiving from %s:%hu", inet_ntoa_r(wiiload.cl.sin.sin_addr, addrstr, sizeof(addrstr)), wiiload.cl.sin.sin_port);
				wiiload_handler(wiiload.cl.sd, tcp_read, message);
				free(message);

				net_close(wiiload.cl.sd);
				wiiload.cl.sd = INVALID_SOCKET;
			}
		}
	}

fail:
	net_close(wiiload.sv.sd);
	wiiload.sv.sd = INVALID_SOCKET;
	return NULL;
}

static void *usb_thread_func(void *arg)
{
	while (usb_thread == LWP_GetSelf()) {
		int chn = *(int *)arg - USBGECKO_MEMCARD_SLOT_A;

		if (usb_isgeckoalive(chn) && usb_checkrecv(chn)) {
			wiiload_handler(chn, usb_read, "Receiving over USB Gecko");
			usb_flush(chn);
		}

		usleep(1000);
	}

	return NULL;
}

__attribute((constructor))
static void init_wiiload(void)
{
	main_thread = LWP_GetSelf();
}

void init_wiiload_tcp_thread(void)
{
	if (net_initialized && tcp_thread == LWP_THREAD_NULL)
		LWP_CreateThread(&tcp_thread, tcp_thread_func, NULL, NULL, 0, LWP_PRIO_NORMAL);
}

void init_wiiload_usb_thread(void *arg)
{
	LWP_CreateThread(&usb_thread, usb_thread_func, arg, NULL, 0, LWP_PRIO_LOWEST);
}
