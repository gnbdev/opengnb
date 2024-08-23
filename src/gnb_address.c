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
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "gnb_platform.h"

#ifdef __UNIX_LIKE_OS__
#include <sys/socket.h>
#include <arpa/inet.h>
#define __USE_MISC 1
#include <netinet/in.h>

#endif

#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>

//#define __USE_MISC 1
//#include <in.h>

#define in_addr_t uint32_t

#define _POSIX
#define __USE_MINGW_ALARM
#endif


#include "gnb_address.h"

uint8_t gnb_addr_secure = 0;

unsigned long long gnb_htonll(unsigned long long val){

#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
      uint64_t rv= 0;
      uint8_t x= 0;
      for (x= 0; x < 8; x++) {
        rv= (rv << 8) | (val & 0xff);
        val >>= 8;
      }
      return rv;
#endif

#if __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
        return val;
#endif

}


unsigned long long gnb_ntohll(unsigned long long val){

#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
      uint64_t rv= 0;
      uint8_t x= 0;
      for (x= 0; x < 8; x++) {
        rv= (rv << 8) | (val & 0xff);
        val >>= 8;
      }
      return rv;
#endif

#if __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
    return val;
#endif

}


char get_netmask_class(uint32_t addr4){

    if (  !(~(IN_CLASSA_NET) &  ntohl(addr4))  ) {
        return 'a';
    }

    if (  !(~(IN_CLASSB_NET) &  ntohl(addr4))  ) {
        return 'b';
    }

    if (  !(~(IN_CLASSC_NET) &  ntohl(addr4))  ) {
        return 'c';
    }

    return 'n';

}


gnb_address_list_t* gnb_create_address_list(size_t size){

    gnb_address_list_t *address_list;

    address_list = (gnb_address_list_t *)malloc( sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*size );

    memset(address_list, 0, sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*size);

    address_list->size = size;
    address_list->num = 0;

    return address_list;

}
void gnb_address_list_release(gnb_address_list_t *address_list){
    free(address_list);
}


int gnb_address_list_find(gnb_address_list_t *address_list, gnb_address_t *address) {

    int i;

    for ( i=0; i < address_list->num; i++ ) {

        if ( address->type != address_list->array[i].type ) {
            continue;
        }

        if ( address->port != address_list->array[i].port ) {
            continue;
        }

        if ( AF_INET == address->type && 0 == memcmp(address_list->array[i].m_address4, address->m_address4, 4) ) {
            return i;
        }

        if ( AF_INET6 == address->type && 0 == memcmp(address_list->array[i].m_address6, address->m_address6, 16)) {
            return i;
        }

    }

    return -1;
}


int gnb_address_list_get_free_idx(gnb_address_list_t *address_list) {

    int free_idx = 0;

    uint64_t min_ts_sec = address_list->array[free_idx].ts_sec;

    int i;

    for ( i=0; i < address_list->size; i++ ) {

        if ( 0 == address_list->array[i].port ) {
            free_idx = i;
            break;
        }

        if ( address_list->array[i].ts_sec <= min_ts_sec ) {
            min_ts_sec = address_list->array[i].ts_sec;
            free_idx = i;
        }

    }

    return free_idx;

}


void gnb_address_list_update(gnb_address_list_t *address_list, gnb_address_t *address){

    int idx = 0;

    idx = gnb_address_list_find(address_list, address);

    if ( -1 != idx ) {
        goto finish;
    }

    idx = gnb_address_list_get_free_idx(address_list);

finish:

    if ( 0 == address_list->array[idx].port ) {

        if ( address_list->num < address_list->size ) {
            address_list->num += 1;
        } else if (address_list->num > address_list->size) {
            address_list->num = address_list->size;
        }

    }

    memcpy( &address_list->array[idx], address, sizeof(gnb_address_t) );

}


void gnb_set_address4(gnb_address_t *address, struct sockaddr_in *in){

    address->type = AF_INET;
    address->port = in->sin_port;
    memcpy(&address->address.addr4, &in->sin_addr.s_addr ,sizeof(struct in_addr));

}


void gnb_set_address6(gnb_address_t *address, struct sockaddr_in6 *in6){

    address->type = AF_INET6;
    address->port = in6->sin6_port;
    memcpy(&address->address.addr6, &in6->sin6_addr ,sizeof(struct in6_addr));

}


char * gnb_get_address4string(void *byte4, char *dest, uint8_t addr_secure){

    inet_ntop(AF_INET, byte4, dest, INET_ADDRSTRLEN);

    char *p;

    if (addr_secure) {

        p = dest;

        while( '\0' != *p ) {

            if ( '.' == *p ) {
                break;
            }

            *p = '*';

            p++;

        }

    }

    return dest;
}


char * gnb_get_address6string(void *byte16, char *dest, uint8_t addr_secure){

    inet_ntop(AF_INET6, byte16, dest, INET6_ADDRSTRLEN);

    char *p;

    if (addr_secure) {

        p = dest;

        while ( '\0' != *p ) {

            if ( ':' == *p ) {
                break;
            }

            *p = '*';

            p++;

        }

    }

    return dest;
}


char * gnb_get_socket4string(struct sockaddr_in *in, char *dest, uint8_t addr_secure){

    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &in->sin_addr, buf, INET_ADDRSTRLEN);
    snprintf(dest, GNB_IP4_PORT_STRING_SIZE,"%s:%d",buf,ntohs(in->sin_port));

    char *p;

    if (addr_secure) {

        p = dest;

        while ( '\0' != *p ) {

            if ( '.' == *p ) {
                break;
            }

            *p = '*';

            p++;

        }

    }

    return dest;
}


char * gnb_get_socket6string(struct sockaddr_in6 *in6, char *dest, uint8_t addr_secure){

    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &in6->sin6_addr, buf, INET6_ADDRSTRLEN);
    snprintf(dest, GNB_IP6_PORT_STRING_SIZE,"[%s:%d]",buf,ntohs(in6->sin6_port));

    char *p;

    if ( addr_secure ) {

        p = dest+1;

        while ( '\0' != *p ) {

            if ( ':' == *p ) {
                break;
            }

            *p = '*';

            p++;

        }

    }

    return dest;
}


char * gnb_get_sockaddress_string(gnb_sockaddress_t *sockaddress, char *dest, uint8_t addr_secure){

    if ( AF_INET6 == sockaddress->addr_type ) {
        dest = gnb_get_socket6string(&sockaddress->addr.in6,dest,addr_secure);
    } else if ( AF_INET == sockaddress->addr_type ) {
        dest = gnb_get_socket4string(&sockaddress->addr.in,dest,addr_secure);
    }

    return dest;

}


char * gnb_get_ip_port_string(gnb_address_t *address, char *dest, uint8_t addr_secure){

    char buf[INET6_ADDRSTRLEN];

    char *p;

    p = dest;

    if ( AF_INET6 == address->type ) {
        inet_ntop(AF_INET6, &address->address.addr6, buf, INET6_ADDRSTRLEN);
        snprintf(dest, GNB_IP6_PORT_STRING_SIZE,"[%s:%d]",buf,ntohs(address->port));
        p++;
    } else if ( AF_INET == address->type ) {
        inet_ntop(AF_INET, &address->address.addr4, buf, INET_ADDRSTRLEN);
        snprintf(dest, GNB_IP6_PORT_STRING_SIZE,"%s:%d",buf,ntohs(address->port));
    } else {
        snprintf(dest, GNB_IP6_PORT_STRING_SIZE,"NONE_ADDRESS");
    }

    if ( addr_secure ) {

        while ( '\0' != *p ) {

            if ( '.' == *p || ':' == *p ) {
                break;
            }

            *p = '*';

            p++;

        }

    }

    return dest;
}


int gnb_cmp_sockaddr_in6(struct sockaddr_in6 *in1, struct sockaddr_in6 *in2){

    if ( in1->sin6_port != in2->sin6_port ) {
        return 1;
    }

    if ( 0 != memcmp( &in1->sin6_addr, &in2->sin6_addr, 16) ) {
        return 2;
    }

    return 0;

}


int gnb_cmp_sockaddr_in(struct sockaddr_in *in1, struct sockaddr_in *in2){

    if ( in1->sin_port != in2->sin_port ) {
        return 1;
    }

    if ( in1->sin_addr.s_addr != in2->sin_addr.s_addr ) {
        return 2;
    }

    return 0;

}


void gnb_set_sockaddress4(gnb_sockaddress_t *sockaddress, int protocol, const char *host, int port){

    sockaddress->addr_type = AF_INET;

    if ( GNB_PROTOCOL_TCP == protocol ) {
        sockaddress->protocol = SOCK_STREAM;
    } else if ( GNB_PROTOCOL_UDP == protocol ) {
        sockaddress->protocol = SOCK_DGRAM;
    } else {
        sockaddress->protocol = SOCK_DGRAM;
    }

    memset(&sockaddress->m_in4,0, sizeof(struct sockaddr_in));

    sockaddress->m_in4.sin_family = AF_INET;

    if ( NULL != host ) {
        inet_pton(AF_INET, host, (struct in_addr *)&sockaddress->m_in4.sin_addr.s_addr);
    } else {
        sockaddress->m_in4.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    sockaddress->m_in4.sin_port = htons(port);

    sockaddress->socklen = sizeof(struct sockaddr_in);

}


void gnb_set_sockaddress6(gnb_sockaddress_t *sockaddress, int protocol, const char *host, int port){

    sockaddress->addr_type = AF_INET6;

    if ( GNB_PROTOCOL_TCP == protocol ) {
        sockaddress->protocol = SOCK_STREAM;
    } else if ( GNB_PROTOCOL_UDP == protocol ) {
        sockaddress->protocol = SOCK_DGRAM;
    } else {
        sockaddress->protocol = SOCK_DGRAM;
    }

    memset(&sockaddress->m_in6,0, sizeof(struct sockaddr_in6));

    sockaddress->m_in6.sin6_family = AF_INET6;

    if ( NULL != host ) {
        inet_pton( AF_INET6, host, &sockaddress->m_in6.sin6_addr);
    } else {
        sockaddress->m_in6.sin6_addr = in6addr_any;
    }

    sockaddress->m_in6.sin6_port = htons(port);

    sockaddress->socklen = sizeof(struct sockaddr_in6);

}


gnb_address_t gnb_get_address4_from_string(const char *sockaddress4_string){

    gnb_address_t address;

    memset(&address,0,sizeof(gnb_address_t));

    unsigned long int ul;

    char sockaddress4_string_copy[16];

    int sockaddress4_string_len = strlen(sockaddress4_string);

    memcpy(sockaddress4_string_copy, sockaddress4_string, sockaddress4_string_len);

    int i;

    char *p = sockaddress4_string_copy;

    for ( i=0; i<sockaddress4_string_len; i++) {

        if ( ':' == *p ) {
            *p = '\0';
            break;
        }

        p++;

    }

    p++;

    ul = strtoul(p, NULL, 10);

    if ( ULONG_MAX == ul ) {
        return address;
    }

    inet_pton(AF_INET, sockaddress4_string_copy, &address.address.addr4);

    uint16_t port = (uint16_t)ul;
    address.port = htons(port);

    address.type = AF_INET;

    return address;

}

char* gnb_hide_adrress_string(char*adrress_string){

    char *p;

    p = adrress_string;

    while ( '\0' != *p ) {

        if ( '.' == *p || ':' == *p ) {
            break;
        }

        *p = '*';

        p++;

    }

    return adrress_string;

}


void gnb_address_list3_fifo(gnb_address_list_t *address_list, gnb_address_t *address){

    int idx;

    if ( 0 == address_list->num ) {
        goto update_fifo;
    }

    idx = gnb_address_list_find(address_list, address);

    if ( idx >= 0 ) {
        address_list->array[idx].ts_sec = address->ts_sec;
        return;
    }

update_fifo:

    switch (address_list->num) {

    case 3:       

        memcpy( &address_list->array[2], &address_list->array[1], sizeof(gnb_address_t) );
        memcpy( &address_list->array[1], &address_list->array[0], sizeof(gnb_address_t) );
        memcpy( &address_list->array[0], address, sizeof(gnb_address_t) );

        break;

    case 2:

        memcpy( &address_list->array[2], &address_list->array[1], sizeof(gnb_address_t) );
        memcpy( &address_list->array[1], &address_list->array[0], sizeof(gnb_address_t) );
        memcpy( &address_list->array[0], address, sizeof(gnb_address_t) );        
        address_list->num = 3;

        break;

    case 1:

        memcpy( &address_list->array[1], &address_list->array[0], sizeof(gnb_address_t) );
        memcpy( &address_list->array[0], address, sizeof(gnb_address_t) );
        address_list->num = 2;

        break;

    case 0:
    default:

        memcpy( &address_list->array[0], address, sizeof(gnb_address_t) );
        address_list->num = 1;

        break;

    }

}


int gnb_determine_subnet6_prefixlen96(struct in6_addr addr6_a, struct in6_addr addr6_b) {

    int ret;

    addr6_a.s6_addr[12] = 0;
    addr6_a.s6_addr[13] = 0;
    addr6_a.s6_addr[14] = 0;
    addr6_a.s6_addr[15] = 0;

    addr6_b.s6_addr[12] = 0;
    addr6_b.s6_addr[13] = 0;
    addr6_b.s6_addr[14] = 0;
    addr6_b.s6_addr[15] = 0;

    /* rfc3542
    This macro returns non-zero if the addresses are equal; otherwise it
    returns zero.    
    */
    ret = IN6_ARE_ADDR_EQUAL( &addr6_a, &addr6_b );

    return ret;

}


int gnb_determine_subnet4(struct in_addr addr4_a, struct in_addr addr4_b, struct in_addr netmask) {
    
    in_addr_t ipv4_network_a;
    in_addr_t ipv4_network_b;

    ipv4_network_a = addr4_a.s_addr & netmask.s_addr;
    ipv4_network_b = addr4_b.s_addr & netmask.s_addr;

    if ( ipv4_network_a == ipv4_network_b ) {
        return 1;
    }

    return 0;

}
