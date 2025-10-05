/* 
 * Copyright (c) 2024-2025, Extrems <extrems@extremscorner.org>
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

#include <network.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "bba.h"
#include "rt4k.h"
#include "swiss.h"

static struct {
	FILE *fp;
	int sd;
	int nodelay;
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
	};
} rt4k = {
	.sd = INVALID_SOCKET,
	.nodelay = 1,
};

static int rt4k_read(void *s, char *data, int size)
{
	return net_read(*(int *)s, data, size);
}

static int rt4k_write(void *s, const char *data, int size)
{
	return net_write(*(int *)s, data, size);
}

static int rt4k_close(void *s)
{
	int ret = net_close(*(int *)s);
	*(int *)s = INVALID_SOCKET;
	return ret;
}

int rt4k_printf(const char *format, ...)
{
	int ret = -1;

	if (rt4k.fp) {
		va_list args;
		va_start(args, format);
		ret = vfprintf(rt4k.fp, format, args);
		va_end(args);
	}

	return ret;
}

bool is_rt4k_alive(void)
{
	return rt4k.fp != NULL && !ferror(rt4k.fp);
}

bool rt4k_init(void)
{
	if (!net_initialized)
		return false;

	rt4k.sin.sin_family = AF_INET;
	rt4k.sin.sin_port = swissSettings.rt4kPort;

	if (!inet_aton(swissSettings.rt4kHostIp, &rt4k.sin.sin_addr))
		return false;

	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	if (!net_getpeername(rt4k.sd, (struct sockaddr *)&addr, &addrlen) &&
		rt4k.sin.sin_family == addr.sin_family &&
		rt4k.sin.sin_port == addr.sin_port &&
		rt4k.sin.sin_addr.s_addr == addr.sin_addr.s_addr)
		goto same;

	rt4k_deinit();
	rt4k.sd = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (rt4k.sd == INVALID_SOCKET)
		goto fail;
	if (net_setsockopt(rt4k.sd, IPPROTO_TCP, TCP_NODELAY, &rt4k.nodelay, sizeof(rt4k.nodelay)) < 0)
		goto fail;
	if (net_connect(rt4k.sd, &rt4k.sa, sizeof(rt4k.sin)) < 0)
		goto fail;

	rt4k.fp = funopen(&rt4k.sd, rt4k_read, rt4k_write, NULL, rt4k_close);

	if (rt4k.fp == NULL)
		goto fail;
	if (setvbuf(rt4k.fp, NULL, _IOLBF, 0) == EOF)
		goto fail;

	rt4k_printf("pwr on\n");
same:
	rt4k_load_profile(swissSettings.rt4kProfile);
	return true;

fail:
	net_close(rt4k.sd);
	rt4k.sd = INVALID_SOCKET;
	return false;
}

void rt4k_load_profile(int profile)
{
	if (in_range(profile, 1, 12))
		rt4k_printf("remote prof%i\n", profile);
}

void rt4k_deinit(void)
{
	fclose(rt4k.fp);
	rt4k.fp = NULL;
}
