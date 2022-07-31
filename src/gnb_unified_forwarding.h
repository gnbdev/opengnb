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

#ifndef GNB_UNIFIED_FORWARDING_H
#define GNB_UNIFIED_FORWARDING_H

#include <stdint.h>

typedef struct _gnb_core_t gnb_core_t;

typedef struct _gnb_payload16_t gnb_payload16_t;

typedef struct _gnb_node_t  gnb_node_t;

typedef struct _gnb_sockaddress_t gnb_sockaddress_t;


int gnb_unified_forwarding_tun(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx);
int gnb_unified_forwarding_with_multi_path_tun(gnb_core_t *gnb_core, gnb_pf_ctx_t *pf_ctx);


#define UNIFIED_FORWARDING_DROP    -2
#define UNIFIED_FORWARDING_ERROR   -1
#define UNIFIED_FORWARDING_TO_TUN   0
#define UNIFIED_FORWARDING_TO_INET  1

int gnb_unified_forwarding_inet(gnb_core_t *gnb_core, gnb_payload16_t *payload);
int gnb_unified_forwarding_multi_path_inet(gnb_core_t *gnb_core, gnb_payload16_t *payload);

#endif
