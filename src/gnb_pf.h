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

#ifndef GNB_PF_H
#define GNB_PF_H


/*

packet filter call_back order

pf install 0 ~ n gnb_pf_dump -> gnb_pf_route_xx -> gnb_pf_crypto_xx -> gnb_pf_zip

gnb_pf_tun:

 pf_tun_frame    gnb_pf_dump[+] -> gnb_pf_route_xx [-] -> gnb_pf_crypto_xx[-] ->  gnb_pf_zip[-]
 pf_tun_route    gnb_pf_dump[-] -> gnb_pf_route_xx [+] -> gnb_pf_crypto_xx[+] ->  gnb_pf_zip[+]
 pf_tun_fwd       gnb_pf_zip[-] -> gnb_pf_crypto_xx[+] ->  gnb_pf_route_xx[-] -> gnb_pf_dump[+]

gnb_pf_inet:

 pf_inet_frame    gnb_pf_zip[-] -> gnb_pf_crypto_xx[+] -> gnb_pf_route_xx [+] -> gnb_pf_dump[-]
 pf_inet_route   gnb_pf_dump[-] ->  gnb_pf_route_xx[+] -> gnb_pf_crypto_xx[+] ->  gnb_pf_zip[+]
 pf_inet_fwd     gnb_pf_dump[+] ->  gnb_pf_route_xx[-] -> gnb_pf_crypto_xx[+] ->  gnb_pf_zip[-]



┌──────────────────────────┬────────────────────────────────────────────────────────────┐
│  gnb payload header      │                      gnb payload data                      │
├────────┬────────┬────────┼────────────────────────┬─────────────────┬─────────────────┤
│  size  │  type  │sub type│ gnb route frame header │     ip frame    │  relay node id  │
├────────┼────────┼────────┼────────────────────────┼─────────────────┼─────────────────┤
│ 2 byte │ 1 byte │ 1 byte │                        │                 │ variable-length │
└────────┴────────┴────────┴────────────────────────┴─────────────────┴─────────────────┘


┌──────────────────────────┬──────────────────────────────────────────────────────────────────┐
│  gnb payload header      │                      gnb payload data                            │
├────────┬────────┬────────┼────────────────────────┬─────────────────────┬───────────────────┤
│  size  │  type  │sub type│ gnb route frame header │       ip frame      │   relay node id   │
├────────┼────────┼────────┼────────────────────────┼─────────────────────┼───────────────────┤
│ 2 byte │ 1 byte │ 1 byte │                        │                     │  variable-length  │
├────────┴────────┴────────┼────────────────────────┼─────────────────────┼──────────┬────────┤
│                          │                        ├──  crypto segment  ─┤          │ 4 byte │
│                          ├─────────────────────    relay crypto segment    ────────┤        │
│                          ├──────────────────      deflate/inflate segment     ──────────────┤


*/


#include <stdio.h>
#include <stdint.h>

typedef struct _gnb_core_t gnb_core_t;

typedef struct _gnb_payload16_t gnb_payload16_t;

typedef struct _gnb_node_t  gnb_node_t;

typedef struct _gnb_sockaddress_t gnb_sockaddress_t;

typedef struct _gnb_pf_ctx_t {

	int pf_fwd;

	int pf_status;

	gnb_uuid_t src_fwd_uuid64;
	uint8_t  in_ttl;

	gnb_uuid_t src_uuid64;
	gnb_uuid_t dst_uuid64;


	gnb_node_t *src_fwd_node;

	//转发到下一跳的node
	gnb_node_t *fwd_node;

	gnb_node_t *src_node;

	//最终目的node, 在 tun_frame_cb 中根据 ip header 的 dst 查表获得，
	//在 inet_frame_cb inet_route_cb 可以根据frame中 dst_uuid 查表获得
	gnb_node_t *dst_node;

	gnb_sockaddress_t *source_node_addr;

	gnb_payload16_t *fwd_payload;

	//使用场景是 main pf 定义了 payload 中的报文，需要通过 gnb_pf_ctx_t 报文里的一些字段需要暴露给下一个pf处理
	//指向 fwd_payload 中的一个 byte 用于 pf 模块用于标识 payload 类型
	unsigned char *pf_type_bits;

	//指向 fwd_payload 中的ip分组首地址
	void   *ip_frame;
	ssize_t ip_frame_size;
	uint8_t ipproto;

	uint8_t relay_forwarding;
	uint8_t unified_forwarding;
	uint8_t direct_forwarding;

	uint8_t std_forwarding;

	uint8_t universal_udp4_relay;

}gnb_pf_ctx_t;


#define GNB_PF_ERROR    0xFF    //当前PF模块过程中出错了，上层调用应该终止这个分组的处理
#define GNB_PF_NEXT     0x00    //当前PF模块处理完成，可以进行一个PF模块的处理，如果是最后一个调用的PF模块， 上层调用
#define GNB_PF_FINISH   0x01    //当前PF模块认为数据分组的处理应该到此为止，上层调用收到这个返回，就不再调用后面的PF模块处理
#define GNB_PF_DROP     0x02    //当前PF模块认为该数据分组应被丢弃
#define GNB_PF_NOROUTE  0x03    //没有找到转发的节点


#define GNB_PF_FWD_INIT 0x0
#define GNB_PF_FWD_TUN  0x1
#define GNB_PF_FWD_INET 0x2

typedef struct _gnb_pf_t gnb_pf_t;

typedef void(*gnb_pf_init_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf);

typedef void(*gnb_pf_conf_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf);

/*
  tun packet filter step 1:
  pf_ctx->fwd_payload->data 中存放的是 从tun设备中得到的数据分组,
  尽可能不在此 call back 中改变 pf_ctx->fwd_payload->data 的内容使得后面调用的 pf 的处理过程能够访问到原始的来自tun的数据分组
*/
typedef int(*gnb_pf_tun_frame_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx);

/*
  tun packet filter step 2:
  在此 call back 中可以确定 payload 的目的节点，对数据分组进行加密，修改 pf_ctx->fwd_payload 的长度
*/
typedef int(*gnb_pf_tun_route_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx);


/*
  tun packet filter step 3:
  如果下一跳是 realy 节点，可以在这里做一次加密
*/
typedef int(*gnb_pf_tun_fwd_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx);


/*
  inet packet filter step 1:

*/
typedef int(*gnb_pf_inet_frame_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx);


/*
  inet packet filter step 2:

*/
typedef int(*gnb_pf_inet_route_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx);


/*
  inet packet filter step 3:
  对来自其他节点的 payload 进行中继时可以在此 call back 中对中转的 payload 加密
*/
typedef int(*gnb_pf_inet_fwd_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf, gnb_pf_ctx_t *pf_ctx);


typedef void(*gnb_pf_release_cb_t)(gnb_core_t *gnb_core, gnb_pf_t *pf);


typedef struct _gnb_pf_t {

	const char *name;

	void *private_ctx;

	gnb_pf_init_cb_t    pf_init;
	gnb_pf_conf_cb_t    pf_conf;

	gnb_pf_tun_frame_cb_t  pf_tun_frame;       //按照 pf 数组的正序调用 0 ~ n
	gnb_pf_tun_route_cb_t  pf_tun_route;       //按照 pf 数组的正序调用 0 ~ n
	gnb_pf_tun_fwd_cb_t    pf_tun_fwd;         //按照 pf 数组的倒序调用 n ~ 0

	gnb_pf_inet_frame_cb_t pf_inet_frame;      //按照 pf 数组的倒序调用 n ~ 0
	gnb_pf_inet_route_cb_t pf_inet_route;      //按照 pf 数组的倒序调用 n ~ 0
	gnb_pf_inet_fwd_cb_t   pf_inet_fwd;        //按照 pf 数组的正序调用 0 ~ n

	gnb_pf_release_cb_t  pf_release;

}gnb_pf_t;


typedef struct _gnb_pf_array_t {

	size_t size;
	size_t num;
	gnb_pf_t *pf[0];

}gnb_pf_array_t;


void gnb_pf_init(gnb_core_t *gnb_core, gnb_pf_array_t *pf_array);

void gnb_pf_conf(gnb_core_t *gnb_core, gnb_pf_array_t *pf_array);

void gnb_pf_tun(gnb_core_t *gnb_core, gnb_pf_array_t *pf_array, gnb_payload16_t *payload);

void gnb_pf_inet(gnb_core_t *gnb_core, gnb_pf_array_t *pf_array, gnb_payload16_t *payload, gnb_sockaddress_t *source_node_addr);

void gnb_pf_release(gnb_core_t *gnb_core, gnb_pf_array_t *pf_array);

void gnb_pf_status_strings_init();

gnb_pf_array_t * gnb_pf_array_init(gnb_heap_t *heap, int size);

int gnb_pf_install(gnb_pf_array_t *pf_array, gnb_pf_t *pf);

#endif
