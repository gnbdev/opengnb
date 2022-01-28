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

#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <netinet/in.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "gnb_payload16.h"

gnb_payload16_t* gnb_payload16_init(char type,uint16_t data_size){

    gnb_payload16_t *gnb_payload16 = (gnb_payload16_t *)malloc(sizeof(gnb_payload16_t) + sizeof(char)*data_size);

    memset(gnb_payload16, 0, sizeof(gnb_payload16_t) + sizeof(char)*data_size);

    gnb_payload16->type = type;

    gnb_payload16->size = htons(sizeof(gnb_payload16_t) + sizeof(char)*data_size);

    return gnb_payload16;
}

gnb_payload16_t* gnb_payload16_create(char type, void *data, uint16_t data_size){

    gnb_payload16_t *gnb_payload16 = gnb_payload16_init(type,data_size);

    memcpy(gnb_payload16->data,data,data_size);

    return gnb_payload16;

}

gnb_payload16_t *gnb_payload16_dup(gnb_payload16_t *gnb_payload16_in){

    uint16_t size = ntohs(gnb_payload16_in->size);

    gnb_payload16_t *gnb_payload16 = (gnb_payload16_t *)malloc( size );

    memcpy(gnb_payload16, gnb_payload16_in, size);

    return gnb_payload16;

}

uint16_t gnb_payload16_set_size(gnb_payload16_t *gnb_payload16, uint16_t new_size){

    gnb_payload16->size = htons(new_size);
    return new_size;

}

uint16_t gnb_payload16_size(gnb_payload16_t *gnb_payload16){
    uint16_t size = ntohs(gnb_payload16->size);
    return size;
}

uint16_t gnb_payload16_set_data_len(gnb_payload16_t *gnb_payload16, uint16_t new_len){
    gnb_payload16->size = htons(new_len+GNB_PAYLOAD16_HEAD_SIZE);
    return new_len;
}

uint16_t gnb_payload16_data_len(gnb_payload16_t *gnb_payload16){
    uint16_t size = ntohs(gnb_payload16->size) - GNB_PAYLOAD16_HEAD_SIZE;
    return size;
}

void gnb_payload16_free(gnb_payload16_t *gnb_payload16){
    free(gnb_payload16);
}


gnb_payload16_ctx_t* gnb_payload16_ctx_init(uint16_t max_payload_size){

    gnb_payload16_ctx_t *gnb_payload16_ctx = (gnb_payload16_ctx_t*)malloc(sizeof(gnb_payload16_ctx_t));

    gnb_payload16_ctx->r_len = 0;
    gnb_payload16_ctx->gnb_payload16    = gnb_payload16_init(0x0, max_payload_size);
    gnb_payload16_ctx->max_payload_size = max_payload_size;
    gnb_payload16_ctx->udata = NULL;

    return gnb_payload16_ctx;

}


void gnb_payload16_ctx_free(gnb_payload16_ctx_t *gnb_payload16_ctx) {

    free(gnb_payload16_ctx->gnb_payload16);
    free(gnb_payload16_ctx);

}


#define GNB_FRAME_STATUS_HEAD 0
#define GNB_FRAME_STATUS_PART 1
int gnb_payload16_handle(void *data, size_t data_size, gnb_payload16_ctx_t *gnb_payload16_ctx, gnb_payload16_handle_cb_t cb) {

    uint32_t receive_bytes_offset = 0;

    ssize_t remain_data_len;

    int frame_status;

    uint16_t frame_size;

    //这里检查到来的数据是否刚好是一个完整的frame，这样对于短小的frame，可以提升处理速度
    if ( 0 == gnb_payload16_ctx->r_len && data_size > GNB_PAYLOAD16_HEAD_SIZE ){

        frame_size = ntohs(*(uint16_t *)data);

        if ( 0==frame_size ) {
            return -1;
        }

        if ( (int)frame_size > GNB_MAX_PAYLOAD_SIZE ) {
            return -2;
        }

        if ( frame_size == data_size) {
            
            if ( frame_size > gnb_payload16_ctx->max_payload_size ) {
                return -3;
            }

            memcpy( (void *)gnb_payload16_ctx->gnb_payload16, data, frame_size);

            cb(gnb_payload16_ctx->gnb_payload16, gnb_payload16_ctx->udata);
            return 0;
        }

    }


    do{

        remain_data_len = data_size - receive_bytes_offset;

        if ( gnb_payload16_ctx->r_len >= 2 ) {
            frame_status = GNB_FRAME_STATUS_PART;
        } else {
            frame_status = GNB_FRAME_STATUS_HEAD;
        }

        switch ( frame_status ) {

        case GNB_FRAME_STATUS_PART:
            //对于长的frame，这个case出现的概率比较大，所以放前面
            frame_size = ntohs(*(uint16_t *)gnb_payload16_ctx->buffer);

            if ( frame_size > gnb_payload16_ctx->max_payload_size ) {
                return -4;
            }

            if ( remain_data_len >= (frame_size - gnb_payload16_ctx->r_len) ) {

                //未处理的数据 大于 整个fram的未接收的数据，此时可以得到一个完整的frame
                memcpy( (void *)gnb_payload16_ctx->gnb_payload16+gnb_payload16_ctx->r_len, data+receive_bytes_offset, (frame_size-gnb_payload16_ctx->r_len) );
                receive_bytes_offset  += (frame_size-gnb_payload16_ctx->r_len);

                gnb_payload16_ctx->r_len += (frame_size-gnb_payload16_ctx->r_len);

                //得到一个完整的 frame 通过 callback函数 传进去处理
                cb(gnb_payload16_ctx->gnb_payload16, gnb_payload16_ctx->udata);

                gnb_payload16_ctx->r_len = 0;
                //此时由于 gnb_payload16_ctx->r_len == 0，状态会切换到 FE_FRAME_STATUS_HEAD

            } else {

                //未处理的数据 小于 整个fram的未接收的数据，此时还不可以得到一个完整的frame
                //退出处理函数，等下次数据的到来
                memcpy( (void *)gnb_payload16_ctx->gnb_payload16+gnb_payload16_ctx->r_len, data+receive_bytes_offset, remain_data_len);

                gnb_payload16_ctx->r_len += remain_data_len;

                receive_bytes_offset  += remain_data_len;
                return 0;

            }

            break;

        case GNB_FRAME_STATUS_HEAD:

            //这里其实不算太复杂，就是要处理一些极端的情况
            if ( 0 == gnb_payload16_ctx->r_len ) {
                //ctx的暂存区为空的情况下，未处理的数据大于 2 byte，此时可以获得frame的首部
                if ( remain_data_len >=2 ) {

                    memcpy(gnb_payload16_ctx->buffer, data+receive_bytes_offset, 2);
                    receive_bytes_offset += 2;
                    gnb_payload16_ctx->r_len = 2;

                } else {
                    //ctx的暂存区为空的情况下，未处理的数据小于 2 byte，此时无法获得frame的首部
                    //先把 < 2byte 的数据放入ctx的暂存区
                    memcpy(gnb_payload16_ctx->buffer, data+receive_bytes_offset, remain_data_len);
                    receive_bytes_offset += remain_data_len;
                    gnb_payload16_ctx->r_len = remain_data_len;

                    return 0;
                }

            } else if ( gnb_payload16_ctx->r_len > 0 ) {

                if ( remain_data_len >= (2 - gnb_payload16_ctx->r_len) ) {
                    //ctx的暂存区已经有数据的情况下，ctx的暂存区的数据 合并上 未处理的数据能够取到 2byte 的frame首部
                    memcpy(gnb_payload16_ctx->buffer+gnb_payload16_ctx->r_len, data+receive_bytes_offset, 2-gnb_payload16_ctx->r_len);
                    receive_bytes_offset  += (2-gnb_payload16_ctx->r_len);
                    gnb_payload16_ctx->r_len += (2-gnb_payload16_ctx->r_len);

                } else {
                    //ctx的暂存区已经有数据的情况下，ctx的暂存区的数据 合并上 未处理的数据还是不足取到 2byte 的frame首部
                    //退出处理函数，等下次数据的到来
                    memcpy(gnb_payload16_ctx->buffer+gnb_payload16_ctx->r_len, data+receive_bytes_offset, remain_data_len);

                    gnb_payload16_ctx->r_len += remain_data_len;

                    return 0;
                }

            }

            if ( 2 == gnb_payload16_ctx->r_len ) {
                //已经取到2byte 的frame首部

                frame_size = ntohs(*(uint16_t *)gnb_payload16_ctx->buffer);

                //检查frame首部(frame长度)是否合法
                if ( 0 == frame_size ) {
                    return -1;
                }

                if ( (int)frame_size > GNB_MAX_PAYLOAD_SIZE ) {
                    return -2;
                }

                //根据 frame_size 创建 payload 结构体
                if ( frame_size > gnb_payload16_ctx->max_payload_size ) {
                    return -3;
                }

                memcpy( (void *)gnb_payload16_ctx->gnb_payload16, gnb_payload16_ctx->buffer, 2);
                //此时，由于gnb_payload16_ctx->r_len == 2，因此在循环的时候，状态会切换到 FE_FRAME_STATUS_PART

            }

            break;

        default:
            //Error！！！
            break;
        }

    }while( (data_size - receive_bytes_offset) > 0 );

    return 0;
}

