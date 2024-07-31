/*
   Copyright (C) gnbdev

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GNB_CTL_BLOCK_H
#define GNB_CTL_BLOCK_H

#include <stdint.h>

#include "gnb_mmap.h"
#include "gnb_payload16.h"
#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_log_type.h"


#define GNB_MAX_PAYLOAD_BLOCK_SIZE 1024*64

#define GNB_PAYLOAD_BUFFER_PADDING_SIZE 1024

#define CTL_BLOCK_ES_MAGIC_IDX 3
#define CTL_BLOCK_VT_MAGIC_IDX 4

typedef struct _gnb_ctl_magic_number_t {

	unsigned char data[16];

}gnb_ctl_magic_number_t;


typedef struct _gnb_ctl_conf_zone_t {

	unsigned char name[8];
	gnb_conf_t conf_st;

}gnb_ctl_conf_zone_t;


typedef struct _gnb_ctl_core_zone_t {

	unsigned char name[8];

	gnb_uuid_t local_uuid;

	unsigned char ed25519_private_key[64];
	unsigned char ed25519_public_key[32];

	uint32_t  tun_if_id;
	unsigned char ifname[256];
	unsigned char if_device_string[256];

	unsigned char wan6_addr[16];
	uint16_t wan6_port;   //网络字节序

	unsigned char wan4_addr[4];
	uint16_t wan4_port;   //网络字节序

	gnb_log_ctx_t log_ctx_st;

	unsigned char index_address_block[ sizeof(gnb_address_list_t) + sizeof(gnb_address_t) * GNB_MAX_ADDR_RING ];

	unsigned char ufwd_address_block[ sizeof(gnb_address_list_t) + sizeof(gnb_address_t) * 16 ];

	unsigned char  tun_payload_block[ GNB_PAYLOAD_BUFFER_PADDING_SIZE + sizeof(gnb_payload16_t) + GNB_MAX_PAYLOAD_BLOCK_SIZE ];
	unsigned char inet_payload_block[ GNB_PAYLOAD_BUFFER_PADDING_SIZE + sizeof(gnb_payload16_t) + GNB_MAX_PAYLOAD_BLOCK_SIZE ];
	
	unsigned char pf_worker_payload_blocks[0];

}gnb_ctl_core_zone_t;


typedef struct _gnb_ctl_status_zone_t {

	uint64_t keep_alive_ts_sec;

}gnb_ctl_status_zone_t;


typedef struct _gnb_ctl_node_zone_t {

	unsigned char name[8];
	int node_num;
	gnb_node_t node[0];

}gnb_ctl_node_zone_t;


typedef struct _gnb_ctl_block_t {

	uint32_t *entry_table256;

	gnb_ctl_magic_number_t *magic_number;

	gnb_ctl_conf_zone_t    *conf_zone;

	gnb_ctl_core_zone_t    *core_zone;

	gnb_ctl_status_zone_t  *status_zone;

	gnb_ctl_node_zone_t    *node_zone;

	gnb_mmap_block_t       *mmap_block;

}gnb_ctl_block_t;


ssize_t gnb_ctl_file_size(const char *filename);


gnb_ctl_block_t *gnb_ctl_block_build(void *memory, uint32_t payload_block_size, size_t node_num, uint8_t pf_worker_num);

void gnb_ctl_block_build_finish(void *memory);

void gnb_ctl_block_setup(gnb_ctl_block_t *ctl_block, void *memory);

gnb_ctl_block_t *gnb_get_ctl_block(const char *ctl_block_file, int flag);

#define MIN_CTL_BLOCK_FILE_SIZE  (sizeof(uint32_t)*256 + sizeof(gnb_ctl_magic_number_t) + sizeof(gnb_ctl_conf_zone_t) + sizeof(gnb_ctl_core_zone_t) + sizeof(gnb_ctl_status_zone_t) + sizeof(gnb_ctl_node_zone_t) + sizeof(gnb_node_t))

#define GNB_CTL_KEEP_ALIVE_TS 15

#endif
