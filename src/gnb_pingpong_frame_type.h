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

#ifndef GNB_PINGPONG_FRAME_TYPE_H
#define GNB_PINGPONG_FRAME_TYPE_H

#include "stdint.h"
#include "gnb_core_frame_type_defs.h"

#pragma pack(push, 1)

typedef struct _node_ping_frame_t {

    struct __attribute__((__packed__)) ping_frame_data {
      gnb_uuid_t src_uuid64;   //发送方的uuid64
      gnb_uuid_t dst_uuid64;   //接收方的uuid64
      uint64_t src_ts_usec;  //发送方的时间戳

      /*让 dst 看到自己的 ip 地址，暂时还没启用*/
      uint8_t  dst_addr4[4];
      uint16_t dst_port4;
      uint8_t  dst_addr6[16];
      uint16_t dst_port6;

      unsigned char crypto_seed[64];

      unsigned char attachment[128+64];

      unsigned char text[32];
    }data;

    unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) node_ping_frame_t;

#define NODE_PING_FRAME_PAYLOAD_SIZE (sizeof(gnb_payload16_t) + sizeof(node_ping_frame_t))

typedef struct _node_pong_frame_t {

    struct __attribute__((__packed__)) pong_frame_data {
      gnb_uuid_t src_uuid64;   //发送方的uuid64
      gnb_uuid_t dst_uuid64;   //接收方的uuid64
      uint64_t src_ts_usec;  //发送方的时间戳
      uint64_t dst_ts_usec;  //接收方上一个ping frame带来的时间戳

      uint8_t  dst_addr4[4];
      uint16_t dst_port4;
      uint8_t  dst_addr6[16];
      uint16_t dst_port6;

      unsigned char crypto_seed[64];

      unsigned char attachment[128+64];

      unsigned char text[32];
    }data;

    unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) node_pong_frame_t;

#define NODE_PONG_FRAME_PAYLOAD_SIZE (sizeof(gnb_payload16_t) + sizeof(node_pong_frame_t))

#define GNB_NODE_ATTACHMENT_TYPE_TUN_EMPTY         0x0
#define GNB_NODE_ATTACHMENT_TYPE_TUN_SOCKADDRESS   0x1

typedef struct _node_attachment_tun_sockaddress_t {

    struct in_addr  tun_addr4;
    uint16_t        tun_sin_port4;

    struct in6_addr tun_ipv6_addr;
    uint16_t        tun_sin_port6;

    uint16_t        es_sin_port4;
    uint16_t        es_sin_port6;

}__attribute__ ((__packed__)) node_attachment_tun_sockaddress_t;


#pragma pack(pop)

#endif
