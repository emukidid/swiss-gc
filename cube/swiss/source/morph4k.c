/* 
 * Copyright (c) 2025, Extrems <extrems@extremscorner.org>, webhdx <webhdx@gmail.com>
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
#include <stdbool.h>
#include <stdio.h> 
#include <string.h>
#include "bba.h"
#include "morph4k.h"
#include "swiss.h"
#include "util.h"

static bool morph4k_http_post(const char *path);

static struct {
    int nodelay;
    union {
        struct sockaddr sa;
        struct sockaddr_in sin;
    };
    bool initialized;
} morph4k = {
    .nodelay = 1,
    .initialized = false
};

static bool morph4k_http_post(const char *path)
{
    print_debug("morph4k_http_post: Sending request to %s\r\n", path);

    int sd = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sd == INVALID_SOCKET) {
        return false;
    }

    if (net_setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &morph4k.nodelay, sizeof(morph4k.nodelay)) < 0) {
        net_close(sd);

        return false;
    }

    if (net_connect(sd, &morph4k.sa, sizeof(morph4k.sin)) < 0) {
        net_close(sd);

        return false;
    }

    char request[512];
    int req_length = sprintf(request, 
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path,
        swissSettings.morph4kHostIp);

    bool success = false;

    if (net_send(sd, request, req_length, 0) == req_length) {
        char response[512] = {0};
        int bytes_received = net_recv(sd, response, sizeof(response) - 1, 0);
        
        if (bytes_received > 0) {
            response[bytes_received] = '\0';
            
            if (strncmp(response, "HTTP/1.1 200", 12) == 0) {
                char drain_buffer[512];
                while (net_recv(sd, drain_buffer, sizeof(drain_buffer), MSG_DONTWAIT) > 0);
                success = true;
            }
        }
    }

    net_close(sd);

    return success;
}

bool morph4k_reset_gameid(void)
{
    if (!is_morph4k_alive()) {
        return false;
    }

    print_debug("morph4k_reset_gameid: Resetting GameID\r\n");

    bool result = morph4k_http_post("/gameid/0");

    return result;
}

bool morph4k_send_gameid(const DiskHeader *header, uint64_t hash)
{
    if (!hash || !is_morph4k_alive()) {
        return false;
    }

    print_debug(
        "morph4k_send_gameid: Sending request with GameID: %016llX%02X%02X\r\n", 
        hash, header->ConsoleID, header->CountryCode
    );

    char path[256];
    sprintf(path, "/gameid/0x06:%016llX%02X%02X", 
        hash, header->ConsoleID, header->CountryCode);

    bool result = morph4k_http_post(path);

    return result;
}

bool morph4k_apply_preset(char *preset_path)
{
    print_debug("morph4k_apply_preset: Applying preset: %s\r\n", strlen(preset_path) > 0 ? preset_path : "no preset");

    if (strlen(preset_path) <= 0) {
        return true;
    }

    if (!is_morph4k_alive()) {
        return false;
    }

    char path[512];
    sprintf(path, "/preset-apply/sdcard/%s", preset_path);

    bool result = morph4k_http_post(path);

    return result;
}

bool is_morph4k_alive(void)
{
    return morph4k.initialized;
}

bool morph4k_init(void)
{
    if (!net_initialized) {
        return false;
    }

    morph4k.sin.sin_family = AF_INET;
    morph4k.sin.sin_port = htons(80);

    if (!inet_aton(swissSettings.morph4kHostIp, &morph4k.sin.sin_addr)) {
        return false;
    }

    print_debug("morph4k_init: Initializing Morph 4K at %s\r\n", swissSettings.morph4kHostIp);

    morph4k.initialized = true;
    
    if (!morph4k_reset_gameid()) {
        morph4k.initialized = false;

        return false;
    }

    if (!morph4k_apply_preset(swissSettings.morph4kPreset)) {
        morph4k.initialized = false;
        
        return false;
    }

    return true;
}

void morph4k_deinit(void)
{
    morph4k.initialized = false;
}
