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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "gnb_es_type.h"
#include "gnb_udp.h"
#include "gnb_time.h"
#include "gnb_mmap.h"

#ifdef __UNIX_LIKE_OS__

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#endif

#ifdef _WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

void gnb_es_dump_address_list(gnb_es_ctx *es_ctx);
void gnb_broadcast_address(gnb_es_ctx *es_ctx);
void gnb_es_upnp(gnb_conf_t *conf, gnb_log_ctx_t *log);
void gnb_resolv_address(gnb_es_ctx *es_ctx);
void gnb_load_wan_ipv6_address(gnb_es_ctx *es_ctx);
void gnb_es_if_up(gnb_es_ctx *es_ctx);
void gnb_es_if_down(gnb_es_ctx *es_ctx);


static void sync_es_time(gnb_es_ctx *es_ctx){

    gettimeofday(&es_ctx->now_timeval, NULL);

    es_ctx->now_time_sec  = (uint64_t)es_ctx->now_timeval.tv_sec;
    es_ctx->now_time_usec = (uint64_t)es_ctx->now_timeval.tv_sec *1000000 + es_ctx->now_timeval.tv_usec;

}


gnb_es_ctx* gnb_es_ctx_init(int is_service, char *ctl_block_file, gnb_log_ctx_t *log){

    ssize_t ctl_file_size = 0;

    gnb_mmap_block_t *mmap_block;

    void *block;

    char *memory;

    gnb_ctl_block_t  *ctl_block = NULL;


    if (0 == is_service ) {

        ctl_block = gnb_get_ctl_block(ctl_block_file, 1);

        if ( NULL != ctl_block ){
            goto gnb_map_init_success;
        } else {
            return NULL;
        }

    }

    int try_count = 0;

    do{

        ctl_block = gnb_get_ctl_block(ctl_block_file, 1);

        if ( NULL == ctl_block ){
            goto wait_gnb_init;
        }

        memory = (char *)ctl_block->entry_table256;

        memory[CTL_BLOCK_ES_MAGIC_IDX] = 'E';

        break;

wait_gnb_init:

        GNB_LOG1(log, GNB_LOG_ID_ES_CORE, "Wait For GNB init...\n");

        GNB_SLEEP_MILLISECOND(300);

        try_count++;

        if ( try_count > 100 ){
            GNB_LOG1(log, GNB_LOG_ID_ES_CORE, "GNB init timeout\n");
        }

    }while(1);

gnb_map_init_success:

    GNB_LOG1(log, GNB_LOG_ID_ES_CORE, "open ctl block success [%s]\n", ctl_block->magic_number->data);

    gnb_node_t *node;

    int node_num;

    gnb_heap_t *heap = gnb_heap_create(8192);

    gnb_es_ctx *es_ctx = (gnb_es_ctx *)gnb_heap_alloc(heap,sizeof(gnb_es_ctx));

    memset(es_ctx, 0, sizeof(gnb_es_ctx));

    es_ctx->heap = heap;

    es_ctx->ctl_block = ctl_block;

    //对 node 进行索引
    es_ctx->uuid_node_map = gnb_hash32_create(es_ctx->heap,1024,1024);

    node_num = es_ctx->ctl_block->node_zone->node_num;

    if ( 0 == node_num ){
        goto finish;
    }

    int i;

    for( i=0; i<node_num; i++ ){
        node = &es_ctx->ctl_block->node_zone->node[i];
        GNB_HASH32_UINT32_SET(es_ctx->uuid_node_map, node->uuid32, node);
    }

finish:

    return es_ctx;
}


#define GNB_RESOLV_INTERVAL_SEC        900
#define GNB_UPNP_INTERVAL_SEC          180
#define GNB_DUMP_ADDRESS_INTERVAL_SEC  15
#define GNB_BROADCAST_INTERVAL_SEC     300

void gnb_start_environment_service(gnb_es_ctx *es_ctx){

    uint64_t last_resolv_address_sec   = 0;
    uint64_t last_upnp_time_sec        = 0;
    uint64_t last_dump_address_sec     = 0;
    uint64_t last_broadcast_addres_sec = 0;

    if ( es_ctx->if_up_opt ) {
        gnb_es_if_up(es_ctx);
        return;
    }

    if ( es_ctx->if_down_opt ) {
        gnb_es_if_down(es_ctx);
        return;
    }

    es_ctx->udp_socket4 = socket(AF_INET, SOCK_DGRAM, 0);
    gnb_bind_udp_socket_ipv4(es_ctx->udp_socket4, "0.0.0.0", 0);

    es_ctx->udp_socket6 = socket(AF_INET6, SOCK_DGRAM, 0);
    gnb_bind_udp_socket_ipv6(es_ctx->udp_socket6, "::",      0);

    do{

        sync_es_time(es_ctx);

        if ( es_ctx->resolv_opt && (es_ctx->now_time_sec - last_resolv_address_sec ) > GNB_RESOLV_INTERVAL_SEC ) {

            gnb_resolv_address(es_ctx);

            gnb_load_wan_ipv6_address(es_ctx);

            last_resolv_address_sec = es_ctx->now_time_sec;

        }

        if ( es_ctx->upnp_opt && (es_ctx->now_time_sec - last_upnp_time_sec ) > GNB_UPNP_INTERVAL_SEC ) {

            gnb_es_upnp(&es_ctx->ctl_block->conf_zone->conf_st,  es_ctx->log);

            last_upnp_time_sec = es_ctx->now_time_sec;

        }

        if ( es_ctx->dump_address_opt && (es_ctx->now_time_sec - last_dump_address_sec ) > GNB_DUMP_ADDRESS_INTERVAL_SEC ) {

            gnb_es_dump_address_list(es_ctx);

            last_dump_address_sec = es_ctx->now_time_sec;

        }

        if ( es_ctx->broadcast_addres_opt && (es_ctx->now_time_sec - last_broadcast_addres_sec ) > GNB_BROADCAST_INTERVAL_SEC) {

            gnb_broadcast_address(es_ctx);

            last_broadcast_addres_sec = es_ctx->now_time_sec;

        }

        GNB_SLEEP_MILLISECOND(300);

    }while(es_ctx->service_opt);


}


void gnb_stop_environment_service(gnb_es_ctx *es_ctx){



}

