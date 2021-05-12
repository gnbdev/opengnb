#ifndef gnb_payload16_h
#define gnb_payload16_h

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma pack(push, 1)
typedef struct _gnb_payload16_t{
	uint16_t size;
	unsigned char type;
	unsigned char sub_type;
	unsigned char data[0];
} __attribute__ ((packed)) gnb_payload16_t;
#pragma pack(pop)

#define GNB_PAYLOAD16_HEAD_SIZE 4

#define GNB_MAX_PAYLOAD_SIZE 65535

gnb_payload16_t* gnb_payload16_init(char type,uint16_t data_size);

gnb_payload16_t* gnb_payload16_create(char type, void *data, uint16_t data_size);

gnb_payload16_t *gnb_payload16_dup(gnb_payload16_t *gnb_payload16_in);

void gnb_payload16_free(gnb_payload16_t *gnb_payload16);

uint16_t gnb_payload16_set_size(gnb_payload16_t *gnb_payload16, uint16_t new_size);

uint16_t gnb_payload16_size(gnb_payload16_t *gnb_payload16);

uint16_t gnb_payload16_set_data_len(gnb_payload16_t *gnb_payload16, uint16_t new_len);

uint16_t gnb_payload16_data_len(gnb_payload16_t *gnb_payload16);

typedef struct _gnb_payload16_ctx_t{

	//当前 当前payload(frame) 已收到的字节数
	//r_len足2字节时，接收的数据存 gnb_payload
	//r_len 不足2字节时，接收的数据存 buffer
	int r_len;

	unsigned char buffer[2];

	//传入payload
	gnb_payload16_t *gnb_payload16;
	//传入的 payload 内存块大小
	uint32_t max_payload_size;

	void *udata;

}gnb_payload16_ctx_t;

gnb_payload16_ctx_t* gnb_payload16_ctx_init(uint16_t max_payload_size);

void gnb_payload16_ctx_free(gnb_payload16_ctx_t *gnb_payload16_ctx);

typedef int (*gnb_payload16_handle_cb_t)(gnb_payload16_t *gnb_payload16, void *ctx);

int gnb_payload16_handle(void *data, size_t data_size, gnb_payload16_ctx_t *gnb_payload16_ctx, gnb_payload16_handle_cb_t cb);

#define GNB_PAYLOAD16_FRAME_SIZE(payload) gnb_payload16_size(payload)
#define GNB_PAYLOAD16_DATA_SIZE(payload)  gnb_payload16_data_len(payload)

#endif
