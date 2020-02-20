#ifndef gnb_address_type_h
#define gnb_address_type_h

#include "gnb_platform.h"


#ifdef __UNIX_LIKE_OS__
#include <netinet/in.h>
#include <sys/socket.h>
#endif


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif


typedef struct _gnb_address_t{

	int  type; //AF_INET AF_INET6

	//最后更新时间
	uint64_t ts_sec;

	//延时 可以经过ping pong后算出
	uint64_t delay_usec;

	union{
		uint8_t  addr4[4];
		uint8_t  addr6[16];
	}address;

	#define m_address4 address.addr4
	#define m_address6 address.addr6

	//网络字节序
	uint16_t port;

}gnb_address_t;


typedef struct _gnb_address_list_t{

	size_t size;
	size_t num;
	uint64_t update_sec;
	gnb_address_t array[0];

}gnb_address_list_t;


#define GNB_MAX_ADDR_RING 128

typedef struct _gnb_address_ring_t{

	int cur_index;

	gnb_address_list_t *address_list;

}gnb_address_ring_t;


typedef struct _gnb_sockaddress_t{

	int  addr_type; // AF_INET AF_INET6
	int  protocol;  //SOCK_STREAM SOCK_DGRAM

	union{
		struct sockaddr_in  in;
		struct sockaddr_in6 in6;
	}addr;
    #define m_in4 addr.in
    #define m_in6 addr.in6

	//在确定 addr_type 是 AF_INET 或 AF_INET6 后，就能确定这个长度了，可以去掉
	socklen_t socklen;

}gnb_sockaddress_t;


#endif
