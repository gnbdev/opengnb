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
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include "gnb_platform.h"
#include "gnb_dir.h"
#include "gnb_log.h"
#include "gnb_conf_type.h"
#include "gnb_ctl_block.h"
#include "gnb_core_frame_type_defs.h"
#include "es/gnb_es_type.h"

gnb_es_ctx* gnb_es_ctx_create(int is_service, char *ctl_block_file,gnb_log_ctx_t *log);
void gnb_es_ctx_init(gnb_es_ctx *es_ctx);

int gnb_daemon();

void save_pid(const char *pid_file);


#define GNB_ES_OPT_INIT          0x91
#define OPT_UPNP                 (GNB_ES_OPT_INIT + 1)
#define OPT_RESOLV               (GNB_ES_OPT_INIT + 2)
#define OPT_BROADCAST_ADDRESS    (GNB_ES_OPT_INIT + 3)
#define OPT_DUMP_ADDRESS         (GNB_ES_OPT_INIT + 4)
#define OPT_IF_UP                (GNB_ES_OPT_INIT + 5)
#define OPT_IF_DOWN              (GNB_ES_OPT_INIT + 6)
#define OPT_IF_LOOP              (GNB_ES_OPT_INIT + 7)
#define PID_FILE                 (GNB_ES_OPT_INIT + 8)
#define WAN_ADDRESS6_FILE        (GNB_ES_OPT_INIT + 9)
#define LOG_UDP6                 (GNB_ES_OPT_INIT + 10)
#define LOG_UDP4                 (GNB_ES_OPT_INIT + 11)
#define LOG_UDP_TYPE             (GNB_ES_OPT_INIT + 12)


void gnb_start_environment_service(gnb_es_ctx *es_ctx);


static void show_useage(int argc,char *argv[]){

    printf("GNB Environment Service version 1.1.0 protocol version 1.1.3\n");

    printf("Build[%s %s]\n", __DATE__, __TIME__);

    printf("Copyright (C) 2019 gnbdev<gnbdev@qq.com>\n");

    printf("Usage: %s -b CTL_BLOCK [OPTION]\n", argv[0]);
    printf("Command Summary:\n");

    printf("  -b, --ctl-block           ctl block mapper file\n");
    printf("  -s, --service             service mode\n");
    printf("  -d, --daemon              daemon\n");
    printf("  -L, --discover-in-lan     discover in lan\n");
    printf("      --upnp                upnp\n");
    printf("      --resolv              resolv\n");
    printf("      --dump-address        dump address\n");
    printf("      --broadcast-address   broadcast address\n");

    printf("      --pid-file            pid file\n");
    printf("      --wan-address6-file   wan address6 file\n");
    printf("      --if-up               call at interface up\n");
    printf("      --if-down             call at interface down\n");
    printf("      --if-loop             call at interface loop\n");

    printf("      --log-udp4            send log to the address ipv4 default is '127.0.0.1:8666'\n");
    printf("      --log-udp-type        the log udp type 'binary' or 'text' default is 'text'\n");

    printf("      --help\n");

    printf("example:\n");
    printf("%s --ctl-block=./gnb.map -s --upnp\n",argv[0]);
    printf("%s --ctl-block=./gnb.map -s --resolv\n",argv[0]);

}


static void setup_log_ctx(gnb_log_ctx_t *log_ctx, char *log_udp_sockaddress4_string, uint8_t log_udp_type){

    int rc;

    log_ctx->output_type = GNB_LOG_OUTPUT_STDOUT;

    snprintf(log_ctx->config_table[GNB_LOG_ID_ES_CORE].log_name, 20,              "ES_CORE");
    log_ctx->config_table[GNB_LOG_ID_ES_CORE].console_level                     = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_CORE].file_level                        = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_CORE].udp_level                         = GNB_LOG_LEVEL1;

    snprintf(log_ctx->config_table[GNB_LOG_ID_ES_UPNP].log_name, 20,              "ES_UPNP");
    log_ctx->config_table[GNB_LOG_ID_ES_UPNP].console_level                     = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_UPNP].file_level                        = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_UPNP].udp_level                         = GNB_LOG_LEVEL1;

    snprintf(log_ctx->config_table[GNB_LOG_ID_ES_RESOLV].log_name, 20,            "ES_RESOLV");
    log_ctx->config_table[GNB_LOG_ID_ES_RESOLV].console_level                   = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_RESOLV].file_level                      = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_RESOLV].udp_level                       = GNB_LOG_LEVEL1;

    snprintf(log_ctx->config_table[GNB_LOG_ID_ES_BROADCAST].log_name, 20,         "ES_BROADCAST");
    log_ctx->config_table[GNB_LOG_ID_ES_BROADCAST].console_level                = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_BROADCAST].file_level                   = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_BROADCAST].udp_level                    = GNB_LOG_LEVEL1;

    snprintf(log_ctx->config_table[GNB_LOG_ID_ES_DISCOVER_IN_LAN].log_name, 20,   "ES_DISCOVER_IN_LAN");
    log_ctx->config_table[GNB_LOG_ID_ES_DISCOVER_IN_LAN].console_level          = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_DISCOVER_IN_LAN].file_level             = GNB_LOG_LEVEL1;
    log_ctx->config_table[GNB_LOG_ID_ES_DISCOVER_IN_LAN].udp_level              = GNB_LOG_LEVEL1;

    gnb_log_udp_open(log_ctx);

    log_ctx->log_udp_type = log_udp_type;
    log_ctx->log_payload_type = GNB_ES_PAYLOAD_TYPE_UDPLOG;

    if ( '\0' != log_udp_sockaddress4_string[0] ) {
        rc = gnb_log_udp_set_addr4_string(log_ctx, log_udp_sockaddress4_string);
        log_ctx->output_type |= GNB_LOG_OUTPUT_UDP;
    }

    return;

}


int main (int argc,char *argv[]){

    char *ctl_block_file = NULL;
    char *pid_file = NULL;
    char *wan_address6_file = NULL;

    int upnp_opt              = 0;
    int resolv_opt            = 0;
    int broadcast_address_opt = 0;
    int discover_in_lan_opt   = 0;
    int dump_address_opt      = 0;

    int if_up_opt   = 0;
    int if_down_opt = 0;
    int if_loop_opt = 0;

    int daemon = 0;
    int service_opt = 0;

    gnb_ctl_block_t *ctl_block;
    uint8_t log_udp_type = GNB_LOG_UDP_TYPE_TEXT;

    char log_udp_sockaddress4_string[16 + 1 + sizeof("65535")];

    memset(log_udp_sockaddress4_string, 0, 16 + 1 + sizeof("65535"));

    int flag;

    struct option long_options[] = {

      { "ctl-block",              required_argument, 0, 'b' },
      { "upnp",                   no_argument,  0, OPT_UPNP },
      { "resolv",                 no_argument,  0, OPT_RESOLV },
      { "broadcast-address",      no_argument,  0, OPT_BROADCAST_ADDRESS },
      { "discover-in-lan",        no_argument,  0, 'L' },
      { "dump-address",           no_argument,  0, OPT_DUMP_ADDRESS },
      { "service",                no_argument, 0, 's' },
      { "daemon",                 no_argument, 0, 'd' },

      { "pid-file",               required_argument,  0, PID_FILE },

      { "wan-address6-file",      required_argument,  0, WAN_ADDRESS6_FILE },

      { "if-up",                  no_argument,  0, OPT_IF_UP },
      { "if-down",                no_argument,  0, OPT_IF_DOWN },
      { "if-loop",                no_argument,  0, OPT_IF_LOOP },

      { "log-udp6",               optional_argument,  &flag, LOG_UDP6 },
      { "log-udp4",               optional_argument,  &flag, LOG_UDP4 },
      { "log-udp-type",           required_argument,  0,     LOG_UDP_TYPE },

      { "help",                   no_argument, 0, 'h' },

      { 0, 0, 0, 0 }

    };

    int opt;

    while (1) {

        int option_index = 0;

        opt = getopt_long (argc, argv, "b:46dsLh",long_options, &option_index);

        if (opt == -1) {
            break;
        }

        switch (opt) {

        case 'b':
            ctl_block_file = optarg;
            break;

        case OPT_UPNP:
            upnp_opt = 1;
            break;

        case OPT_RESOLV:
            resolv_opt = 1;
            break;

        case 'L':
            discover_in_lan_opt = 1;
            break;

        case OPT_BROADCAST_ADDRESS:
            broadcast_address_opt = 1;
            break;

        case OPT_DUMP_ADDRESS:
            dump_address_opt = 1;
            break;

        case PID_FILE:
            pid_file = optarg;
            break;

        case WAN_ADDRESS6_FILE:
            wan_address6_file = optarg;
            break;

        case OPT_IF_UP:
            if_up_opt = 1;
            break;

        case OPT_IF_DOWN:
            if_down_opt = 1;
            break;

        case OPT_IF_LOOP:
            if_loop_opt = 1;
            break;

        case LOG_UDP_TYPE:

            if ( !strncmp(optarg, "binary", 16) ) {
                log_udp_type = GNB_LOG_UDP_TYPE_BINARY;
            } else {
                log_udp_type = GNB_LOG_UDP_TYPE_TEXT;
            }

            break;

        case 'd':
            daemon = 1;
            break;

        case 's':
            service_opt = 1;
            break;

        case 'h':
            show_useage(argc,argv);
            exit(0);

        default:
            break;
        }

        if ( 0 == opt ) {

            switch (flag) {

            case LOG_UDP4:

                if ( NULL != optarg ) {
                    snprintf(log_udp_sockaddress4_string, 16 + 1 + sizeof("65535"), "%s", optarg);
                } else {
                    snprintf(log_udp_sockaddress4_string, 16 + 1 + sizeof("65535"), "%s", "127.0.0.1:8666");
                }

                break;

            default:
                break;
            }

            continue;

        }

    }


    if ( NULL == ctl_block_file ) {
        show_useage(argc,argv);
        exit(0);
    }


#ifdef __UNIX_LIKE_OS__
    if (daemon) {
        gnb_daemon();
    }
#endif

    if (daemon) {
        service_opt = 1;
    }

    gnb_log_ctx_t *log;

    log = gnb_log_ctx_create();

    setup_log_ctx(log, log_udp_sockaddress4_string, log_udp_type);

    gnb_es_ctx *es_ctx = gnb_es_ctx_create(service_opt, ctl_block_file, log);

    if ( NULL == es_ctx ) {
        printf("es ctx init error [%s]......\n",ctl_block_file);
        return 1;
    }

    es_ctx->log = log;

    gnb_conf_t *conf = &es_ctx->ctl_block->conf_zone->conf_st;

    if ( 1 == conf->public_index_service ) {
        printf("gnb run as public index service map_file=%s\n", ctl_block_file);
        return 1;
    }

    if ( '\0' == conf->conf_dir[0] ) {

        if(  0 == conf->lite_mode ) {
            printf("gnb config dir is not set map_file=%s\n", ctl_block_file);
            return 1;
        }

    }

    es_ctx->pid_file = malloc(PATH_MAX+NAME_MAX);

    char  resolved_path[PATH_MAX+NAME_MAX];

    #ifdef __UNIX_LIKE_OS__

    if ( NULL != pid_file ) {

        if ( 0 == conf->lite_mode ) {
            snprintf(es_ctx->pid_file, PATH_MAX+NAME_MAX,"%s", pid_file);
        } else {
            snprintf(conf->pid_file,   PATH_MAX+NAME_MAX, "%s/gnb_es.%d.pid", conf->binary_dir, conf->udp4_ports[0]);
        }

        if ( NULL != gnb_realpath(es_ctx->pid_file,resolved_path) ) {
            strncpy(es_ctx->pid_file, resolved_path, PATH_MAX);
        }

    } else {

        snprintf(es_ctx->pid_file, PATH_MAX+NAME_MAX, "%s/gnb_es.pid", conf->conf_dir);

    }

    #endif

    if ( NULL != wan_address6_file ) {

        es_ctx->wan_address6_file = malloc(PATH_MAX+NAME_MAX);
        snprintf(es_ctx->wan_address6_file, PATH_MAX+NAME_MAX,"%s", wan_address6_file);

        if ( NULL != gnb_realpath(es_ctx->wan_address6_file,resolved_path) ) {
            strncpy(es_ctx->wan_address6_file, resolved_path, PATH_MAX);
        }

    }

    es_ctx->upnp_opt    = upnp_opt;
    es_ctx->resolv_opt  = resolv_opt;
    es_ctx->broadcast_address_opt = broadcast_address_opt;
    es_ctx->discover_in_lan_opt   = discover_in_lan_opt;
    es_ctx->dump_address_opt      = dump_address_opt;
    es_ctx->if_up_opt   = if_up_opt;
    es_ctx->if_down_opt = if_down_opt;
    es_ctx->if_loop_opt = if_loop_opt;

    es_ctx->daemon = daemon;
    es_ctx->service_opt = service_opt;

#ifdef _WIN32
    WSADATA wsaData;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsaData );
#endif


#ifdef __UNIX_LIKE_OS__
    if ( 1==es_ctx->service_opt || 1==es_ctx->daemon ) {
        save_pid(es_ctx->pid_file);
    }
#endif

    gnb_es_ctx_init(es_ctx);

    gnb_start_environment_service(es_ctx);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;

}
