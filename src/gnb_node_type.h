#ifndef gnb_node_type_h
#define gnb_node_type_h

#include "stdint.h"

#include "gnb_address_type.h"

typedef struct _gnb_node_t{

	uint32_t uuid32;

	uint64_t in_bytes;
	uint64_t out_bytes;

	#define GNB_NODE_TYPE_STD               (0x0)
	#define GNB_NODE_TYPE_IDX               (0x1)
	#define GNB_NODE_TYPE_FWD               (0x1 << 1)

	#define GNB_NODE_TYPE_STATIC_ADDR       (0x1 << 2)
	#define GNB_NODE_TYPE_DYNAMIC_ADDR      (0x1 << 3)

	unsigned char type;

	struct in_addr  tun_addr4;
	struct in_addr  tun_netmask_addr4;
	struct in_addr  tun_subnet_addr4;
	uint16_t tun_sin_port4;

	struct in6_addr tun_ipv6_addr;

	struct sockaddr_in  udp_sockaddr4;
	struct sockaddr_in6 udp_sockaddr6;

	uint8_t            socket6_idx;
	uint8_t            socket4_idx;


	#define GNB_NODE_STATIC_ADDRESS_NUM   6
	#define GNB_NODE_DYNAMIC_ADDRESS_NUM 16
	#define GNB_NODE_RESOLV_ADDRESS_NUM   6
	#define GNB_NODE_PUSH_ADDRESS_NUM     6

	//静态地址，来自 address.conf 不更新,send_detect_addr_frame 时使用
	unsigned char static_address_block[sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*GNB_NODE_STATIC_ADDRESS_NUM];

	//在 handle_detect_addr_frame 中更新,send_detect_addr_frame 时使用
	unsigned char dynamic_address_block[sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*GNB_NODE_DYNAMIC_ADDRESS_NUM];

	//通过 address.conf 中的域名异步更新这个表,send_detect_addr_frame 时使用
	unsigned char resolv_address_block[sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*GNB_NODE_RESOLV_ADDRESS_NUM];

	//handle_push_addr_frame时更新, 及在启动时加载自文件缓存
	unsigned char push_address_block[sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*GNB_NODE_PUSH_ADDRESS_NUM];


	unsigned char detect_address4_block[sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*3];
	uint8_t         detect_address4_idx;
	struct in_addr  detect_addr4;
	uint16_t        detect_port4;


	#define GNB_NODE_MAX_DETECT_TIMES 32
	int detect_count;

	//上次发对该node发送 ping 的时间戳，不区分ipv4和ipv6
	uint64_t ping_ts_sec;
	uint64_t ping_ts_usec;

	#define GNB_NODE_STATUS_UNREACHABL  (0x0)
	#define GNB_NODE_STATUS_IPV4        (0x1)
	#define GNB_NODE_STATUS_IPV6        (0x1 << 1)

	//初始值为 GNB_NODE_STATUS_UNREACHABL, 实现ping pong 后用 GNB_NODE_STATUS_IPV4  GNB_NODE_STATUS_IPV6 置位
	unsigned int udp_addr_status;

	int64_t addr6_ping_latency_usec;
	int64_t addr4_ping_latency_usec;

	//上次node发来 ping4 或 pong4 时间戳
	int addr4_update_ts_sec;
	//上次node发来 ping6 或 pong6 时间戳
	int addr6_update_ts_sec;

	//ed25519 public key
	unsigned char public_key[32];

	//ed25519 产生的 share key
	unsigned char shared_secret[32];

	//shared_secret 与 gnb_core->time_seed & 运算后再经过sha512的摘要信息
	unsigned char crypto_key[64];

	unsigned char key512[64];


	//上次向 index 节点查询的时间戳
	uint64_t last_request_addr_sec;

	uint64_t last_push_addr_sec;

	uint64_t last_detect_sec;


}gnb_node_t;


#define GNB_MAX_NODE_RING 128
typedef struct _gnb_node_ring_t{

	int num;
	int cur_index;
	gnb_node_t *nodes[GNB_MAX_NODE_RING];

}gnb_node_ring_t;


#endif

