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

#ifndef GNB_FWDU2_FRAME_TYPE_H
#define GNB_FWDU2_FRAME_TYPE_H

#include "stdint.h"

#pragma pack(push, 1)

typedef struct _gnb_fwdu2_frame_head_t{

	uint32_t dst_uuid32;     //网络字节序

	uint8_t  fwd_addr_type;
	uint8_t  fwd_addr[16];
	uint16_t fwd_in_port;    //网络字节序
	uint16_t fwd_out_port;   //网络字节序

	uint8_t  dst_addr_type;
	uint8_t  dst_addr[16];
	uint16_t dst_port;       //网络字节序

	unsigned char passcode[4];

}__attribute__ ((__packed__)) gnb_fwdu2_frame_head_t;

#pragma pack(pop)


#define GNB_PAYLOAD_TYPE_FWDU2              0x46 //'F'

#define GNB_PAYLOAD_SUB_TYPE_FWDU2_IPV6     '6'
#define GNB_PAYLOAD_SUB_TYPE_FWDU2_IPV4     '4'

#define GNB_MAX_FWDU2_FRAME_SIZE       (32*1024)

#endif
