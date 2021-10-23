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
#include <limits.h>
#include <stddef.h>

#include "gnb_es_type.h"

#ifdef __UNIX_LIKE_OS__
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_ctl_block.h"
#include "gnb_payload16.h"
#include "gnb_address.h"
#include "gnb_time.h"
#include "gnb_binary.h"
#include "gnb_udp.h"


#pragma pack(push, 1)

#define INDEX_ED25519_SIGN_SIZE   64
#define NODE_RANDOM_SEQUENCE_SIZE 32
#define INDEX_ATTACHMENT_SIZE           (4+128)

typedef struct _push_addr_frame_t {

    struct __attribute__((__packed__)) push_addr_frame_data {

        #define PUSH_ADDR_ACTION_NOTIFY  'N'
        #define PUSH_ADDR_ACTION_CONNECT 'C'

        unsigned char arg0;               //保留
        unsigned char arg1;               //保留
        unsigned char arg2;               //保留
        unsigned char arg3;               //保留

        unsigned char node_key[64];
        uint32_t  node_uuid32;

        uint8_t  addr6_a[16];
        uint16_t port6_a;

        uint8_t  addr6_b[16];
        uint16_t port6_b;

        uint8_t  addr6_c[16];
        uint16_t port6_c;

        uint8_t  addr4_a[4];
        uint16_t port4_a;

        uint8_t  addr4_b[4];
        uint16_t port4_b;

        uint8_t  addr4_c[4];
        uint16_t port4_c;

        unsigned char node_random_sequence[NODE_RANDOM_SEQUENCE_SIZE];
        unsigned char node_random_sequence_sign[INDEX_ED25519_SIGN_SIZE];

        uint64_t src_ts_usec;
        uint64_t idx_ts_usec;

        char text[32];

        char attachment[INDEX_ATTACHMENT_SIZE];

    }data;

    unsigned char src_sign[INDEX_ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) push_addr_frame_t;


#pragma pack(pop)

#define GNB_PAYLOAD_TYPE_INDEX     0x08
#define PAYLOAD_SUB_TYPE_PUSH_ADDR      0x4


static void send_address_to_node(gnb_es_ctx *es_ctx, gnb_node_t *src_node, gnb_node_t *dst_node){

    gnb_ctl_block_t  *ctl_block;

    ctl_block = es_ctx->ctl_block;

    gnb_log_ctx_t *log = es_ctx->log;

    gnb_payload16_t *payload = gnb_payload16_init(0,sizeof(push_addr_frame_t));

    payload->type = GNB_PAYLOAD_TYPE_INDEX;

    payload->sub_type = PAYLOAD_SUB_TYPE_PUSH_ADDR;

    gnb_payload16_set_data_len( payload,  sizeof(push_addr_frame_t) );

    push_addr_frame_t *push_addr_frame = (push_addr_frame_t *)payload->data;

    push_addr_frame->data.node_uuid32 = htonl(src_node->uuid32);
    memcpy(push_addr_frame->data.node_key, src_node->key512, 64);

    push_addr_frame->data.arg0 = 'N';

    memcpy(&push_addr_frame->data.addr6_a, &src_node->udp_sockaddr6.sin6_addr, 16);
    push_addr_frame->data.port6_a = src_node->udp_sockaddr6.sin6_port;


    memcpy(&push_addr_frame->data.addr4_a, &src_node->udp_sockaddr4.sin_addr.s_addr, 4);
    push_addr_frame->data.port4_a = src_node->udp_sockaddr4.sin_port;

    snprintf(push_addr_frame->data.text,32,"%u>%u>%u", ctl_block->core_zone->local_uuid, src_node->uuid32, dst_node->uuid32);

    struct sockaddr_in udp_sockaddr4;
    memset(&udp_sockaddr4, 0, sizeof(struct sockaddr_in));
    memcpy(&udp_sockaddr4.sin_addr.s_addr,&dst_node->tun_addr4.s_addr,4);
    udp_sockaddr4.sin_port = dst_node->tun_sin_port4;
    udp_sockaddr4.sin_family = AF_INET;
    sendto(es_ctx->udp_socket4, (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&udp_sockaddr4, sizeof(struct sockaddr_in));

    GNB_LOG1(log, GNB_LOG_ID_ES_BROADCAST, "send_address_to_node %s\n", GNB_SOCKADDR4STR1(&udp_sockaddr4));

    gnb_payload16_free(payload);

}


static void broadcast_address_to_node(gnb_es_ctx *es_ctx, gnb_node_t *src_node){

    gnb_ctl_block_t  *ctl_block;

    ctl_block = es_ctx->ctl_block;

    gnb_log_ctx_t *log = es_ctx->log;

    gnb_node_t *dst_node;
    int node_num;
    int i;

    node_num = ctl_block->node_zone->node_num;

    for( i=0; i<node_num; i++ ){

        dst_node = &ctl_block->node_zone->node[i];

        if ( dst_node->uuid32 == src_node->uuid32 ){
            continue;
        }

        if ( src_node->uuid32 == ctl_block->core_zone->local_uuid ){
            continue;
        }

        if ( dst_node->uuid32 == ctl_block->core_zone->local_uuid ){
            continue;
        }


        if ( !( (GNB_NODE_STATUS_IPV6_PONG|GNB_NODE_STATUS_IPV4_PONG) & dst_node->udp_addr_status ) ){
            continue;
        }

        if ( 0 == dst_node->tun_sin_port4 ){
            continue;
        }

        #if 1
        if ( (dst_node->type & GNB_NODE_TYPE_IDX) || (dst_node->type & GNB_NODE_TYPE_FWD) ){
            continue;
        }
        #endif

        GNB_LOG1(log, GNB_LOG_ID_ES_BROADCAST, "broadcast_address_to_node [%u] ==> [%u]\n",src_node->uuid32, dst_node->uuid32);

        send_address_to_node(es_ctx, src_node, dst_node);

    }

}


void gnb_broadcast_address(gnb_es_ctx *es_ctx){

    int socket4;
    int socket6;

    gnb_ctl_block_t  *ctl_block;

    ctl_block = es_ctx->ctl_block;

    gnb_node_t *node;
    int node_num;
    int i;

    node_num = ctl_block->node_zone->node_num;

    if ( 0 == node_num ){
        return;
    }

    for( i=0; i<node_num; i++ ){

        node = &ctl_block->node_zone->node[i];

        if ( !( (GNB_NODE_STATUS_IPV6_PONG|GNB_NODE_STATUS_IPV4_PONG) & node->udp_addr_status ) ){
            continue;
        }

        if ( (node->type & GNB_NODE_TYPE_IDX) || (node->type & GNB_NODE_TYPE_FWD) ){
            continue;
        }

        broadcast_address_to_node(es_ctx, node);

    }

}

