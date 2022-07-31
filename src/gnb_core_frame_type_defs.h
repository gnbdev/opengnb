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

#ifndef GNB_CORE_FRAME_TYPE_DEFS_H
#define GNB_CORE_FRAME_TYPE_DEFS_H

#define GNB_PAYLOAD_TYPE_UR0                  (0x3)

#define GNB_PAYLOAD_TYPE_IPFRAME              (0x4)
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_INIT                (0x0)
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_STD                 (0x1)
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_RELAY               (0x1 << 1)
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_UNIFIED             (0x1 << 2)
#define GNB_PAYLOAD_SUB_TYPE_IPFRAME_UNIFIED_MULTI_PATH  (0x1 << 3)

#define GNB_PAYLOAD_TYPE_INDEX                (0x8)
#define PAYLOAD_SUB_TYPE_POST_ADDR            (0x1)
#define PAYLOAD_SUB_TYPE_ECHO_ADDR            (0x2)
#define PAYLOAD_SUB_TYPE_REQUEST_ADDR         (0x3)
#define PAYLOAD_SUB_TYPE_PUSH_ADDR            (0x4)
#define PAYLOAD_SUB_TYPE_DETECT_ADDR          (0x5)

#define GNB_PAYLOAD_TYPE_NODE                 (0x9)
#define PAYLOAD_SUB_TYPE_PING                 (0x1)
#define PAYLOAD_SUB_TYPE_PONG                 (0x2)
#define PAYLOAD_SUB_TYPE_PONG2                (0x3)
#define PAYLOAD_SUB_TYPE_LAN_PING             (0x4)
#define PAYLOAD_SUB_TYPE_NODE_UNIFIED_NOTIFY  (0x5)

#define GNB_PAYLOAD_TYPE_LAN_DISCOVER         (0x43)

#define GNB_PAYLOAD_TYPE_UDPLOG               (0x44)
#define GNB_ES_PAYLOAD_TYPE_UDPLOG            (0x45)

#define GNB_PAYLOAD_TYPE_UR1                  (0x46)
#define GNB_PAYLOAD_SUB_TYPE_UR1_IPV6         '6'
#define GNB_PAYLOAD_SUB_TYPE_UR1_IPV4         '4'

#define ED25519_SIGN_SIZE   64

#endif
