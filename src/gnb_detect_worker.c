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

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define _POSIX
#define __USE_MINGW_ALARM
#endif

#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "gnb.h"
#include "gnb_node.h"
#include "gnb_worker.h"
#include "gnb_time.h"
#include "gnb_binary.h"
#include "ed25519/ed25519.h"
#include "gnb_index_frame_type.h"


typedef struct _detect_worker_ctx_t{

    gnb_core_t *gnb_core;

    gnb_payload16_t   *index_frame_payload;

    struct timeval now_timeval;
    uint64_t now_time_sec;
    uint64_t now_time_usec;

    uint8_t  is_send_detect;

    pthread_t thread_worker;

}detect_worker_ctx_t;

#define GNB_DETECT_PUSH_ADDRESS_INTERVAL_SEC   145

static void detect_node_address(gnb_worker_t *gnb_detect_worker, gnb_node_t *node){

    detect_worker_ctx_t *detect_worker_ctx = gnb_detect_worker->ctx;

    gnb_core_t *gnb_core = detect_worker_ctx->gnb_core;

    gnb_address_t address_st;

    if ( 0 == node->detect_port4 ) {
        return;
    }

    memcpy(&address_st.m_address4, &node->detect_addr4, 4);
    address_st.port = htons(node->detect_port4);
    address_st.type = AF_INET;

    detect_worker_ctx->index_frame_payload->sub_type = PAYLOAD_SUB_TYPE_DETECT_ADDR;

    gnb_payload16_set_data_len( detect_worker_ctx->index_frame_payload, sizeof(detect_addr_frame_t) );

    detect_addr_frame_t *detect_addr_frame = (detect_addr_frame_t *)detect_worker_ctx->index_frame_payload->data;

    memset(detect_addr_frame, 0, sizeof(detect_addr_frame_t));

    detect_addr_frame->data.arg0 = 'd';

    memcpy(detect_addr_frame->data.src_key512, gnb_core->local_node->key512, 64);

    detect_addr_frame->data.src_uuid64 = gnb_htonll(gnb_core->local_node->uuid64);
    detect_addr_frame->data.dst_uuid64 = gnb_htonll(node->uuid64);
    detect_addr_frame->data.src_ts_usec = gnb_htonll(detect_worker_ctx->now_time_usec);

    //debug_text
    snprintf(detect_addr_frame->data.text,32,"[%llu]FULL_DETECT[%llu]", gnb_core->local_node->uuid64, node->uuid64);

    ed25519_sign(detect_addr_frame->src_sign, (const unsigned char *)&detect_addr_frame->data, sizeof(struct detect_addr_frame_data), gnb_core->ed25519_public_key, gnb_core->ed25519_private_key);

    address_st.port = htons(node->detect_port4);

    gnb_send_to_address_through_all_sockets(gnb_core, &address_st, detect_worker_ctx->index_frame_payload);

    detect_worker_ctx->is_send_detect = 1;

    node->last_send_detect_usec = detect_worker_ctx->now_time_usec;

    GNB_LOG4(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "Try node address [%llu]->[%llu]%s status=%d\n", gnb_core->local_node->uuid64, node->uuid64, GNB_IP_PORT_STR1(&address_st), node->udp_addr_status);

}


static void detect_node_set_address(gnb_worker_t *gnb_detect_worker, gnb_node_t *node){

    detect_worker_ctx_t *detect_worker_ctx = gnb_detect_worker->ctx;

    gnb_core_t *gnb_core = detect_worker_ctx->gnb_core;

    gnb_address_list_t *address_list;

    address_list = (gnb_address_list_t *)&node->detect_address4_block;

    memcpy(&node->detect_addr4, &address_list->array[node->detect_address4_idx].m_address4, 4);

    if ( 0 == node->detect_addr4.s_addr ) {

        if ( 2==node->detect_address4_idx ) {
            node->detect_address4_idx = 0;
            node->last_detect_sec = detect_worker_ctx->now_time_sec;
        } else {
            node->detect_address4_idx += 1;
        }

        node->detect_port4 = gnb_core->conf->port_detect_start;

        return;
    }

    if ( node->detect_port4 < gnb_core->conf->port_detect_start ) {
        node->detect_port4 = gnb_core->conf->port_detect_start;
    } else {
        node->detect_port4 += 1;
    }

    if ( node->detect_port4 > gnb_core->conf->port_detect_end ) {

        if ( 2==node->detect_address4_idx ) {
            node->detect_address4_idx = 0;
        } else {
            node->detect_address4_idx += 1;
        }

        node->detect_port4 = gnb_core->conf->port_detect_start;

    }

}


static void detect_loop(gnb_worker_t *gnb_detect_worker){

    detect_worker_ctx_t *detect_worker_ctx = gnb_detect_worker->ctx;

    gnb_core_t *gnb_core = detect_worker_ctx->gnb_core;

    gnb_node_t *node;

    uint64_t time_difference;
    uint64_t full_detect_time_difference;

    size_t num = gnb_core->ctl_block->node_zone->node_num;

    if ( 0==num ) {
        return;
    }

    int i;

    for ( i=0; i<num; i++ ) {

        node = &gnb_core->ctl_block->node_zone->node[i];
        if ( gnb_core->local_node->uuid64 == node->uuid64 ) {
            continue;
        }

        //如果本地节点带有 GNB_NODE_TYPE_SLIENCE 属性 将只探测带有 GNB_NODE_TYPE_FWD 属性的节点的地址
        if ( (gnb_core->local_node->type & GNB_NODE_TYPE_SLIENCE) && !(node->type & GNB_NODE_TYPE_FWD) ) {
            GNB_LOG5(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "Skip Detect [%llu]->[%llu] node type is SLIENCE and not FWD\n", gnb_core->local_node->uuid64, node->uuid64);
            continue;
        }

        if ( (GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & node->udp_addr_status ) {
            GNB_LOG5(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "Skip Detect [%llu]->[%llu] already P2P status=%d\n", gnb_core->local_node->uuid64, node->uuid64, node->udp_addr_status);
            continue;
        }

        time_difference = detect_worker_ctx->now_time_sec - node->last_push_addr_sec;
        if ( time_difference > GNB_DETECT_PUSH_ADDRESS_INTERVAL_SEC ) {
            GNB_LOG5(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "Skip Detect [%llu]->[%llu] status=%d time_difference(%"PRIu64") > GNB_DETECT_PUSH_ADDRESS_INTERVAL_SEC(%"PRIu64") last_push_addr_sec=%"PRIu64"\n", 
                     gnb_core->local_node->uuid64, node->uuid64, node->udp_addr_status, time_difference, GNB_DETECT_PUSH_ADDRESS_INTERVAL_SEC, node->last_push_addr_sec);
            continue;
        }

        full_detect_time_difference = detect_worker_ctx->now_time_sec - node->last_full_detect_sec;
        if ( full_detect_time_difference < gnb_core->conf->full_detect_interval_sec ) {

            GNB_LOG5(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "Skip Detect [%llu]->[%llu] status=%d full_detect_time_difference(%"PRIu64") < full_detect_interval_sec(%"PRIu64") last_full_detect_sec=%"PRIu64"\n", 
                     gnb_core->local_node->uuid64, node->uuid64, node->udp_addr_status, full_detect_time_difference, gnb_core->conf->full_detect_interval_sec, node->last_full_detect_sec);

            continue;
        }

        if ( gnb_core->conf->address_detect_interval_usec > 0 ) {
            GNB_SLEEP_MILLISECOND( gnb_core->conf->address_detect_interval_usec/1000 );
        }

        detect_node_set_address(gnb_detect_worker, node);

        if ( 0 == node->detect_addr4.s_addr ) {
            GNB_LOG5(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "Skip Detect [%llu]->[%llu] detect_addr4=0 status=%d \n", 
                     gnb_core->local_node->uuid64, node->uuid64, node->udp_addr_status);
            continue;
        }

        if ( gnb_core->conf->port_detect_start == node->detect_port4 ) {
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "#START FULL DECETE node[%llu] idx[%d]\n", node->uuid64, node->detect_address4_idx);
        }

        detect_node_address(gnb_detect_worker, node);

        if ( gnb_core->conf->port_detect_end == node->detect_port4 ) {
            node->last_full_detect_sec = detect_worker_ctx->now_time_sec;
            GNB_LOG3(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "#END FULL DECETE node[%llu] idx[%d]\n", node->uuid64, node->detect_address4_idx);
        }

    }

}


static void* thread_worker_func( void *data ) {

    int ret;

    gnb_worker_t *gnb_detect_worker = (gnb_worker_t *)data;

    detect_worker_ctx_t *detect_worker_ctx = gnb_detect_worker->ctx;

    gnb_core_t *gnb_core = detect_worker_ctx->gnb_core;

    gnb_detect_worker->thread_worker_flag     = 1;
    gnb_detect_worker->thread_worker_run_flag = 1;

    gnb_worker_wait_primary_worker_started(gnb_core);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_DETECT_WORKER, "start %s success!\n", gnb_detect_worker->name);

    do{

        gnb_worker_sync_time(&detect_worker_ctx->now_time_sec, &detect_worker_ctx->now_time_usec);

        if ( 0==gnb_core->index_address_ring.address_list->num ) {
            GNB_SLEEP_MILLISECOND(1000);
            continue;
        }

        detect_worker_ctx->is_send_detect = 0;

        detect_loop(gnb_detect_worker);

        if ( detect_worker_ctx->is_send_detect ) {
            GNB_SLEEP_MILLISECOND(1);
        } else {
            GNB_SLEEP_MILLISECOND(1000);
        }

    }while(gnb_detect_worker->thread_worker_flag);

    return NULL;

}


static void init(gnb_worker_t *gnb_worker, void *ctx){

    gnb_core_t *gnb_core = (gnb_core_t *)ctx;

    detect_worker_ctx_t *detect_worker_ctx =  (detect_worker_ctx_t *)gnb_heap_alloc(gnb_core->heap, sizeof(detect_worker_ctx_t));
    memset(detect_worker_ctx, 0, sizeof(detect_worker_ctx_t));

    detect_worker_ctx->index_frame_payload =  gnb_payload16_init(0,GNB_MAX_PAYLOAD_SIZE);
    detect_worker_ctx->index_frame_payload->type = GNB_PAYLOAD_TYPE_INDEX;
    detect_worker_ctx->gnb_core = (gnb_core_t *)ctx;
    gnb_worker->ctx = detect_worker_ctx;

    GNB_LOG1(gnb_core->log,GNB_LOG_ID_DETECT_WORKER,"%s init finish\n", gnb_worker->name);

}


static void release(gnb_worker_t *gnb_worker){

    detect_worker_ctx_t *detect_worker_ctx =  (detect_worker_ctx_t *)gnb_worker->ctx;
    gnb_core_t *gnb_core = detect_worker_ctx->gnb_core;

}


static int start(gnb_worker_t *gnb_worker){

    detect_worker_ctx_t *detect_worker_ctx = gnb_worker->ctx;
    pthread_create(&detect_worker_ctx->thread_worker, NULL, thread_worker_func, gnb_worker);
    pthread_detach(detect_worker_ctx->thread_worker);

    return 0;

}


static int stop(gnb_worker_t *gnb_worker){

    detect_worker_ctx_t *detect_worker_ctx = gnb_worker->ctx;
    gnb_core_t *gnb_core = detect_worker_ctx->gnb_core;
    gnb_worker_t *gnb_detect_worker = gnb_core->detect_worker;
    gnb_detect_worker->thread_worker_flag = 0;

    return 0;

}


static int notify(gnb_worker_t *gnb_worker){
    return 0;
}


gnb_worker_t gnb_detect_worker_mod = {

    .name      = "gnb_detect_worker",
    .init      = init,
    .release   = release,
    .start     = start,
    .stop      = stop,
    .notify    = notify,
    .ctx       = NULL

};
