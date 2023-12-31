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

#include "gnb_node.h"

#include <limits.h>
#include <stddef.h>

#include <time.h>
#include <sys/time.h>

#ifdef __UNIX_LIKE_OS__
#include <arpa/inet.h>
#endif


#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#include "gnb_keys.h"
#include "gnb_time.h"
#include "gnb_udp.h"
#include "ed25519/ed25519.h"
#include "ed25519/sha512.h"
#include "gnb_binary.h"


gnb_node_t * gnb_node_init(gnb_core_t *gnb_core, uint32_t uuid32){

    gnb_node_t *node = &gnb_core->ctl_block->node_zone->node[gnb_core->node_nums];

    memset(node,0,sizeof(gnb_node_t));

    node->uuid32 = uuid32;

    node->type =  GNB_NODE_TYPE_STD;

    uint32_t node_id_network_order;
    uint32_t local_node_id_network_order;

    gnb_address_list_t *static_address_list;
    gnb_address_list_t *dynamic_address_list;
    gnb_address_list_t *resolv_address_list;
    gnb_address_list_t *push_address_list;
    gnb_address_list_t *detect_address_list;

    static_address_list  = (gnb_address_list_t *)node->static_address_block;
    dynamic_address_list = (gnb_address_list_t *)node->dynamic_address_block;
    resolv_address_list  = (gnb_address_list_t *)node->resolv_address_block;
    push_address_list    = (gnb_address_list_t *)node->push_address_block;

    detect_address_list  = (gnb_address_list_t *)node->detect_address4_block;

    static_address_list->size  = GNB_NODE_STATIC_ADDRESS_NUM;
    dynamic_address_list->size = GNB_NODE_DYNAMIC_ADDRESS_NUM;
    resolv_address_list->size  = GNB_NODE_RESOLV_ADDRESS_NUM;
    push_address_list->size    = GNB_NODE_PUSH_ADDRESS_NUM;

    detect_address_list->size  = 3;

    if ( 0 == gnb_core->conf->lite_mode ) {

        if ( gnb_core->conf->local_uuid != uuid32 ) {

            gnb_load_public_key(gnb_core, uuid32, node->public_key);
            ed25519_key_exchange(node->shared_secret, node->public_key, gnb_core->ed25519_private_key);

        } else {

            memcpy(node->public_key, gnb_core->ed25519_public_key, 32);
            memset(node->shared_secret, 0, 32);
            memset(node->crypto_key, 0, 64);

        }

    } else {

        //lite mode
        if ( gnb_core->conf->local_uuid == uuid32 ) {
            memset(gnb_core->ed25519_private_key, 0, 64);
            memset(gnb_core->ed25519_public_key,  0,32);
        }

        memset(node->public_key, 0, 32);

        node_id_network_order       = htonl(uuid32);
        local_node_id_network_order = htonl(gnb_core->conf->local_uuid);

        memcpy(node->public_key, &node_id_network_order, 4);

        memset(node->shared_secret, 0, 32);
        memcpy(node->shared_secret, gnb_core->conf->crypto_passcode, 4);

        if ( node_id_network_order > local_node_id_network_order ) {

            memcpy(node->shared_secret+4, &node_id_network_order, 4);
            memcpy(node->shared_secret+8, &local_node_id_network_order, 4);

        } else {

            memcpy(node->shared_secret+4, &local_node_id_network_order, 4);
            memcpy(node->shared_secret+8, &node_id_network_order, 4);

        }

    }

    return node;

}


void gnb_init_node_key512(gnb_core_t *gnb_core){

    int num = gnb_core->ctl_block->node_zone->node_num;

    int i;

    uint32_t uuid32;

    gnb_node_t *node;

    unsigned char buffer[32+4];

    for (i=0;i<num;i++) {

        node = &gnb_core->ctl_block->node_zone->node[i];

        if ( NULL==node ) {
            continue;
        }

        memcpy(buffer,    node->public_key,32);
        memcpy(buffer+32, gnb_core->conf->crypto_passcode, 4);

        sha512(buffer, 32+4, node->key512);

    }

}


void gnb_add_forward_node_ring(gnb_core_t *gnb_core, uint32_t uuid32){

    gnb_node_t *node;

    node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, uuid32);

    if ( NULL == node ) {
        return;
    }

    if ( gnb_core->fwd_node_ring.num >= GNB_MAX_NODE_RING ) {
        gnb_core->fwd_node_ring.num = GNB_MAX_NODE_RING;
        return;
    }

    node->type |= GNB_NODE_TYPE_IDX;

    gnb_core->fwd_node_ring.nodes[gnb_core->fwd_node_ring.num] = node;

    gnb_core->fwd_node_ring.cur_index = gnb_core->fwd_node_ring.num;

    gnb_core->fwd_node_ring.num++;

}


gnb_node_t* gnb_select_forward_node(gnb_core_t *gnb_core){

    int i;

    gnb_node_t *node;

    if ( 0 == gnb_core->fwd_node_ring.num ) {
        return NULL;
    }

    if ( 1 == gnb_core->fwd_node_ring.num ) {
        return gnb_core->fwd_node_ring.nodes[0];
    }

    if ( gnb_core->conf->pf_worker_num > 0 ) {
        return gnb_core->fwd_node_ring.nodes[0];
    }

    if ( GNB_MULTI_ADDRESS_TYPE_SIMPLE_FAULT_TOLERANT == gnb_core->conf->multi_forward_type ) {
        goto SIMPLE_FAULT_TOLERANT;
    }

    if ( GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE == gnb_core->conf->multi_forward_type ) {
        goto SIMPLE_LOAD_BALANCE;
    }

SIMPLE_FAULT_TOLERANT:

    for ( i=0; i<gnb_core->fwd_node_ring.num; i++ ) {

        if ( (GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & gnb_core->fwd_node_ring.nodes[i]->udp_addr_status ) {
            return gnb_core->fwd_node_ring.nodes[i];
        }

    }

    return gnb_core->fwd_node_ring.nodes[0];

SIMPLE_LOAD_BALANCE:

    for ( i=0; i<gnb_core->fwd_node_ring.num; i++ ) {

        node = gnb_core->fwd_node_ring.nodes[ gnb_core->fwd_node_ring.cur_index ];

        if ( (GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & node->udp_addr_status ) {

            gnb_core->fwd_node_ring.cur_index++;

            if ( gnb_core->fwd_node_ring.cur_index >= gnb_core->fwd_node_ring.num ) {
                gnb_core->fwd_node_ring.cur_index = 0;
            }

            return node;

        }

    }

    return gnb_core->fwd_node_ring.nodes[0];

}


void gnb_send_to_address(gnb_core_t *gnb_core, gnb_address_t *address, gnb_payload16_t *payload){

    struct sockaddr_in  in;
    struct sockaddr_in6 in6;

    if ( 0 == address->port ) {
        return;
    }

    if ( AF_INET6 == address->type ) {

        memset(&in6,0,sizeof(struct sockaddr_in6));

        in6.sin6_family = AF_INET6;
        in6.sin6_port = address->port;
        memcpy(&in6.sin6_addr, address->address.addr6, 16);

        sendto(gnb_core->udp_ipv6_sockets[0], (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&in6, sizeof(struct sockaddr_in6) );

    }

    if ( AF_INET == address->type ) {

        memset(&in,0,sizeof(struct sockaddr_in));

        in.sin_family = AF_INET;
        in.sin_port = address->port;
        memcpy(&in.sin_addr, address->address.addr4, 4);

        sendto(gnb_core->udp_ipv4_sockets[0], (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&in, sizeof(struct sockaddr_in));

    }

}


void gnb_send_udata_to_address(gnb_core_t *gnb_core, gnb_address_t *address, void *udata, size_t udata_size){

    struct sockaddr_in  in;
    struct sockaddr_in6 in6;

    if ( 0 == address->port ) {
        return;
    }

    if ( AF_INET6 == address->type ) {

        memset(&in6,0,sizeof(struct sockaddr_in6));

        in6.sin6_family = AF_INET6;
        in6.sin6_port = address->port;

        memcpy(&in6.sin6_addr, address->address.addr6, 16);

        sendto(gnb_core->udp_ipv6_sockets[0], udata, udata_size, 0, (struct sockaddr *)&in6, sizeof(struct sockaddr_in6) );

    }

    if ( AF_INET == address->type ) {

        memset(&in,0,sizeof(struct sockaddr_in));

        in.sin_family = AF_INET;
        in.sin_port = address->port;
        memcpy(&in.sin_addr, address->address.addr4, 4);

        sendto(gnb_core->udp_ipv4_sockets[0], udata, udata_size, 0, (struct sockaddr *)&in, sizeof(struct sockaddr_in));

    }

}


void gnb_send_address_list(gnb_core_t *gnb_core, gnb_address_list_t *address_list, gnb_payload16_t *payload){

    int i;

    for ( i=0; i<address_list->num; i++ ) {
         gnb_send_to_address(gnb_core, &address_list->array[i], payload);
    }

}


void gnb_send_available_address_list(gnb_core_t *gnb_core, gnb_address_list_t *address_list, gnb_payload16_t *payload, uint64_t now_sec){

    int i;

    for ( i=0; i<address_list->num; i++ ) {

        if ( now_sec - address_list->array[i].ts_sec > GNB_ADDRESS_LIFE_TIME_TS_SEC ) {
            continue;
        }

        gnb_send_to_address(gnb_core, &address_list->array[i], payload);

    }

}


void gnb_send_address_list_through_all_sockets(gnb_core_t *gnb_core, gnb_address_list_t *address_list, gnb_payload16_t *payload){

    int i;

    for ( i=0; i<address_list->num; i++ ) {
         gnb_send_to_address_through_all_sockets(gnb_core, &address_list->array[i], payload);
    }

}


void gnb_send_to_address_through_all_sockets(gnb_core_t *gnb_core, gnb_address_t *address, gnb_payload16_t *payload){

    struct sockaddr_in  in;
    struct sockaddr_in6 in6;

    if ( 0 == address->port ) {
        return;
    }

    int i;

    if ( AF_INET6 == address->type ) {

        memset(&in6,0,sizeof(struct sockaddr_in6));

        in6.sin6_family = AF_INET6;
        in6.sin6_port = address->port;

        memcpy(&in6.sin6_addr, address->address.addr6, 16);

        for (i=0; i<gnb_core->conf->udp6_socket_num; i++) {
            sendto(gnb_core->udp_ipv6_sockets[i], (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&in6, sizeof(struct sockaddr_in6) );
        }

    }

    if ( AF_INET == address->type ) {

        memset(&in,0,sizeof(struct sockaddr_in));

        in.sin_family = AF_INET;
        in.sin_port = address->port;
        memcpy(&in.sin_addr, address->address.addr4, 4);

        for (i=0; i<gnb_core->conf->udp4_socket_num; i++) {
            sendto(gnb_core->udp_ipv4_sockets[i], (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&in, sizeof(struct sockaddr_in));
        }

    }

}


gnb_address_t* gnb_select_index_address(gnb_core_t *gnb_core, uint64_t now_sec){

    int i;

    gnb_address_t *gnb_address;

    if ( 0 == gnb_core->index_address_ring.address_list->num ) {
        return NULL;
    }

    if ( 1 == gnb_core->index_address_ring.address_list->num ) {
        return &gnb_core->index_address_ring.address_list->array[0];
    }

    if ( GNB_MULTI_ADDRESS_TYPE_SIMPLE_FAULT_TOLERANT == gnb_core->conf->multi_index_type ) {
        goto SIMPLE_FAULT_TOLERANT;
    }

    if ( GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE == gnb_core->conf->multi_index_type ) {
        goto SIMPLE_LOAD_BALANCE;
    }

SIMPLE_FAULT_TOLERANT:

    for ( i=0; i<gnb_core->index_address_ring.address_list->num; i++ ) {

        gnb_address = &gnb_core->index_address_ring.address_list->array[i];

        if ( !(gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4) && ( AF_INET6 == gnb_address->type) ) {
            continue;
        }

        if ( GNB_ADDRESS_AVAILABLE(gnb_address, now_sec) ) {
            return &gnb_core->index_address_ring.address_list->array[i];
        }

    }

    return &gnb_core->index_address_ring.address_list->array[0];

SIMPLE_LOAD_BALANCE:

    for ( i=0; i<gnb_core->index_address_ring.address_list->num; i++ ) {

        gnb_address = &gnb_core->index_address_ring.address_list->array[ gnb_core->index_address_ring.cur_index ];

        gnb_core->index_address_ring.cur_index++;

        if ( gnb_core->index_address_ring.cur_index >= gnb_core->index_address_ring.address_list->num ) {
            gnb_core->index_address_ring.cur_index = 0;
        }

        if ( !(gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4) && ( AF_INET6 == gnb_address->type) ) {
            continue;
        }

        if ( GNB_ADDRESS_AVAILABLE(gnb_address,now_sec) ) {
            return gnb_address;
        }

    }

    return &gnb_core->index_address_ring.address_list->array[0];

}


gnb_address_t* gnb_select_available_address4(gnb_core_t *gnb_core, gnb_node_t *node){

    gnb_address_t *address = NULL;

    int i;

    gnb_address_list_t *address_list;

    address_list = (gnb_address_list_t *)&node->push_address_block;

    for ( i=0; i<address_list->num; i++ ) {

        if ( AF_INET != address_list->array[i].type ) {
            continue;
        }

        if ( 0 == address_list->array[i].port ) {
            continue;
        }

        //如果存在多个地址，选择更新时间是最新的
        if ( NULL != address ) {

            if ( address->ts_sec > address_list->array[i].ts_sec ) {
                continue;
            }

        }

        address = &address_list->array[i];

    }

    if ( NULL != address ) {
        goto finish;
    }

    address_list = (gnb_address_list_t *)&node->dynamic_address_block;

    for ( i=0; i<address_list->num; i++ ) {

        if ( AF_INET != address_list->array[i].type ) {
            continue;
        }

        if ( 0 == address_list->array[i].port ) {
            continue;
        }

        //如果存在多个地址，选择更新时间是最新的
        if ( NULL != address ) {

            if ( address_list->array[i].ts_sec > address->ts_sec ) {
                continue;
            }

        }

        address = &address_list->array[i];

    }

    if ( NULL != address ) {
        goto finish;
    }

    address_list = (gnb_address_list_t *)&node->resolv_address_block;

    for ( i=0; i<address_list->num; i++ ) {

        if ( AF_INET != address_list->array[i].type ) {
            continue;
        }

        if ( 0 == address_list->array[i].port ) {
            continue;
        }

        //如果存在多个地址，选择更新时间是最新的
        if ( NULL != address ) {

            if ( address_list->array[i].ts_sec > address->ts_sec ) {
                continue;
            }

        }

        address = &address_list->array[i];

    }

    if ( NULL != address ) {
        goto finish;
    }

    address_list = (gnb_address_list_t *)&node->static_address_block;

    for ( i=0; i<address_list->num; i++ ) {

        if ( AF_INET != address_list->array[i].type ) {
            continue;
        }

        if ( 0 == address_list->array[i].port ) {
            continue;
        }

        address = &address_list->array[i];

        break;

    }

finish:

    return address;

}


int gnb_send_to_node(gnb_core_t *gnb_core, gnb_node_t *node, gnb_payload16_t *payload, unsigned char addr_type_bits){

    if ( GNB_ADDR_TYPE_IPV6 == gnb_core->conf->udp_socket_type ) {
        goto send_by_ipv6;
    }

    int i;

    if ( (GNB_ADDR_TYPE_IPV4 & addr_type_bits) && (gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV4) && INADDR_ANY != node->udp_sockaddr4.sin_addr.s_addr ) {

        for (i=0; i<gnb_core->conf->udp4_socket_num; i++) {
            sendto(gnb_core->udp_ipv4_sockets[ i ], (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&node->udp_sockaddr4, sizeof(struct sockaddr_in));
        }

    }

    if ( GNB_ADDR_TYPE_IPV4 == gnb_core->conf->udp_socket_type ) {
        goto finish;
    }

send_by_ipv6:

    if ( (GNB_ADDR_TYPE_IPV6 & addr_type_bits) && (gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6) > 0 && memcmp(&node->udp_sockaddr6.sin6_addr,&in6addr_any,sizeof(struct in6_addr)) ) {

        for (i=0; i<gnb_core->conf->udp6_socket_num; i++) {
            sendto(gnb_core->udp_ipv6_sockets[i],(void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&node->udp_sockaddr6, sizeof(struct sockaddr_in6) );
        }

    }

finish:

    return 0;

}


int gnb_forward_payload_to_node(gnb_core_t *gnb_core, gnb_node_t *node, gnb_payload16_t *payload){

    int ret;

    if ( GNB_ADDR_TYPE_IPV4 == gnb_core->conf->udp_socket_type ) {
        goto send_by_ipv4;
    } else if ( GNB_ADDR_TYPE_IPV6 == gnb_core->conf->udp_socket_type ) {
        goto send_by_ipv6;
    }

    if ( (node->udp_addr_status & GNB_NODE_STATUS_IPV6_PONG) && (node->udp_addr_status & GNB_NODE_STATUS_IPV4_PONG) ) {

        if ( 0 == node->addr4_ping_latency_usec ) {
            goto send_by_ipv6;
        }

        if ( 0 == node->addr6_ping_latency_usec ) {
            goto send_by_ipv4;
        }

        if ( node->addr4_ping_latency_usec >= node->addr6_ping_latency_usec ) {
            goto send_by_ipv6;
        } else {
            goto send_by_ipv4;
        }

    }

send_by_ipv6:

    if ( (node->udp_addr_status & GNB_NODE_STATUS_IPV6_PONG) && (gnb_core->conf->udp_socket_type & GNB_ADDR_TYPE_IPV6) && memcmp(&node->udp_sockaddr6.sin6_addr,&in6addr_any,sizeof(struct in6_addr)) ) {

        sendto(gnb_core->udp_ipv6_sockets[node->socket6_idx],(void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&node->udp_sockaddr6, sizeof(struct sockaddr_in6) );

        goto finish;

    }

send_by_ipv4:

    ret = sendto(gnb_core->udp_ipv4_sockets[ node->socket4_idx ], (void *)payload, GNB_PAYLOAD16_FRAME_SIZE(payload), 0, (struct sockaddr *)&node->udp_sockaddr4, sizeof(struct sockaddr_in));

finish:

    return 0;

}
