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
#include <unistd.h>

#include "gnb.h"

#ifdef __UNIX_LIKE_OS__
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <sys/time.h>

#include "gnb_conf_file.h"
#include "gnb_config_lite.h"
#include "gnb_arg_list.h"
#include "gnb_tun_drv.h"
#include "gnb_exec.h"
#include "gnb_binary.h"
#include "gnb_payload16.h"
#include "gnb_time.h"
#include "gnb_keys.h"
#include "gnb_mmap.h"
#include "gnb_time.h"

gnb_pf_t* gnb_find_pf_mod_by_name(const char *name);

void gnb_set_env(const char *name, const char *value);

void log_out_description(gnb_log_ctx_t *log);

extern  gnb_arg_list_t *gnb_es_arg_list;
extern int is_verbose;
extern int is_trace;

static void gnb_setup_env(gnb_core_t *gnb_core){

    char env_value_string[64];

    gnb_set_env("GNB_IF_NAME", gnb_core->ifname);
    snprintf(env_value_string, 64, "%d", gnb_core->conf->mtu);
    gnb_set_env("GNB_MTU", env_value_string);
    gnb_set_env("GNB_TUN_IPV4", GNB_ADDR4STR1(&gnb_core->local_node->tun_addr4));
    gnb_set_env("GNB_TUN_IPV6", GNB_ADDR6STR1(&gnb_core->local_node->tun_ipv6_addr));

}

static void init_ctl_block(gnb_core_t *gnb_core, gnb_conf_t *conf){

    gnb_mmap_block_t *mmap_block;

    void *memory;

    size_t node_num = 0;

    if ( 0 == conf->public_index_service && 0 == conf->lite_mode ) {
        //大致算出 node 的数量
        node_num = gnb_get_node_num_from_file(conf);
    }

    if ( 0==node_num ) {
        node_num = 256;
    }

    size_t block_size = sizeof(uint32_t)*256 + sizeof(gnb_ctl_magic_number_t) + sizeof(gnb_ctl_conf_zone_t) + sizeof(gnb_ctl_core_zone_t) + sizeof(gnb_ctl_status_zone_t) + sizeof(gnb_ctl_node_zone_t) + sizeof(gnb_node_t)*node_num + sizeof(gnb_block32_t) * 5;

    unlink(conf->map_file);

    mmap_block = gnb_mmap_create(conf->map_file, block_size, GNB_MMAP_TYPE_READWRITE|GNB_MMAP_TYPE_CREATE);

    if ( NULL==mmap_block ) {
        printf("init_ctl_block error[%p] map_file=%s\n",mmap_block, conf->map_file);
        exit(1);
    }

    memory = gnb_mmap_get_block(mmap_block);
    gnb_core->ctl_block = gnb_ctl_block_build(memory, node_num);
    gnb_core->ctl_block->mmap_block = mmap_block;

}


static void setup_log_ctx(gnb_conf_t *conf, gnb_log_ctx_t *log){

    int rc;
    int pid;

    log->output_type = GNB_LOG_OUTPUT_STDOUT;

    if ( 0!=conf->daemon || 0!=conf->quiet ) {
        log->output_type = GNB_LOG_OUTPUT_NONE;
    }

    if ( '\0' != conf->log_path[0] ) {

        snprintf(log->log_file_path, PATH_MAX, "%s", conf->log_path);

        if ( 0 == conf->public_index_service ) {
            snprintf(log->log_file_name_std,   PATH_MAX+NAME_MAX, "%s/std.log",   conf->log_path);
            snprintf(log->log_file_name_debug, PATH_MAX+NAME_MAX, "%s/debug.log", conf->log_path);
            snprintf(log->log_file_name_error, PATH_MAX+NAME_MAX, "%s/error.log", conf->log_path);
        } else {
            snprintf(log->log_file_name_std,   PATH_MAX+NAME_MAX, "%s/std.%d.log",   conf->log_path, conf->udp4_ports[0]);
            snprintf(log->log_file_name_debug, PATH_MAX+NAME_MAX, "%s/debug.%d.log", conf->log_path, conf->udp4_ports[0]);
            snprintf(log->log_file_name_error, PATH_MAX+NAME_MAX, "%s/error.%d.log", conf->log_path, conf->udp4_ports[0]);
        }

        log->output_type |= GNB_LOG_OUTPUT_FILE;

    } else {
        log->log_file_path[0] = '\0';
    }

    if ( GNB_LOG_LEVEL_UNSET == conf->console_log_level ) {

        if ( 0 == conf->lite_mode ) {
            conf->console_log_level = GNB_LOG_LEVEL1;
        } else {
            conf->console_log_level = GNB_LOG_LEVEL3;
        }

    }

    if ( GNB_LOG_LEVEL_UNSET == conf->file_log_level ) {

        if ( 0 == conf->lite_mode ) {
            conf->file_log_level    = GNB_LOG_LEVEL1;
        } else {
            conf->file_log_level    = GNB_LOG_LEVEL3;
        }

    }

    if ( GNB_LOG_LEVEL_UNSET == conf->udp_log_level ) {

        if ( 0 == conf->lite_mode ) {
            conf->udp_log_level     = GNB_LOG_LEVEL1;
        } else {
            conf->udp_log_level     = GNB_LOG_LEVEL3;
        }

    }

    if ( 1 == conf->lite_mode ) {

        if ( GNB_LOG_LEVEL_UNSET == conf->core_log_level ) {
            conf->core_log_level = GNB_LOG_LEVEL3;
        }

        if ( GNB_LOG_LEVEL_UNSET == conf->pf_log_level ) {
            conf->pf_log_level = GNB_LOG_LEVEL3;
        }

        if ( GNB_LOG_LEVEL_UNSET == conf->core_log_level ) {
            conf->core_log_level = GNB_LOG_LEVEL3;
        }

        if ( GNB_LOG_LEVEL_UNSET == conf->main_log_level ) {
            conf->main_log_level = GNB_LOG_LEVEL3;
        }

        if ( GNB_LOG_LEVEL_UNSET == conf->node_log_level ) {
            conf->node_log_level = GNB_LOG_LEVEL3;
        }

        if ( GNB_LOG_LEVEL_UNSET == conf->index_log_level ) {
            conf->index_log_level = GNB_LOG_LEVEL3;
        }

        if ( GNB_LOG_LEVEL_UNSET == conf->index_service_log_level ) {
            conf->index_service_log_level = GNB_LOG_LEVEL3;
        }

    }

    snprintf(log->config_table[GNB_LOG_ID_CORE].log_name, 20, "CORE");
    log->config_table[GNB_LOG_ID_CORE].console_level          = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_CORE].file_level             = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_CORE].udp_level              = GNB_LOG_LEVEL1;

    snprintf(log->config_table[GNB_LOG_ID_PF].log_name, 20,   "PF");
    log->config_table[GNB_LOG_ID_PF].console_level            = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_PF].file_level               = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_PF].udp_level                = GNB_LOG_LEVEL1;

    snprintf(log->config_table[GNB_LOG_ID_MAIN_WORKER].log_name, 20, "MAIN");
    log->config_table[GNB_LOG_ID_MAIN_WORKER].console_level   = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_MAIN_WORKER].file_level      = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_MAIN_WORKER].udp_level       = GNB_LOG_LEVEL1;

    snprintf(log->config_table[GNB_LOG_ID_NODE_WORKER].log_name, 20, "NODE");
    log->config_table[GNB_LOG_ID_NODE_WORKER].console_level   = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_NODE_WORKER].file_level      = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_NODE_WORKER].udp_level       = GNB_LOG_LEVEL1;

    snprintf(log->config_table[GNB_LOG_ID_INDEX_WORKER].log_name, 20, "INDEX");
    log->config_table[GNB_LOG_ID_INDEX_WORKER].console_level  = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_INDEX_WORKER].file_level     = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_INDEX_WORKER].udp_level      = GNB_LOG_LEVEL1;

    snprintf(log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].log_name, 20, "INDEX_SERVICE");
    log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].console_level  = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].file_level     = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].udp_level      = GNB_LOG_LEVEL1;

    snprintf(log->config_table[GNB_LOG_ID_DETECT_WORKER].log_name, 20, "DETECT");
    log->config_table[GNB_LOG_ID_DETECT_WORKER].console_level = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_DETECT_WORKER].file_level    = GNB_LOG_LEVEL1;
    log->config_table[GNB_LOG_ID_DETECT_WORKER].udp_level     = GNB_LOG_LEVEL1;


    if ( GNB_LOG_LEVEL_UNSET != conf->core_log_level ) {

        if ( 0 == conf->lite_mode ) {

            if ( conf->console_log_level >= conf->core_log_level ) {
                log->config_table[GNB_LOG_ID_CORE].console_level = conf->core_log_level;
            }

            if ( conf->file_log_level >= conf->core_log_level ) {
                log->config_table[GNB_LOG_ID_CORE].file_level = conf->core_log_level;
            }

            if ( conf->udp_log_level >= conf->core_log_level ) {
                log->config_table[GNB_LOG_ID_CORE].udp_level = conf->core_log_level;
            }

        } else {

            log->config_table[GNB_LOG_ID_CORE].console_level = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_CORE].file_level    = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_CORE].udp_level     = GNB_LOG_LEVEL3;

        }

    }

    if ( GNB_LOG_LEVEL_UNSET != conf->pf_log_level ) {

        if ( 0 == conf->lite_mode ) {

            if ( conf->pf_log_level >= log->config_table[GNB_LOG_ID_PF].console_level ) {
                log->config_table[GNB_LOG_ID_PF].console_level = conf->pf_log_level;
            }

            if ( conf->pf_log_level >= log->config_table[GNB_LOG_ID_PF].file_level ) {
                log->config_table[GNB_LOG_ID_PF].file_level = conf->pf_log_level;
            }

            if ( conf->pf_log_level >= log->config_table[GNB_LOG_ID_PF].udp_level ) {
                log->config_table[GNB_LOG_ID_PF].udp_level = conf->pf_log_level;
            }

        } else {

            log->config_table[GNB_LOG_ID_PF].console_level = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_PF].file_level    = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_PF].udp_level     = GNB_LOG_LEVEL3;

        }

    }

    if ( GNB_LOG_LEVEL_UNSET != conf->main_log_level ) {

        if ( 0 == conf->lite_mode ) {

            if ( conf->console_log_level >= conf->main_log_level ) {
                log->config_table[GNB_LOG_ID_MAIN_WORKER].console_level = conf->main_log_level;
            }

            if ( conf->file_log_level >= conf->main_log_level ) {
                log->config_table[GNB_LOG_ID_MAIN_WORKER].file_level = conf->main_log_level;
            }

            if ( conf->udp_log_level >= conf->main_log_level ) {
                log->config_table[GNB_LOG_ID_MAIN_WORKER].udp_level = conf->main_log_level;
            }

        } else {

            log->config_table[GNB_LOG_ID_MAIN_WORKER].console_level = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_MAIN_WORKER].file_level    = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_MAIN_WORKER].udp_level     = GNB_LOG_LEVEL3;

        }

    }

    if ( GNB_LOG_LEVEL_UNSET != conf->node_log_level ) {

        if ( 0 == conf->lite_mode ) {

            if ( conf->console_log_level >= conf->node_log_level ) {
                log->config_table[GNB_LOG_ID_NODE_WORKER].console_level = conf->node_log_level;
            }

            if ( conf->file_log_level >= conf->node_log_level ) {
                log->config_table[GNB_LOG_ID_NODE_WORKER].file_level = conf->node_log_level;
            }

            if ( conf->udp_log_level >= conf->node_log_level ) {
                log->config_table[GNB_LOG_ID_NODE_WORKER].udp_level = conf->node_log_level;
            }

        } else {

            log->config_table[GNB_LOG_ID_NODE_WORKER].console_level = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_NODE_WORKER].file_level    = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_NODE_WORKER].udp_level     = GNB_LOG_LEVEL3;

        }

    }

    if ( GNB_LOG_LEVEL_UNSET != conf->index_log_level ) {

        if ( 0 == conf->lite_mode ){

            if ( conf->console_log_level >= conf->index_log_level ) {
                log->config_table[GNB_LOG_ID_INDEX_WORKER].console_level = conf->index_log_level;
            }

            if ( conf->file_log_level > conf->index_log_level ) {
                log->config_table[GNB_LOG_ID_INDEX_WORKER].file_level = conf->index_log_level;
            }

            if ( conf->udp_log_level > conf->index_log_level ) {
                log->config_table[GNB_LOG_ID_INDEX_WORKER].udp_level = conf->index_log_level;
            }

        } else {

            log->config_table[GNB_LOG_ID_INDEX_WORKER].console_level = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_INDEX_WORKER].file_level    = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_INDEX_WORKER].udp_level     = GNB_LOG_LEVEL3;

        }

    }

    if ( GNB_LOG_LEVEL_UNSET != conf->index_service_log_level ) {

        if ( 0 == conf->lite_mode ) {

            if ( conf->console_log_level >= conf->index_service_log_level ) {
                log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].console_level = conf->index_service_log_level;
            }

            if ( conf->file_log_level > conf->index_service_log_level ) {
                log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].file_level = conf->index_service_log_level;
            }

            if ( conf->udp_log_level > conf->index_service_log_level ) {
                log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].udp_level = conf->index_service_log_level;
            }

        } else {

            log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].console_level = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].file_level    = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_INDEX_SERVICE_WORKER].udp_level     = GNB_LOG_LEVEL3;

        }

    }

    if ( GNB_LOG_LEVEL_UNSET != conf->detect_log_level ) {

        if ( 0 == conf->lite_mode ) {

            if ( conf->console_log_level > conf->detect_log_level ) {
                log->config_table[GNB_LOG_ID_DETECT_WORKER].console_level = conf->detect_log_level;
            }

            if ( conf->file_log_level >= conf->detect_log_level ) {
                log->config_table[GNB_LOG_ID_DETECT_WORKER].file_level = conf->detect_log_level;
            }

            if ( conf->udp_log_level >= conf->detect_log_level ) {
                log->config_table[GNB_LOG_ID_DETECT_WORKER].udp_level = conf->detect_log_level;
            }

        } else {

            log->config_table[GNB_LOG_ID_DETECT_WORKER].console_level = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_DETECT_WORKER].file_level    = GNB_LOG_LEVEL3;
            log->config_table[GNB_LOG_ID_DETECT_WORKER].udp_level     = GNB_LOG_LEVEL3;

        }

    }

    gnb_log_file_rotate(log);
    gnb_log_udp_open(log);

    log->log_udp_type = conf->log_udp_type;
    log->log_payload_type = GNB_PAYLOAD_TYPE_UDPLOG;

    if ( '\0' != conf->log_udp_sockaddress4_string[0] ) {
        rc = gnb_log_udp_set_addr4_string(log, conf->log_udp_sockaddress4_string);
        log->output_type |= GNB_LOG_OUTPUT_UDP;
    }

    return;

}


gnb_core_t* gnb_core_create(gnb_conf_t *conf){

    gnb_core_t *gnb_core;

    gnb_heap_t *heap = gnb_heap_create(8192);

    gnb_core = gnb_heap_alloc(heap, sizeof(gnb_core_t));

    memset(gnb_core, 0, sizeof(gnb_core_t));

    gnb_core->heap = heap;

    init_ctl_block(gnb_core, conf);

    gnb_core->conf = &gnb_core->ctl_block->conf_zone->conf_st;
    memcpy(gnb_core->conf, conf, sizeof(gnb_conf_t));

    gnb_core->log = &gnb_core->ctl_block->core_zone->log_ctx_st;

    gnb_log_ctx_t *log = gnb_log_ctx_create();
    memcpy(gnb_core->log, log, sizeof(gnb_log_ctx_t));
    free(log);

    gnb_core->ed25519_private_key = gnb_core->ctl_block->core_zone->ed25519_private_key;
    gnb_core->ed25519_public_key  = gnb_core->ctl_block->core_zone->ed25519_public_key;

    gnb_core->index_address_ring.address_list = (gnb_address_list_t *)gnb_core->ctl_block->core_zone->index_address_block;
    gnb_core->index_address_ring.address_list->num  = 0;
    gnb_core->index_address_ring.address_list->size = GNB_MAX_ADDR_RING;

    gnb_core->fwdu0_address_ring.address_list = (gnb_address_list_t *)gnb_core->ctl_block->core_zone->ufwd_address_block;
    gnb_core->fwdu0_address_ring.address_list->num = 0;
    gnb_core->fwdu0_address_ring.address_list->size = 16;

    gnb_core->ifname = (char *)gnb_core->ctl_block->core_zone->ifname;
    gnb_core->if_device_string = (char *)gnb_core->ctl_block->core_zone->if_device_string;

    gnb_core->uuid_node_map   = gnb_hash32_create(gnb_core->heap, 1024,1024); //以节点的uuid32作为key的 node 表
    gnb_core->ipv4_node_map   = gnb_hash32_create(gnb_core->heap, 1024,1024);

    //以节点的subnet(uint32)作为key的 node 表
    gnb_core->subneta_node_map = gnb_hash32_create(gnb_core->heap, 1024,1024);
    gnb_core->subnetb_node_map = gnb_hash32_create(gnb_core->heap, 1024,1024);
    gnb_core->subnetc_node_map = gnb_hash32_create(gnb_core->heap, 1024,1024);

    int64_t now_sec = gnb_timestamp_sec();
    gnb_update_time_seed(gnb_core, now_sec);


    if ( 0 == gnb_core->conf->lite_mode ) {

        //加载 node.conf
        local_node_file_config(gnb_core);

        if ( 0==gnb_core->conf->daemon ) {

            if ( 1==is_verbose ) {
                gnb_core->conf->console_log_level        = 2;
                gnb_core->conf->core_log_level           = 2;
                gnb_core->conf->pf_log_level             = 2;
                gnb_core->conf->main_log_level           = 2;
                gnb_core->conf->node_log_level           = 2;
                gnb_core->conf->index_log_level          = 2;
                gnb_core->conf->index_service_log_level  = 2;
                gnb_core->conf->detect_log_level         = 2;
            }

            if ( 1==is_trace ) {
                gnb_core->conf->console_log_level        = 3;
                gnb_core->conf->core_log_level           = 3;
                gnb_core->conf->pf_log_level             = 3;
                gnb_core->conf->main_log_level           = 3;
                gnb_core->conf->node_log_level           = 3;
                gnb_core->conf->index_log_level          = 3;
                gnb_core->conf->index_service_log_level  = 3;
                gnb_core->conf->detect_log_level         = 3;
            }

        }

        setup_log_ctx(gnb_core->conf, gnb_core->log);
        gnb_config_file(gnb_core);

    } else {

        if ( 0==gnb_core->conf->daemon ) {

            if ( 1==is_verbose ) {
                gnb_core->conf->console_log_level        = 2;
                gnb_core->conf->core_log_level           = 2;
                gnb_core->conf->pf_log_level             = 2;
                gnb_core->conf->main_log_level           = 2;
                gnb_core->conf->node_log_level           = 2;
                gnb_core->conf->index_log_level          = 2;
                gnb_core->conf->index_service_log_level  = 2;
                gnb_core->conf->detect_log_level         = 2;
            }

            if ( 1==is_trace ) {
                gnb_core->conf->console_log_level        = 3;
                gnb_core->conf->core_log_level           = 3;
                gnb_core->conf->pf_log_level             = 3;
                gnb_core->conf->main_log_level           = 3;
                gnb_core->conf->node_log_level           = 3;
                gnb_core->conf->index_log_level          = 3;
                gnb_core->conf->index_service_log_level  = 3;
                gnb_core->conf->detect_log_level         = 3;
            }

        }

        gnb_config_lite(gnb_core);
        setup_log_ctx(gnb_core->conf, gnb_core->log);
    }


    log_out_description(gnb_core->log);

    void *memory = (void *)gnb_core->ctl_block->entry_table256;

    gnb_ctl_block_build_finish(memory);

    snprintf(gnb_core->ifname,256,"%s", gnb_core->conf->ifname);

    gnb_core->pf_array = gnb_pf_array_init(gnb_core->heap, 32);
    gnb_core->pf_ctx_array = gnb_pf_ctx_array_init(gnb_core->heap,32);

    gnb_pf_t *pf;

    pf = gnb_find_pf_mod_by_name("gnb_pf_dump");

    if ( 1==gnb_core->conf->if_dump ) {
        gnb_pf_install(gnb_core->pf_array, pf);
    }

    pf = gnb_find_pf_mod_by_name(gnb_core->conf->pf_route);

    if ( NULL== pf ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "pf_route '%s' not exist\n", gnb_core->conf->pf_route);
        return NULL;
    }

    gnb_pf_install(gnb_core->pf_array, pf);

    if ( GNB_PF_TYPE_CRYPTO_NONE == conf->crypto_type ) {
        goto skip_crypto;
    }

    if ( conf->crypto_type & GNB_PF_TYPE_CRYPTO_XOR ) {
        pf = gnb_find_pf_mod_by_name("gnb_pf_crypto_xor");
        gnb_pf_install(gnb_core->pf_array, pf);
    }

    if ( conf->crypto_type & GNB_PF_TYPE_CRYPTO_ARC4 ) {
        pf = gnb_find_pf_mod_by_name("gnb_pf_crypto_arc4");
        gnb_pf_install(gnb_core->pf_array, pf);
    }


skip_crypto:

    gnb_pf_init(gnb_core);

    gnb_pf_conf(gnb_core);

    if ( NULL==gnb_core->local_node ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_CORE, "local node is miss\n");
        return NULL;
    }

    gnb_core->tun_payload0  = (gnb_payload16_t *)gnb_core->ctl_block->core_zone->tun_payload_block;
    gnb_core->inet_payload0 = (gnb_payload16_t *)gnb_core->ctl_block->core_zone->inet_payload_block;
    gnb_core->tun_payload  = (void *)gnb_core->tun_payload0  + GNB_PAYLOAD_BUFFER_PADDING_SIZE;
    gnb_core->inet_payload = (void *)gnb_core->inet_payload0 + GNB_PAYLOAD_BUFFER_PADDING_SIZE;

#if defined(__FreeBSD__)
    gnb_core->drv = &gnb_tun_drv_freebsd;
#endif


#if defined(__APPLE__)
    gnb_core->drv = &gnb_tun_drv_darwin;
#endif


#if defined(__OpenBSD__)
    gnb_core->drv = &gnb_tun_drv_openbsd;
#endif

#if defined(__linux__)
    gnb_core->drv = &gnb_tun_drv_linux;
#endif


#if defined(_WIN32)

    if ( GNB_IF_DRV_TYPE_TAP_WINDOWS == conf->if_drv ) {
        gnb_core->drv = &gnb_tun_drv_win32;
    } else if ( GNB_IF_DRV_TYPE_TAP_WINTUN == conf->if_drv ) {
        gnb_core->drv = &gnb_tun_drv_wintun;
    } else {
        gnb_core->drv = &gnb_tun_drv_win32;
    }

#endif

    if ( gnb_core->conf->activate_tun ) {
        gnb_core->drv->init_tun(gnb_core);
    }

    if ( gnb_core->conf->activate_node_worker ) {
        gnb_core->node_worker = gnb_worker_init("gnb_node_worker", gnb_core);
    }

    if ( gnb_core->conf->activate_index_worker ) {
        gnb_core->index_worker  = gnb_worker_init("gnb_index_worker",  gnb_core);
    }

    if ( gnb_core->conf->activate_detect_worker ) {
        gnb_core->detect_worker = gnb_worker_init("gnb_detect_worker", gnb_core);
    }

    if ( gnb_core->conf->activate_index_service_worker ) {
        gnb_core->index_service_worker  = gnb_worker_init("gnb_index_service_worker",  gnb_core);
    }

    gnb_core->main_worker = gnb_worker_init("gnb_main_worker", gnb_core);

    return gnb_core;

}


gnb_core_t* gnb_core_index_service_create(gnb_conf_t *conf){

    gnb_core_t *gnb_core;

    gnb_heap_t *heap = gnb_heap_create(8192);

    gnb_core = gnb_heap_alloc(heap, sizeof(gnb_core_t));

    memset(gnb_core, 0, sizeof(gnb_core_t));

    gnb_core->heap = heap;

    init_ctl_block(gnb_core, conf);

    gnb_core->conf = &gnb_core->ctl_block->conf_zone->conf_st;
    memcpy(gnb_core->conf, conf, sizeof(gnb_conf_t));

    gnb_core->log =  &gnb_core->ctl_block->core_zone->log_ctx_st;

    gnb_log_ctx_t *log = gnb_log_ctx_create();
    memcpy(gnb_core->log, log, sizeof(gnb_log_ctx_t));

    free(log);

    gnb_core->ed25519_private_key = gnb_core->ctl_block->core_zone->ed25519_private_key;
    gnb_core->ed25519_public_key  = gnb_core->ctl_block->core_zone->ed25519_public_key;

    gnb_core->ifname = (char *)gnb_core->ctl_block->core_zone->ifname;
    gnb_core->if_device_string = (char *)gnb_core->ctl_block->core_zone->if_device_string;

    if ( 0==conf->daemon ) {

        if ( 1==is_verbose ) {
            gnb_core->conf->console_log_level        = 2;
            gnb_core->conf->core_log_level           = 2;
            gnb_core->conf->pf_log_level             = 2;
            gnb_core->conf->main_log_level           = 2;
            gnb_core->conf->node_log_level           = 2;
            gnb_core->conf->index_log_level          = 2;
            gnb_core->conf->index_service_log_level  = 2;
            gnb_core->conf->detect_log_level         = 2;
        }

        if ( 1==is_trace ) {
            gnb_core->conf->console_log_level        = 3;
            gnb_core->conf->core_log_level           = 3;
            gnb_core->conf->pf_log_level             = 3;
            gnb_core->conf->main_log_level           = 3;
            gnb_core->conf->node_log_level           = 3;
            gnb_core->conf->index_log_level          = 3;
            gnb_core->conf->index_service_log_level  = 3;
            gnb_core->conf->detect_log_level         = 3;
        }

        if ( 1==gnb_core->conf->if_dump ) {
            gnb_core->conf->console_log_level        = 3;
            gnb_core->conf->pf_log_level             = 3;
        }

    }

    setup_log_ctx(gnb_core->conf, gnb_core->log);

    log_out_description(gnb_core->log);

    void *memory = (void *)gnb_core->ctl_block->entry_table256;

    gnb_ctl_block_build_finish(memory);

    snprintf(gnb_core->ifname,256,"%s", gnb_core->conf->ifname);

    gnb_core->tun_payload0  = (gnb_payload16_t *)gnb_core->ctl_block->core_zone->tun_payload_block;
    gnb_core->inet_payload0 = (gnb_payload16_t *)gnb_core->ctl_block->core_zone->inet_payload_block;

    gnb_core->tun_payload  = gnb_core->tun_payload0  + GNB_PAYLOAD_BUFFER_PADDING_SIZE;
    gnb_core->inet_payload = gnb_core->inet_payload0 + GNB_PAYLOAD_BUFFER_PADDING_SIZE;

    gnb_core->index_service_worker  = gnb_worker_init("gnb_index_service_worker",  gnb_core);
    gnb_core->main_worker           = gnb_worker_init("gnb_main_worker", gnb_core);

    return gnb_core;

}

void gnb_core_release(gnb_core_t *gnb_core){

    //gnb_core 结构体内还有一些成员内存没做释放处理
    if ( gnb_core->conf->public_index_service ) {
        goto PUBLIC_INDEX_RELEASE;
    }

    gnb_pf_release(gnb_core);

PUBLIC_INDEX_RELEASE:

    gnb_heap_free(gnb_core->heap ,gnb_core);

}


void gnb_core_index_service_start(gnb_core_t *gnb_core){

    int ret;

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"GNB Public Index Service Start.....\n");

    gnb_core->index_service_worker->start(gnb_core->index_service_worker);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"%s start\n", gnb_core->index_service_worker->name);

    gnb_core->main_worker->start(gnb_core->main_worker);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"%s start\n", gnb_core->main_worker->name);

}


void gnb_core_start(gnb_core_t *gnb_core){

    int ret;

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "Start.....\n");

    gnb_setup_env(gnb_core);

    if ( gnb_core->conf->activate_tun ) {

        ret = gnb_core->drv->open_tun(gnb_core);

        if ( 0!=ret ) {

            if ( -1 == ret ) {
                GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "if[%s] already open\n", gnb_core->ifname);
            } else {
                GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE, "if[%s] error\n", gnb_core->ifname);
            }

            return;
        }

        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"if[%s] opened\n", gnb_core->ifname);
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"node[%d] ipv4[%s]\n", gnb_core->local_node->uuid32, GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4));

    }

    if ( gnb_core->conf->activate_index_worker ) {
        gnb_core->index_worker->start(gnb_core->index_worker);
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"%s start\n", gnb_core->index_worker->name);
    }

    if ( gnb_core->conf->activate_detect_worker ) {
        gnb_core->detect_worker->start(gnb_core->detect_worker);
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"%s start\n", gnb_core->detect_worker->name);
    }

    if ( gnb_core->conf->activate_index_service_worker ) {
        gnb_core->index_service_worker->start(gnb_core->index_service_worker);
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"%s start\n", gnb_core->index_service_worker->name);
    }

    if ( gnb_core->conf->activate_node_worker ) {
        gnb_core->node_worker->start(gnb_core->node_worker);
        GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"%s start\n", gnb_core->node_worker->name);
    }

    gnb_core->main_worker->start(gnb_core->main_worker);
    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"%s start\n", gnb_core->main_worker->name);

}

void gnb_core_stop(gnb_core_t *gnb_core){

    gnb_core->main_worker->stop(gnb_core->main_worker);

    if ( gnb_core->conf->activate_tun ) {
        gnb_core->drv->close_tun(gnb_core);
    }

    if ( gnb_core->conf->activate_index_worker ) {
        gnb_core->index_worker->stop(gnb_core->index_worker);
    }

    if ( gnb_core->conf->activate_detect_worker ) {
        gnb_core->detect_worker->stop(gnb_core->detect_worker);
    }

    if ( gnb_core->conf->activate_index_service_worker ) {
        gnb_core->index_service_worker->stop(gnb_core->index_service_worker);
    }

    if ( gnb_core->conf->activate_node_worker ) {
        gnb_core->node_worker->stop(gnb_core->node_worker);
    }

    gnb_core->loop_flag = 0;

    int i;

#ifdef __UNIX_LIKE_OS__

    for (i=0; i<gnb_core->conf->udp6_socket_num; i++) {
        close(gnb_core->udp_ipv6_sockets[i]);
    }

    for (i=0; i<gnb_core->conf->udp4_socket_num; i++) {
        close(gnb_core->udp_ipv4_sockets[i]);
    }

#endif

#ifdef _WIN32

    for (i=0; i<gnb_core->conf->udp6_socket_num; i++) {
        closesocket(gnb_core->udp_ipv6_sockets[i]);
    }

    for (i=0; i<gnb_core->conf->udp4_socket_num; i++) {
        closesocket(gnb_core->udp_ipv4_sockets[i]);
    }

#endif

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_CORE,"if[%s] closeed\n", gnb_core->ifname);

}


#ifdef __UNIX_LIKE_OS__
static void exec_es(gnb_core_t *gnb_core) {

    pid_t  pid_gnb_es = 0;
    int ret;
    char gnb_es_bin_path[PATH_MAX+NAME_MAX];
    char es_arg_string[GNB_ARG_STRING_MAX_SIZE];

    snprintf(gnb_es_bin_path,   PATH_MAX+NAME_MAX, "%s/gnb_es",       gnb_core->conf->binary_dir);

    ret = gnb_arg_list_to_string(gnb_es_arg_list, es_arg_string, GNB_ARG_STRING_MAX_SIZE);

    if ( 0 != ret ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "gnb_es argv error, skip exec '%s'\n", gnb_es_bin_path);
        return;
    }

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "exec gnb_es argv '%s'\n", es_arg_string);

    pid_gnb_es = gnb_exec(gnb_es_bin_path, gnb_core->conf->binary_dir, gnb_es_arg_list, GNB_EXEC_WAIT);

    if ( -1 == pid_gnb_es ) {
        return;
    }

}
#endif


#ifdef _WIN32
static void exec_es(gnb_core_t *gnb_core) {

    DWORD  pid_gnb_es = 0;
    int ret;
    char gnb_es_bin_path[PATH_MAX+NAME_MAX];
    char es_arg_string[GNB_ARG_STRING_MAX_SIZE];

    snprintf(gnb_es_bin_path,   PATH_MAX+NAME_MAX, "%s\\gnb_es.exe",      gnb_core->conf->binary_dir);

    ret = gnb_arg_list_to_string(gnb_es_arg_list, es_arg_string, GNB_ARG_STRING_MAX_SIZE);

    if ( 0 != ret ) {
        GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "gnb_es argv error, skip exec '%s'\n", gnb_es_bin_path);
        return;
    }

    GNB_LOG3(gnb_core->log, GNB_LOG_ID_CORE, "exec gnb_es argv '%s'\n", es_arg_string);

    pid_gnb_es = gnb_exec(gnb_es_bin_path, gnb_core->conf->binary_dir, gnb_es_arg_list, GNB_EXEC_BACKGROUND|GNB_EXEC_WAIT );

}
#endif



void exec_loop_script(gnb_core_t *gnb_core, const char *script_file_name){

    char script_dir[PATH_MAX];
    char script_file[PATH_MAX+NAME_MAX];

    int arg_list_size = 1;

    pid_t  pid = 0;

    gnb_arg_list_t *arg_list = (gnb_arg_list_t *)alloca( sizeof(gnb_arg_list_t) + sizeof(char *) * arg_list_size );

    arg_list->size = arg_list_size;
    arg_list->argc = 0;

    strncpy(script_dir, gnb_core->conf->conf_dir, PATH_MAX);
    strncat(script_dir, "/scripts", PATH_MAX-strlen(script_dir));

    snprintf(script_file, PATH_MAX+NAME_MAX,"%s/%s", script_dir, script_file_name);

    gnb_arg_append(arg_list, script_file);

    pid = gnb_exec(script_file, script_dir, arg_list, GNB_EXEC_WAIT);

    return;

}


#define GNB_EXEC_ES_INTERVAL_TIME_SEC      (60*5)
#define GNB_EXEC_SCRIPT_INTERVAL_TIME_SEC  (60)

void primary_process_loop( gnb_core_t *gnb_core ){

    int ret;

    uint64_t last_exec_es_ts_sec = 0;
    uint64_t last_exec_loop_script_ts_sec = 0;

    do{

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

        if ( gnb_core->ctl_block->status_zone->keep_alive_ts_sec - last_exec_es_ts_sec > GNB_EXEC_ES_INTERVAL_TIME_SEC ) {

            if ( 0 == gnb_core->conf->public_index_service ) {
                exec_es(gnb_core);
            }

            last_exec_es_ts_sec = gnb_core->ctl_block->status_zone->keep_alive_ts_sec;
        }

        if ( gnb_core->ctl_block->status_zone->keep_alive_ts_sec - last_exec_loop_script_ts_sec > GNB_EXEC_SCRIPT_INTERVAL_TIME_SEC ) {

            #if defined(__FreeBSD__)
            exec_loop_script(gnb_core,"if_loop_freebsd.sh");
            #endif

            #if defined(__APPLE__)
            exec_loop_script(gnb_core,"if_loop_darwin.sh");
            #endif

            #if defined(__OpenBSD__)
            exec_loop_script(gnb_core,"if_loop_linux.sh");
            #endif

            #if defined(__linux__)
            exec_loop_script(gnb_core,"if_loop_linux.sh");
            #endif

            #if defined(_WIN32)
            #endif

            last_exec_loop_script_ts_sec = gnb_core->ctl_block->status_zone->keep_alive_ts_sec;

        }

    }while(1);

}
