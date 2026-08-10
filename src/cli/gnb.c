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
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include "gnb.h"
#include "gnb_core.h"
#include "gnb_arg_list.h"
#include "gnb_exec.h"
#include "gnb_time.h"
#include "gnb_address.h"
#include "gnb_arg_list.h"
#include "gnb_conf_file.h"
#include "gnb_log.h"

#ifdef _WIN32
#include <windows.h>
#endif

gnb_core_t *gnb_core;
gnb_core_t *gnb_core_new;

#define GNB_CORE_FLAG_UNSET   0
#define GNB_CORE_FLAG_START   1
#define GNB_CORE_FLAG_RESTART 2
int gnb_core_flag;

int gnb_daemon();
void save_pid(const char *pid_file);
gnb_conf_t* gnb_argv(int argc,char *argv[]);
void primary_process_loop(gnb_core_t *gnb_core);
extern gnb_pf_t *gnb_pf_mods[];
extern int is_self_test;

void signal_alrm_handler(int signum) {
    return;
}

void signal_handler(int signum) {
    #ifdef __UNIX_LIKE_OS__
    if ( SIGHUP == signum ) {
        gnb_core_flag = GNB_CORE_FLAG_RESTART;
        gnb_core_stop(gnb_core);
        return;
    }
    #endif
    
    if ( SIGTERM == signum ) {
        goto finish;
    }
    if ( SIGINT == signum ) {
        goto finish;
    }
finish:
    gnb_core_stop(gnb_core);
    unlink(gnb_core->conf->pid_file);
    printf("\ngnb process exit\n");
    exit(0);
}

static void self_test() {
    int i;
    int j;
    int node_num;
    gnb_ctl_block_t *ctl_block;
    gnb_address_list_t *static_address_list;
    gnb_address_t *gnb_address;
    gnb_node_t *node;
    int ret;
    char es_arg_string[GNB_ARG_STRING_MAX_SIZE];
    char address_string1[GNB_IP6_PORT_STRING_SIZE];
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST daemon='%d'\n", gnb_core->conf->daemon );
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST systemd_daemon='%d'\n", gnb_core->conf->systemd_daemon );

    if ( 1 == gnb_core->conf->activate_tun && 0 == gnb_core->conf->public_index_service ) {
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST local node=%lu\n", gnb_core->local_node->uuid64);
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST tun ipv4[%s]\n",
                 gnb_in4_addr_str(&gnb_core->local_node->tun_addr4,address_string1,0)
        );
    }

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST binary_dir='%s'\n", gnb_core->conf->binary_dir);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST conf_dir='%s'\n", gnb_core->conf->conf_dir);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST map_file='%s'\n", gnb_core->conf->map_file);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST pid_file='%s'\n", gnb_core->conf->pid_file);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST log_path='%s'\n", gnb_core->conf->log_path);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST node_cache_file='%s'\n", gnb_core->conf->node_cache_file);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST listen='%s'\n", gnb_core->conf->listen_address6_string);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST listen6='%s'\n", gnb_core->conf->listen_address4_string);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST mtu=%d\n", gnb_core->conf->mtu);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST addr_secure=%d\n", gnb_core->conf->addr_secure);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST lite_mode=%d\n", gnb_core->conf->lite_mode);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST public_index_service=%d\n", gnb_core->conf->public_index_service);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST multi_socket=%d\n", gnb_core->conf->multi_socket);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST direct_forwarding=%d\n", gnb_core->conf->direct_forwarding);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST pf_worker_num=%d\n", gnb_core->conf->pf_worker_num);

    switch (gnb_core->conf->memory) {
    case GNB_MEMORY_SCALE_TINY:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST memory scale tiny\n");
        break;
    case GNB_MEMORY_SCALE_SMALL:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST memory scale samll\n");
        break;
    case GNB_MEMORY_SCALE_LARGE:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST memory scale lage\n");
        break;
    case GNB_MEMORY_SCALE_HUGE:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST memory scale huge\n");
        break;    
    default:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST memory scale error\n");
        break;
    }

    switch (gnb_core->conf->zip) {
    case GNB_ZIP_AUTO:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST zip auto\n");
        break;
    case GNB_ZIP_FORCE:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST zip force\n");
        break;
    default:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST zip config error\n");
        break;
    }

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST zip_level=%d\n", gnb_core->conf->zip_level);

    if ( GNB_PF_BITS_NONE == gnb_core->conf->pf_bits ) {
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST crypto none\n");
    }
    if ( gnb_core->conf->pf_bits & GNB_PF_BITS_CRYPTO_XOR ) {
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST crypto xor\n");
    }
    if ( gnb_core->conf->pf_bits & GNB_PF_BITS_CRYPTO_ARC4 ) {
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST crypto arc4\n");
    }

    switch (gnb_core->conf->unified_forwarding) {
    case GNB_UNIFIED_FORWARDING_OFF:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST unified_forwarding=off\n");
        break;
    case GNB_UNIFIED_FORWARDING_FORCE:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST unified_forwarding=force\n");
        break;

    case GNB_UNIFIED_FORWARDING_AUTO:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST unified_forwarding=auto\n");
        break;

    case GNB_UNIFIED_FORWARDING_SUPER:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST unified_forwarding=super\n");
        break;
    case GNB_UNIFIED_FORWARDING_HYPER:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST unified_forwarding=hyper\n");
        break;
    default:
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST unified_forwarding config error\n");
        break;
    }

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST activate tun=%d\n", gnb_core->conf->activate_tun);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST universal relay0 ur0=%d\n", gnb_core->conf->universal_relay0);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST universal relay1 ur1=%d\n", gnb_core->conf->universal_relay1);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST console_log_level=%d\n", gnb_core->conf->console_log_level);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST file_log_level=%d\n",    gnb_core->conf->file_log_level);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST udp_log_level=%d\n",     gnb_core->conf->udp_log_level);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST log_udp_type=%d\n",      gnb_core->conf->log_udp_type);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST GNB_LOG_ID_CORE console_level=%d, file_level=%d, udp_level=%d\n",
            gnb_core->log->config_table[GNB_LOG_ID_CORE].console_level, gnb_core->log->config_table[GNB_LOG_ID_CORE].file_level, gnb_core->log->config_table[GNB_LOG_ID_CORE].udp_level);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST GNB_LOG_ID_PF console_level=%d, file_level=%d, udp_level=%d\n",
            gnb_core->log->config_table[GNB_LOG_ID_PF].console_level, gnb_core->log->config_table[GNB_LOG_ID_PF].file_level, gnb_core->log->config_table[GNB_LOG_ID_PF].udp_level);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST GNB_LOG_ID_MAIN_WORKER console_level=%d,file_level=%d, udp_level=%d\n",
            gnb_core->log->config_table[GNB_LOG_ID_MAIN_WORKER].console_level, gnb_core->log->config_table[GNB_LOG_ID_MAIN_WORKER].file_level, gnb_core->log->config_table[GNB_LOG_ID_MAIN_WORKER].udp_level);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST GNB_LOG_ID_NODE_WORKER console_level=%d, file_level=%d, udp_level=%d\n",
            gnb_core->log->config_table[GNB_LOG_ID_NODE_WORKER].console_level, gnb_core->log->config_table[GNB_LOG_ID_NODE_WORKER].file_level, gnb_core->log->config_table[GNB_LOG_ID_NODE_WORKER].udp_level);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST GNB_LOG_ID_INDEX_WORKER console_level=%d, file_level=%d, udp_level=%d\n",
            gnb_core->log->config_table[GNB_LOG_ID_INDEX_WORKER].console_level, gnb_core->log->config_table[GNB_LOG_ID_INDEX_WORKER].file_level, gnb_core->log->config_table[GNB_LOG_ID_INDEX_WORKER].udp_level);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST GNB_LOG_ID_INDEX_SERVICE_WORKER console_level=%d, file_level=%d, udp_level=%d\n",
            gnb_core->log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].console_level, gnb_core->log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].file_level, gnb_core->log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].udp_level);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST GNB_LOG_ID_DETECT_WORKER console_level=%d, file_level=%d, udp_level=%d\n",
            gnb_core->log->config_table[GNB_LOG_ID_DETECT_WORKER].console_level, gnb_core->log->config_table[GNB_LOG_ID_DETECT_WORKER].file_level, gnb_core->log->config_table[GNB_LOG_ID_DETECT_WORKER].udp_level);

    if ( 1 == gnb_core->conf->activate_tun && 0 == gnb_core->conf->public_index_service ) {
        ctl_block = gnb_core->ctl_block;
        node_num = ctl_block->node_zone->node_num;
        for ( i=0; i<node_num; i++ ) {
            node = &ctl_block->node_zone->node[i];
            if ( node->uuid64 != ctl_block->core_zone->local_uuid ) {
                GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST ----- remote node %llu -----\n", node->uuid64);
            } else {
                GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST local  node %llu\n", node->uuid64);
            }
            GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST tun_ipv6 %s\n", gnb_in6_addr_str(&node->tun_ipv6_addr,address_string1,0));
            GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST tun_ipv4 %s\n", gnb_in4_addr_str(&node->tun_addr4,address_string1,0));
            static_address_list  = (gnb_address_list_t *)&node->static_address_block;
            for ( j=0; j<static_address_list->num; j++ ) {
                gnb_address = &static_address_list->array[j];
                if ( 0 == gnb_address->port ) {
                    continue;
                }
                GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"SELF-TEST address %s\n",
                        gnb_address_port_str(gnb_address,address_string1,gnb_core->conf->addr_secure)
                );
            }
        }

        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST route_frame_head_size %lu\n", gnb_core->route_frame_head_size);

        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST num of index=%d\n", gnb_core->index_address_ring.address_list->num);
        for ( i=0; i< gnb_core->index_address_ring.address_list->num; i++ ) {
            GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST index node '%s'\n",
                     gnb_address_port_str(&gnb_core->index_address_ring.address_list->array[i],address_string1,gnb_core->conf->addr_secure)
            );
        }
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"SELF-TEST num of fwd node:%d\n", gnb_core->fwd_node_ring.num);
        for ( i=0; i<gnb_core->fwd_node_ring.num; i++ ) {
            GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "SELF-TEST fwd node=%llu\n", gnb_core->fwd_node_ring.nodes[i]->uuid64);
        }
    }
}

#ifdef _WIN32
BOOL CALLBACK CosonleHandler(DWORD ev) {
    switch (ev) {
    case CTRL_CLOSE_EVENT:
    case    CTRL_C_EVENT:
        gnb_core_stop(gnb_core);
        exit(0);
    default:
        exit(0);
        break;
    }
    return TRUE;
}
#endif

void log_out_description(gnb_log_ctx_t *log) {
    GNB_LOG1(log, GNB_LOG_ID_CORE, "%s\n", GNB_VERSION_STRING);
    GNB_LOG1(log, GNB_LOG_ID_CORE, "%s\n", GNB_COPYRIGHT_STRING);
    GNB_LOG1(log, GNB_LOG_ID_CORE, "Site: %s\n", GNB_URL_STRING);
    GNB_LOG1(log, GNB_LOG_ID_CORE, "%s\n", GNB_BUILD_STRING);
}

void show_description() {
    printf("%s\n", GNB_VERSION_STRING);
    printf("%s\n", GNB_BUILD_STRING);
    printf("%s\n", GNB_COPYRIGHT_STRING);
    printf("Site: %s\n", GNB_URL_STRING);
    #if defined(GNB_OPENWRT_BUILD)
    printf("%s\n","Note:charleschan<hollidgelongsun157@gmail.com>(https://github.com/orgs/opengnb/people/charleschan2006-alias) made great efforts for OpenGNB porting to OpenWRT platform and provides technical supoort.");
    #endif

    #ifdef _WIN32
    printf("%s\n","Note:wuqiong(https://www.github.com/wuqiong) made great efforts for OpenGNB supoort wintun Driver and provides technical supoort.");
    #endif

    int idx = 0;
    printf("registered packet filter:");
    while ( NULL != gnb_pf_mods[idx] ) {
        printf(" %s", gnb_pf_mods[idx]->name);
        idx++;
    }
    printf("\n");
    printf("%s\n", GNB_BUILD_STRING);
}
static void exec_loop_script(gnb_core_t *gnb_core, const char *script_file_name) {
    char script_dir[PATH_MAX];
    char script_file[PATH_MAX+NAME_MAX];
    int arg_list_size = 1;
    pid_t  pid = 0;
    gnb_arg_list_t *arg_list = (gnb_arg_list_t *)alloca( sizeof(gnb_arg_list_t) + sizeof(char *) * arg_list_size );
    arg_list->size = arg_list_size;
    arg_list->argc = 0;
    arg_list->data_prt = arg_list->data;
    strncpy(script_dir, gnb_core->conf->conf_dir, PATH_MAX);
    strncat(script_dir, "/scripts", PATH_MAX-strlen(script_dir));
    snprintf(script_file, PATH_MAX+NAME_MAX,"%s/%s", script_dir, script_file_name);
    gnb_arg_append(arg_list, script_file);
    pid = gnb_exec(script_file, script_dir, arg_list, GNB_EXEC_WAIT);
    return;
}

#define GNB_EXEC_SCRIPT_INTERVAL_TIME_SEC  (60)
void gnb_primary_process_loop( gnb_core_t *gnb_core ) {
    int ret;
    uint64_t last_exec_loop_script_ts_sec = 0;
    do {
        ret = gettimeofday(&gnb_core->now_timeval,NULL);
        if ( 0!=ret ) {
            perror("gettimeofday");
            exit(1);
        }
        gnb_core->now_time_sec  = gnb_core->now_timeval.tv_sec;
        gnb_core->now_time_usec = gnb_core->now_timeval.tv_sec * 1000000 + gnb_core->now_timeval.tv_usec;
        gnb_core->ctl_block->status_zone->keep_alive_ts_sec = (uint64_t)gnb_core->now_timeval.tv_sec;
        gnb_log_file_rotate(gnb_core->log);
        #ifdef __UNIX_LIKE_OS__
        sleep(1);
        #endif
        #ifdef _WIN32
        Sleep(1000);
        #endif
        if ( gnb_core->ctl_block->status_zone->keep_alive_ts_sec - last_exec_loop_script_ts_sec > GNB_EXEC_SCRIPT_INTERVAL_TIME_SEC ) {
            #if defined(__FreeBSD__)
            exec_loop_script(gnb_core,"if_loop_freebsd.sh");
            #endif

            #if defined(__APPLE__)
            exec_loop_script(gnb_core,"if_loop_darwin.sh");
            #endif

            #if defined(__OpenBSD__)
            exec_loop_script(gnb_core,"if_loop_openbsd.sh");
            #endif

            #if defined(__NetBSD__)
            exec_loop_script(gnb_core,"if_loop_netbsd.sh");
            #endif

            #if defined(__linux__)
            exec_loop_script(gnb_core,"if_loop_linux.sh");
            #endif

            #if defined(_WIN32)
            #endif
            last_exec_loop_script_ts_sec = gnb_core->ctl_block->status_zone->keep_alive_ts_sec;
        }
    } while(1);
}


int main (int argc,char *argv[]) {
    gnb_conf_t *conf;
    int pid;
    int ret;
    setvbuf(stdout,NULL,_IOLBF,0);
    #ifdef _WIN32
    WSADATA wsaData;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsaData );
    SetConsoleCtrlHandler(CosonleHandler, TRUE);
    #endif

    conf = gnb_argv(argc, argv);
    if ( 0 == conf->public_index_service ) {
        if ( 0 == conf->lite_mode ) {
            //加载 node.conf
            local_node_file_config(conf);
        }
        gnb_core = gnb_core_create(conf);
    } else {
        gnb_core = gnb_core_index_service_create(conf);
    }
    free(conf);
    if ( NULL == gnb_core ) {
        printf("gnb core create error!\n");
        return 1;
    }
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "gnb core created!\n");

    #ifdef __UNIX_LIKE_OS__
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, signal_alrm_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT,  signal_handler);
    if ( gnb_core->conf->daemon ) {
        gnb_daemon();
    }
    save_pid(gnb_core->conf->pid_file);
    #endif

    ret = gettimeofday(&gnb_core->now_timeval,NULL);
    gnb_core->now_time_sec  = gnb_core->now_timeval.tv_sec;
    gnb_core->now_time_usec = gnb_core->now_timeval.tv_sec * 1000000 + gnb_core->now_timeval.tv_usec;

    if ( 1 == is_self_test ) {
        self_test();
    }

    if ( 0 == gnb_core->conf->public_index_service ) {
        gnb_core_start(gnb_core);
    } else {
        gnb_core_index_service_start(gnb_core);
    }
    
primary_process_loop:
    gnb_core_flag = GNB_CORE_FLAG_START;
    gnb_primary_process_loop(gnb_core);
    if ( 1 == conf->public_index_service || 1 == conf->lite_mode ) {
        goto finish;
    }
    if ( GNB_CORE_FLAG_RESTART != gnb_core_flag ) {
        goto finish;
    }

    // hot restart gnb
    conf = gnb_argv(argc, argv);
    free(conf);

    unlink(gnb_core->conf->map_file);
    
    local_node_file_config(conf);
    gnb_core_new = gnb_core_create(conf);
    if ( NULL == gnb_core_new ) {
        printf("hot restart​ gnb core create error!\n");
        return 1;
    }

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "hot restart​ new gnb core created!\n");

    gnb_hot_restart_prepare(gnb_core_new,gnb_core);
    gnb_core_release(gnb_core);
    gnb_core = gnb_core_new;
    gnb_core_start(gnb_core);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "hot restart​ finish!\n");
    goto primary_process_loop;

finish:
    #ifdef _WIN32
    WSACleanup();
    #endif
    return 0;
}
