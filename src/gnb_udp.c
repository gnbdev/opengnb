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
#include <fcntl.h>
#include <errno.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#endif

#ifdef _WIN32

#define _POSIX

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#include "gnb_udp.h"


int gnb_bind_udp_socket_ipv4(int socketfd,const char *host, int port){

    struct sockaddr_in svr_addr;

    memset(&svr_addr, 0, sizeof(struct sockaddr_in));

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(port);

    if ( NULL != host ) {
        svr_addr.sin_addr.s_addr = inet_addr(host);
    } else {
        svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    int on = 1;
    setsockopt( socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on) );

    if ( bind(socketfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr_in)) < 0 ) {
        perror("bind");
        return -1;
    }

    return 0;

}


int gnb_bind_udp_socket_ipv6(int socketfd,const char *host, int port){

    struct sockaddr_in6 svr_addr;

    memset(&svr_addr,0, sizeof(struct sockaddr_in6));

    svr_addr.sin6_family = AF_INET6;
    svr_addr.sin6_port   = htons(port);

    if ( NULL != host ) {
        inet_pton( AF_INET6, host, &svr_addr.sin6_addr);
    } else {
        svr_addr.sin6_addr = in6addr_any;
    }

    int on;

    on = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,(const char *)&on, sizeof(on) );

    on = 1;
    setsockopt(socketfd, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on, sizeof(on) );

    if ( bind(socketfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr_in6))<0 ) {
        printf("bind host[%s] port[%d]\n",host, port);
        perror("bind");
        return -1;
    }

    return 0;

}
