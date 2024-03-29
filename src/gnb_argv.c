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
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "gnb.h"
#include "gnb_address_type.h"
#include "gnb_keys.h"
#include "gnb_dir.h"
#include "gnb_arg_list.h"
#include "gnb_config_lite.h"

extern gnb_conf_ext_lite_t gnb_conf_ext_lite;

void show_description();

static void show_useage(int argc,char *argv[]);

int check_listen_string(char *listen_string);
void gnb_setup_listen_addr_port(char *listen_address6_string, uint16_t *port_ptr, char *sockaddress_string, int addr_type);
void gnb_setup_es_argv(char *es_argv_string);


#define GNB_OPT_INIT                   0x91

#define SET_ADDR_SECURE                (GNB_OPT_INIT + 1)

#define SET_DIRECT_FORWARDING          (GNB_OPT_INIT + 5)
#define SET_MTU                        (GNB_OPT_INIT + 6)

#define SET_CRYPTO_TPYE                (GNB_OPT_INIT + 7)
#define SET_CRYPTO_KEY_UPDATE_INTERVAL (GNB_OPT_INIT + 8)

#define SET_MULTI_INDEX_TYPE           (GNB_OPT_INIT + 9)
#define SET_MULTI_FORWARD_TYPE         (GNB_OPT_INIT + 10)

#define SET_MULTI_SOCKET               (GNB_OPT_INIT + 11)

#define SET_SOCKET_IF_NAME             (GNB_OPT_INIT + 12)

#define SET_NODE_WORKER_QUEUE          (GNB_OPT_INIT + 13)
#define SET_INDEX_WORKER_QUEUE         (GNB_OPT_INIT + 14)
#define SET_INDEX_SERVICE_WORKER_QUEUE (GNB_OPT_INIT + 15)
#define SET_PACKET_FILTER_WORKER_QUEUE (GNB_OPT_INIT + 16)

#define SET_PORT_DETECT                (GNB_OPT_INIT + 17)

#define SET_DETECT_INTERVAL            (GNB_OPT_INIT + 18)

#define SET_PID_FILE                   (GNB_OPT_INIT + 20)
#define SET_NODE_CACHE_FILE            (GNB_OPT_INIT + 21)

#define SET_LOG_FILE_PATH              (GNB_OPT_INIT + 22)
#define SET_LOG_UDP6                   (GNB_OPT_INIT + 23)
#define SET_LOG_UDP4                   (GNB_OPT_INIT + 24)
#define SET_LOG_UDP_TYPE               (GNB_OPT_INIT + 25)

#define SET_CONSOLE_LOG_LEVEL          (GNB_OPT_INIT + 26)
#define SET_FILE_LOG_LEVEL             (GNB_OPT_INIT + 27)
#define SET_UDP_LOG_LEVEL              (GNB_OPT_INIT + 28)

#define SET_CORE_LOG_LEVEL             (GNB_OPT_INIT + 29)
#define SET_PF_LOG_LEVEL               (GNB_OPT_INIT + 30)
#define SET_MAIN_LOG_LEVEL             (GNB_OPT_INIT + 31)

#define SET_NODE_LOG_LEVEL             (GNB_OPT_INIT + 32)
#define SET_INDEX_LOG_LEVEL            (GNB_OPT_INIT + 33)
#define SET_INDEX_SERVICE_LOG_LEVEL    (GNB_OPT_INIT + 34)
#define SET_DETECT_LOG_LEVEL           (GNB_OPT_INIT + 35)

#define SET_IF_DUMP                    (GNB_OPT_INIT + 36)
#define SET_PF_TRACE                   (GNB_OPT_INIT + 37)
#define SET_PF_ROUTE                   (GNB_OPT_INIT + 38)

#define SET_TUN                        (GNB_OPT_INIT + 40)
#define SET_INDEX_WORKER               (GNB_OPT_INIT + 41)
#define SET_INDEX_SERVICE_WORKER       (GNB_OPT_INIT + 42)
#define SET_DETECT_WORKER              (GNB_OPT_INIT + 43)

#define SET_UR0                        (GNB_OPT_INIT + 44)
#define SET_UR1                        (GNB_OPT_INIT + 45)

#define SET_SYSTEMD_DAEMON             (GNB_OPT_INIT + 46)

#define SET_IF_DRV                     (GNB_OPT_INIT + 47)

#define SET_PF_WORKER_NUM              (GNB_OPT_INIT + 48)

#define SET_ZIP                        (GNB_OPT_INIT + 49)
#define SET_ZIP_LEVEL                  (GNB_OPT_INIT + 50)


#define SET_MEMORY_SCALE               (GNB_OPT_INIT + 51)

gnb_arg_list_t *gnb_es_arg_list;

int is_self_test = 0;
int is_verbose   = 0;
int is_trace     = 0;

gnb_conf_t* gnb_argv(int argc,char *argv[]){

    gnb_conf_t *conf;

    uint16_t port_host;

    char *ctl_block_file = NULL;

    int ret;

    int num;

    conf = malloc( sizeof(gnb_conf_t) );

    memset(conf,0,sizeof(gnb_conf_t));

    memset(&gnb_conf_ext_lite,0,sizeof(gnb_conf_ext_lite_t));

    gnb_es_arg_list = gnb_arg_list_init(32);

    char listen_sockaddress4_string[GNB_IP4_PORT_STRING_SIZE];
    memset(listen_sockaddress4_string,0,GNB_IP4_PORT_STRING_SIZE);

    char listen_sockaddress6_string[GNB_IP6_PORT_STRING_SIZE];
    memset(listen_sockaddress6_string,0,GNB_IP6_PORT_STRING_SIZE);

    conf->addr_secure = 1;

    conf->activate_tun                  = 1;
    conf->activate_node_worker          = 1;
    conf->activate_index_worker         = 1;
    conf->activate_index_service_worker = 1;
    conf->activate_detect_worker        = 1;

    conf->pf_worker_num = 0;

    conf->direct_forwarding  = 1;

    conf->unified_forwarding = GNB_UNIFIED_FORWARDING_AUTO;

    conf->universal_relay0   = 1;

    /*
    IPv4最小MTU=576bytes
    IPv6最小MTU=1280bytes
    如果 MTU 小于 1280 将无法设置 ipv6 地址
    ipv4 的MTU设为532是因为减掉 payload 的首部占用的字节
    */
    conf->mtu            = 1280;

    conf->pf_bits |= GNB_PF_BITS_CRYPTO_XOR;

    conf->crypto_key_update_interval = GNB_CRYPTO_KEY_UPDATE_INTERVAL_NONE;

    conf->crypto_passcode[0] = 0x07;
    conf->crypto_passcode[1] = 0x81;
    conf->crypto_passcode[2] = 0x07;
    conf->crypto_passcode[3] = 0x9d;

    conf->zip_level = 0;

    conf->memory = GNB_MEMORY_SCALE_TINY;

    conf->multi_index_type   = GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE;
    conf->multi_forward_type = GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE;

    conf->if_dump = 0;

    conf->log_udp_type = GNB_LOG_UDP_TYPE_BINARY;

    conf->console_log_level = GNB_LOG_LEVEL_UNSET;
    conf->file_log_level    = GNB_LOG_LEVEL_UNSET;
    conf->udp_log_level     = GNB_LOG_LEVEL_UNSET;

    conf->core_log_level    = GNB_LOG_LEVEL_UNSET;
    conf->pf_log_level      = GNB_LOG_LEVEL_UNSET;
    conf->main_log_level    = GNB_LOG_LEVEL_UNSET;
    conf->node_log_level    = GNB_LOG_LEVEL_UNSET;
    conf->index_log_level   = GNB_LOG_LEVEL_UNSET;
    conf->index_service_log_level = GNB_LOG_LEVEL_UNSET;
    conf->detect_log_level  = GNB_LOG_LEVEL_UNSET;

    conf->udp_socket_type = GNB_ADDR_TYPE_IPV4 | GNB_ADDR_TYPE_IPV6;


    conf->pf_woker_in_queue_length  = 0xFFF;
    conf->pf_woker_out_queue_length = 0xFFF;
    conf->node_woker_queue_length   = 0xFF;
    conf->index_woker_queue_length  = 0xFF;
    conf->index_service_woker_queue_length = 0xFF;


    conf->udp6_socket_num = 1;
    conf->udp4_socket_num = 1;

    conf->port_detect_range = DETECT_PORT_RANGE;
    conf->port_detect_start = DETECT_PORT_START;
    conf->port_detect_end   = DETECT_PORT_END;

    conf->address_detect_interval_usec = GNB_ADDRESS_DETECT_INTERVAL_USEC;
    conf->full_detect_interval_sec     = GNB_FULL_DETECT_INTERVAL_SEC;

    conf->daemon = 0;
    conf->systemd_daemon = 0;

    conf->if_drv = GNB_IF_DRV_TYPE_DEFAULT;

    #if defined(__FreeBSD__)
    snprintf(conf->ifname,NAME_MAX,"%s","tun0");
    #endif


    #if defined(__APPLE__)
    snprintf(conf->ifname,NAME_MAX,"%s","utun1");
    #endif

    #if defined(__OpenBSD__)
    snprintf(conf->ifname,NAME_MAX,"%s","tun0");
    #endif

    #if defined(__linux__)
    snprintf(conf->ifname,NAME_MAX,"%s","gnb_tun");
    #endif

    #if defined(_WIN32)
    snprintf(conf->ifname,NAME_MAX,"%s","gnb_tun");
    conf->if_drv = GNB_IF_DRV_TYPE_TAP_WINDOWS;
    #endif

    char *binary_dir;
    binary_dir = gnb_get_file_dir(argv[0], conf->binary_dir);

    if ( NULL==binary_dir ) {
        //这种情况几乎不会发生
        #ifdef __UNIX_LIKE_OS__
        snprintf(conf->binary_dir, PATH_MAX, "%s", "/tmp");
        #endif

        #ifdef _WIN32
        snprintf(conf->binary_dir, PATH_MAX+NAME_MAX, "%s", "c:\\");
        #endif
    }

    #ifdef __UNIX_LIKE_OS__
    char gnb_es_bin_path[PATH_MAX+NAME_MAX];
    snprintf(gnb_es_bin_path,   PATH_MAX+NAME_MAX, "%s/gnb_es",          conf->binary_dir);
    gnb_arg_append(gnb_es_arg_list, gnb_es_bin_path);
    #endif

    #ifdef _WIN32
    char gnb_es_bin_path_q[PATH_MAX+NAME_MAX];
    snprintf(gnb_es_bin_path_q, PATH_MAX+NAME_MAX, "\"%s\\gnb_es.exe\"",  conf->binary_dir);
    gnb_arg_append(gnb_es_arg_list, gnb_es_bin_path_q);
    #endif

    int flag;

    struct option long_options[] = {

      { "conf",     required_argument, 0, 'c' },
      { "nodeid",   required_argument, 0, 'n' },

      { "public-index-service",  no_argument, 0, 'P' },

      { "index-address",  required_argument, 0, 'I' },
      { "node-address",   required_argument, 0, 'a' },
      { "node-route",     required_argument, 0, 'r' },

      { "ifname",   required_argument, 0, 'i' },
      { "if-drv",   required_argument, 0, SET_IF_DRV },
      { "mtu",      required_argument, 0, SET_MTU },
      { "crypto",   required_argument, 0, SET_CRYPTO_TPYE },
      { "passcode", required_argument,  0, 'p' },
      { "crypto-key-update-interval", required_argument,  0, SET_CRYPTO_KEY_UPDATE_INTERVAL },

      { "zip",       required_argument,  0, SET_ZIP },
      { "zip-level", required_argument,  0, SET_ZIP_LEVEL },

      { "memory",    required_argument,  0, SET_MEMORY_SCALE },

      { "multi-index-type",    required_argument,  0, SET_MULTI_INDEX_TYPE },
      { "multi-forward-type",  required_argument,  0, SET_MULTI_FORWARD_TYPE },
      { "socket-if-name",      required_argument,  0, SET_SOCKET_IF_NAME },
      { "address-secure",      required_argument,  0, SET_ADDR_SECURE },

      { "listen",              required_argument,  0, 'l' },
      { "ctl-block",           required_argument,  0, 'b' },
      { "if-dump",             required_argument,  0, SET_IF_DUMP },

      { "ipv4-only", no_argument,   0, '4'},
      { "ipv6-only", no_argument,   0, '6'},

      { "daemon",    no_argument,   0, 'd' },

      { "quiet",     no_argument,   0, 'q' },
      { "selftest",  no_argument,   0, 't' },
      { "verbose",   no_argument,   0, 'V' },
      { "trace",     no_argument,   0, 'T' },

#if defined(__linux__)
      { "systemd",   no_argument,   0, SET_SYSTEMD_DAEMON },
#endif

      { "es-argv",                   required_argument,  0,   'e' },

      { "node-woker-queue",          required_argument,  0, SET_NODE_WORKER_QUEUE },
      { "index-woker-queue",         required_argument,  0, SET_INDEX_WORKER_QUEUE },
      { "index-woker-service-queue", required_argument,  0, SET_INDEX_SERVICE_WORKER_QUEUE },
      { "packet-filter-woker-queue", required_argument,  0, SET_PACKET_FILTER_WORKER_QUEUE },
    
      { "port-detect",               required_argument,  0, SET_PORT_DETECT },

      { "detect-interval",          required_argument,  0, SET_DETECT_INTERVAL },

      { "set-tun",                   required_argument,  0, SET_TUN },
      { "index-worker",              required_argument,  0, SET_INDEX_WORKER },
      { "index-service-worker",      required_argument,  0, SET_INDEX_SERVICE_WORKER },
      { "node-detect-worker",        required_argument,  0, SET_DETECT_WORKER },

      { "pf-worker",                 required_argument,  0, SET_PF_WORKER_NUM },

      { "multi-socket",              required_argument,  0,  SET_MULTI_SOCKET },
      { "set-ur0",                   required_argument,  0,  SET_UR0 },

      { "pf-route",                  required_argument,  0, SET_PF_ROUTE },
      { "unified-forwarding",        required_argument,  0, 'U' },
      { "direct-forwarding",         required_argument,  0, SET_DIRECT_FORWARDING },
      { "pid-file",                  required_argument,  0, SET_PID_FILE },
      { "node-cache-file",           required_argument,  0, SET_NODE_CACHE_FILE },

      { "log-file-path",             required_argument,  0,     SET_LOG_FILE_PATH },
      { "log-udp6",                  optional_argument,  &flag, SET_LOG_UDP6 },
      { "log-udp4",                  optional_argument,  &flag, SET_LOG_UDP4 },

      { "log-udp-type",              required_argument,  0,   SET_LOG_UDP_TYPE },

      { "console-log-level",         required_argument,  0,   SET_CONSOLE_LOG_LEVEL },
      { "file-log-level",            required_argument,  0,   SET_FILE_LOG_LEVEL },
      { "udp-log-level",             required_argument,  0,   SET_UDP_LOG_LEVEL },

      { "core-log-level",            required_argument,  0,   SET_CORE_LOG_LEVEL },
      { "pf-log-level",              required_argument,  0,   SET_PF_LOG_LEVEL },
      { "main-log-level",            required_argument,  0,   SET_MAIN_LOG_LEVEL },
      { "node-log-level",            required_argument,  0,   SET_NODE_LOG_LEVEL },
      { "index-log-level",           required_argument,  0,   SET_INDEX_LOG_LEVEL },
      { "index-service-log-level",   required_argument,  0,   SET_INDEX_SERVICE_LOG_LEVEL },
      { "node-detect-log-level",     required_argument,  0,   SET_DETECT_LOG_LEVEL },      

      { "help",     no_argument, 0, 'h' },

      { 0, 0, 0, 0 }

    };

    int opt;

    int option_index = 0;

    while (1) {

        opt = getopt_long(argc, argv, "c:n:a:r:PI:b:l:i:46dp:e:tqVTh",long_options, &option_index);

        if ( -1 == opt ) {
            break;
        }

        switch (opt) {

        case 'c':
            snprintf(conf->conf_dir, PATH_MAX, "%s", optarg);
            break;

        case 'P':
            conf->public_index_service = 1;
            break;

        case 'I':
            gnb_conf_ext_lite.index_address_string = optarg;
            break;

        case 'a':
            gnb_conf_ext_lite.node_address_string = optarg;
            break;

        case 'r':
            gnb_conf_ext_lite.node_route_string = optarg;
            break;

        case 'n':
            conf->local_uuid = (uint16_t)strtoul(optarg, NULL, 10);
            conf->lite_mode = 1;
            break;

        case 'i':

            #if defined(__linux__) ||  defined(_WIN32)
            snprintf(conf->ifname, 16, "%s", optarg);
            conf->ifname[15] = '\0';
            #endif

            break;

        case SET_IF_DRV:

            if ( !strncmp(optarg, "wintun", sizeof("wintun")-1) ) {
                conf->if_drv = GNB_IF_DRV_TYPE_TAP_WINTUN;
            } else if ( !strncmp(optarg, "tap-windows", sizeof("tap-windows")-1) ) {
                conf->if_drv = GNB_IF_DRV_TYPE_TAP_WINDOWS;
            } else {
                conf->if_drv = GNB_IF_DRV_TYPE_DEFAULT;
            }

            break;

        case 'l':

            ret = check_listen_string(optarg);

            if ( 0 == ret ) {
                port_host = (uint16_t)strtoul(optarg, NULL, 10);
                snprintf(listen_sockaddress6_string, GNB_IP6_PORT_STRING_SIZE, "[:::%d]",    port_host);
                snprintf(listen_sockaddress4_string, GNB_IP4_PORT_STRING_SIZE, "0.0.0.0:%d", port_host);
            } else if ( 4 == ret ) {
                snprintf(listen_sockaddress4_string, GNB_IP4_PORT_STRING_SIZE, "%s", optarg);
            } else if( 6 == ret ) {
                snprintf(listen_sockaddress6_string, GNB_IP6_PORT_STRING_SIZE, "%s", optarg);
            }

            break;

        case 'b':
            ctl_block_file = optarg;
            break;

        case '4':
            conf->udp_socket_type = GNB_ADDR_TYPE_IPV4;
            break;

        case '6':
            conf->udp_socket_type = GNB_ADDR_TYPE_IPV6;
            break;

        case 'd':
            conf->daemon = 1;
            break;

        case 'p':
            gnb_build_passcode(conf->crypto_passcode, optarg);
            break;

        case 'q':
            conf->quiet = 1;
            break;

        case 't':
            is_self_test = 1;
            break;
        case 'V':
            is_verbose = 1;
            break;

        case 'T':
            is_trace = 1;
            break;

        case SET_SYSTEMD_DAEMON:
            conf->systemd_daemon = 1;
            break;

        case SET_NODE_WORKER_QUEUE:
            conf->node_woker_queue_length = (uint16_t)strtoul(optarg, NULL, 10);
            break;

        case SET_INDEX_WORKER_QUEUE:
            conf->index_woker_queue_length = (uint16_t)strtoul(optarg, NULL, 10);
            break;

        case SET_INDEX_SERVICE_WORKER_QUEUE:
            conf->index_service_woker_queue_length = (uint16_t)strtoul(optarg, NULL, 10);
            break;

        case SET_PORT_DETECT:

            num = sscanf(optarg, "%hu,%hu,%hu", &conf->port_detect_range, &conf->port_detect_start, &conf->port_detect_end);

            if ( 3 != num ) {
                conf->port_detect_range = DETECT_PORT_RANGE;
                conf->port_detect_start = DETECT_PORT_START;
                conf->port_detect_end   = DETECT_PORT_END;
            }

            break;

        case SET_DETECT_INTERVAL:

            num = sscanf(optarg, "%u,%u", &conf->address_detect_interval_usec, &conf->full_detect_interval_sec);

            if ( 2 != num ) {
                conf->address_detect_interval_usec = GNB_ADDRESS_DETECT_INTERVAL_USEC;
                conf->full_detect_interval_sec     = GNB_FULL_DETECT_INTERVAL_SEC;
            }
        
            break;

        case SET_MTU:
            conf->mtu = (unsigned int)strtoul(optarg, NULL, 10);
            break;

        case SET_LOG_FILE_PATH:
            snprintf(conf->log_path , PATH_MAX, "%s", optarg);
            break;

        case SET_LOG_UDP_TYPE:

            if ( !strncmp(optarg, "binary", 16) ) {
                conf->log_udp_type = GNB_LOG_UDP_TYPE_BINARY;
            } else {
                conf->log_udp_type = GNB_LOG_UDP_TYPE_TEXT;
            }

            break;

        case SET_CONSOLE_LOG_LEVEL:
            conf->console_log_level = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_FILE_LOG_LEVEL:
            conf->file_log_level    = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_UDP_LOG_LEVEL:
            conf->udp_log_level     = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_CORE_LOG_LEVEL:
            conf->core_log_level    = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_PF_LOG_LEVEL:
            conf->pf_log_level      = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_MAIN_LOG_LEVEL:
            conf->main_log_level    = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_NODE_LOG_LEVEL:
            conf->node_log_level    = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_INDEX_LOG_LEVEL:
            conf->index_log_level   = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_INDEX_SERVICE_LOG_LEVEL:
            conf->index_service_log_level   = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_DETECT_LOG_LEVEL:
            conf->detect_log_level  = (uint8_t)strtoul(optarg, NULL, 10);
            break;

        case SET_CRYPTO_TPYE:

            if ( !strncmp(optarg, "none", 16) ) {
                conf->pf_bits &= ~(GNB_PF_BITS_CRYPTO_XOR | GNB_PF_BITS_CRYPTO_ARC4);
            } else if ( !strncmp(optarg, "xor", 16) ) {
                conf->pf_bits |= GNB_PF_BITS_CRYPTO_XOR;
            } else if ( !strncmp(optarg, "arc4", 16) ) {
                conf->pf_bits |= GNB_PF_BITS_CRYPTO_ARC4;
            } else {
                conf->pf_bits |= GNB_PF_BITS_CRYPTO_XOR;
            }

            break;

        case SET_ZIP:

            if ( !strncmp(optarg, "auto", sizeof("auto")-1) ) {
                conf->zip = GNB_ZIP_AUTO;
            } else if ( !strncmp(optarg, "force", sizeof("force")-1) ) {
                conf->zip = GNB_ZIP_FORCE;
            } else {
                conf->zip = GNB_ZIP_AUTO;
            }

            break;

        case SET_ZIP_LEVEL:

            conf->zip_level = (int8_t)strtoul(optarg, NULL, 10);

            if ( conf->zip_level > 9 ) {
                conf->zip_level = 9;
            }

            if ( conf->zip_level < -1 ) {
                conf->zip_level = -1;
            }

            break;

        case SET_MEMORY_SCALE:

            if ( !strncmp(optarg, "tiny", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_TINY;
            } else if ( !strncmp(optarg, "small", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_SMALL;
            } else if ( !strncmp(optarg, "large", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_LARGE;
            } else if ( !strncmp(optarg, "huge", 16) ) {
                conf->memory = GNB_MEMORY_SCALE_HUGE;
            } else {
                conf->memory = GNB_MEMORY_SCALE_TINY;
            }
        
            break;

        case SET_MULTI_INDEX_TYPE:

            if ( !strncmp(optarg, "simple-fault-tolerant", 16) ) {
                conf->multi_index_type = GNB_MULTI_ADDRESS_TYPE_SIMPLE_FAULT_TOLERANT;
            } else if ( !strncmp(optarg, "simple-load-balance", 16) ) {
                conf->multi_index_type = GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE;
            } else if ( !strncmp(optarg, "full", 16) ) {
                conf->multi_index_type = GNB_MULTI_ADDRESS_TYPE_FULL;
            } else {
                conf->multi_index_type = GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE;
            }

            break;

        case SET_MULTI_FORWARD_TYPE:

            if ( !strncmp(optarg, "simple-fault-tolerant", sizeof("simple-fault-tolerant")-1) ) {
                conf->multi_forward_type = GNB_MULTI_ADDRESS_TYPE_SIMPLE_FAULT_TOLERANT;
            } else if ( !strncmp(optarg, "simple-load-balance", sizeof("simple-load-balance")-1) ) {
                conf->multi_forward_type = GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE;
            } else {
                conf->multi_forward_type = GNB_MULTI_ADDRESS_TYPE_SIMPLE_FAULT_TOLERANT;
            }

            break;

        case SET_CRYPTO_KEY_UPDATE_INTERVAL:

            if ( !strncmp(optarg, "hour", sizeof("hour")-1 ) ) {
                conf->crypto_key_update_interval  = GNB_CRYPTO_KEY_UPDATE_INTERVAL_HOUR;
            } else if ( !strncmp(optarg, "minute", sizeof("minute")-1) ) {
                conf->crypto_key_update_interval = GNB_CRYPTO_KEY_UPDATE_INTERVAL_MINUTE;
            } else if ( !strncmp(optarg, "none", sizeof("none")-1) ) {
                conf->crypto_key_update_interval = GNB_CRYPTO_KEY_UPDATE_INTERVAL_NONE;
            } else {
                conf->crypto_key_update_interval = GNB_CRYPTO_KEY_UPDATE_INTERVAL_NONE;
            }

            break;

        case SET_MULTI_SOCKET:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->multi_socket = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->multi_socket = 0;
            } else {
                conf->multi_socket = 0;
            }

            break;

        case SET_PF_ROUTE:

            snprintf(conf->pf_route, NAME_MAX, "%s", optarg);

            break;

        case SET_TUN:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->activate_tun  = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->activate_tun  = 0;
            } else {
                conf->activate_tun  = 1;
            }

            break;

        case SET_INDEX_WORKER:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->activate_index_worker = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->activate_index_worker = 0;
            } else {
                conf->activate_index_worker = 1;
            }

            break;

        case SET_INDEX_SERVICE_WORKER:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->activate_index_service_worker = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->activate_index_service_worker = 0;
            } else {
                conf->activate_index_service_worker = 1;
            }

            break;

        case SET_DETECT_WORKER:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->activate_detect_worker = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->activate_detect_worker = 0;
            } else {
                conf->activate_detect_worker = 1;
            }

            break;

        case SET_PF_WORKER_NUM:

            conf->pf_worker_num = (unsigned int)strtoul(optarg, NULL, 10);

            break;

        case SET_UR0:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->universal_relay0 = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->universal_relay0 = 0;
            } else {
                conf->universal_relay0 = 1;
            }

            break;

        case 'U':

            if ( !strncmp(optarg, "off", sizeof("off")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_OFF;
            } else if ( !strncmp(optarg, "auto", sizeof("auto")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_AUTO;
            } else if ( !strncmp(optarg, "force", sizeof("force")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_FORCE;
            } else if ( !strncmp(optarg, "super", sizeof("super")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_SUPER;
            }else if ( !strncmp(optarg, "hyper", sizeof("hyper")-1) ) {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_HYPER;
            } else {
                conf->unified_forwarding = GNB_UNIFIED_FORWARDING_AUTO;
            }

            break;

        case SET_DIRECT_FORWARDING:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->direct_forwarding = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->direct_forwarding = 0;
            } else {
                conf->direct_forwarding = 1;
            }

            break;

        case SET_ADDR_SECURE:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->addr_secure = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->addr_secure = 0;
            } else {
                conf->addr_secure = 1;
            }

            break;

        case SET_IF_DUMP:

            if ( !strncmp(optarg, "on", 2) ) {
                conf->if_dump = 1;
            } else if ( !strncmp(optarg, "off", 3) ) {
                conf->if_dump = 0;
            } else {
                conf->if_dump = 0;
            }

            break;

        case SET_SOCKET_IF_NAME:
            snprintf(conf->socket_ifname, 16, "%s", optarg);
            break;

        case SET_PID_FILE:
            snprintf(conf->pid_file, PATH_MAX, "%s", optarg);
            break;

        case SET_NODE_CACHE_FILE:
            snprintf(conf->node_cache_file, PATH_MAX, "%s", optarg);
            break;

        case 'e':
            gnb_setup_es_argv(optarg);
            break;

        case 'h':
            show_useage(argc, argv);
            exit(0);
            break;
        default:
            break;
        }


        if ( 0 == opt ) {

            switch (flag) {

            case SET_LOG_UDP4:

                if ( NULL != optarg ) {
                    snprintf(conf->log_udp_sockaddress4_string, GNB_IP4_PORT_STRING_SIZE, "%s", optarg);
                } else {
                    snprintf(conf->log_udp_sockaddress4_string, GNB_IP4_PORT_STRING_SIZE, "%s", "127.0.0.1:8666");
                }

                break;

            default:
                break;

            }

            continue;

        }

    }


    if ( 0 != conf->systemd_daemon ) {
        conf->daemon = 0;
    }

    if ( 1 == conf->public_index_service ) {
        conf->activate_tun                  = 0;
        conf->activate_node_worker          = 0;
        conf->activate_index_worker         = 0;
        conf->activate_index_service_worker = 1;
        conf->activate_detect_worker        = 0;
        conf->index_service_woker_queue_length = 0x7FF;
    }

    if ( conf->pf_worker_num > 128 ) {
        conf->pf_worker_num = 128;
    }

    if ( conf->pf_worker_num > 0 ) {
        conf->unified_forwarding = GNB_UNIFIED_FORWARDING_OFF;
    }

    gnb_addr_secure = conf->addr_secure;

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

    if ( 1 == conf->multi_socket ) {
        conf->udp6_socket_num = 1;
        conf->udp4_socket_num = 5;
    }

    if ( conf->node_woker_queue_length < GNB_WORKER_MIN_QUEUE ) {
        conf->node_woker_queue_length = GNB_WORKER_MIN_QUEUE;
    }

    if ( conf->node_woker_queue_length > GNB_WORKER_MAX_QUEUE ) {
        conf->node_woker_queue_length = GNB_WORKER_MAX_QUEUE;
    }

    if ( conf->index_woker_queue_length < GNB_WORKER_MIN_QUEUE ) {
        conf->index_woker_queue_length = GNB_WORKER_MIN_QUEUE;
    }

    if ( conf->index_woker_queue_length > GNB_WORKER_MAX_QUEUE ) {
        conf->index_woker_queue_length = GNB_WORKER_MAX_QUEUE;
    }

    if ( conf->index_service_woker_queue_length < GNB_WORKER_MIN_QUEUE ) {
        conf->index_service_woker_queue_length = GNB_WORKER_MIN_QUEUE;
    }

    if ( conf->index_service_woker_queue_length > GNB_WORKER_MAX_QUEUE ) {
        conf->index_service_woker_queue_length = GNB_WORKER_MAX_QUEUE;
    }

    if ( conf->pf_woker_in_queue_length < GNB_WORKER_MIN_QUEUE ) {
        conf->pf_woker_in_queue_length = GNB_WORKER_MIN_QUEUE;
    }

    if ( conf->pf_woker_in_queue_length > GNB_WORKER_MAX_QUEUE ) {
        conf->pf_woker_in_queue_length = GNB_WORKER_MAX_QUEUE;
    }

    conf->pf_woker_out_queue_length = conf->pf_woker_in_queue_length;

    if ( GNB_ADDR_TYPE_IPV6 == conf->udp_socket_type && conf->mtu < 1280 ) {
        conf->mtu = 1280;
    }

    if ( '\0' == conf->pf_route[0] ) {
        snprintf(conf->pf_route, NAME_MAX, "gnb_pf_route");
    }

    if ( '\0' != listen_sockaddress6_string[0] ) {
        gnb_setup_listen_addr_port(conf->listen_address6_string, &conf->udp6_ports[0], listen_sockaddress6_string, AF_INET6);
    } else {
        strncpy(conf->listen_address6_string,"::", sizeof("::")-1);
        conf->udp6_ports[0] = 9001;
    }

    if ( '\0' != listen_sockaddress4_string[0] ) {
        gnb_setup_listen_addr_port(conf->listen_address4_string, &conf->udp4_ports[0], listen_sockaddress4_string, AF_INET);
    } else {
        strncpy(conf->listen_address4_string,"0.0.0.0", sizeof("0.0.0.0")-1);
        conf->udp4_ports[0] = 9001;
    }

    if ( '\0' == conf->conf_dir[0] ) {

        if ( 0 == conf->public_index_service && 0 == conf->lite_mode ) {
            show_useage(argc, argv);
            exit(0);
        }

    }

    if ( 0 == conf->lite_mode && 0==conf->public_index_service ) {

        if ( '\0' == conf->map_file[0] ) {
            snprintf(conf->map_file, PATH_MAX+NAME_MAX, "%s/%s", conf->conf_dir, "gnb.map");
        }

        if ( '\0' == conf->pid_file[0] ) {
            snprintf(conf->pid_file, PATH_MAX+NAME_MAX,"%s/%s", conf->conf_dir, "gnb.pid");
        }

        if ( '\0' == conf->node_cache_file[0] ) {
            snprintf(conf->node_cache_file, PATH_MAX+NAME_MAX,"%s/%s", conf->conf_dir, "node_cache.dump");
        }

    } else {

        conf->conf_dir[0] = '\0';

        #ifdef __UNIX_LIKE_OS__
        snprintf(conf->pid_file,        PATH_MAX+NAME_MAX, "/tmp/gnb.%d.pid",       conf->udp4_ports[0]);
        snprintf(conf->map_file,        PATH_MAX+NAME_MAX, "/tmp/gnb.%d.map",       conf->udp4_ports[0]);
        snprintf(conf->node_cache_file, PATH_MAX+NAME_MAX, "/tmp/node_cache.%d.dump", conf->udp4_ports[0]);
        #endif

        #ifdef _WIN32
        snprintf(conf->map_file,        PATH_MAX+NAME_MAX, "%s/gnb.%d.map",         conf->binary_dir, conf->udp4_ports[0]);
        snprintf(conf->pid_file,        PATH_MAX+NAME_MAX, "%s/gnb.%d.pid",         conf->binary_dir, conf->udp4_ports[0]);
        snprintf(conf->node_cache_file, PATH_MAX+NAME_MAX, "%s/node_cache.%d.dump", conf->binary_dir, conf->udp4_ports[0]);
        #endif

    }

    if ( NULL != ctl_block_file ) {
        snprintf(conf->map_file,        PATH_MAX+NAME_MAX, "%s",       ctl_block_file);
    }

    char  resolved_path[PATH_MAX+NAME_MAX];

    if ( '\0' != conf->conf_dir[0] && NULL != gnb_realpath(conf->conf_dir,resolved_path) ) {
        strncpy(conf->conf_dir, resolved_path, PATH_MAX);
    }

    if ( NULL != gnb_realpath(conf->map_file,resolved_path) ) {
        strncpy(conf->map_file, resolved_path, PATH_MAX);
    }

    if ( NULL != gnb_realpath(conf->pid_file,resolved_path) ) {
        strncpy(conf->pid_file, resolved_path, PATH_MAX);
    }

    if ( '\0' != conf->node_cache_file[0] && NULL != gnb_realpath(conf->node_cache_file,resolved_path) ) {
        strncpy(conf->node_cache_file, resolved_path, PATH_MAX);
    }

    #ifdef __UNIX_LIKE_OS__
    gnb_arg_append(gnb_es_arg_list, "-b");
    gnb_arg_append(gnb_es_arg_list, conf->map_file);
    #endif


    #ifdef _WIN32
    char gnb_map_path_q[PATH_MAX+NAME_MAX];
    snprintf(gnb_map_path_q,    PATH_MAX+NAME_MAX, "\"%s\"",              conf->map_file);
    gnb_arg_append(gnb_es_arg_list, "-b");
    gnb_arg_append(gnb_es_arg_list, gnb_map_path_q);
    #endif

    gnb_arg_append(gnb_es_arg_list, "--if-loop");

    if ( 1 == conf->lite_mode ) {
        gnb_arg_append(gnb_es_arg_list, "--upnp");
    }

    return conf;

}


static void show_useage(int argc,char *argv[]){

    show_description();

    printf("Usage: %s [-i IFNAME] -c CONFIG_PATH [OPTION]\n", argv[0]);
    printf("Command Summary:\n");

    printf("  -c, --conf                       config path\n");
    printf("  -n, --nodeid                     nodeid\n");
    printf("  -P, --public-index-service       run as public index service\n");

    printf("  -I, --index-address              setup index address\n");
    printf("  -a, --node-address               setup node ip address\n");
    printf("  -r, --node-route                 setup node route\n");
    printf("  -i, --ifname                     TUN Device Name\n");

    printf("  -4, --ipv4-only                  Use IPv4 Only\n");
    printf("  -6, --ipv6-only                  Use IPv6 Only\n");
    printf("  -d, --daemon                     daemon\n");
    printf("  -q, --quiet                      disabled console output\n");
    printf("  -t, --selftest                   self test\n");
    printf("  -p, --passcode                   a hexadecimal string of 32-bit unsigned integer,use to strengthen safety default:0x9d078107\n");
    printf("  -U, --unified-forwarding         \"off\",\"force\",\"auto\",\"super\",\"hyper\" default:\"auto\"; cannot be used with --pf-worker\n");


    printf("  -l, --listen                     listen address default:\"0.0.0.0:9001\"\n");
    printf("  -b, --ctl-block                  ctl block mapper file\n");
    printf("  -e, --es-argv                    pass-through gnb_es argv\n");
    printf("  -V, --verbose                    verbose mode\n");
    printf("  -T, --trace                      trace mode\n");

#if defined(__linux__)
    printf("      --systemd                    systemd daemon\n");
#endif

    printf("      --node-worker-queue           node  worker queue length\n");
    printf("      --index-worker-queue          index worker queue length\n");
    printf("      --index-service-worker-queue  index service worker queue length\n");
    printf("      --packet-filter-worker-queue  packet filter worker queue length\n");

    printf("      --port-detect                node address detect port range,start,end\n");
    printf("      --detect-interval            node address detect interval default %u,%u\n", GNB_ADDRESS_DETECT_INTERVAL_USEC,GNB_FULL_DETECT_INTERVAL_SEC);

    printf("      --mtu                        TUN Device MTU ipv4:532~1500,ipv6:1280~1500\n");
    printf("      --crypto                     ip frame crypto \"xor\",\"arc4\",\"none\" default:\"xor\"\n");
    printf("      --crypto-key-update-interval crypto key update interval, \"hour\",\"minute\",none default:\"none\"\n");
    printf("      --multi-index-type           \"simple-fault-tolerant\",\"simple-load-balance\",\"full\" default:\"simple-load-balance\"\n");

    printf("      --zip                        \"auto\", \"force\" default:\"auto\"\n");
    printf("      --zip-level                  \"0\": no compression \"1\": best speed,\"9\": best compression\n");

    printf("      --multi-forward-type         \"simple-fault-tolerant\",\"simple-load-balance\" default:\"simple-fault-tolerant\"\n");

    #ifdef _WIN32
    printf("      --if-drv                     interface driver \"tap-windows\",\"wintun\" default:\"tap-windows\"\n");
    #endif

    #ifdef __UNIX_LIKE_OS__
    printf("      --socket-if-name             example: \"eth0\",\"eno1\", only for unix-like os\n");
    #endif

    printf("      --address-secure             hide part of ip address in logs \"on\",\"off\" default:\"on\"\n");
    printf("      --if-dump                    dump the interface data frame \"on\",\"off\" default:\"off\"\n");
    printf("      --pf-route                   packet filter route\n");
    printf("      --multi-socket               \"on\",\"off\" default:\"off\"\n");
    printf("      --direct-forwarding          \"on\",\"off\" default:\"on\"\n");
    printf("      --set-tun                    \"on\",\"off\" default:\"on\"\n");
    printf("      --index-worker               \"on\",\"off\" default:\"on\"\n");
    printf("      --index-service-worker       \"on\",\"off\" default:\"on\"\n");
    printf("      --node-detect-worker         \"on\",\"off\" default:\"on\"\n");

    #ifdef __UNIX_LIKE_OS__
    printf("      --pf-worker                  [0-128] number of the packet filter worker default:0; cannot be used with --unified-forwarding, only for unix-like os\n");
    #endif


    printf("      --memory                     \"tiny\",\"small\",\"large\",\"huge\" default:\"tiny\"\n");
    printf("      --set-ur0                    \"on\",\"off\" default:\"on\"\n");
    printf("      --pid-file                   pid file\n");
    printf("      --node-cache-file            node address cache file\n");
    printf("      --log-file-path              log file path\n");
    printf("      --log-udp4                   send log to the address ipv4 default:\"127.0.0.1:8666\"\n");
    printf("      --log-udp-type               log udp type \"binary\",\"text\" default:\"binary\"\n");
    printf("      --console-log-level          log console level 0-3\n");
    printf("      --file-log-level             log file level    0-3\n" );
    printf("      --udp-log-level              log udp level     0-3\n");
    printf("      --core-log-level             core log level           0-3\n");
    printf("      --pf-log-level               packet filter log level  0-3\n");
    printf("      --main-log-level             main log level           0-3\n");
    printf("      --node-log-level             node log level           0-3\n");
    printf("      --index-log-level            index log level          0-3\n");
    printf("      --index-service-log-level    index service log level  0-3\n");
    printf("      --node-detect-log-level      node detect log level    0-3\n");

    printf("      --help\n");

    printf("Example:\n");
    printf("  %s -i gnbtun -c $node_conf_dir -e \"--upnp\"\n",argv[0]);
    printf("  %s -P\n",argv[0]);
    printf("  %s -P --console-log-level=3 --index-service-log-level=3\n",argv[0]);

    printf("  %s -n 1001 -I \"$public_index_ip/$port\" --multi-socket=on -p $passcode\n",argv[0]);
    printf("  %s -n 1002 -I \"$public_index_ip/$port\" --multi-socket=on -p $passcode\n",argv[0]);

    printf("  %s -n 1001 -a \"i/0/$public_index_ip/$port\" --multi-socket=on -p $passcode\n",argv[0]);
    printf("  %s -n 1002 -a \"i/0/$public_index_ip/$port\" --multi-socket=on -p $passcode\n",argv[0]);

}

void gnb_setup_es_argv(char *es_argv_string){

    int num;
    size_t len;

    char argv_string0[1024];
    char argv_string1[1024];

    len = strlen(es_argv_string);

    if ( len >1024 ) {
        return;
    }

    num = sscanf(es_argv_string,"%256[^ ] %256s", argv_string0, argv_string1);

    if ( 1 == num ) {
        gnb_arg_append(gnb_es_arg_list, argv_string0);
    } else if ( 2 == num ) {
        gnb_arg_append(gnb_es_arg_list, argv_string0);
        gnb_arg_append(gnb_es_arg_list, argv_string1);
    }

    return;

}
