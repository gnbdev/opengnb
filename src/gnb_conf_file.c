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

#ifdef __UNIX_LIKE_OS__
#include <arpa/inet.h>
#endif


#ifdef _WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "gnb_conf_file.h"
#include "gnb_node.h"
#include "gnb_keys.h"
#include "gnb_udp.h"

#include "ed25519/ed25519.h"
#include "ed25519/sha512.h"

char * check_domain_name(char *host_string);
char * check_node_route(char *config_line_string);
gnb_node_t * gnb_node_init(gnb_core_t *gnb_core, gnb_uuid_t uuid64);
int check_listen_string(char *listen_string);
void gnb_setup_listen_addr_port(char *listen_address6_string, uint16_t *port_ptr, char *sockaddress_string, int addr_type);
void gnb_setup_es_argv(char *es_argv_string);
int gnb_test_field_separator(char *config_string);

static void address_file_config(gnb_core_t *gnb_core){

    int ret;

    FILE *file;

    gnb_node_t *node;

    gnb_address_t address_st;

    gnb_address_list_t *static_address_list;
    gnb_address_list_t *dynamic_address_list;
    gnb_address_list_t *resolv_address_list;
    gnb_address_list_t *push_address_list;

    char idx_addr_file[PATH_MAX+NAME_MAX];

    int num;

    snprintf(idx_addr_file, PATH_MAX+NAME_MAX, "%s/%s", gnb_core->conf->conf_dir, "address.conf");

    file = fopen(idx_addr_file,"r");

    if ( NULL == file ) {
        return;
    }

    char line_buffer[1024+1];

    char attrib_string[16+1];
    gnb_uuid_t uuid64;
    char     host_string[INET6_ADDRSTRLEN+1];
    uint16_t port = 0;

    int i = 0;

    i = 0;

    do{

        num = fscanf(file,"%1024s\n",line_buffer);

        if ( EOF == num ) {
            break;
        }

        if ( '#' == line_buffer[0] ) {
            continue;
        }

        ret = gnb_test_field_separator(line_buffer);

        if ( GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH == ret ) {
            num = sscanf(line_buffer,"%16[^/]/%llu/%46[^/]/%hu\n", attrib_string, &uuid64, host_string, &port);
        } else if ( GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL == ret ) {
            num = sscanf(line_buffer,"%16[^|]|%llu|%46[^|]|%hu\n", attrib_string, &uuid64, host_string, &port);
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
        if ( NULL != strchr(attrib_string, 'i') && uuid64 != gnb_core->local_node->uuid64 ) {
            gnb_address_list_update(gnb_core->index_address_ring.address_list, &address_st);
        }

        if ( NULL != strchr(attrib_string, 'u') && uuid64 != gnb_core->local_node->uuid64 ) {
            gnb_address_list_update(gnb_core->fwdu0_address_ring.address_list, &address_st);
        }

        node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, uuid64);

        if ( NULL == node ) {
            continue;
        }

        static_address_list = (gnb_address_list_t *)&node->static_address_block;
        dynamic_address_list = (gnb_address_list_t *)&node->dynamic_address_block;
        resolv_address_list = (gnb_address_list_t *)&node->resolv_address_block;
        push_address_list = (gnb_address_list_t *)&node->push_address_block;

        if ( AF_INET6 == address_st.type) {

            gnb_address_list_update(static_address_list, &address_st);

            node->udp_addr_status |= GNB_NODE_STATUS_IPV6_STATIC;
            node->udp_sockaddr6.sin6_family = AF_INET6;
            node->udp_sockaddr6.sin6_port = address_st.port;
            memcpy(&node->udp_sockaddr6.sin6_addr, address_st.m_address6, 16);


        } else if( AF_INET == address_st.type ) {

            gnb_address_list_update(static_address_list, &address_st);

            node->udp_addr_status |= GNB_NODE_STATUS_IPV4_STATIC;
            node->udp_sockaddr4.sin_family = AF_INET;
            node->udp_sockaddr4.sin_port = address_st.port;
            memcpy(&node->udp_sockaddr4.sin_addr, address_st.m_address4, 4);

        }

        if ( NULL != strchr(attrib_string, 'i') ) {
            node->type |= GNB_NODE_TYPE_IDX;
            if ( 0 != uuid64 && uuid64 != gnb_core->local_node->uuid64 ) {
                gnb_add_index_node_ring(gnb_core, uuid64);
            }
        }

        if ( NULL != strchr(attrib_string, 'r') ) {
            node->type |= GNB_NODE_TYPE_RELAY;
        }

        if ( NULL != strchr(attrib_string, 's') ) {
            node->type |= GNB_NODE_TYPE_SLIENCE;
        }

        if ( NULL != strchr(attrib_string, 'f') && uuid64 != gnb_core->local_node->uuid64 ) {
            node->type |= GNB_NODE_TYPE_FWD;
            gnb_add_forward_node_ring(gnb_core, uuid64);
        }


    }while(1);

    fclose(file);

    return;

}



static void load_node_cache(gnb_core_t *gnb_core){

    int ret;

    FILE *file;

    gnb_node_t *node;

    gnb_address_t address_st;

    gnb_address_list_t *static_address_list;
    gnb_address_list_t *dynamic_address_list;
    gnb_address_list_t *resolv_address_list;
    gnb_address_list_t *push_address_list;

    int num;

    file = fopen(gnb_core->conf->node_cache_file,"r");

    if (NULL==file) {
        return;
    }

    char line_buffer[1024+1];

    char attrib_string[16+1];
    gnb_uuid_t uuid64;
    char     host_string[INET6_ADDRSTRLEN+1];
    uint16_t port = 0;
    char key512_hex_string[128+1];

    int i = 0;

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
            num = sscanf(line_buffer,"%16[^/]/%llu/%46[^/]/%hu/%128s\n", attrib_string, &uuid64, host_string, &port, key512_hex_string);
        } else if ( GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL == ret ) {
            num = sscanf(line_buffer,"%16[^|]|%llu|%46[^|]|%hu|%128s\n", attrib_string, &uuid64, host_string, &port, key512_hex_string);
        } else {
            num = 0;
        }

        if ( 5 != num ) {
            continue;
        }

        if ( 0 == port ) {
            continue;
        }

        memset(&address_st, 0, sizeof(gnb_address_t));
        address_st.port = htons(port);

        if ( NULL != strchr(host_string, '.') ) {
            //ipv4
            ret = inet_pton(AF_INET, host_string, &address_st.m_address4);\
            if ( ret >0 ) {
                address_st.type = AF_INET;
            } else {
                address_st.type = AF_UNSPEC;
            }

        } else if ( NULL != strchr(host_string, ':') ) {
            //ipv6
            ret = inet_pton(AF_INET6, host_string, &address_st.m_address6);
            if ( ret >0 ) {
                address_st.type = AF_INET6;
            } else {
                address_st.type = AF_UNSPEC;
            }

        } else {
            continue;
        }

        node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, uuid64);

        if ( NULL==node ) {
            continue;
        }

        push_address_list = (gnb_address_list_t *)&node->push_address_block;

        if ( AF_INET6 == address_st.type ) {
            gnb_address_list_update(push_address_list, &address_st);
        } else if ( AF_INET == address_st.type ) {
            gnb_address_list_update(push_address_list, &address_st);
        }

    }while(1);

    fclose(file);

    return;

}


void local_node_file_config(gnb_conf_t *conf){

    FILE *file;

    char node_conf_file[PATH_MAX+NAME_MAX];

    uint32_t log_level;
    uint32_t mtu;

    snprintf(node_conf_file, PATH_MAX+NAME_MAX,"%s/%s", conf->conf_dir, "node.conf");

    file = fopen(node_conf_file,"r");

    if ( NULL==file ) {
        printf("miss file [%s]\n", node_conf_file);
        exit(1);
    }

    char line_buffer[1024+1];

    char field[256];
    char value[256];
    int  value_int;

    char listen_sockaddress4_string[GNB_IP4_PORT_STRING_SIZE];
    char listen_sockaddress6_string[GNB_IP6_PORT_STRING_SIZE];
    uint16_t port_host = 0;
    int ret;

    int num;

    int listen6_idx = 0;
    int listen4_idx = 0;

    do{

        num = fscanf(file,"%1024[^\n]\n",line_buffer);

        if ( EOF == num ) {
            break;
        }

        if ('#' == line_buffer[0]){
            continue;
        }

        if ( !strncmp(line_buffer, "ifname", sizeof("ifname")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config ifname error in [%s]\n", node_conf_file);
                exit(1);
            }

            memset(conf->ifname, 0, 16);
            strncpy(conf->ifname,value, 16);
            conf->ifname[15] = '\0';

        }


        if ( !strncmp(line_buffer, "if-drv", sizeof("if-drv")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %32s", field, value);

            if ( 2 != num ) {
                printf("config if-drv error in [%s]\n", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "wintun", sizeof("wintun")-1) ) {
                conf->if_drv = GNB_IF_DRV_TYPE_TAP_WINTUN;
            } else if ( !strncmp(value, "tap-windows", sizeof("tap-windows")-1) ) {
                conf->if_drv = GNB_IF_DRV_TYPE_TAP_WINDOWS;
            } else {
                conf->if_drv = GNB_IF_DRV_TYPE_DEFAULT;
            }

        }


        if ( !strncmp(line_buffer, "nodeid", sizeof("nodeid")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %llu", field, &conf->local_uuid);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "nodeid", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "listen", sizeof("listen")-1) ) {

            num = sscanf(line_buffer, "%128[^ ] %128s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "listen", node_conf_file);
                exit(1);
            }

            ret = check_listen_string(value);

            if ( 0 == ret ) {

                port_host = (uint16_t)strtoul(value, NULL, 10);

                snprintf(listen_sockaddress6_string, GNB_IP6_PORT_STRING_SIZE, "[:::%d]",   port_host);
                snprintf(listen_sockaddress4_string, GNB_IP4_PORT_STRING_SIZE, "0.0.0.0:%d", port_host);

                if ( '[' != conf->listen_address6_string[0] ) {

                    gnb_setup_listen_addr_port(conf->listen_address6_string, &conf->udp6_ports[listen6_idx], listen_sockaddress6_string, AF_INET6);

                } else {

                    conf->udp6_ports[listen6_idx] = port_host;

                }

                if ( '\0' != conf->listen_address4_string[0] ) {

                    gnb_setup_listen_addr_port(conf->listen_address4_string, &conf->udp4_ports[listen4_idx], listen_sockaddress4_string, AF_INET);

                } else {

                    conf->udp4_ports[listen4_idx] = port_host;

                }

                listen6_idx++;
                listen4_idx++;

            } else if ( 6 == ret ) {

                strncpy(listen_sockaddress6_string, value, GNB_IP6_PORT_STRING_SIZE);
                listen_sockaddress6_string[GNB_IP6_PORT_STRING_SIZE-1] = '\0';
                gnb_setup_listen_addr_port(conf->listen_address6_string, &conf->udp6_ports[listen6_idx], listen_sockaddress6_string, AF_INET6);
                listen6_idx++;

            } else if ( 4 == ret ) {

                strncpy(listen_sockaddress4_string, value, GNB_IP4_PORT_STRING_SIZE);
                listen_sockaddress4_string[GNB_IP4_PORT_STRING_SIZE-1] = '\0';
                gnb_setup_listen_addr_port(conf->listen_address4_string, &conf->udp4_ports[listen4_idx], listen_sockaddress4_string, AF_INET);
                listen4_idx++;

            }

        }


        if ( !strncmp(line_buffer, "ctl-block", sizeof("ctl-block")-1) && 0 == conf->systemd_daemon ) {

            num = sscanf(line_buffer, "%32[^ ] %s", field, conf->map_file);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "ctl-block", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "socket-if-name", sizeof("socket-if-name")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %s", field, conf->socket_ifname);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "socket-if-name", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "pid-file", sizeof("pid-file")-1) && 0 == conf->systemd_daemon ) {

            num = sscanf(line_buffer, "%32[^ ] %s", field, conf->pid_file);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "pid-file", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "es-argv", sizeof("es-argv")-1) ) {

            num = sscanf(line_buffer,"%32[^ ] %s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "es-argv", node_conf_file);
                exit(1);
            }

            gnb_setup_es_argv(value);

        }


        if (!strncmp(line_buffer, "multi-socket", sizeof("multi-socket")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "multi-socket", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->multi_socket = 1;
            } else {
                conf->multi_socket = 0;
            }

        }


        if (!strncmp(line_buffer, "direct-forwarding", sizeof("direct-forwarding")-1) ) {

            num = sscanf(line_buffer,"%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "direct-forwarding", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->direct_forwarding = 1;
            } else {
                conf->direct_forwarding = 0;
            }

        }


        if (!strncmp(line_buffer, "unified-forwarding", sizeof("unified-forwarding")-1) ) {

            num = sscanf(line_buffer,"%32[^ ] %10s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "unified-forwarding", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "off", sizeof("off")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_OFF;
            } else if ( !strncmp(value, "auto", sizeof("auto")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_AUTO;
            } else if ( !strncmp(value, "force", sizeof("force")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_FORCE;
            } else if ( !strncmp(value, "super", sizeof("super")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_SUPER;
            } else if ( !strncmp(value, "hyper", sizeof("hyper")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_HYPER;
            } else {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_AUTO;
            }

        }


        if (!strncmp(line_buffer, "ipv4-only", sizeof("ipv4-only")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "ipv4-only", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->udp_socket_type = GNB_ADDR_TYPE_IPV4;
            }

        }


        if (!strncmp(line_buffer, "ipv6-only", sizeof("ipv6-only")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "ipv6-only", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->udp_socket_type = GNB_ADDR_TYPE_IPV6;
            }

        }


        if ( !strncmp(line_buffer, "passcode", sizeof("passcode")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %10s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "passcode", node_conf_file);
                exit(1);
            }

            gnb_build_passcode(conf->crypto_passcode, value);

        }


        if ( !strncmp(line_buffer, "quiet", sizeof("quiet")-1) ) {

            num = sscanf(line_buffer,"%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "quiet", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->quiet = 1;
            } else {
                conf->quiet = 0;
            }

        }


        if ( !strncmp(line_buffer, "daemon", sizeof("daemon")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "daemon", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) && 0 == conf->systemd_daemon ) {
                conf->daemon = 1;
            } else {
                conf->daemon = 0;
            }

        }


        if ( !strncmp(line_buffer, "mtu", sizeof("mtu")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %d", field, &conf->mtu);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "mtu", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "set-tun", sizeof("set-tun")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "set-tun", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->activate_tun = 1;
            } else {
                conf->activate_tun = 0;
            }

        }


        if ( !strncmp(line_buffer, "address-secure", sizeof("address-secure")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "address-secure", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->addr_secure = 1;
            } else {
                conf->addr_secure = 0;
            }

            gnb_addr_secure = conf->addr_secure;

        }


        if ( !strncmp(line_buffer, "node-worker", sizeof("node-worker")-1) ) {

            num = sscanf(line_buffer,"%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "node-worker", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->activate_node_worker = 1;
            } else {
                conf->activate_node_worker = 0;
            }

        }


        if ( !strncmp(line_buffer, "index-worker", sizeof("index-worker")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "index-worker", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->activate_index_worker = 1;
            } else {
                conf->activate_index_worker = 0;
            }

        }


        if ( !strncmp(line_buffer, "index-service-worker", sizeof("index-service-worker")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "index-service-worker", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->activate_index_service_worker = 1;
            } else {
                conf->activate_index_service_worker = 0;
            }

        }

        if ( !strncmp(line_buffer, "node-detect-worker", sizeof("node-detect-worker")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "node-detect-worker", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->activate_detect_worker = 1;
            } else {
                conf->activate_detect_worker = 0;
            }

        }


        if ( !strncmp(line_buffer, "pf-worker", sizeof("pf-worker")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %hhd", field, &conf->pf_worker_num );

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "pf-worker", node_conf_file);
                exit(1);
            }

            if ( conf->pf_worker_num > 128 ) {
                conf->pf_worker_num = 128;
            }

        }


        if ( !strncmp(line_buffer, "safe-index", sizeof("safe-index")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "safe-index", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "on", sizeof("on")-1) ) {
                conf->safe_index = 1;
            } else {
                conf->safe_index = 0;
            }

        }


        if ( !strncmp(line_buffer, "port-detect", sizeof("port-detect")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %hu,%hu,%hu", field, &conf->port_detect_range, &conf->port_detect_start, &conf->port_detect_end);

            if ( 4 != num ) {
                printf("config %s error in [%s]\n", "port-detect", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "detect-interval", sizeof("detect-interval")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u,%u", field, &conf->address_detect_interval_usec, &conf->full_detect_interval_sec);

            if ( 3 != num ) {
                printf("config %s error in [%s]\n", "detect-interval-usec", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "log-file-path", sizeof("log-file-path")-1) && 0 == conf->systemd_daemon ) {

            num = sscanf(line_buffer, "%32[^ ] %s", field, conf->log_path);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "log-file-path", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "log-udp4", sizeof("log-udp4")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %s", field, conf->log_udp_sockaddress4_string);

            if ( 1 == num ) {
                snprintf(conf->log_udp_sockaddress4_string, 16 + 1 + sizeof("65535"), "%s", "127.0.0.1:9000");
            }

            if ( 2 != num && 1 != num ) {
                printf("config %s error in [%s]\n", "log-udp4", node_conf_file);
                exit(1);
            }

        }


        if (!strncmp(line_buffer, "log-udp-type", sizeof("log-udp-type")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %2s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "log-udp-type", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "binary", sizeof("binary")-1) ) {
                conf->log_udp_type = GNB_LOG_UDP_TYPE_BINARY;
            } else {
                conf->log_udp_type = GNB_LOG_UDP_TYPE_TEXT;
            }

        }


        if ( !strncmp(line_buffer, "console-log-level", sizeof("console-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "console-log-level", node_conf_file);
                exit(1);
            }

            conf->console_log_level = log_level;

        }


        if ( !strncmp(line_buffer, "file-log-level", sizeof("file-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "file-log-level", node_conf_file);
                exit(1);
            }

            conf->file_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "udp-log-level", sizeof("udp-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "udp-log-level", node_conf_file);
                exit(1);
            }

            conf->udp_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "core-log-level", sizeof("core-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "core-log-level", node_conf_file);
                exit(1);
            }

            conf->core_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "pf-log-level", sizeof("pf-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "pf-log-level", node_conf_file);
                exit(1);
            }

            conf->pf_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "main-log-level", sizeof("main-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "main-log-level", node_conf_file);
                exit(1);
            }

            conf->main_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "node-log-level", sizeof("node-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "node-log-level", node_conf_file);
                exit(1);
            }

            conf->node_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "index-log-level", sizeof("index-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "index-log-level", node_conf_file);
                exit(1);
            }

            conf->index_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "index-service-log-level", sizeof("index-service-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "index-service-log-level", node_conf_file);
                exit(1);
            }

            conf->index_service_log_level = log_level;

        }

        if ( !strncmp(line_buffer, "node-detect-log-level", sizeof("node-detect-log-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %u", field, &log_level);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "node-detect-log-level", node_conf_file);
                exit(1);
            }

            conf->detect_log_level = log_level;

        }


        if ( !strncmp(line_buffer, "log-file-path", sizeof("log-file-path")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %512s", field, conf->log_path);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "log-file-path", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "pf-route", sizeof("pf-route")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %128s", field, conf->pf_route);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "pf-route", node_conf_file);
                exit(1);
            }

        }


        if ( !strncmp(line_buffer, "crypto", sizeof("crypto")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %16s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "crypto", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "none", 16) ) {
                conf->pf_bits &= ~(GNB_PF_BITS_CRYPTO_XOR | GNB_PF_BITS_CRYPTO_ARC4);
            } else if ( !strncmp(value, "xor", 16) ) {
                conf->pf_bits |= GNB_PF_BITS_CRYPTO_XOR;
            } else if ( !strncmp(value, "arc4", 16) ) {
                conf->pf_bits |= GNB_PF_BITS_CRYPTO_ARC4;
            } else {
                conf->pf_bits |= GNB_PF_BITS_CRYPTO_XOR;
            }

        }


        if (!strncmp(line_buffer, "zip", sizeof("zip")-1) ) {

            num = sscanf(line_buffer,"%32[^ ] %10s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "zip", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "auto", sizeof("auto")-1) ) {
                conf->zip = GNB_ZIP_AUTO;
            } else if ( !strncmp(value, "force", sizeof("force")-1) ) {
                conf->zip = GNB_ZIP_FORCE;
            } else {
                conf->zip = GNB_ZIP_AUTO;
            }

        }


        if ( !strncmp(line_buffer, "zip-level", sizeof("zip-level")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %d", field, &value_int);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "zip-level", node_conf_file);
                exit(1);
            }

            if ( value_int > 9 ) {
                value_int = 9;
            }

            if ( value_int < -1 ) {
                value_int = -1;
            }

            conf->zip_level = value_int;

        }


        if ( !strncmp(line_buffer, "memory", sizeof("memory")-1) ) {

            num = sscanf(line_buffer, "%32[^ ] %16s", field, value);

            if ( 2 != num ) {
                printf("config %s error in [%s]\n", "memory", node_conf_file);
                exit(1);
            }

            if ( !strncmp(value, "tiny", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_TINY;
            } else if ( !strncmp(value, "small", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_SMALL;
            } else if ( !strncmp(value, "large", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_LARGE;
            } else if ( !strncmp(value, "huge", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_HUGE;
            } else {
                conf->memory = GNB_MEMORY_SCALE_TINY;
            }

        }


    }while(1);

    fclose(file);    

    if ( 1 == conf->multi_socket ) {
        conf->udp6_socket_num = 1;
        conf->udp4_socket_num = 5;
    }

    if ( 0 == conf->udp6_ports[ 0 ] ) {
        conf->udp6_ports[ 0 ] = 9001;
    }

    if ( 0 == conf->udp4_ports[ 0 ] ) {
        conf->udp4_ports[ 0 ] = 9001;
    }

    if ( conf->pf_worker_num > 0 ) {
        conf->unified_forwarding = GNB_UNIFIED_FORWARDING_OFF;
    }

    switch (conf->memory) {

    case GNB_MEMORY_SCALE_TINY:
        conf->payload_block_size = 1024*4;
        conf->max_heap_fragment  = 1024*8;
        break;
    case GNB_MEMORY_SCALE_SMALL:
        conf->payload_block_size = 1024*16;
        conf->max_heap_fragment  = 1024*16;
        break;
    case GNB_MEMORY_SCALE_LARGE:
        conf->payload_block_size = 1024*32;
        conf->max_heap_fragment  = 1024*32;
        break;
    case GNB_MEMORY_SCALE_HUGE:
        conf->payload_block_size = 1024*64;
        conf->max_heap_fragment  = 1024*64;
        break;
    default:
        conf->payload_block_size = 1024*4;
        conf->max_heap_fragment  = 1024*8;
        break;

    }

}


size_t gnb_get_node_num_from_file(gnb_conf_t *conf){

    size_t n = 0;

    FILE *file;

    gnb_uuid_t uuid64;

    char tun_ipv4_string[INET_ADDRSTRLEN+1];
    char tun_netmask_string[INET_ADDRSTRLEN+1];

    char route_file[PATH_MAX+NAME_MAX];
    snprintf(route_file, PATH_MAX+NAME_MAX, "%s/%s", conf->conf_dir, "route.conf");

    file = fopen(route_file,"r");

    if ( NULL == file ) {
        printf("miss route.conf\n");
        exit(1);
    }

    char line_buffer[1024+1];

    int num;

    do{

        num = fscanf(file,"%1024s\n",line_buffer);

        if ( EOF == num ) {
            break;
        }

        if ( '#' == line_buffer[0] ) {
            continue;
        }

        if ( NULL != check_node_route(line_buffer) ) {
            continue;
        }

        num = sscanf(line_buffer,"%llu|%16[^|]|%16[^|]",
                &uuid64,
                tun_ipv4_string,
                tun_netmask_string
        );

        if ( 3 != num ) {
            continue;
        }

        n++;

    }while(1);

    fclose(file);

    return n;
}



static void load_route_config(gnb_core_t *gnb_core){

    int ret;

    FILE *file;

    char route_file[PATH_MAX+NAME_MAX];

    gnb_uuid_t uuid64;
    uint32_t tun_addr4;
    uint32_t tun_subnet_addr4;
    uint32_t tun_netmask_addr4;

    char tun_ipv4_string[INET_ADDRSTRLEN+1];
    char tun_netmask_string[INET_ADDRSTRLEN+1];

    char tun_ipv6_string[INET6_ADDRSTRLEN+1];

    int num;

    gnb_node_t *node;

    snprintf(route_file, PATH_MAX+NAME_MAX, "%s/%s", gnb_core->conf->conf_dir, "route.conf");

    file = fopen(route_file,"r");

    if ( NULL == file ) {
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "miss route.conf\n");
        exit(1);
    }

    char line_buffer[1024+1];

    char netmask_class;

    gnb_core->node_nums = 0;

    do{

        num = fscanf(file,"%1024s\n",line_buffer);

        if ( EOF == num ) {
            break;
        }

        if ( '#' == line_buffer[0] ) {
            continue;
        }

        if ( NULL != check_node_route(line_buffer) ) {
            continue;
        }

        ret = gnb_test_field_separator(line_buffer);

        if ( GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH == ret ) {
            num = sscanf(line_buffer,"%llu/%16[^/]/%16[^/]", &uuid64, tun_ipv4_string, tun_netmask_string);
        } else if ( GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL == ret ) {
            num = sscanf(line_buffer,"%llu|%16[^|]|%16[^|]", &uuid64, tun_ipv4_string, tun_netmask_string);
        } else {
            num = 0;
        }

        if ( 3 != num ) {
            continue;
        }

        node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, uuid64);

        if ( NULL==node ) {
            node = gnb_node_init(gnb_core, uuid64);
            GNB_HASH32_UINT64_SET(gnb_core->uuid_node_map, uuid64, node);
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

    fclose(file);

}


static void set_node_route(gnb_core_t *gnb_core, gnb_uuid_t uuid64, char *relay_nodeids_string){

    char *p;

    char *endptr;

    gnb_node_t *node;

    node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, uuid64);

    if ( NULL==node ) {
        return;
    }

    int line;

    for( line=0; line<GNB_MAX_NODE_ROUTE; line++ ) {

        if ( 0 == node->route_node[line][0] ) {
            break;
        }

    }

    if ( GNB_MAX_NODE_ROUTE == line ) {
        return;
    }

    gnb_uuid_t relay_nodeid;

    p = relay_nodeids_string;

    int row;

    for ( row = 0; row < GNB_MAX_NODE_RELAY; row++ ) {

        relay_nodeid = strtoull(p, &endptr, 10);

        if ( NULL == endptr ) {
            break;
        }

        if ( 0 == relay_nodeid ) {
            break;
        }

        node->route_node[line][row] = relay_nodeid;
        node->route_node_ttls[line]++;

        if ( '\0' == *endptr ) {
            break;
        }

        p = endptr;
        p++;

    }

}


static void set_node_route_mode(gnb_core_t *gnb_core, gnb_uuid_t uuid64, char *route_mode_string){

    gnb_node_t *node;

    node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, uuid64);

    if ( NULL==node ) {
        return;
    }

    node->node_relay_mode = GNB_NODE_RELAY_DISABLE;

    if ( NULL != strstr(route_mode_string,"force") ) {
        node->node_relay_mode &= ~GNB_NODE_RELAY_AUTO;
        node->node_relay_mode |= GNB_NODE_RELAY_FORCE;
    }

    if ( NULL != strstr(route_mode_string,"auto") ) {
        node->node_relay_mode &= ~GNB_NODE_RELAY_FORCE;
        node->node_relay_mode |= GNB_NODE_RELAY_AUTO;
    }

    if ( NULL != strstr(route_mode_string,"balance") ) {
        node->node_relay_mode |= GNB_NODE_RELAY_BALANCE;
    }

    if ( NULL != strstr(route_mode_string,"static") ) {
        node->node_relay_mode |= GNB_NODE_RELAY_STATIC;
    }

}


static void load_route_node_config(gnb_core_t *gnb_core){

    int ret;

    FILE *file;

    char route_file[PATH_MAX+NAME_MAX];

    gnb_uuid_t uuid64;
    char row2_string[512+1];

    gnb_conf_t *conf = gnb_core->conf;

    snprintf(route_file, PATH_MAX+NAME_MAX, "%s/%s", conf->conf_dir, "route.conf");

    file = fopen(route_file,"r");

    if ( NULL == file ) {
        printf("miss route.conf\n");
        exit(1);
    }

    char line_buffer[1024+1];

    int num;

    do{

        num = fscanf(file,"%1024s\n",line_buffer);

        if ( EOF == num ) {
            break;
        }

        if ( '#' == line_buffer[0] ) {
            continue;
        }

        if ( NULL == check_node_route(line_buffer) ) {
            continue;
        }

        ret = gnb_test_field_separator(line_buffer);

        if ( GNB_CONF_FIELD_SEPARATOR_TYPE_SLASH == ret ) {
            num = sscanf(line_buffer,"%llu/%512s]", &uuid64, row2_string);
        } else if ( GNB_CONF_FIELD_SEPARATOR_TYPE_VERTICAL == ret ) {
            num = sscanf(line_buffer,"%llu|%512s]", &uuid64, row2_string);
        } else {
            num = 0;
        }

        if ( 2 != num ) {
            continue;
        }

        if ( row2_string[0] < '0' || row2_string[0] > '9' ) {
            set_node_route_mode(gnb_core, uuid64, row2_string);
        } else {
            set_node_route(gnb_core, uuid64, row2_string);
        }

    }while(1);

    fclose(file);

}


void gnb_config_safe(gnb_core_t *gnb_core){

    //装载local node的公私钥
    gnb_load_keypair(gnb_core);

    load_route_config(gnb_core);

    gnb_core->ctl_block->node_zone->node_num = gnb_core->node_nums;

    gnb_init_node_key512(gnb_core);

    gnb_core->local_node = GNB_HASH32_UINT64_GET_PTR(gnb_core->uuid_node_map, gnb_core->conf->local_uuid);

    if ( NULL==gnb_core->local_node ) {
        printf("miss local_node[%llu] is NULL\n", gnb_core->conf->local_uuid);
        exit(1);
    }

    gnb_core->local_node->udp_sockaddr6.sin6_port = htons(gnb_core->conf->udp6_ports[0]);
    gnb_core->local_node->udp_sockaddr4.sin_port  = htons(gnb_core->conf->udp4_ports[0]);

    gnb_core->local_node->tun_sin_port4 = htons(gnb_core->conf->udp4_ports[0]);


#if 0
    //这里需要确保 tun_subnet_addr4 至少是一个C段，这样设定路由的时候，数据才会到 tun
    if( 0 == strncmp(gnb_core->local_node->tun_ipv4_netmask, "255.255.255.255", INET_ADDRSTRLEN) ) {
        snprintf(gnb_core->local_node->tun_ipv4_netmask,INET_ADDRSTRLEN,"%s", "255.255.255.0");
    }
#endif

    load_route_node_config(gnb_core);

    //加载 address.conf

    address_file_config(gnb_core);

    //加载 node_cache.dump
    if ( '\0' != gnb_core->conf->node_cache_file[0] ) {
        load_node_cache(gnb_core);
    }

}
