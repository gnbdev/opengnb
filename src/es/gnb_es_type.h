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

#ifndef gnb_es_type_h
#define gnb_es_type_h

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
#define __UNIX_LIKE_OS__ 1
#endif

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include "gnb_alloc.h"
#include "gnb_conf_type.h"
#include "gnb_ctl_block.h"
#include "gnb_hash32.h"
#include "gnb_node_type.h"
#include "gnb_worker_type.h"
#include "gnb_log.h"


typedef struct _gnb_es_ctx{

	gnb_heap_t *heap;

	struct timeval now_timeval;

	uint64_t now_time_sec;
	uint64_t now_time_usec;

	int udp_socket4;
	int udp_socket6;

	int udp_discover_recv_socket4;

	gnb_ctl_block_t  *ctl_block;

	gnb_hash32_map_t *uuid_node_map;

    gnb_node_t *local_node;

	gnb_worker_t *discover_in_lan_worker;

	char *pid_file;

	char *wan_address6_file;

	int service_opt;
	int upnp_opt;
	int resolv_opt;

	int broadcast_address_opt;
	int discover_in_lan_opt;
	int dump_address_opt;

	int if_up_opt;
	int if_down_opt;
	int if_loop_opt;

	gnb_log_ctx_t    *log;

	int daemon;

}gnb_es_ctx;


#define GNB_LOG_ID_ES_CORE                0
#define GNB_LOG_ID_ES_UPNP                1
#define GNB_LOG_ID_ES_RESOLV              2
#define GNB_LOG_ID_ES_BROADCAST           3
#define GNB_LOG_ID_ES_DISCOVER_IN_LAN     4


#define DISCOVER_LAN_IN_BROADCAST_PORT    8998

#endif
