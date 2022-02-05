/* 
 * Copyright (c) 2017-2022, Extrems <extrems@extremscorner.org>
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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "bba.h"
#include "common.h"
#include "dolphin/exi.h"
#include "dolphin/os.h"
#include "frag.h"

#ifndef QUEUE_SIZE
#define QUEUE_SIZE 2
#endif

#define MIN_FRAME_SIZE 60

#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_VLAN 0x8100

#define HW_ETHERNET 1

#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP  17

enum {
	ARP_REQUEST = 1,
	ARP_REPLY,
};

enum {
	CC_VERSION = 0x10,
	CC_ERR     = 0x40,
	CC_GET_DIR,
	CC_GET_FILE,
};

struct eth_addr {
	uint64_t addr : 48;
} __attribute((packed));

struct ipv4_addr {
	uint32_t addr;
} __attribute((packed));

typedef struct {
	struct eth_addr dst_addr;
	struct eth_addr src_addr;
	uint16_t type;
	uint8_t data[];
} __attribute((packed)) eth_header_t;

typedef struct {
	uint16_t pcp : 3;
	uint16_t dei : 1;
	uint16_t vid : 12;
	uint16_t type;
	uint8_t data[];
} __attribute((packed)) vlan_header_t;

typedef struct {
	uint16_t hardware_type;
	uint16_t protocol_type;
	uint8_t hardware_length;
	uint8_t protocol_length;
	uint16_t operation;

	struct eth_addr src_mac;
	struct ipv4_addr src_ip;
	struct eth_addr dst_mac;
	struct ipv4_addr dst_ip;
} __attribute((packed)) arp_packet_t;

typedef struct {
	uint8_t version : 4;
	uint8_t words   : 4;
	uint8_t dscp    : 6;
	uint8_t ecn     : 2;
	uint16_t length;
	uint16_t id;
	uint16_t flags  : 3;
	uint16_t offset : 13;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t checksum;
	struct ipv4_addr src_addr;
	struct ipv4_addr dst_addr;
	uint8_t data[];
} __attribute((packed)) ipv4_header_t;

typedef struct {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t length;
	uint16_t checksum;
	uint8_t data[];
} __attribute((packed)) udp_header_t;

typedef struct {
	uint8_t command;
	uint8_t checksum;
	uint16_t key;
	uint16_t sequence;
	uint16_t data_length;
	uint32_t position;
	uint8_t data[];
} __attribute((packed)) fsp_header_t;

static struct eth_addr  *const _client_mac = (struct eth_addr  *)VAR_CLIENT_MAC;
static struct ipv4_addr *const _client_ip  = (struct ipv4_addr *)VAR_CLIENT_IP;
static struct eth_addr  *const _server_mac = (struct eth_addr  *)VAR_SERVER_MAC;
static struct ipv4_addr *const _server_ip  = (struct ipv4_addr *)VAR_SERVER_IP;
static uint16_t         *const _port       = (uint16_t         *)VAR_SERVER_PORT;

static struct {
	uint8_t command;
	uint16_t key;
	uint16_t sequence;
	uint16_t data_length;
	uint32_t position;
	uint16_t fragment_id;
	struct {
		void *buffer;
		uint32_t length;
		uint32_t offset;
		uint32_t position;
		const char *path;
		uint16_t pathlen;
		frag_callback callback;
	} queue[QUEUE_SIZE], *queued;
} _fsp = {0};

static uint16_t ipv4_checksum(ipv4_header_t *header)
{
	uint16_t *data = (uint16_t *)header;
	uint32_t sum[2] = {0};

	for (int i = 0; i < header->words; i++) {
		sum[0] += *data++;
		sum[1] += *data++;
	}

	sum[0] += sum[1];
	sum[0] += sum[0] >> 16;
	return ~sum[0];
}

static uint8_t fsp_checksum(fsp_header_t *header, size_t size)
{
	uint8_t *data = (uint8_t *)header;
	uint32_t sum = size;

	for (int i = 0; i < size; i++)
		sum += *data++;

	sum += sum >> 8;
	return sum;
}

static void fsp_get_file(uint32_t offset, uint32_t length, const void *path, uint16_t pathlen)
{
	uint8_t data[MIN_FRAME_SIZE + pathlen];
	eth_header_t *eth = (eth_header_t *)data;
	ipv4_header_t *ipv4 = (ipv4_header_t *)eth->data;
	udp_header_t *udp = (udp_header_t *)ipv4->data;
	fsp_header_t *fsp = (fsp_header_t *)udp->data;

	_fsp.command = CC_GET_FILE;
	_fsp.sequence++;
	_fsp.position = offset;
	_fsp.data_length = MIN(length, UINT16_MAX);

	fsp->command = _fsp.command;
	fsp->checksum = 0x00;
	fsp->key = _fsp.key;
	fsp->sequence = _fsp.sequence;
	fsp->position = _fsp.position;
	fsp->data_length = pathlen;
	*(uint16_t *)(memcpy(fsp->data, path, pathlen) + fsp->data_length) = _fsp.data_length;
	fsp->checksum = fsp_checksum(fsp, sizeof(*fsp) + fsp->data_length + sizeof(uint16_t));

	udp->src_port = *_port;
	udp->dst_port = *_port;
	udp->length = sizeof(*udp) + sizeof(*fsp) + fsp->data_length + sizeof(uint16_t);
	udp->checksum = 0x0000;

	ipv4->version = 4;
	ipv4->words = sizeof(*ipv4) / 4;
	ipv4->dscp = 46;
	ipv4->ecn = 0b00;
	ipv4->length = sizeof(*ipv4) + udp->length;
	ipv4->id = 0;
	ipv4->flags = 0b000;
	ipv4->offset = 0;
	ipv4->ttl = 64;
	ipv4->protocol = IP_PROTO_UDP;
	ipv4->checksum = 0x0000;
	ipv4->src_addr = *_client_ip;
	ipv4->dst_addr = *_server_ip;
	ipv4->checksum = ipv4_checksum(ipv4);

	eth->dst_addr = *_server_mac;
	eth->src_addr = *_client_mac;
	eth->type = ETH_TYPE_IPV4;
	bba_transmit(eth, sizeof(*eth) + ipv4->length);
}

static void fsp_read_queued(void)
{
	if (!bba.lock && !EXILock(EXI_CHANNEL_0, EXI_DEVICE_2, (EXICallback)fsp_read_queued))
		return;

	void *buffer = _fsp.queued->buffer + _fsp.queued->offset;
	uint32_t length = _fsp.queued->length - _fsp.queued->offset;
	uint32_t offset = _fsp.queued->position + _fsp.queued->offset;
	const char *path = _fsp.queued->path;
	uint16_t pathlen = _fsp.queued->pathlen;

	fsp_get_file(offset, length, path, pathlen);

	OSSetAlarm(&read_alarm, OSSecondsToTicks(1), (OSAlarmHandler)fsp_read_queued);

	if (!bba.lock) EXIUnlock(EXI_CHANNEL_0);
}

static void fsp_done_queued(void)
{
	void *buffer = _fsp.queued->buffer;
	uint32_t length = _fsp.queued->length;
	uint32_t offset = _fsp.queued->offset;
	const char *path = _fsp.queued->path;
	uint16_t pathlen = _fsp.queued->pathlen;

	_fsp.queued->callback(buffer, offset);

	_fsp.queued->callback = NULL;
	_fsp.queued = NULL;

	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (_fsp.queue[i].callback != NULL) {
			_fsp.queued = &_fsp.queue[i];
			fsp_read_queued();
			return;
		}
	}
}

bool do_read_disc(void *buffer, uint32_t length, uint32_t offset, const frag_t *frag, frag_callback callback)
{
	for (int i = 0; i < QUEUE_SIZE; i++) {
		if (_fsp.queue[i].callback == NULL) {
			_fsp.queue[i].buffer = buffer;
			_fsp.queue[i].length = length;
			_fsp.queue[i].offset = 0;
			_fsp.queue[i].position = offset;
			_fsp.queue[i].path = frag->path;
			_fsp.queue[i].pathlen = frag->pathlen;
			_fsp.queue[i].callback = callback;

			if (_fsp.queued == NULL) {
				_fsp.queued = &_fsp.queue[i];
				fsp_read_queued();
			}
			return true;
		}
	}

	return false;
}

static void fsp_input(bba_page_t page, eth_header_t *eth, ipv4_header_t *ipv4, udp_header_t *udp, fsp_header_t *fsp, size_t size)
{
	if (size < sizeof(*fsp))
		return;
	if (udp->length < sizeof(*udp) + sizeof(*fsp) + fsp->data_length)
		return;

	size -= sizeof(*fsp);

	switch (fsp->command) {
		case CC_ERR:
			break;
		case CC_GET_FILE:
			if (fsp->command  == _fsp.command  &&
				fsp->sequence == _fsp.sequence &&
				fsp->position == _fsp.position) {

				_fsp.fragment_id = ipv4->id;
				_fsp.data_length = fsp->data_length;
			}
			break;
	}

	_fsp.key = fsp->key;
}

static void udp_input(bba_page_t page, eth_header_t *eth, ipv4_header_t *ipv4, udp_header_t *udp, size_t size)
{
	if (ipv4->src_addr.addr == (*_server_ip).addr &&
		ipv4->dst_addr.addr == (*_client_ip).addr) {

		*_server_mac = eth->src_addr;

		if (ipv4->offset == 0) {
			if (size < sizeof(*udp))
				return;
			if (udp->length < sizeof(*udp))
				return;

			size -= sizeof(*udp);

			if (udp->src_port == *_port &&
				udp->dst_port == *_port)
				fsp_input(page, eth, ipv4, udp, (void *)udp->data, size);
		}

		if (ipv4->id == _fsp.fragment_id) {
			switch (_fsp.command) {
				case CC_GET_FILE:
				{
					uint8_t *data = _fsp.queued->buffer + _fsp.queued->offset;
					int data_size = MIN(_fsp.data_length, _fsp.queued->length - _fsp.queued->offset);

					int offset = ipv4->offset * 8 - sizeof(udp_header_t) - sizeof(fsp_header_t);
					int udp_offset = MAX(-offset, 0);
					int data_offset = MAX(offset, 0);
					int page_offset = (uint8_t *)udp + udp_offset - page;

					size = MIN(MAX(data_size - data_offset, 0), size);

					int page_size = MIN(size, sizeof(bba_page_t) - page_offset);
					memcpy(data + data_offset, page + page_offset, page_size);
					data_offset += page_size;
					size        -= page_size;

					if (ipv4->flags & 0b001) {
						bba_receive_end(page, data + data_offset, size);
					} else {
						OSCancelAlarm(&read_alarm);

						_fsp.command = 0x00;
						_fsp.queued->offset += data_size;

						if (_fsp.queued->offset != _fsp.queued->length)
							fsp_read_queued();

						bba_receive_end(page, data + data_offset, size);

						if (_fsp.queued->offset == _fsp.queued->length)
							fsp_done_queued();
					}
					break;
				}
			}
		}
	}
}

static void ipv4_input(bba_page_t page, eth_header_t *eth, ipv4_header_t *ipv4, size_t size)
{
	if (ipv4->version != 4)
		return;
	if (ipv4->words < 5 || ipv4->words * 4 > ipv4->length)
		return;
	if (size < ipv4->length)
		return;
	if (ipv4_checksum(ipv4))
		return;

	size = ipv4->length - ipv4->words * 4;

	switch (ipv4->protocol) {
		case IP_PROTO_UDP:
			udp_input(page, eth, ipv4, (void *)ipv4 + ipv4->words * 4, size);
			break;
	}
}

static void arp_input(bba_page_t page, eth_header_t *eth, arp_packet_t *arp, size_t size)
{
	if (arp->hardware_type != HW_ETHERNET || arp->hardware_length != sizeof(struct eth_addr))
		return;
	if (arp->protocol_type != ETH_TYPE_IPV4 || arp->protocol_length != sizeof(struct ipv4_addr))
		return;

	switch (arp->operation) {
		case ARP_REQUEST:
			if ((!arp->dst_mac.addr ||
				arp->dst_mac.addr == (*_client_mac).addr) &&
				arp->dst_ip.addr  == (*_client_ip).addr) {

				arp->operation = ARP_REPLY;

				arp->dst_mac = arp->src_mac;
				arp->dst_ip  = arp->src_ip;

				arp->src_mac = *_client_mac;
				arp->src_ip  = *_client_ip;

				eth->dst_addr = arp->dst_mac;
				eth->src_addr = arp->src_mac;

				bba_transmit(eth, MIN_FRAME_SIZE);
			}
			break;
		case ARP_REPLY:
			if (arp->dst_mac.addr == (*_client_mac).addr &&
				arp->dst_ip.addr  == (*_client_ip).addr &&
				arp->src_ip.addr  == (*_server_ip).addr) {

				*_server_mac = arp->src_mac;
			}
			break;
	}
}

static void eth_input(bba_page_t page, eth_header_t *eth, size_t size)
{
	if (size < MIN_FRAME_SIZE)
		return;

	size -= sizeof(*eth);

	switch (eth->type) {
		case ETH_TYPE_ARP:
			arp_input(page, eth, (void *)eth->data, size);
			break;
		case ETH_TYPE_IPV4:
			ipv4_input(page, eth, (void *)eth->data, size);
			break;
	}
}
