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
#include "gnb_type.h"
#include "gnb_core_frame_type_defs.h"

#pragma pack(push, 1)

typedef struct _gnb_ur1_frame_head_t{

	gnb_uuid_t src_uuid64;       //由第一个转发的node设置,应用构造时设置为0, 网络字节序
	gnb_uuid_t dst_uuid64;       //由应用构造时设置,网络字节序

	uint8_t  src_addr[16];       //src_addr,src_port 由转发的node设置
	uint16_t src_port;           //网络字节序

	uint8_t  dst_addr[16];       //由应用构造时设置
	uint16_t dst_port;           //网络字节序

	unsigned char passcode[4];
	unsigned char verifycode[4];  //由第一个转发的node设置

	uint8_t ttl;                  //由第一个转发的node设置为2, 应用构造时设置为0,

}__attribute__ ((__packed__)) gnb_ur1_frame_head_t;

#pragma pack(pop)

#define GNB_MAX_UR1_FRAME_SIZE       (32*1024)

#endif
