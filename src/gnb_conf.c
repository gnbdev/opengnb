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

#include "gnb_conf_type.h"
#include "gnb.h"


char * check_domain_name(char *host_string){

    if ( NULL != strchr(host_string, ':') ) {
        return NULL;
    }

    int i;

    for( i=0; i<NAME_MAX; i++ ) {

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

/*判断 配置行第二列是ip地址 还是node id*/
char * check_node_route(char *config_line_string){

    if ( NULL != strchr(config_line_string, ':')  || NULL != strchr(config_line_string, '.') ) {
        return NULL;
    }

    return config_line_string;

}


int gnb_test_field_separator(char *config_string){

    int i;

    for ( i=0; i<strlen(config_string); i++  ) {

        if ( '/' == config_string[i] ) {
            return GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH;
        } else if ( '|' == config_string[i] ) {
            return GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL;
        }

    }

    return GNB_CONF_FIELD_SEPARATOR_TYPE_ERROR;

}


/*
return value:
0    port
4    ipv4:port
6    [ipv6:port]
*/
int check_listen_string(char *listen_string){

    if ( '[' == listen_string[0] ) {
        return 6;
    }

    int i;

    for ( i=0; i<strlen(listen_string); i++ ) {

        if ( ':' == listen_string[i] ) {
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

    if ( AF_INET6 == addr_type ) {

        if ( '[' != sockaddress_string[0] ) {
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

    for ( i=0; i<sockaddress_string_len; i++ ) {

        if ( ':' == *p ) {
            host_string_end = p;
        }

        p++;

    }

    p = pb;

    for ( i=0; i<sockaddress_string_len; i++ ) {

        if( p == host_string_end ) {
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
