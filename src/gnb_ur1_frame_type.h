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

#ifndef GNB_UR1_FRAME_TYPE_H
#define GNB_UR1_FRAME_TYPE_H

#include "stdint.h"
#include "gnb_core_frame_type_defs.h"

#pragma pack(push, 1)

typedef struct _gnb_ur1_frame_head_t{

	gnb_uuid_t dst_uuid64;     //网络字节序

	uint8_t  relay_addr_type;
	uint8_t  relay_addr[16];
	uint16_t relay_in_port;    //网络字节序
	uint16_t relay_out_port;   //网络字节序 //废弃

	uint8_t  dst_addr_type;
	uint8_t  dst_addr[16];
	uint16_t dst_port;       //网络字节序

	unsigned char passcode[4];

}__attribute__ ((__packed__)) gnb_ur1_frame_head_t;

#pragma pack(pop)

#define GNB_MAX_UR1_FRAME_SIZE       (32*1024)

#endif
