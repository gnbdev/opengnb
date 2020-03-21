#ifndef gnb_ctl_block_h
#define gnb_ctl_block_h

#include <stdint.h>

#include "gnb_conf_type.h"
#include "gnb_node_type.h"
#include "gnb_log_type.h"


typedef struct _gnb_ctl_magic_number_t {

	unsigned char data[16];

}gnb_ctl_magic_number_t;


typedef struct _gnb_ctl_conf_zone_t {

	unsigned char name[8];

	gnb_conf_t conf_st;

}gnb_ctl_conf_zone_t;


typedef struct _gnb_ctl_core_zone_t {

	unsigned char name[8];

	uint32_t  tun_if_id;
	unsigned char ifname[256];
	unsigned char if_device_string[256];

	uint32_t local_uuid;


	unsigned char wan_addr6[16];
	uint16_t wan_port6;   //网络字节序

	unsigned char wan_addr4[4];
	uint16_t wan_port4;   //网络字节序

	gnb_log_ctx_t log_ctx_st;

	unsigned char index_address_block[ sizeof(gnb_address_list_t) + sizeof(gnb_address_t) * GNB_MAX_ADDR_RING ];

}gnb_ctl_core_zone_t;


typedef struct _gnb_ctl_status_zone_t {

	int var;

}gnb_ctl_status_zone_t;


typedef struct _gnb_ctl_node_zone_t {

	unsigned char name[8];
	int node_num;
	gnb_node_t node[0];

}gnb_ctl_node_zone_t;



typedef struct _gnb_ctl_block_t {

	uint32_t *entry_table256;

	gnb_ctl_magic_number_t *magic_number;

	gnb_ctl_conf_zone_t    *conf_zone;

	gnb_ctl_core_zone_t    *core_zone;

	gnb_ctl_status_zone_t  *status_zone;

	gnb_ctl_node_zone_t    *node_zone;

}gnb_ctl_block_t;


ssize_t gnb_ctl_file_size(const char *filename);

gnb_ctl_block_t *gnb_ctl_block_build(void *memory,size_t node_num);

gnb_ctl_block_t *gnb_ctl_block_setup(void *memory);


gnb_ctl_block_t *gnb_ctl_block_init_readwrite(const char *ctl_block_file);

gnb_ctl_block_t *gnb_ctl_block_init_readonly(const char *ctl_block_file);

#define MIN_CTL_BLOCK_FILE_SIZE  (sizeof(uint32_t)*256 + sizeof(gnb_ctl_magic_number_t) + sizeof(gnb_ctl_conf_zone_t) + sizeof(gnb_ctl_core_zone_t) + sizeof(gnb_ctl_status_zone_t) + sizeof(gnb_ctl_node_zone_t) + sizeof(gnb_node_t))

#endif

