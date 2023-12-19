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

#include "gnb_config_lite.h"
#include "gnb_node.h"

gnb_conf_ext_lite_t gnb_conf_ext_lite;

char * check_domain_name(char *host_string);
char * check_node_route(char *config_line_string);
gnb_node_t * gnb_node_init(gnb_core_t *gnb_core, uint32_t uuid32);

int gnb_test_field_separator(char *config_string);


static char* gnb_string_pattern(char *in_buf, char delimiter, char *out_buf, size_t *out_buf_size){

    char *p;

    p = in_buf;

    if ( *p == '\0' ) {
        p = NULL;
        return p;
    }

    size_t c = 0;

    do{

        if ( delimiter == *p ) {
            p++;
            break;
        }

        if ( '\0' == *p ) {
            break;
        }

        if ( c == *out_buf_size ) {
            break;
        }

        *out_buf = *p;

        out_buf++;
        p++;
        c++;

    }while(*p);

    *out_buf = '\0';
    *out_buf_size = c;

    return p;

}


static void setup_node_address(gnb_core_t *gnb_core, char *node_address_string) {

    int ret;

    char *p = node_address_string;

    char line_buffer[1024];
    size_t line_buffer_size;

    gnb_node_t *node;

    gnb_address_t address_st;

    gnb_address_list_t *static_address_list;
    gnb_address_list_t *dynamic_address_list;
    gnb_address_list_t *resolv_address_list;
    gnb_address_list_t *push_address_list;

    int num;

    char attrib_string[16+1];
    uint32_t uuid32;
    char     host_string[INET6_ADDRSTRLEN+1];
    uint16_t port = 0;

    int i = 0;

    i = 0;

    do{

        line_buffer_size = 1024;

        p = gnb_string_pattern(p, ',', line_buffer, &line_buffer_size);

        if ( NULL == p ) {
            break;
        }

        ret = gnb_test_field_separator(line_buffer);

        if ( GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH == ret ) {
            num = sscanf(line_buffer,"%16[^/]/%u/%46[^/]/%hu\n", attrib_string, &uuid32, host_string, &port);
        } else if ( GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL == ret ) {
            num = sscanf(line_buffer,"%16[^|]|%u|%46[^|]|%hu\n", attrib_string, &uuid32, host_string, &port);
        } else {
            num = 0;
        }

        if ( 4 != num ) {
            continue;
        }

        if ( NULL!=check_domain_name(host_string) ) {
            //域名
            continue;
        }

        memset(&address_st, 0, sizeof(gnb_address_t));
        address_st.port = htons(port);

        if ( NULL != strchr(host_string, '.') ) {
            //ipv4
            inet_pton(AF_INET, host_string, (struct in_addr *)&address_st.m_address4);
            address_st.type = AF_INET;

        } else if ( NULL != strchr(host_string, ':') ) {
            //ipv6
            inet_pton(AF_INET6, host_string, (struct in_addr *)&address_st.m_address6);
            address_st.type = AF_INET6;
        } else {
            continue;
        }

        //加入到 index address list
        if ( NULL != strchr(attrib_string, 'i') && uuid32 != gnb_core->local_node->uuid32 ) {
            gnb_address_list_update(gnb_core->index_address_ring.address_list, &address_st);
        }

        if ( NULL != strchr(attrib_string, 'u') && uuid32 != gnb_core->local_node->uuid32 ) {
            gnb_address_list_update(gnb_core->fwdu0_address_ring.address_list, &address_st);
        }

        node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, uuid32);

        if ( NULL==node ) {
            continue;
        }

        static_address_list = (gnb_address_list_t *)&node->static_address_block;
        dynamic_address_list = (gnb_address_list_t *)&node->dynamic_address_block;
        resolv_address_list = (gnb_address_list_t *)&node->resolv_address_block;
        push_address_list = (gnb_address_list_t *)&node->push_address_block;

        if ( AF_INET6 == address_st.type ) {

            gnb_address_list_update(static_address_list, &address_st);

            node->udp_addr_status |= GNB_NODE_STATUS_IPV6_STATIC;
            node->udp_sockaddr6.sin6_family = AF_INET6;
            node->udp_sockaddr6.sin6_port = address_st.port;
            memcpy(&node->udp_sockaddr6.sin6_addr, address_st.m_address6, 16);


        } else if ( AF_INET == address_st.type ) {

            gnb_address_list_update(static_address_list, &address_st);

            node->udp_addr_status |= GNB_NODE_STATUS_IPV4_STATIC;
            node->udp_sockaddr4.sin_family = AF_INET;
            node->udp_sockaddr4.sin_port = address_st.port;
            memcpy(&node->udp_sockaddr4.sin_addr, address_st.m_address4, 4);

        }

        if ( NULL != strchr(attrib_string, 'i') ) {
            node->type |= GNB_NODE_TYPE_IDX;
        }

        if ( NULL != strchr(attrib_string, 'r') ) {
            node->type |= GNB_NODE_TYPE_RELAY;
        }

        if ( NULL != strchr(attrib_string, 's') ) {
            node->type |= GNB_NODE_TYPE_SLIENCE;
        }

        if ( NULL != strchr(attrib_string, 'f') && uuid32 != gnb_core->local_node->uuid32 ) {
            node->type |= GNB_NODE_TYPE_FWD;
            gnb_add_forward_node_ring(gnb_core, uuid32);
        }


    }while (p);


}


static void setup_node_route(gnb_core_t *gnb_core, char *node_route_string) {

    int ret;

    char *p = node_route_string;

    char line_buffer[1024+1];
    size_t line_buffer_size;

    uint32_t uuid32;
    uint32_t tun_addr4;
    uint32_t tun_subnet_addr4;
    uint32_t tun_netmask_addr4;

    char tun_ipv4_string[INET_ADDRSTRLEN+1];
    char tun_netmask_string[INET_ADDRSTRLEN+1];
    char tun_ipv6_string[INET6_ADDRSTRLEN+1];

    int num;

    gnb_node_t *node;

    char netmask_class;

    gnb_core->node_nums = 0;

    do{

        line_buffer_size = 1024;

        p = gnb_string_pattern(p, ',', line_buffer, &line_buffer_size);

        if ( NULL == p ) {
            break;
        }

        if ( NULL != check_node_route(line_buffer) ) {
            continue;
        }

        ret = gnb_test_field_separator(line_buffer);

        if ( GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH == ret ) {
            num = sscanf(line_buffer,"%u/%16[^/]/%16[^/]", &uuid32, tun_ipv4_string, tun_netmask_string);
        } else if ( GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL == ret ) {
            num = sscanf(line_buffer,"%u|%16[^|]|%16[^|]", &uuid32, tun_ipv4_string, tun_netmask_string);
        } else {
            num = 0;
        }

        if ( 3 != num ) {
            continue;
        }

        node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, uuid32);

        if ( NULL==node ) {
            node = gnb_node_init(gnb_core, uuid32);
            GNB_HASH32_UINT32_SET(gnb_core->uuid_node_map, uuid32, node);
            gnb_core->node_nums++;
        }

        inet_pton(AF_INET, tun_ipv4_string, (struct in_addr *)&tun_addr4);
        inet_pton(AF_INET, tun_netmask_string, (struct in_addr *)&tun_netmask_addr4);

        tun_subnet_addr4 = tun_addr4 & tun_netmask_addr4;

        char *p = (char *)&tun_addr4;

        //根据ip地址最后一位判断是主机还是网络，如果是主机就作为 tun 的ip
        //tun_addr4 当前版本只能被设一次，今后可能会支持多个虚拟ip
        if ( 0 != p[3] && 0 == node->tun_addr4.s_addr ) {

            node->tun_addr4.s_addr = tun_addr4;
            node->tun_netmask_addr4.s_addr = tun_netmask_addr4;
            node->tun_subnet_addr4.s_addr = tun_subnet_addr4;

            snprintf(tun_ipv6_string, INET6_ADDRSTRLEN, "64:ff9b::%s", tun_ipv4_string);
            inet_pton(AF_INET6, tun_ipv6_string, (struct in6_addr *)&node->tun_ipv6_addr);

            GNB_HASH32_UINT32_SET(gnb_core->ipv4_node_map, node->tun_addr4.s_addr, node);

        } else {

            netmask_class = get_netmask_class(tun_netmask_addr4);

            if ( 'c' == netmask_class ) {
                GNB_HASH32_UINT32_SET(gnb_core->subnetc_node_map, tun_subnet_addr4, node);
            }

            if ( 'b' == netmask_class ) {
                GNB_HASH32_UINT32_SET(gnb_core->subnetb_node_map, tun_subnet_addr4, node);
            }

            if ( 'a' == netmask_class ) {
                GNB_HASH32_UINT32_SET(gnb_core->subneta_node_map, tun_subnet_addr4, node);
            }

        }

    }while(1);

}


void gnb_config_lite(gnb_core_t *gnb_core){

    char index_node_address_string[1024];

    int i;

    if ( NULL == gnb_conf_ext_lite.node_route_string ) {
        gnb_conf_ext_lite.node_route_string = "1001|10.1.0.1|255.255.0.0,1002|10.1.0.2|255.255.0.0,1003|10.1.0.3|255.255.0.0,1004|10.1.0.4|255.255.0.0,1005|10.1.0.5|255.255.0.0";
    }

    setup_node_route(gnb_core, gnb_conf_ext_lite.node_route_string);

    gnb_core->ctl_block->node_zone->node_num   = gnb_core->node_nums;
    gnb_core->ctl_block->core_zone->local_uuid = gnb_core->conf->local_uuid;

    gnb_init_node_key512(gnb_core);

    gnb_core->local_node = GNB_HASH32_UINT32_GET_PTR(gnb_core->uuid_node_map, gnb_core->conf->local_uuid);

    if ( NULL==gnb_core->local_node ) {
        printf("miss local_node[%u] is NULL\n", gnb_core->conf->local_uuid);
        exit(1);
    }

    if ( NULL != gnb_conf_ext_lite.index_address_string ) {

        snprintf(index_node_address_string,1024,"i|0|%s", gnb_conf_ext_lite.index_address_string);

        for ( i=0; i < strlen(index_node_address_string); i++ ) {

            if ( '/' == index_node_address_string[i]  ) {
                index_node_address_string[i] = '|';
            }

        }

        setup_node_address(gnb_core, index_node_address_string);

    }

    if ( NULL != gnb_conf_ext_lite.node_address_string ) {
        setup_node_address(gnb_core, gnb_conf_ext_lite.node_address_string);
    }

    gnb_core->local_node->udp_sockaddr6.sin6_port = htons(gnb_core->conf->udp6_ports[0]);
    gnb_core->local_node->udp_sockaddr4.sin_port  = htons(gnb_core->conf->udp4_ports[0]);

    gnb_core->local_node->tun_sin_port4 = htons(gnb_core->conf->udp4_ports[0]);

}
