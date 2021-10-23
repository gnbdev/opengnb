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

#ifndef GNB_UDP_H
#define GNB_UDP_H

int gnb_bind_udp_socket_ipv4(int socketfd,const char *host, int port);

int gnb_send_udp_ipv4(int socketfd, char *host, int port,void *data, size_t data_size);

int gnb_bind_udp_socket_ipv6(int socketfd,const char *host, int port);

#endif
