#ifndef gnb_conf_type_h
#define gnb_conf_type_h

#include <stdint.h>
#include <limits.h>


#ifdef _WIN32

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#endif


#define GNB_CRYPTO_TPYE_NONE (0x1)
#define GNB_CRYPTO_TPYE_XOR  (0x1 << 1)
#define GNB_CRYPTO_TPYE_RC4  (0x1 << 2)
#define GNB_CRYPTO_TPYE_AES  (0x1 << 3)

#define GNB_CRYPTO_KEY_UPDATE_INTERVAL_NONE    0x0
#define GNB_CRYPTO_KEY_UPDATE_INTERVAL_MINUTE  0x1
#define GNB_CRYPTO_KEY_UPDATE_INTERVAL_HOUR    0x2

#define GNB_MULTI_ADDRESS_TYPE_SIMPLE_FAULT_TOLERANT    0x1
#define GNB_MULTI_ADDRESS_TYPE_SIMPLE_LOAD_BALANCE      0x2
#define GNB_MULTI_ADDRESS_TYPE_FULL                     0x3


#define GNB_WORKER_MIN_QUEUE         16
#define GNB_WORKER_MAX_QUEUE         4096

typedef struct _gnb_conf_t {

	char conf_dir[PATH_MAX];

	uint32_t local_uuid;

	char binary_dir[PATH_MAX];

	char map_file[PATH_MAX+NAME_MAX];

	char pid_file[PATH_MAX+NAME_MAX];

	char node_cache_file[PATH_MAX+NAME_MAX];

	char log_path[PATH_MAX];

	uint8_t log_console_level;
	uint8_t log_file_level;
	uint8_t log_udp_level;

	uint8_t log_udp_type;

	char log_udp_sockaddress4_string[16 + 1 + sizeof("65535")];

	char ifname[256];

	//由 IFNAMSIZ 定义为16
	char socket_ifname[16];

	int mtu;

	unsigned char crypto_type;
	unsigned char crypto_key_update_interval;

	unsigned char multi_index_type;
	unsigned char multi_forward_type;

	unsigned char if_dump;

	unsigned char udp_socket_type;

	uint8_t multi_socket;

	uint8_t forward_all;

	uint8_t direct_forward;

	uint8_t activate_index_worker;

	uint8_t activate_tun;

	uint8_t ufwd;

	unsigned char ufwd_passcode[4];


	char listen_address4_string[16 + 1 + sizeof("65535")];

	#define GNB_MAX_UDP6_SOCKET_NUM 4
	#define GNB_MAX_UDP4_SOCKET_NUM 16

	//host 格式存放
	uint16_t udp6_ports[GNB_MAX_UDP6_SOCKET_NUM];
	uint16_t udp4_ports[GNB_MAX_UDP4_SOCKET_NUM];

	//upnp 映射的端口，可由 upnp client 写入
	uint16_t udp4_ext_ports[GNB_MAX_UDP4_SOCKET_NUM];

	uint8_t udp6_socket_num;
	uint8_t udp4_socket_num;

	uint16_t node_woker_queue_length;
	uint16_t index_woker_queue_length;

	uint16_t port_detect_start;
	uint16_t port_detect_end;
	uint16_t port_detect_range;

	uint8_t addr_secure;

	uint8_t daemon;

	uint8_t quiet;

}gnb_conf_t;


#endif
