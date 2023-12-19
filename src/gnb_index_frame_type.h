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

#ifndef GNB_INDEX_FRAME_TYPE_H
#define GNB_INDEX_FRAME_TYPE_H

#include "stdint.h"
#include "gnb_core_frame_type_defs.h"

//向 index node 提交地址时间间隔，默认为 60 秒
#define GNB_POST_ADDR_INTERVAL_TIME_SEC  60

//向 index  提交节点地址的频率限制 默认为 20秒
#define GNB_POST_ADDR_LIMIT_SEC        20

//向 index node 请求节点地址时间间隔，默认为 39 秒
#define GNB_REQUEST_ADDR_INTERVAL_SEC   39

//节点向index 节点请求地址的频率限制 默认为 20毫秒
#define GNB_REQUEST_ADDR_LIMIT_USEC     1000*20

#define INDEX_ATTACHMENT_SIZE           (4+128)


#pragma pack(push, 1)

#define NODE_RANDOM_SEQUENCE_SIZE 32
#define GNB_KEY_ADDRESS_NUM 3

//node to index
typedef struct _post_addr_frame_t {

	struct __attribute__((__packed__)) post_addr_frame_data {

		// arg0 = 'p'
		unsigned char arg0;              //保留

		//当 arg0='p', arg1='a' 或 arg1='b' 请求把 attachment 保存在 gnb_key_address_t 中的 attachmenta 或 attachmentb 里面
		unsigned char arg1;

		unsigned char arg2;              //保留
		unsigned char arg3;              //保留

		unsigned char src_key512[64];    //发送节点的key
		uint32_t src_uuid32;             //发送方的uuid32,可选
		uint64_t src_ts_usec;            //发送方的时间戳,可选

		//源节点对自身探测的wan地址
		uint8_t  wan6_addr[16];
		uint16_t wan6_port;

		uint8_t  wan4_addr[4];
		uint16_t wan4_port;

		/* 节点提交地址时生成一组随机数，然后对这组随机数进行数字签名
		 * index 节点会保存最后收到的 node_random_sequence，node_random_sequence_sign
		 * PUSH ADDR 给到请求节点上，请求节点会用公钥检验这个签名, 这样包括 attachment 在内的数据都可以防止伪造
		 * */
		unsigned char node_random_sequence[NODE_RANDOM_SEQUENCE_SIZE];
		unsigned char node_random_sequence_sign[ED25519_SIGN_SIZE];

		char text[32];

		char attachment[INDEX_ATTACHMENT_SIZE];

	}data;

	unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) post_addr_frame_t;


//index to node
typedef struct _echo_addr_frame_t {

	struct __attribute__((__packed__)) echo_addr_frame_data {

		unsigned char arg0;               //保留
		unsigned char arg1;               //保留
		unsigned char arg2;               //保留
		unsigned char arg3;               //保留

		unsigned char dst_key512[64];     //发送post addr节点的key
		uint32_t dst_uuid32;              //发送post addr节点的uuid32
		uint64_t src_ts_usec;             //发送方的时间戳,可选

		char     addr_type;               //ipv6='6' or ipv4='4'
		uint8_t  addr[16];
		uint16_t port;

		char text[80];

	}data;

	unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) echo_addr_frame_t;


//node to index
typedef struct _request_addr_frame_t {

	struct __attribute__((__packed__)) request_addr_frame_data {

		// 'g' 获取 attachment
		unsigned char arg0;

		//当 arg0='g', arg1='a' 或 arg1='b' 请求获取保存在 gnb_key_address_t 中的 attachmenta 或 attachmentb
		unsigned char arg1;


		unsigned char arg2;               //保留
		unsigned char arg3;               //保留

		unsigned char src_key512[64];     //发送节点的key
		unsigned char dst_key512[64];     //被查询节点的key

		uint32_t src_uuid32;              //发送方的uuid32,可选
		uint32_t dst_uuid32;              //被查询节点的uuid可选
		uint64_t src_ts_usec;

		char text[32];

	}data;

	unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) request_addr_frame_t;



//index to node
typedef struct _push_addr_frame_t {

	struct __attribute__((__packed__)) push_addr_frame_data {

		#define PUSH_ADDR_ACTION_NOTIFY  'N'
		#define PUSH_ADDR_ACTION_CONNECT 'C'
		unsigned char arg0;

		unsigned char arg1;
		unsigned char arg2;               //保留
		unsigned char arg3;               //保留

		unsigned char node_key[64];
		uint32_t  node_uuid32;

		uint8_t  addr6_a[16];
		uint16_t port6_a;

		uint8_t  addr6_b[16];
		uint16_t port6_b;

		uint8_t  addr6_c[16];
		uint16_t port6_c;

		uint8_t  addr4_a[4];
		uint16_t port4_a;

		uint8_t  addr4_b[4];
		uint16_t port4_b;

		uint8_t  addr4_c[4];
		uint16_t port4_c;

		unsigned char node_random_sequence[NODE_RANDOM_SEQUENCE_SIZE];
		unsigned char node_random_sequence_sign[ED25519_SIGN_SIZE];

		uint64_t src_ts_usec;
		uint64_t idx_ts_usec;

		char text[32];

		char attachment[INDEX_ATTACHMENT_SIZE];

	}data;

	unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) push_addr_frame_t;

#define PUSH_ADDR_FRAME_PAYLOAD_SIZE    (sizeof(gnb_payload16_t) + sizeof(push_addr_frame_t))

typedef struct _detect_addr_frame_t {

	struct __attribute__((__packed__)) detect_addr_frame_data {

		unsigned char arg0;              // 'd' first detect 'e' echo detect '\0'
		unsigned char arg1;              //保留
		unsigned char arg2;              //保留
		unsigned char arg3;              //保留

		unsigned char src_key512[64];    //发送节点的key
		uint32_t src_uuid32;             //发送方的uuid32,可选
		uint32_t dst_uuid32;             //目的方的uuid32,可选

		uint64_t src_ts_usec;            //发送方的时间戳

		char text[32];

		char attachment[INDEX_ATTACHMENT_SIZE];

	}data;

	unsigned char src_sign[ED25519_SIGN_SIZE];

}__attribute__ ((__packed__)) detect_addr_frame_t;


#pragma pack(pop)

#endif
