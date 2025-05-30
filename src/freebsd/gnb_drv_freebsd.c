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
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netdb.h>

#include <net/if.h>
#include <net/if_tun.h>
#include <net/route.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include "gnb.h"
#include "gnb_tun_drv.h"
#include "gnb_payload16.h"


void bind_socket_if(gnb_core_t *gnb_core){

    if ( '\0' == gnb_core->conf->socket_ifname[0] ) {
        return;
    }

    int i;

    for ( i=0; i < gnb_core->conf->udp6_socket_num; i++ ) {
        setsockopt(gnb_core->udp_ipv6_sockets[i], SOL_SOCKET,IP_RECVIF, gnb_core->conf->socket_ifname, strlen(gnb_core->conf->socket_ifname));
    }

    for ( i=0; i < gnb_core->conf->udp4_socket_num; i++ ) {
        setsockopt(gnb_core->udp_ipv4_sockets[i], SOL_SOCKET,IP_RECVIF, gnb_core->conf->socket_ifname, strlen(gnb_core->conf->socket_ifname));
    }

}


static void if_up_script(gnb_core_t *gnb_core){

    int ret;

    char cmd[1024];

    snprintf(cmd,1024,"\"%s/scripts/%s\" > /dev/null 2>&1",gnb_core->conf->conf_dir,"if_up_freebsd.sh");

    ret = system(cmd);

    if ( -1==ret || 0 ==ret ) {
        return;
    }

    return;

}

static void if_down_script(gnb_core_t *gnb_core){

    int ret;

    char cmd[1024];

    snprintf(cmd,1024,"\"%s/scripts/%s\" > /dev/null 2>&1",gnb_core->conf->conf_dir,"if_down_freebsd.sh");

    ret = system(cmd);

    if ( -1==ret || 0 ==ret ) {
        return;
    }

    return;

}


/*
 set_route4的作用是 创建一条路由，以 tun ip 为 10.1.0.15 的local node为例：
 route -n add -net 10.1.0.0 -netmask 255.255.255.0 10.1.0.15
 */
static void set_route4(gnb_core_t *gnb_core){

    struct{
        struct  rt_msghdr hdr;
        struct  sockaddr_in dst;
        struct  sockaddr_in gateway;
        struct  sockaddr_in mask;
    }rtmsg;
    
    uint32_t network_u32;
    
    int     s;
    
    ssize_t wlen;
    
    //算出 节点的 ipv4 network
    network_u32 = gnb_core->local_node->tun_netmask_addr4.s_addr & gnb_core->local_node->tun_addr4.s_addr;
    
    s = socket(PF_ROUTE, SOCK_RAW, 0);
    
    if ( s < 0 ) {
        printf("error: socket\n");
        return;
    }
    
    shutdown(s, SHUT_RD);
    
    bzero(&rtmsg, sizeof(rtmsg));
    
    rtmsg.hdr.rtm_type = RTM_ADD;
    rtmsg.hdr.rtm_version = RTM_VERSION;
    
    rtmsg.hdr.rtm_addrs = 0;
    
    rtmsg.hdr.rtm_addrs |= RTA_DST;
    rtmsg.hdr.rtm_addrs |= RTA_GATEWAY;
    rtmsg.hdr.rtm_addrs |= RTA_NETMASK;
    
    
    rtmsg.hdr.rtm_flags = RTF_STATIC;
    rtmsg.hdr.rtm_flags |= RTF_GATEWAY;
    rtmsg.hdr.rtm_flags |= RTF_GATEWAY;
    
    rtmsg.dst.sin_len = sizeof(rtmsg.dst);
    rtmsg.dst.sin_family = AF_INET;
    rtmsg.dst.sin_addr.s_addr = network_u32;
    
    rtmsg.mask.sin_len = sizeof(rtmsg.mask);
    rtmsg.mask.sin_family = AF_INET;
    rtmsg.mask.sin_addr.s_addr = gnb_core->local_node->tun_netmask_addr4.s_addr;
    
    
    rtmsg.gateway.sin_len = sizeof(rtmsg.gateway);
    rtmsg.gateway.sin_family = AF_INET;
    rtmsg.gateway.sin_addr.s_addr = gnb_core->local_node->tun_addr4.s_addr;
    
    rtmsg.hdr.rtm_msglen = sizeof(rtmsg);
    
    wlen = write(s, &rtmsg, sizeof(rtmsg));
    
    if ( -1==wlen ) {
        perror("#set_route4 write");
        return;
    }
    
    return;
    
}

static void setifmtu(char *if_name,int mtu) {

    int socket_fd;

    struct    ifreq ifr;

    memset(&ifr,0,sizeof(struct    ifreq));

    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

    ifr.ifr_mtu = mtu;

    if ( (socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket ");

    }

    int ret =  ioctl(socket_fd, SIOCSIFMTU, &ifr);

    if ( -1==ret ) {
        perror("ioctl");
    }

    close(socket_fd);
}


static int set_addr4(char *if_name, char *ip, char *netmask) {
    
    struct addrinfo *srcres, *dstres, *netmaskres;
    
    struct ifaliasreq in_addreq;
    
    memset(&in_addreq, 0, sizeof(in_addreq));
    
    getaddrinfo(ip, NULL, NULL, &srcres);
    getaddrinfo(ip, NULL, NULL, &dstres);
    getaddrinfo(netmask, NULL, NULL, &netmaskres);
    
    strncpy(in_addreq.ifra_name,    if_name, IFNAMSIZ);
    memcpy(&in_addreq.ifra_addr,    srcres->ai_addr, srcres->ai_addr->sa_len);
    memcpy(&in_addreq.ifra_dstaddr, dstres->ai_addr, dstres->ai_addr->sa_len);
    memcpy(&in_addreq.ifra_mask,    netmaskres->ai_addr, netmaskres->ai_addr->sa_len);
    
    freeaddrinfo(srcres);
    freeaddrinfo(dstres);
    freeaddrinfo(netmaskres);
    
    int socket_fd;
    
    if ( (socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket ");
        return -1;
    }
    
    int ret = ioctl(socket_fd, SIOCAIFADDR, &in_addreq);
    
    if ( -1==ret ) {
        perror("ioctl");
    }
    
    close(socket_fd);

    return 0;
}



/*
struct  in6_aliasreq {
        char    ifra_name[IFNAMSIZ];
        struct  sockaddr_in6 ifra_addr;
        struct  sockaddr_in6 ifra_dstaddr;
        struct  sockaddr_in6 ifra_prefixmask;
        int     ifra_flags;
        struct in6_addrlifetime ifra_lifetime;
        int     ifra_vhid;
};
*/
static int set_addr6(char *if_name, char *ip, char *netmask) {

    //不要设置 in6_addreq.ifra_dstaddr 成员,  ioctl 会提示参数不正确
    struct in6_aliasreq in6_addreq =
      { { 0 },
        { 0 },
        { 0 },
        { 0 },
        0,
        { 0, 0, ND6_INFINITE_LIFETIME, ND6_INFINITE_LIFETIME } };

    struct addrinfo *srcres, *netmaskres;

    struct addrinfo hints;
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;

    getaddrinfo(ip, NULL, &hints, &srcres);

    getaddrinfo(netmask, NULL, &hints, &netmaskres);

    strncpy(in6_addreq.ifra_name, if_name, IFNAMSIZ);

    memcpy(&in6_addreq.ifra_addr,       srcres->ai_addr, srcres->ai_addr->sa_len);
    memcpy(&in6_addreq.ifra_prefixmask, netmaskres->ai_addr, netmaskres->ai_addr->sa_len);

    freeaddrinfo(srcres);
    freeaddrinfo(netmaskres);

    int socket_fd;

    if ( (socket_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket ");
        return -1;
    }

    int ret = ioctl(socket_fd, SIOCAIFADDR_IN6, &in6_addreq);

    if ( -1==ret ) {
        perror("ioctl");
    }

    close(socket_fd);

    return ret;
}


int init_tun_freebsd(gnb_core_t *gnb_core){

    gnb_core->tun_fd = -1;

    return 0;

}


static int open_tun_freebsd(gnb_core_t *gnb_core){

    if ( -1 != gnb_core->tun_fd ) {
        return -1;
    }
    
    char name[PATH_MAX];

    int t = 0;

    snprintf(name, PATH_MAX, "/dev/%s",gnb_core->ifname);

    gnb_core->tun_fd = open(name, O_RDWR);

    if ( -1==gnb_core->tun_fd ) {
        perror("open");
        exit(1);
    }

    int flags;
    flags = fcntl(gnb_core->tun_fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(gnb_core->tun_fd, F_SETFD, flags);


    /*
    TUNSIFHEAD The argument should be    a pointer to an    int; a non-zero    value
    turns off ``link-layer'' mode,    and enables ``multi-af'' mode,
    where every packet is preceded    with a four byte address fam-ily.
    */
    int on = 0;
    ioctl(gnb_core->tun_fd, TUNSIFHEAD, &on, sizeof(on));

    set_addr4(gnb_core->ifname, GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4),  GNB_ADDR4STR_PLAINTEXT2(&gnb_core->local_node->tun_netmask_addr4));
    set_addr6(gnb_core->ifname, GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr), "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:0000:0000");
    set_route4(gnb_core);
    setifmtu(gnb_core->ifname, gnb_core->conf->mtu);
    
    if_up_script(gnb_core);

    return 0;

}


static int read_tun_freebsd(gnb_core_t *gnb_core, void *buf, size_t buf_size){

    ssize_t rlen;
    rlen = read(gnb_core->tun_fd, buf, buf_size);
    return rlen;

}


static int write_tun_freebsd(gnb_core_t *gnb_core, void *buf, size_t buf_size){

    ssize_t wlen;
    wlen = write(gnb_core->tun_fd, buf, buf_size);
    return wlen;

}


static int close_tun_freebsd(gnb_core_t *gnb_core){

    if_down_script(gnb_core);
    close(gnb_core->tun_fd);

    return 0;

}

static int release_tun_freebsd(gnb_core_t *gnb_core){

    return 0;
}


gnb_tun_drv_t gnb_tun_drv_freebsd = {

    init_tun_freebsd,

    open_tun_freebsd,

    read_tun_freebsd,

    write_tun_freebsd,

    close_tun_freebsd,

    release_tun_freebsd

};
