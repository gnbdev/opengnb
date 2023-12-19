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

#ifndef GNB_WORKER_TYPE_H
#define GNB_WORKER_TYPE_H

#include <stdint.h>
#include "gnb_ring_buffer_fixed.h"


typedef struct _gnb_worker_t gnb_worker_t;

typedef struct _gnb_ring_buffer_t gnb_ring_buffer_t;

typedef void(*gnb_worker_init_func_t)(gnb_worker_t *gnb_worker, void *ctx);

typedef void(*gnb_worker_release_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_start_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_stop_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_notify_func_t)(gnb_worker_t *gnb_worker);

typedef int(*gnb_worker_notify_func_t)(gnb_worker_t *gnb_worker);

typedef struct _gnb_worker_t {

	char *name;

	gnb_worker_init_func_t      init;

	gnb_worker_release_func_t   release;

	gnb_worker_start_func_t     start;

	gnb_worker_stop_func_t      stop;

	gnb_worker_notify_func_t    notify;

	gnb_ring_buffer_fixed_t *ring_buffer_in;
	gnb_ring_buffer_fixed_t *ring_buffer_out;

	volatile int thread_worker_flag;

	volatile int thread_worker_ready_flag;

	volatile int thread_worker_run_flag;

	void *ctx;

}gnb_worker_t;


typedef struct _gnb_worker_ring_t {

	uint8_t size;
	uint8_t cur_idx;
	gnb_worker_t *worker[0];

}gnb_worker_ring_t;

#endif


