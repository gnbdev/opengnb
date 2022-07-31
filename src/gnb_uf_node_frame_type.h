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

#ifndef GNB_UF_NODE_FRAME_TYPE_H
#define GNB_UF_NODE_FRAME_TYPE_H

#pragma pack(push, 1)

typedef struct _uf_node_notify_frame_t {

    struct __attribute__((__packed__)) uf_node_notify_frame_data {
      uint32_t src_uuid32;   //发送方的uuid32
      uint32_t dst_uuid32;   //接收方的uuid32
      uint32_t df_uuid32;    //direct forwarding node uuid32
      int64_t  addr6_ping_latency_usec;
      int64_t  addr4_ping_latency_usec;
      unsigned char attachment[128];
      unsigned char text[64];
    }data;

    unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) uf_node_notify_frame_t;

#define UF_NODE_NOTIFY_FRAME_PAYLOAD_SIZE (sizeof(gnb_payload16_t) + sizeof(uf_node_notify_frame_t))

#pragma pack(pop)

#endif
