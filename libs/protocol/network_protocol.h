#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <stdio.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <netdb.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#endif


#if defined(_WIN32)
#include <windows.h>
#endif


#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef ARPHRD_ETHER
#define ARPHRD_ETHER 1
#endif

#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif

#ifndef ETH_P_ARP
#define ETH_P_ARP 0x0806
#endif

#ifndef ETH_P_IPV6
#define ETH_P_IPV6 0x86DD
#endif

#ifndef ETH_P_8021Q
#define ETH_P_8021Q 0x8100
#endif

struct iphdr {

#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
	u_char	ihl:4,		    /* header length */
        version:4;	        /* version */
#endif

#if __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
	u_char	version:4,	    /* version */
    ihl:4;		            /* header length */
#endif

    u_char  tos;            /* type of service */
    short   tot_len;        /* total length */
    u_short id;             /* identification */
    short   off;            /* fragment offset field */
    u_char  ttl;            /* time to live */
    u_char  protocol;       /* protocol */
    u_short check;          /* checksum */
    struct  in_addr saddr;
    struct  in_addr daddr;  /* source and dest address */

};


#pragma pack(push, 1)

struct gnb_in6_addr {

    union {
	  uint8_t  __u6_addr8[16];
	  uint16_t __u6_addr16[8];
	  uint32_t __u6_addr32[4];
    } __in6_u;

}__attribute__ ((__packed__));

struct ip6_hdr {

  	union {

  		struct ip6_hdrctl {
  			uint32_t ip6_un1_flow;	/* 20 bits of flow-ID */
  			uint16_t ip6_un1_plen;	/* payload length */
  			uint8_t  ip6_un1_nxt;	/* next header */
  			uint8_t  ip6_un1_hlim;	/* hop limit */
  		} ip6_un1;

  		uint8_t ip6_un2_vfc;	    /* 4 bits version, top 4 bits class */

  	} ip6_ctlun;

  	struct gnb_in6_addr ip6_src;	/* source address */
  	struct gnb_in6_addr ip6_dst;	/* destination address */

} __attribute__ ((__packed__));

#pragma pack(pop)

#endif
