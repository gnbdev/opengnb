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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include "gnb.h"
#include "gnb_worker.h"
#include "gnb_worker_queue_data.h"

/*
每个 worker 都是一个独立线程，设计上允许多个线程并发处理payload，但对于处理 node 的数据来说不是很必要。
如果把 pf 也设计为 worker 的形式，对于加密运算比较重的分组，可以发挥多核的特性
*/
extern gnb_worker_t gnb_primary_worker_mod;
extern gnb_worker_t gnb_node_worker_mod;
extern gnb_worker_t gnb_index_worker_mod;
extern gnb_worker_t gnb_secure_index_worker_mod;
extern gnb_worker_t gnb_detect_worker_mod;

extern gnb_worker_t gnb_index_service_worker_mod;
extern gnb_worker_t gnb_secure_index_service_worker_mod;

extern gnb_worker_t gnb_pf_worker_mod;

static gnb_worker_t *gnb_worker_array[] = {
    &gnb_primary_worker_mod,
    &gnb_index_worker_mod,
    &gnb_secure_index_worker_mod,
    &gnb_index_service_worker_mod,
    &gnb_secure_index_service_worker_mod,
    &gnb_detect_worker_mod,
    &gnb_node_worker_mod,
    &gnb_pf_worker_mod,
    NULL,
};

static gnb_worker_t* find_worker_mod_by_name(const char *name){

    int num =  sizeof(gnb_worker_array)/sizeof(gnb_worker_t *);

    int i;

    for ( i=0; i<num; i++ ) {

        if ( NULL==gnb_worker_array[i] ) {
            break;
        }

        if ( 0 == strncmp(gnb_worker_array[i]->name,name,128) ) {
            return gnb_worker_array[i];
        }

    }

    return NULL;

}

gnb_worker_t *gnb_worker_init(const char *name, void *ctx){

    gnb_worker_t *gnb_worker_mod = find_worker_mod_by_name(name);

    if ( NULL==gnb_worker_mod ) {
        printf("find_worker_by_name name[%s] is NULL\n",name);
        return NULL;
    }

    gnb_worker_t *gnb_worker = (gnb_worker_t *) malloc(sizeof(gnb_worker_t));

    *gnb_worker = *gnb_worker_mod;

    gnb_worker->thread_worker_flag = 0;
    gnb_worker->thread_worker_run_flag = 0;

    gnb_worker->init(gnb_worker, ctx);

    return gnb_worker;

}


void gnb_worker_wait_primary_worker_started(gnb_core_t *gnb_core){

    do{

        if ( 1==gnb_core->primary_worker->thread_worker_run_flag ) {
            break;
        }

    }while(1);

}


void gnb_worker_sync_time(uint64_t *now_time_sec_ptr, uint64_t *now_time_usec_ptr){

    struct timeval now_timeval;

    gettimeofday(&now_timeval, NULL);

    *now_time_sec_ptr  = (uint64_t)now_timeval.tv_sec;
    *now_time_usec_ptr = (uint64_t)now_timeval.tv_sec *1000000 + now_timeval.tv_usec;

}

void gnb_worker_release(gnb_worker_t *gnb_worker){

    gnb_worker->release(gnb_worker);

    free(gnb_worker);

    return;

}
