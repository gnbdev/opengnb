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

#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_ctl_block.h"
#include "gnb_es_type.h"

#include "gnb_address.h"
#include "gnb_binary.h"


void gnb_es_dump_address_list(gnb_es_ctx *es_ctx){

    gnb_address_t *gnb_address;

    gnb_address_list_t *static_address_list;
    gnb_address_list_t *dynamic_address_list;
    gnb_address_list_t *resolv_address_list;
    gnb_address_list_t *push_address_list;

    gnb_node_t *node;

    int node_num;

    gnb_conf_t *conf;

    conf = &es_ctx->ctl_block->conf_zone->conf_st;

    if ( '\0' == conf->node_cache_file[0] ) {
        return;
    }

    node_num = es_ctx->ctl_block->node_zone->node_num;

    if ( 0==node_num ) {
        return;
    }

    FILE *file;

    file = fopen(conf->node_cache_file,"wb");

    if ( NULL == file ) {
        return;
    }

    int i,j;

    for( i=0; i<node_num; i++ ) {

        node = &es_ctx->ctl_block->node_zone->node[i];

        static_address_list = (gnb_address_list_t *)&node->static_address_block;
        dynamic_address_list = (gnb_address_list_t *)&node->dynamic_address_block;
        resolv_address_list = (gnb_address_list_t *)&node->resolv_address_block;
        push_address_list = (gnb_address_list_t *)&node->push_address_block;

        for ( j=0; j<static_address_list->size; j++ ) {

            gnb_address = &static_address_list->array[j];
            if (0==gnb_address->port) {
                continue;
            }

            if ( AF_INET6 == gnb_address->type ) {
                fprintf( file, "s|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR6STR_PLAINTEXT1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else if ( AF_INET == gnb_address->type ) {
                fprintf( file, "s|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR4STR_PLAINTEXT1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else {
                continue;
            }

        }

        for ( j=0; j<dynamic_address_list->size; j++ ) {

            gnb_address = &dynamic_address_list->array[j];
            if (0==gnb_address->port) {
                continue;
            }

            if ( AF_INET6 == gnb_address->type ) {
                fprintf( file, "d|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR6STR_PLAINTEXT1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else if ( AF_INET == gnb_address->type ) {
                fprintf( file, "d|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR4STR_PLAINTEXT1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else {
                continue;
            }

        }

        for ( j=0; j<resolv_address_list->size; j++ ) {

            gnb_address = &resolv_address_list->array[j];

            if (0==gnb_address->port) {
                continue;
            }

            if ( AF_INET6 == gnb_address->type ) {
                fprintf( file, "r|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR6STR_PLAINTEXT1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else if ( AF_INET == gnb_address->type ) {
                fprintf( file, "r|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR4STR_PLAINTEXT1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else {
                continue;
            }

        }

        for ( j=0; j<push_address_list->size; j++ ) {

            gnb_address = &push_address_list->array[j];

            if (0==gnb_address->port) {
                continue;
            }

            if ( AF_INET6 == gnb_address->type ) {
                fprintf( file, "p|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR6STR_PLAINTEXT1(&gnb_address->address.addr6), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else if ( AF_INET == gnb_address->type ) {
                fprintf( file, "p|%llu|%s|%d|%s\n", node->uuid64, GNB_ADDR4STR_PLAINTEXT1(&gnb_address->address.addr4), ntohs(gnb_address->port), GNB_HEX1_BYTE128(node->key512) );
            } else {
                continue;
            }

        }

    }

    fclose(file);

}
