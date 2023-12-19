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
#include <string.h>
#include <unistd.h>

#include "gnb.h"

#ifdef __UNIX_LIKE_OS__

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>

#endif


#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#define _POSIX
#define __USE_MINGW_ALARM
#endif

#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "gnb_node.h"
#include "gnb_ring_buffer_fixed.h"
#include "gnb_worker_queue_data.h"
#include "gnb_ur1_frame_type.h"

#include "gnb_time.h"
#include "gnb_udp.h"


#ifdef __UNIX_LIKE_OS__
void bind_socket_if(gnb_core_t *gnb_core);
#endif


gnb_pf_t* gnb_find_pf_mod_by_name(const char *name);

typedef struct _pf_worker_ctx_t{

    gnb_core_t *gnb_core;
    gnb_pf_array_t     *pf_array;

    pthread_t thread_worker;

}pf_worker_ctx_t;


static void handle_queue(gnb_core_t *gnb_core, gnb_worker_t *pf_worker){

    int i;

    pf_worker_ctx_t *pf_worker_ctx = pf_worker->ctx;


    gnb_payload16_t *payload_from_tun;
    
    gnb_worker_queue_data_t *receive_queue_data;
    gnb_worker_queue_data_t *send_queue_data;

    gnb_payload16_t *payload_from_inet;
    gnb_sockaddress_t *node_addr;

    int ret;

    for ( i=0; i<1024; i++ ) {

        receive_queue_data = gnb_ring_buffer_fixed_pop( pf_worker->ring_buffer_in );

        if ( NULL != receive_queue_data ) {

            payload_from_inet = &receive_queue_data->data.node_in.payload_st;
            node_addr = &receive_queue_data->data.node_in.node_addr_st;
            gnb_pf_inet(gnb_core, pf_worker_ctx->pf_array, payload_from_inet, node_addr);
            gnb_ring_buffer_fixed_pop_submit( pf_worker->ring_buffer_in );

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "[%s] handle queue frome inet\n", pf_worker->name);

        }

        send_queue_data = gnb_ring_buffer_fixed_pop( pf_worker->ring_buffer_out );

        if ( NULL != send_queue_data ) {

            payload_from_tun = &send_queue_data->data.node_in.payload_st;
            gnb_pf_tun(gnb_core, pf_worker_ctx->pf_array, payload_from_tun);
            gnb_ring_buffer_fixed_pop_submit( pf_worker->ring_buffer_out );

            GNB_LOG3(gnb_core->log, GNB_LOG_ID_PF, "[%s] handle queue frome tun\n", pf_worker->name);

        }

        if ( NULL == receive_queue_data && NULL == send_queue_data ) {
            break;
        }

    }

}


static void* thread_worker_func( void *data ) {

    gnb_worker_t *pf_worker = (gnb_worker_t *)data;

    pf_worker_ctx_t *pf_worker_ctx = pf_worker->ctx;

    gnb_core_t *gnb_core = pf_worker_ctx->gnb_core;

    pf_worker->thread_worker_flag     = 1;
    pf_worker->thread_worker_run_flag = 1;

    gnb_worker_wait_primary_worker_started(gnb_core);

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_PF, "start %s success!\n", pf_worker->name);

    do{

        handle_queue(gnb_core, pf_worker);

        GNB_SLEEP_MILLISECOND(100);

    }while(pf_worker->thread_worker_flag);

    pf_worker->thread_worker_run_flag = 0;

    return NULL;

}


static void init(gnb_worker_t *gnb_worker, void *ctx){

    gnb_core_t *gnb_core = (gnb_core_t *)ctx;
    
    pf_worker_ctx_t *pf_worker_ctx = (pf_worker_ctx_t *)gnb_heap_alloc(gnb_core->heap, sizeof(pf_worker_ctx_t));
    
    char *p;

    gnb_pf_t *find_pf;
    gnb_pf_t *pf;

    void *memory;
    size_t memory_size;

    memset(pf_worker_ctx, 0, sizeof(pf_worker_ctx_t));
    pf_worker_ctx->gnb_core = gnb_core;    
    pf_worker_ctx->pf_array = gnb_pf_array_init(gnb_core->heap, 32);

    if ( 1==gnb_core->conf->if_dump ) {
        find_pf = gnb_find_pf_mod_by_name("gnb_pf_dump");
        pf = (gnb_pf_t *)gnb_heap_alloc(gnb_core->heap, sizeof(gnb_pf_t));
        *pf = *find_pf;
        gnb_pf_install(pf_worker_ctx->pf_array, pf);
    }

    find_pf = gnb_find_pf_mod_by_name(gnb_core->conf->pf_route);

    if ( NULL == find_pf ) {
        GNB_ERROR1(gnb_core->log, GNB_LOG_ID_PF, "pf_route '%s' not exist\n", gnb_core->conf->pf_route);
        exit(1);
    }

    pf = (gnb_pf_t *)gnb_heap_alloc(gnb_core->heap, sizeof(gnb_pf_t));
    *pf = *find_pf;
    gnb_pf_install(pf_worker_ctx->pf_array, pf);

    if ( !(GNB_PF_BITS_CRYPTO_XOR & gnb_core->conf->pf_bits) && !(GNB_PF_BITS_CRYPTO_ARC4 & gnb_core->conf->pf_bits) ) {
        goto skip_crypto;
    }

    if ( gnb_core->conf->pf_bits & GNB_PF_BITS_CRYPTO_XOR ) {
        find_pf = gnb_find_pf_mod_by_name("gnb_pf_crypto_xor");
        pf = (gnb_pf_t *)gnb_heap_alloc(gnb_core->heap, sizeof(gnb_pf_t));
        *pf = *find_pf;
        gnb_pf_install(pf_worker_ctx->pf_array, pf);
    }

    if ( gnb_core->conf->pf_bits & GNB_PF_BITS_CRYPTO_ARC4 ) {
        find_pf = gnb_find_pf_mod_by_name("gnb_pf_crypto_arc4");
        pf = (gnb_pf_t *)gnb_heap_alloc(gnb_core->heap, sizeof(gnb_pf_t));
        *pf = *find_pf;
        gnb_pf_install(pf_worker_ctx->pf_array, pf);
    }


skip_crypto:


    if ( 0==gnb_core->conf->zip_level ) {
        goto skip_zip;
    }

    find_pf = gnb_find_pf_mod_by_name("gnb_pf_zip");
    pf = (gnb_pf_t *)gnb_heap_alloc(gnb_core->heap, sizeof(gnb_pf_t));
    *pf = *find_pf;
    gnb_pf_install(pf_worker_ctx->pf_array, pf);


skip_zip:


    gnb_pf_init(gnb_core, pf_worker_ctx->pf_array);
    gnb_pf_conf(gnb_core, pf_worker_ctx->pf_array);

    p = gnb_worker->name;
    gnb_worker->name = (char *)gnb_heap_alloc(gnb_core->heap, 16);
    snprintf(gnb_worker->name, 16, "%s_%d", p, gnb_core->pf_worker_ring->cur_idx);
    
    memory_size = gnb_ring_buffer_fixed_sum_size(sizeof(gnb_payload16_t) + gnb_core->conf->payload_block_size, gnb_core->conf->pf_woker_in_queue_length);
    memory = gnb_heap_alloc(gnb_core->heap, memory_size);
    gnb_worker->ring_buffer_in = gnb_ring_buffer_fixed_init(memory, sizeof(gnb_payload16_t) + gnb_core->conf->payload_block_size, gnb_core->conf->pf_woker_in_queue_length);

    memory_size = gnb_ring_buffer_fixed_sum_size(sizeof(gnb_payload16_t) + gnb_core->conf->payload_block_size, gnb_core->conf->pf_woker_out_queue_length);
    memory = gnb_heap_alloc(gnb_core->heap, memory_size);
    gnb_worker->ring_buffer_out = gnb_ring_buffer_fixed_init(memory, sizeof(gnb_payload16_t) + gnb_core->conf->payload_block_size, gnb_core->conf->pf_woker_out_queue_length);
    gnb_worker->ctx = pf_worker_ctx;

    GNB_LOG1(gnb_core->log, GNB_LOG_ID_PF, "%s init finish\n", gnb_worker->name);

}


static void release(gnb_worker_t *gnb_worker){

    pf_worker_ctx_t *pf_worker_ctx =  (pf_worker_ctx_t *)gnb_worker->ctx;

}


static int start(gnb_worker_t *gnb_worker){

    pf_worker_ctx_t *pf_worker_ctx =  (pf_worker_ctx_t *)gnb_worker->ctx;

    pthread_create(&pf_worker_ctx->thread_worker, NULL, thread_worker_func, gnb_worker);

    pthread_detach(pf_worker_ctx->thread_worker);

    return 0;
}


static int stop(gnb_worker_t *gnb_worker){

    pf_worker_ctx_t *pf_worker_ctx =  (pf_worker_ctx_t *)gnb_worker->ctx;

    gnb_core_t *gnb_core = pf_worker_ctx->gnb_core;

    gnb_worker->thread_worker_flag = 0;

    return 0;

}


static int notify(gnb_worker_t *gnb_worker){

    int ret;

    pf_worker_ctx_t *pf_worker_ctx =  (pf_worker_ctx_t *)gnb_worker->ctx;

    ret = pthread_kill(pf_worker_ctx->thread_worker,SIGALRM);

    return 0;

}


gnb_worker_t gnb_pf_worker_mod = {
    .name      = "gnb_pf_worker",
    .init      = init,
    .release   = release,
    .start     = start,
    .stop      = stop,
    .notify    = notify,
    .ctx       = NULL
};
