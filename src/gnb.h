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

#ifndef GNB_H
#define GNB_H

#include "gnb_platform.h"

#ifdef __UNIX_LIKE_OS__

#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#endif


#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#define _POSIX

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#include "gnb_alloc.h"

#include "gnb_hash32.h"

#include "gnb_payload16.h"
#include "gnb_tun_drv.h"
#include "gnb_pf.h"
#include "gnb_address.h"
#include "gnb_worker.h"
#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_ctl_block.h"
#include "gnb_log.h"


typedef struct _gnb_core_t{

	gnb_heap_t *heap;

	char *ifname;

	char *if_device_string;

	gnb_node_t *local_node;

	gnb_node_t *select_fwd_node;

	gnb_node_ring_t fwd_node_ring;

	gnb_address_ring_t index_address_ring;

	gnb_address_ring_t fwdu0_address_ring;

	int time_seed_update_factor;
	unsigned char time_seed[64];

	unsigned char ed25519_private_key[64];
	unsigned char ed25519_public_key[32];

	gnb_conf_t *conf;

	uint32_t node_nums;

	gnb_hash32_map_t *uuid_node_map;   //以节点的uuid32作为key的 node 表
	gnb_hash32_map_t *ipv4_node_map;

	gnb_hash32_map_t *subneta_node_map;
	gnb_hash32_map_t *subnetb_node_map;
	gnb_hash32_map_t *subnetc_node_map;


	//不同主模块可以按照模块内部的方式使用这些表
	//由使用的相关联的模块来初始化这两组表

	//这组张表是整型为key
	gnb_hash32_map_t *int32_map0;
	gnb_hash32_map_t *int32_map1;
	gnb_hash32_map_t *int32_map2;


	//这组表是整型为 string
	gnb_hash32_map_t *string_map0;
	gnb_hash32_map_t *string_map1;
	gnb_hash32_map_t *string_map2;

	int tun_fd;

	int udp_ipv6_sockets[GNB_MAX_UDP6_SOCKET_NUM];
	int udp_ipv4_sockets[GNB_MAX_UDP4_SOCKET_NUM];


	int loop_flag;

	gnb_tun_drv_t *drv;

	gnb_pf_array_t     *pf_array;
	gnb_pf_ctx_array_t *pf_ctx_array;

	gnb_payload16_t     *inet_payload;
	gnb_payload16_t     *tun_payload;

	void *platform_ctx;

	gnb_worker_t   *main_worker;

	gnb_worker_t   *node_worker;

	gnb_worker_t   *index_worker;

	gnb_worker_t   *index_service_worker;

	gnb_worker_t   *detect_worker;

	gnb_worker_t   *upnp_worker;

#if 0
	//暂未使用
	gnb_worker_array_t *worker_array;
#endif

	struct timeval now_timeval;

	//把 tun 数据读入 payload 时给pf过程构建的frame首部预留的空间
	//这样构建的 payload 可以直接发送出去，尽量避免了pf过程发生内存拷贝
	size_t tun_payload_offset;

	//route ip frame 类型的 paylaod 的 head size
	size_t route_frame_head_size;

	gnb_ctl_block_t  *ctl_block;

	gnb_log_ctx_t    *log;

}gnb_core_t;


#define GNB_PAYLOAD_TYPE_FWDU0              0x03

#define GNB_PAYLOAD_TYPE_IPFRAME            0x04
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_INIT   (0x0)
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_STD    (0x1)
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY  (0x1 << 1)


#define GNB_PAYLOAD_TYPE_INDEX              0x08
#define PAYLOAD_SUB_TYPE_POST_ADDR          (0x1)
#define PAYLOAD_SUB_TYPE_ECHO_ADDR          (0x2)
#define PAYLOAD_SUB_TYPE_REQUEST_ADDR       (0x3)
#define PAYLOAD_SUB_TYPE_PUSH_ADDR          (0x4)
#define PAYLOAD_SUB_TYPE_DETECT_ADDR        (0x5)


#define GNB_PAYLOAD_TYPE_NODE               0x09
#define GNB_PAYLOAD_TYPE_UDPLOG             0x44

#define GNB_ADDR_TYPE_NONE              (0x0)
#define GNB_ADDR_TYPE_IPV4              (0x1)
#define GNB_ADDR_TYPE_IPV6              (0x1 << 1)


#define GNB_LOG_ID_CORE                  0
#define GNB_LOG_ID_PF                    1
#define GNB_LOG_ID_MAIN_WORKER           2
#define GNB_LOG_ID_NODE_WORKER           3
#define GNB_LOG_ID_INDEX_WORKER          4
#define GNB_LOG_ID_INDEX_SERVICE_WORKER  5
#define GNB_LOG_ID_DETECT_WORKER         6


#endif
