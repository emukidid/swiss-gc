/* 
 * Copyright (c) 2014-2023, Extrems <extrems@extremscorner.org>
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
#include <zlib.h>
#include "aram/sidestep.h"
#include "bba.h"
#include "elf.h"

typedef struct {
	uint32_t magic;
	uint16_t version;
	uint16_t args_size;
	uint32_t deflate_size;
	uint32_t inflate_size;
} ATTRIBUTE_PACKED wiiload_header_t;

static lwp_t thread = LWP_THREAD_NULL;

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

static void *wiiload_read(int sd, size_t insize, size_t outsize)
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
		ret = tcp_read(sd, inbuf, MIN(insize - pos, sizeof(inbuf)));
		if (ret < 0) goto fail;
		else pos += ret;

		zstream.next_in  = inbuf;
		zstream.avail_in = ret;
		ret = inflate(&zstream, Z_NO_FLUSH);
		if (ret < 0) goto fail;
	}

	inflateEnd(&zstream);
	return outbuf;

fail:
	inflateEnd(&zstream);
	free(outbuf);
	return NULL;
}

static void *wiiload_read_args(int sd, size_t size)
{
	void *args = malloc(size);

	if (!args)
		goto fail;
	if (tcp_read(sd, args, size) < size)
		goto fail;

	return args;

fail:
	free(args);
	return NULL;
}

static void wiiload_handler(int sd)
{
	wiiload_header_t header;

	if (tcp_read(sd, &header, sizeof(header)) < sizeof(header))
		return;
	if (memcmp(&header.magic, "HAXX", 4))
		return;
	if (header.version != 5)
		return;

	void *buffer = wiiload_read(sd, header.deflate_size, header.inflate_size);
	void *args = wiiload_read_args(sd, header.args_size);

	if (buffer) {
		if (!memcmp(buffer, ELFMAG, SELFMAG))
			ELFtoARAM(buffer, args, header.args_size);
		else
			DOLtoARAM(buffer, args, header.args_size);
	}

	free(args);
	free(buffer);
}

static void *thread_func(void *arg)
{
	wiiload.sv.sd = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (wiiload.sv.sd == INVALID_SOCKET)
		goto fail;
	if (net_bind(wiiload.sv.sd, &wiiload.sv.sa, sizeof(wiiload.sv.sin)) < 0)
		goto fail;
	if (net_listen(wiiload.sv.sd, 0) < 0)
		goto fail;

	while (true) {
		fd_set readset;
		FD_ZERO(&readset);
		FD_SET(wiiload.sv.sd, &readset);
		net_select(FD_SETSIZE, &readset, NULL, NULL, &wiiload.sv.tv);

		if (FD_ISSET(wiiload.sv.sd, &readset)) {
			socklen_t addrlen = sizeof(wiiload.cl.sin);
			wiiload.cl.sd = net_accept(wiiload.sv.sd, &wiiload.cl.sa, &addrlen);

			if (wiiload.cl.sd != INVALID_SOCKET) {
				wiiload_handler(wiiload.cl.sd);
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

void init_wiiload_thread(void)
{
	if (net_initialized)
		LWP_CreateThread(&thread, thread_func, NULL, NULL, 0, LWP_PRIO_NORMAL);
}
