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

#ifndef GNB_WORKER_H
#define GNB_WORKER_H

#include "gnb_worker_type.h"

gnb_worker_t *gnb_worker_init(const char *name, void *ctx);

void gnb_worker_wait_primary_worker_started(gnb_core_t *gnb_core);

void gnb_worker_sync_time(uint64_t *now_time_sec_ptr, uint64_t *now_time_usec_ptr);

void gnb_worker_release(gnb_worker_t *gnb_worker);

#endif

