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

#ifndef GNB_NODE_WORKER_QUEUE_DATA_H
#define GNB_NODE_WORKER_QUEUE_DATA_H

#include <stdint.h>

typedef struct _gnb_node_t gnb_node_t;

typedef struct _gnb_payload16_t gnb_payload16_t;

typedef struct _gnb_sockaddress_t gnb_sockaddress_t;


typedef struct _gnb_node_worker_in_data_t {

	gnb_sockaddress_t  node_addr_st;

	uint8_t            socket_idx;

	gnb_payload16_t    payload_st;

}gnb_worker_in_data_t;


typedef struct _gnb_worker_queue_data_t {

  #define GNB_WORKER_QUEUE_DATA_TYPE_NODE_IN   0x1
  #define GNB_WORKER_QUEUE_DATA_TYPE_NODE_OUT  0x2
	int  type;

  union worker_data {
	  gnb_worker_in_data_t    node_in;
  }data;

  //这里可以定义宏方便操作union

}gnb_worker_queue_data_t;

//这个块不能太大，在嵌入设备上，这里是占用内存的大头
#define GNB_NODE_WORKER_QUEUE_BLOCK_SIZE          720
#define GNB_INDEX_WORKER_QUEUE_BLOCK_SIZE         720
#define GNB_INDEX_SERVICE_WORKER_QUEUE_BLOCK_SIZE 720

#endif
