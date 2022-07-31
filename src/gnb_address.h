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

#ifndef GNB_ADDRESS_H
#define GNB_ADDRESS_H

#include "gnb_address_type.h"

#define GNB_ADDRESS_LIFE_TIME_TS_SEC   120
#define GNB_ADDRESS_AVAILABLE(address,now_sec) (now_sec - address->ts_sec) < GNB_ADDRESS_LIFE_TIME_TS_SEC ? 1:0

/*
当 gnb_addr_secure 为非零时将不输出的完整的ip地址
*/
extern uint8_t gnb_addr_secure;


unsigned long long gnb_ntohll(unsigned long long val);
unsigned long long gnb_htonll(unsigned long long val);

char get_netmask_class(uint32_t addr4);

gnb_address_list_t* gnb_create_address_list(size_t size);
void gnb_address_list_release(gnb_address_list_t *address_list);

//返回 -1 没有找到,  >0 为 address 在 list 中的索引编号
int gnb_address_list_find(gnb_address_list_t *address_list, gnb_address_t *address);

//把 address 放进列表中一个空的项里
void gnb_address_list_update(gnb_address_list_t *address_list, gnb_address_t *address);


void gnb_set_address4(gnb_address_t *address, struct sockaddr_in *in);
void gnb_set_address6(gnb_address_t *address, struct sockaddr_in6 *in6);

static char gnb_static_ip_port_string_buffer1[GNB_IP6_PORT_STRING_SIZE];
static char gnb_static_ip_port_string_buffer2[GNB_IP6_PORT_STRING_SIZE];
static char gnb_static_ip_port_string_buffer3[GNB_IP6_PORT_STRING_SIZE];

char * gnb_get_ip_port_string(gnb_address_t *address, char *dest, uint8_t addr_secure);

#define GNB_IP_PORT_STR1(address)  gnb_get_ip_port_string(address,gnb_static_ip_port_string_buffer1,gnb_addr_secure)
#define GNB_IP_PORT_STR2(address)  gnb_get_ip_port_string(address,gnb_static_ip_port_string_buffer2,gnb_addr_secure)


char * gnb_get_address4string(void *byte4, char *dest, uint8_t addr_secure);
char * gnb_get_address6string(void *byte16, char *dest, uint8_t addr_secure);

#define GNB_ADDR4STR1(byte4)  gnb_get_address4string(byte4,gnb_static_ip_port_string_buffer1,gnb_addr_secure)
#define GNB_ADDR4STR2(byte4)  gnb_get_address4string(byte4,gnb_static_ip_port_string_buffer2,gnb_addr_secure)
#define GNB_ADDR6STR1(byte16) gnb_get_address6string(byte16,gnb_static_ip_port_string_buffer1,gnb_addr_secure)
#define GNB_ADDR6STR2(byte16) gnb_get_address6string(byte16,gnb_static_ip_port_string_buffer2,gnb_addr_secure)


#define GNB_ADDR4STR_PLAINTEXT1(byte4)  gnb_get_address4string(byte4,gnb_static_ip_port_string_buffer1,0)
#define GNB_ADDR4STR_PLAINTEXT2(byte4)  gnb_get_address4string(byte4,gnb_static_ip_port_string_buffer2,0)
#define GNB_ADDR6STR_PLAINTEXT1(byte16) gnb_get_address6string(byte16,gnb_static_ip_port_string_buffer1,0)
#define GNB_ADDR6STR_PLAINTEXT2(byte16) gnb_get_address6string(byte16,gnb_static_ip_port_string_buffer2,0)


char * gnb_get_socket4string(struct sockaddr_in  *in,  char *dest, uint8_t addr_secure);
char * gnb_get_socket6string(struct sockaddr_in6 *in6, char *dest, uint8_t addr_secure);

#define GNB_SOCKADDR4STR1(in)  gnb_get_socket4string(in, gnb_static_ip_port_string_buffer1,gnb_addr_secure)
#define GNB_SOCKADDR6STR1(in6) gnb_get_socket6string(in6,gnb_static_ip_port_string_buffer1,gnb_addr_secure)
#define GNB_SOCKADDR4STR2(in)  gnb_get_socket4string(in, gnb_static_ip_port_string_buffer2,gnb_addr_secure)
#define GNB_SOCKADDR6STR2(in6) gnb_get_socket6string(in6,gnb_static_ip_port_string_buffer2,gnb_addr_secure)
#define GNB_SOCKADDR4STR3(in)  gnb_get_socket4string(in, gnb_static_ip_port_string_buffer3,gnb_addr_secure)
#define GNB_SOCKADDR6STR3(in6) gnb_get_socket6string(in6,gnb_static_ip_port_string_buffer3,gnb_addr_secure)


char * gnb_get_sockaddress_string(gnb_sockaddress_t *sockaddress, char *dest, uint8_t addr_secure);
#define GNB_SOCKETADDRSTR1(sockaddress) gnb_get_sockaddress_string(sockaddress,gnb_static_ip_port_string_buffer1,gnb_addr_secure)
#define GNB_SOCKETADDRSTR2(sockaddress) gnb_get_sockaddress_string(sockaddress,gnb_static_ip_port_string_buffer2,gnb_addr_secure)



int gnb_cmp_sockaddr_in6(struct sockaddr_in6 *in1, struct sockaddr_in6 *in2);
int gnb_cmp_sockaddr_in(struct sockaddr_in *in1, struct sockaddr_in *in2);


#define GNB_PROTOCOL_UDP   0x1
#define GNB_PROTOCOL_TCP   0x2
void gnb_set_sockaddress4(gnb_sockaddress_t *sockaddress, int protocol, const char *host, int port);
void gnb_set_sockaddress6(gnb_sockaddress_t *sockaddress, int protocol, const char *host, int port);

gnb_address_t gnb_get_address4_from_string(const char *sockaddress4_string);

void gnb_address_list3_fifo(gnb_address_list_t *address_list, gnb_address_t *address);

#endif
