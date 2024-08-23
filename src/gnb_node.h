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

#ifndef GNB_NODE_H
#define GNB_NODE_H

#include "gnb.h"

void gnb_init_node_key512(gnb_core_t *gnb_core);

void gnb_add_forward_node_ring(gnb_core_t *gnb_core, gnb_uuid_t uuid64);
void gnb_add_index_node_ring(gnb_core_t *gnb_core, gnb_uuid_t uuid64);
gnb_node_t* gnb_select_forward_node(gnb_core_t *gnb_core);

int gnb_node_sign_verify(gnb_core_t *gnb_core, gnb_uuid_t uuid64, unsigned char *sign, void *data, size_t data_size);

void gnb_send_to_address(gnb_core_t *gnb_core, gnb_address_t *address, gnb_payload16_t *payload);
void gnb_send_udata_to_address(gnb_core_t *gnb_core, gnb_address_t *address, void *udata, size_t udata_size);

void gnb_send_address_list(gnb_core_t *gnb_core, gnb_address_list_t *address_list, gnb_payload16_t *payload);

void gnb_send_to_address_through_all_sockets(gnb_core_t *gnb_core, gnb_address_t *address, gnb_payload16_t *payload);
void gnb_send_address_list_through_all_sockets(gnb_core_t *gnb_core, gnb_address_list_t *address_list, gnb_payload16_t *payload);

void gnb_send_available_address_list(gnb_core_t *gnb_core, gnb_address_list_t *address_list, gnb_payload16_t *payload, uint64_t now_sec);

gnb_address_t* gnb_select_index_address(gnb_core_t *gnb_core, uint64_t now_sec);

gnb_node_t* gnb_select_index_nodes(gnb_core_t *gnb_core);

gnb_address_t* gnb_select_available_address4(gnb_core_t *gnb_core, gnb_node_t *node);

int gnb_send_to_node(gnb_core_t *gnb_core, gnb_node_t *node, gnb_payload16_t *payload, unsigned char addr_type_bits);

int gnb_forward_payload_to_node(gnb_core_t *gnb_core, gnb_node_t *node, gnb_payload16_t *payload);

#endif
