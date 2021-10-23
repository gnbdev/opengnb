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

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "ed25519/ed25519.h"
#include "ed25519/sha512.h"

#include "gnb_node.h"
#include "gnb_keys.h"
#include "gnb_arg_list.h"

extern  gnb_arg_list_t *gnb_es_arg_list;

char * check_domain_name(char *host_string){

    if ( NULL != strchr(host_string, ':') ){
        return NULL;
    }

    int i;

    for( i=0; i<NAME_MAX; i++ ){

        if( '\0' == host_string[i] ){
            return NULL;
        }

        if ( '.' == host_string[i] ){
            continue;
        }

        if ( host_string[i] >= 'a' && host_string[i] <= 'z' ){
            return host_string;
        }

    }

    return NULL;
}

/*判断 配置行第二列是ip地址 还是node id*/
char * check_node_route(char *config_line_string){

    if ( NULL != strchr(config_line_string, ':')  || NULL != strchr(config_line_string, '.') ){
        return NULL;
    }

    return config_line_string;

}


int gnb_test_field_separator(char *config_string){

    int i;

    for( i=0; i<strlen(config_string); i++  ){

        if ( '/' == config_string[i] ){
            return GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH;
        } else if( '|' == config_string[i] ){
            return GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL;
        }

    }

    return GNB_CONF_FIELD_SEPARATOR_TYPE_ERROR;

}



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

    if ( 0 == gnb_core->conf->lite_mode ){

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

        }else{

            memcpy(node->shared_secret+4, &local_node_id_network_order, 4);
            memcpy(node->shared_secret+8, &node_id_network_order, 4);

        }

    }

    return node;

}

/*
return value:
0    port
4    ipv4:port
6    [ipv6:port]
*/
int check_listen_string(char *listen_string){

    if ( '[' == listen_string[0] ){
        return 6;
    }

    int i;

    for ( i=0; i<strlen(listen_string); i++ ) {
        if ( ':' == listen_string[i] ){
            return 4;
        }
    }

    return 0;

}


void gnb_setup_listen_addr_port(char *listen_address_string, uint16_t *port_ptr, char *sockaddress_string, int addr_type){

    int i;

    char *host_string_end;

    char *p;

    int sockaddress_string_len;

    sockaddress_string_len = strlen(sockaddress_string);

    char *pb;

    if ( AF_INET6 == addr_type ){

        if ( '[' != sockaddress_string[0] ){

            return;
        }

        host_string_end = sockaddress_string+1;
        pb = sockaddress_string+1;
        sockaddress_string_len -= 1;
    } else {

        host_string_end = sockaddress_string;
        pb = sockaddress_string;

    }

    p = pb;

    for ( i=0; i<sockaddress_string_len; i++ ){

        if ( ':' == *p ){
            host_string_end = p;
        }

        p++;

    }

    p = pb;

    for ( i=0; i<sockaddress_string_len; i++ ){

        if( p == host_string_end ){
            break;
        }

        listen_address_string[i] = *p;

        p++;

    }

    listen_address_string[i] = '\0';

    p++;

    *port_ptr = strtoul(p, NULL, 10);

    return;

}


void gnb_setup_es_argv(char *es_argv_string){

    int num;
    size_t len;

    char argv_string0[1024];
    char argv_string1[1024];

    len = strlen(es_argv_string);

    if ( len >1024 ){
        return;
    }

    num = sscanf(es_argv_string,"%256[^ ] %256s", argv_string0, argv_string1);

    if ( 1 == num ){
        gnb_arg_append(gnb_es_arg_list, argv_string0);
    }else if( 2 == num ){
        gnb_arg_append(gnb_es_arg_list, argv_string0);
        gnb_arg_append(gnb_es_arg_list, argv_string1);
    }

    return;

}

