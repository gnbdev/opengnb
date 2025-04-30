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

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
#define __UNIX_LIKE_OS__ 1
#endif

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <stddef.h>

#ifdef __UNIX_LIKE_OS__
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if defined(_WIN32)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "gnb_node_type.h"
#include "gnb_address.h"
#include "gnb_es_type.h"

int gnb_test_field_separator(char *config_string);

static char * check_domain_name(char *host_string){

    if ( NULL != strchr(host_string, ':') ){
        return NULL;
    }

    int i;

    for ( i=0; i<NAME_MAX; i++ ) {

        if ( '\0' == host_string[i] ) {
            return NULL;
        }

        if ( '.' == host_string[i] ) {
            continue;
        }

        if ( host_string[i] >= 'a' && host_string[i] <= 'z' ) {
            return host_string;
        }

    }

    return NULL;

}


static void gnb_do_resolv_node_address(gnb_node_t *node, char *host_string, uint16_t port, gnb_log_ctx_t *log){

    int ret;

    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    struct addrinfo *result;
    struct addrinfo *cur;

    ret = getaddrinfo(host_string, NULL, &hints, &result);

    if ( -1 == ret ) {
        return;
    }

    gnb_address_list_t *resolv_address_list;

    gnb_address_t address_st;

    resolv_address_list = (gnb_address_list_t *)&node->resolv_address_block;

    for ( cur=result; NULL!=cur; cur=cur->ai_next ) {

        memset(&address_st, 0, sizeof(gnb_address_t));
        address_st.port = htons(port);

        switch(cur->ai_addr->sa_family) {

            case AF_INET6:

                address_st.type = AF_INET6;

                memcpy(&address_st.m_address6, &(((struct sockaddr_in6 *)(cur->ai_addr))->sin6_addr), 16);

                gnb_address_list_update(resolv_address_list, &address_st);

                break;

            case AF_INET:

                address_st.type = AF_INET;

                memcpy(&address_st.m_address4, &(((struct sockaddr_in *)(cur->ai_addr))->sin_addr), 4);

                gnb_address_list_update(resolv_address_list, &address_st);

                break;

            default:
                break;

        }

    }

    freeaddrinfo(result);

    gnb_address_t *gnb_address;

    int j;

    for( j=0; j<resolv_address_list->size; j++ ) {

        gnb_address = &resolv_address_list->array[j];

        if (0==gnb_address->port) {
            continue;
        }

        GNB_LOG1(log, GNB_LOG_ID_ES_RESOLV, "resolv [%s]>[%s]\n", host_string, GNB_IP_PORT_STR1(gnb_address));

    }

}


void gnb_resolv_address(gnb_es_ctx *es_ctx){

    gnb_log_ctx_t *log = es_ctx->log;

    char *conf_dir = es_ctx->ctl_block->conf_zone->conf_st.conf_dir;

    char address_file[PATH_MAX+NAME_MAX];

    snprintf(address_file, PATH_MAX+NAME_MAX, "%s/%s", conf_dir, "address.conf");

    FILE *file;

    file = fopen(address_file,"r");

    if ( NULL==file ) {
        return;
    }

    char line_buffer[1024+1];

    char attrib_string[16+1];

    gnb_uuid_t uuid64;

    char host_string[256+1];

    uint16_t port = 0;

    gnb_node_t *node;

    int num;

    int ret;

    do{

        num = fscanf(file,"%1024s\n",line_buffer);

        if ( EOF == num ) {
            break;
        }

        if ('#' == line_buffer[0]) {
            continue;
        }

        ret = gnb_test_field_separator(line_buffer);

        if ( GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH == ret ) {
            num = sscanf(line_buffer,"%16[^/]/%llu/%256[^/]/%hu\n", attrib_string, &uuid64, host_string, &port);
        } else if ( GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL == ret ) {
            num = sscanf(line_buffer,"%16[^|]|%llu|%256[^|]|%hu\n", attrib_string, &uuid64, host_string, &port);
        } else {
            num = 0;
        }

        if ( 4 != num ) {
            continue;
        }

        if ( 0 == port ) {
            continue;
        }

        if ( NULL == check_domain_name(host_string) ) {
            continue;
        }

        node = (gnb_node_t *)GNB_HASH32_UINT64_GET_PTR(es_ctx->uuid_node_map, uuid64);

        if ( NULL == node ) {
            continue;
        }

        gnb_do_resolv_node_address(node, host_string, port,log);


    }while(1);

    fclose(file);

}

/*
dig -6 TXT +short o-o.myaddr.l.google.com @ns1.google.com | awk -F'"' '{ print $2}'
*/
void gnb_load_wan_ipv6_address(gnb_es_ctx *es_ctx){

    gnb_log_ctx_t *log = es_ctx->log;

    if ( NULL == es_ctx->wan_address6_file ) {
        return;
    }

    FILE *file;

    file = fopen(es_ctx->wan_address6_file,"r");

    if ( NULL == file ) {
        return;
    }

    gnb_conf_t *conf = &es_ctx->ctl_block->conf_zone->conf_st;

    char host_string[46+1];

    int num;

    num = fscanf(file,"%46s\n",host_string);

    int s;

    s = inet_pton(AF_INET6, host_string, (struct in_addr *)&es_ctx->ctl_block->core_zone->wan6_addr);

    if (s <= 0) {
        memset(&es_ctx->ctl_block->core_zone->wan6_addr,0,16);
        es_ctx->ctl_block->core_zone->wan6_port = 0;
    } else {
        es_ctx->ctl_block->core_zone->wan6_port = htons(conf->udp6_ports[0]);
        GNB_LOG1(log, GNB_LOG_ID_ES_RESOLV, "load wan address6[%s:%d]\n", host_string, conf->udp6_ports[0]);
    }

}

