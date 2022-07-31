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
#include <inttypes.h>

#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_address.h"
#include "gnb_binary.h"
#include "gnb_time.h"
#include "gnb_udp.h"
#include "gnb_ctl_block.h"

#include "ed25519/sha512.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif



void gnb_ctl_dump_status(gnb_ctl_block_t *ctl_block, int reachabl_opt){

    #define LINE_SIZE 1024
    char line_string[LINE_SIZE];
    char *p;
    int line_string_len;
    int wlen;

    gnb_conf_t *conf = NULL;

    gnb_address_t *gnb_address;

    char time_string[128];

    char shared_secret_sha512[64];

    conf = &ctl_block->conf_zone->conf_st;

    printf("conf->conf_dir[%s]\n",conf->conf_dir);

    gnb_address_list_t *static_address_list;
    gnb_address_list_t *dynamic_address_list;
    gnb_address_list_t *resolv_address_list;
    gnb_address_list_t *push_address_list;

    gnb_node_t *node;

    int node_num;

    node_num = ctl_block->node_zone->node_num;

    printf("node_num[%d]\n",node_num);

    printf("wan_port6[%d]\n", ntohs(ctl_block->core_zone->wan_port6) );

    int i,j;

    for ( i=0; i<node_num; i++ ) {

        node = &ctl_block->node_zone->node[i];

        if ( 0 != reachabl_opt && !((GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & node->udp_addr_status) && node->uuid32 != ctl_block->core_zone->local_uuid ) {
            continue;
        }

        static_address_list  = (gnb_address_list_t *)&node->static_address_block;
        dynamic_address_list = (gnb_address_list_t *)&node->dynamic_address_block;
        resolv_address_list  = (gnb_address_list_t *)&node->resolv_address_block;
        push_address_list    = (gnb_address_list_t *)&node->push_address_block;


        printf("\n====================\n");

        printf("node %u\n",node->uuid32);

        printf("addr4_ping_latency_usec %"PRIu64"\n",node->addr4_ping_latency_usec);

        printf("tun_ipv4 %s\n",GNB_ADDR4STR1(&node->tun_addr4));

        printf("tun_ipv6 %s\n",GNB_ADDR6STR1(&node->tun_ipv6_addr));

        printf("public_key %s\n",GNB_HEX1_BYTE64(node->public_key));

        sha512((const unsigned char *)(node->shared_secret), 32, (unsigned char *)shared_secret_sha512);

        printf("shared_secret_sha512 %s\n",GNB_HEX1_BYTE128(shared_secret_sha512));

        printf("crypto_key %s\n",GNB_HEX1_BYTE128(node->crypto_key));

        printf("key512 %s\n",GNB_HEX1_BYTE64(node->key512));


        if ( node->uuid32 != ctl_block->core_zone->local_uuid ) {

            if ( (node->udp_addr_status & GNB_NODE_STATUS_IPV6_PONG) ) {
                printf("ipv6 REACHABL\n");
            } else {
                printf("ipv6 UNREACHABL\n");
            }

            if ( (node->udp_addr_status & GNB_NODE_STATUS_IPV4_PONG) ) {
                printf("ipv4 REACHABL\n");
            } else {
                printf("ipv4 UNREACHABL\n");
            }

        }else {
            printf("ipv6 Local node\n");
            printf("ipv4 Local node\n");
        }

        gnb_timef("%Y-%m-%d %H:%M:%S", (time_t)node->ping_ts_sec, time_string, 128);
        printf("ping_ts_sec:%"PRIu64"(%s)\n", node->ping_ts_sec, time_string);

        printf("addr6_ping_latency_usec:%"PRIu64"\n", node->addr6_ping_latency_usec);
        printf("addr4_ping_latency_usec:%"PRIu64"\n", node->addr4_ping_latency_usec);

        printf("detect_count %d\n", node->detect_count);

        printf("wan_ipv4 %s\n", GNB_SOCKADDR4STR1(&node->udp_sockaddr4));
        printf("wan_ipv6 %s\n", GNB_SOCKADDR6STR1(&node->udp_sockaddr6));

        printf("static_address_list num[%lu]:\n",static_address_list->num);

        for( j=0; j<static_address_list->num; j++ ){
            gnb_address = &static_address_list->array[j];
            if (0==gnb_address->port) {
                continue;
            }
            gnb_timef("%Y-%m-%d %H:%M:%S", (time_t)gnb_address->ts_sec, time_string, 128);
            printf("address %s ts_sec[%"PRIu64"](%s) delay_usec[%"PRIu64"]\n", GNB_IP_PORT_STR1(gnb_address), gnb_address->ts_sec, time_string, gnb_address->delay_usec);
        }

        printf("dynamic_address_list num[%lu]:\n", dynamic_address_list->num);

        for( j=0; j<dynamic_address_list->num; j++ ){
            gnb_address = &dynamic_address_list->array[j];
            if (0==gnb_address->port) {
                continue;
            }
            gnb_timef("%Y-%m-%d %H:%M:%S", (time_t)gnb_address->ts_sec, time_string, 128);
            printf("address idx=%u %s ts_sec[%"PRIu64"](%s) delay_usec[%"PRIu64"]\n", gnb_address->socket_idx, GNB_IP_PORT_STR1(gnb_address), gnb_address->ts_sec, time_string, gnb_address->delay_usec);
        }

        printf("resolv_address_list num[%lu]:\n",resolv_address_list->num);

        for( j=0; j<resolv_address_list->num; j++ ){
            gnb_address = &resolv_address_list->array[j];
            if (0==gnb_address->port) {
                continue;
            }
            gnb_timef("%Y-%m-%d %H:%M:%S", (time_t)gnb_address->ts_sec, time_string, 128);
            printf("address %s ts_sec[%"PRIu64"](%s) delay_usec[%"PRIu64"]\n", GNB_IP_PORT_STR1(gnb_address), gnb_address->ts_sec, time_string, gnb_address->delay_usec);
        }

        printf("push_address_list num[%lu]:\n",push_address_list->num);

        for( j=0; j<push_address_list->num; j++ ){
            gnb_address = &push_address_list->array[j];
            if (0==gnb_address->port) {
                continue;
            }
            gnb_timef("%Y-%m-%d %H:%M:%S", (time_t)gnb_address->ts_sec, time_string, 128);
            printf("address idx=%u %s ts_sec[%"PRIu64"](%s) delay_usec[%"PRIu64"]\n", gnb_address->socket_idx, GNB_IP_PORT_STR1(gnb_address), gnb_address->ts_sec, time_string, gnb_address->delay_usec);
        }

        p = line_string;
        wlen = 0;
        line_string_len = LINE_SIZE;

        for ( j=0; j<GNB_UNIFIED_FORWARDING_NODE_ARRAY_SIZE; j++ ) {

            wlen = snprintf(p, line_string_len-wlen,  "%u,", node->unified_forwarding_node_array[j].uuid32);

            line_string_len -= wlen;

            if ( line_string_len <= 16 ) {
                break;
            }

            p += wlen;

        }

        printf("unified node:  %s\n", line_string);

        p = line_string;
        wlen = 0;
        line_string_len = LINE_SIZE;

        for ( j=0; j<UNIFIED_FORWARDING_RECV_SEQ_ARRAY_SIZE; j++ ) {

            wlen = snprintf(p, line_string_len-wlen, "%"PRIu64",", node->unified_forwarding_recv_seq_array[j]);

            line_string_len -= wlen;

            if ( line_string_len <= 16 ) {
                break;
            }


        }

        printf("unified_forwarding_recv_seq_array:  %s\n", line_string);


    }

}


void gnb_ctl_dump_address_list(gnb_ctl_block_t *ctl_block,int reachabl_opt){

    gnb_address_t *gnb_address;

    gnb_address_list_t *static_address_list;
    gnb_address_list_t *dynamic_address_list;
    gnb_address_list_t *resolv_address_list;
    gnb_address_list_t *push_address_list;

    gnb_node_t *node;

    int node_num;

    node_num = ctl_block->node_zone->node_num;

    int i,j;

    for( i=0; i<node_num; i++ ){

        node = &ctl_block->node_zone->node[i];

        if (node->uuid32 == ctl_block->core_zone->local_uuid){
            printf( "l|%u|%s\n", node->uuid32, GNB_SOCKADDR6STR1(&node->udp_sockaddr6) );
            printf( "l|%u|%s\n", node->uuid32, GNB_SOCKADDR4STR1(&node->udp_sockaddr4) );
            continue;
        }

        if ( 0 != reachabl_opt && !((GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & node->udp_addr_status) ){
            continue;
        }

        if ( 0 != reachabl_opt ){

            if ( GNB_NODE_STATUS_IPV6_PONG & node->udp_addr_status  ){
                printf( "w|%u|%s\n", node->uuid32, GNB_SOCKADDR6STR1(&node->udp_sockaddr6) );
            }

            if ( GNB_NODE_STATUS_IPV4_PONG & node->udp_addr_status  ){
                printf( "w|%u|%s\n", node->uuid32, GNB_SOCKADDR4STR1(&node->udp_sockaddr4) );
            }

            continue;

        }

        static_address_list  = (gnb_address_list_t *)&node->static_address_block;
        dynamic_address_list = (gnb_address_list_t *)&node->dynamic_address_block;
        resolv_address_list  = (gnb_address_list_t *)&node->resolv_address_block;
        push_address_list    = (gnb_address_list_t *)&node->push_address_block;

        for( j=0; j<static_address_list->size; j++ ){
            gnb_address = &static_address_list->array[j];
            if (0==gnb_address->port){
                continue;
            }

            if ( AF_INET6 == gnb_address->type ){
                printf( "s|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR6STR1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else if( AF_INET == gnb_address->type ){
                printf( "s|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR4STR1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else{
                continue;
            }

        }

        for( j=0; j<dynamic_address_list->size; j++ ){

            gnb_address = &dynamic_address_list->array[j];
            if (0==gnb_address->port){
                continue;
            }

            if ( AF_INET6 == gnb_address->type ){
                printf( "d|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR6STR1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else if( AF_INET == gnb_address->type ){
                printf( "d|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR4STR1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else{
                continue;
            }

        }


        for( j=0; j<resolv_address_list->size; j++ ){

            gnb_address = &resolv_address_list->array[j];
            if (0==gnb_address->port){
                continue;
            }

            if ( AF_INET6 == gnb_address->type ){
                printf( "r|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR6STR1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else if( AF_INET == gnb_address->type ){
                printf( "r|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR4STR1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else{
                continue;
            }

        }


        for( j=0; j<push_address_list->size; j++ ){

            gnb_address = &push_address_list->array[j];
            if (0==gnb_address->port){
                continue;
            }

            if ( AF_INET6 == gnb_address->type ){
                printf( "p|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR6STR1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else if( AF_INET == gnb_address->type ){
                printf( "p|%u|%s|%d|%s\n", node->uuid32, GNB_ADDR4STR1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            }else{
                continue;
            }

        }

    }

}


void gnb_ctl_dump_node_wan_address(gnb_ctl_block_t *ctl_block){

    gnb_node_t *node;

    char node_type_string[16];
    int type_idx;

    int node_num;

    node_num = ctl_block->node_zone->node_num;

    int i;

    for( i=0; i<node_num; i++ ){

        node = &ctl_block->node_zone->node[i];

        if (node->uuid32 == ctl_block->core_zone->local_uuid){
            continue;
        }

        if ( !((GNB_NODE_STATUS_IPV6_PONG | GNB_NODE_STATUS_IPV4_PONG) & node->udp_addr_status) ){
            continue;
        }

        memset(node_type_string, 0, 16);
        type_idx=0;
        node_type_string[type_idx] = 'n';
        type_idx++;

        if ( GNB_NODE_TYPE_IDX & node->type ) {
            node_type_string[type_idx] = 'i';
            type_idx++;
        }

        if ( GNB_NODE_TYPE_FWD & node->type ) {
            node_type_string[type_idx] = 'f';
            type_idx++;
        }

        if ( GNB_NODE_TYPE_RELAY & node->type ) {
            node_type_string[type_idx] = 'r';
            type_idx++;
        }

        if ( GNB_NODE_TYPE_SLIENCE & node->type ) {
            node_type_string[type_idx] = 'S';
            type_idx++;
        }

        if ( GNB_NODE_TYPE_STATIC_ADDR & node->type ) {
            node_type_string[type_idx] = 's';
            type_idx++;
        }

        if ( GNB_NODE_STATUS_IPV6_PONG & node->udp_addr_status ) {
            printf( "%s|%u|%s\n", node_type_string, node->uuid32, GNB_SOCKADDR6STR1(&node->udp_sockaddr6) );
        }

        if ( GNB_NODE_STATUS_IPV4_PONG & node->udp_addr_status ) {
            printf( "%s|%u|%s\n", node_type_string, node->uuid32, GNB_SOCKADDR4STR1(&node->udp_sockaddr4) );
        }

    }

}

