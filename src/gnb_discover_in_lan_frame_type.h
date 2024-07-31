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

#ifndef GNB_DISCOVER_IN_LAN_FRAME_TYPE_H
#define GNB_DISCOVER_IN_LAN_FRAME_TYPE_H

#include "stdint.h"
#include "gnb_core_frame_type_defs.h"

#pragma pack(push, 1)

typedef struct _discover_lan_in_frame_t {

    struct __attribute__((__packed__)) discover_lan_in_data {

    	unsigned char arg0;
    	unsigned char arg1;
    	unsigned char arg2;
    	unsigned char arg3;

    	unsigned char src_key512[64];
    	gnb_uuid_t      src_uuid64;

    	uint8_t  src_addr6_a[16];
    	uint16_t src_port6_a;

    	uint8_t  src_addr4[4];
    	uint16_t src_port4;

    	uint64_t src_ts_usec;

    	char text[256];
    	char attachment[128];

    }data;

    unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) discover_lan_in_frame_t;

#pragma pack(pop)

#define PAYLOAD_DISCOVER_LAN_IN_FRAME_PAYLOAD_SIZE (sizeof(gnb_payload16_t) + sizeof(discover_lan_in_frame_t))

#endif
