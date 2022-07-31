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
#include <signal.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include <arpa/inet.h>

#include <net/if.h>
#include <net/route.h>

#include <linux/if_tun.h>
#include <linux/ipv6.h>

#include "gnb.h"
#include "gnb_arg_list.h"
#include "gnb_exec.h"
#include "gnb_tun_drv.h"
#include "gnb_payload16.h"


void bind_socket_if(gnb_core_t *gnb_core){

    if ( '\0' == gnb_core->conf->socket_ifname[0] ) {
        return;
    }

    struct ifreq nif;

    memset(&nif,0,sizeof(struct ifreq));

    strncpy(nif.ifr_name, gnb_core->conf->socket_ifname, IFNAMSIZ);

    int i;

    for ( i=0; i < gnb_core->conf->udp6_socket_num; i++ ) {
        setsockopt(gnb_core->udp_ipv6_sockets[i], SOL_SOCKET, SO_BINDTODEVICE, (char *)&nif, sizeof(nif));
    }

    for ( i=0; i < gnb_core->conf->udp4_socket_num; i++ ) {
        setsockopt(gnb_core->udp_ipv4_sockets[i], SOL_SOCKET, SO_BINDTODEVICE, (char *)&nif, sizeof(nif));
    }

}


static void if_up_script(gnb_core_t *gnb_core){

    int ret;

    char cmd[PATH_MAX+NAME_MAX];

    snprintf(cmd,PATH_MAX+NAME_MAX,"\"%s/scripts/%s\" > /dev/null 2>&1",gnb_core->conf->conf_dir,"if_up_linux.sh");

    ret = system(cmd);

    if ( -1==ret || 0 ==ret ) {
        return;
    }

    return;

}


static void if_down_script(gnb_core_t *gnb_core){

    int ret;

    char cmd[PATH_MAX+NAME_MAX];

    snprintf(cmd,PATH_MAX+NAME_MAX,"\"%s/scripts/%s\" > /dev/null 2>&1",gnb_core->conf->conf_dir,"if_down_linux.sh");

    ret = system(cmd);

    if ( -1==ret || 0 ==ret ) {
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

    int ret = ioctl(socket_fd, SIOCSIFMTU, &ifr);

    if (-1==ret) {
        perror("ioctl");
    }

    close(socket_fd);
}


static int set_addr4(char *interface_name, char *ip, char *netmask){

    int socket_fd;

    if ( (socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
        return -1;
    }

    struct ifreq ifr;

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = PF_INET;
    inet_aton(ip, &addr.sin_addr);

    memcpy(&ifr.ifr_ifru.ifru_addr, &addr, sizeof(struct sockaddr_in));

    if ( ioctl(socket_fd, SIOCSIFADDR, &ifr) < 0 ) {
        perror("ioctl addr4 SIOCSIFADDR");
        exit(1);
    }

    struct ifreq ifr_mask;

    memset(&ifr_mask, 0, sizeof(struct ifreq));

    strncpy(ifr_mask.ifr_name, interface_name, IFNAMSIZ);

    struct sockaddr_in *sin_net_mask;

    sin_net_mask = (struct sockaddr_in *)&ifr_mask.ifr_addr;

    sin_net_mask -> sin_family = AF_INET;

    inet_pton(AF_INET, netmask, &sin_net_mask ->sin_addr);

    if ( ioctl(socket_fd, SIOCSIFNETMASK, &ifr_mask ) < 0 ) {
        perror("ioctl addr4 SIOCSIFNETMASK");
        exit(1);
    }

    close(socket_fd);

    return 0;

}


static int set_addr6(char *interface_name, char *ip, char *netmask){

    struct ifreq ifr;

    struct in6_ifreq ifr6;

    int socket_fd;

    int ret;

    memset(&ifr,  0, sizeof(struct ifreq));
    memset(&ifr6, 0, sizeof(struct in6_ifreq));

    socket_fd = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);

    if ( socket_fd < 0) {
        return -1;
    }

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    ret = ioctl(socket_fd, SIOGIFINDEX, &ifr);

    if ( ret< 0) {
        goto finish;
    }

    ifr6.ifr6_ifindex = ifr.ifr_ifindex;

    inet_pton(AF_INET6, ip, (void *)&ifr6.ifr6_addr);

    ifr6.ifr6_prefixlen = 96;

    ret = ioctl(socket_fd, SIOCSIFADDR, &ifr6);

    if ( ret< 0 ) {
        goto finish;
    }

finish:

    close(socket_fd);

    return ret;

}


static int interface_up(char *interface_name) {

    int socket_fd;

    if ( (socket_fd = socket(PF_INET,SOCK_STREAM,0)) < 0 ) {
        return -1;
    }

    struct ifreq ifr;

    strncpy(ifr.ifr_name,interface_name,IFNAMSIZ);

    short flag;

    flag = IFF_UP;

    if ( ioctl(socket_fd, SIOCGIFFLAGS, &ifr) < 0 ) {
        return -2;
    }

    ifr.ifr_ifru.ifru_flags |= flag;

    if ( ioctl(socket_fd, SIOCSIFFLAGS, &ifr) < 0 ) {
        return -3;
    }

    close(socket_fd);

    return 0;

}


static int tun_alloc(char *dev) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if ( (fd = open(clonedev , O_RDWR)) < 0 ) {
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

  strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if ( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    //perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  return fd;

}


int init_tun_linux(gnb_core_t *gnb_core){

    gnb_core->tun_fd = -1;

    return 0;

}


static int open_tun_linux(gnb_core_t *gnb_core){

    if ( -1 != gnb_core->tun_fd ) {
        return -1;
    }

    gnb_core->tun_fd = tun_alloc(gnb_core->ifname);

    set_addr4(gnb_core->ifname, GNB_ADDR4STR_PLAINTEXT1(&gnb_core->local_node->tun_addr4), GNB_ADDR4STR_PLAINTEXT2(&gnb_core->local_node->tun_netmask_addr4));

    if (GNB_ADDR_TYPE_IPV4 != gnb_core->conf->udp_socket_type) {
        set_addr6(gnb_core->ifname, GNB_ADDR6STR_PLAINTEXT1(&gnb_core->local_node->tun_ipv6_addr), "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:0000:0000");
    }

    interface_up(gnb_core->ifname);
    setifmtu(gnb_core->ifname, gnb_core->conf->mtu);

    if_up_script(gnb_core);

    return 0;

}


static int read_tun_linux(gnb_core_t *gnb_core, void *buf, size_t buf_size){

    ssize_t rlen;

    rlen = read(gnb_core->tun_fd, buf, buf_size);

    return rlen;
}


static int write_tun_linux(gnb_core_t *gnb_core, void *buf, size_t buf_size){

    ssize_t wlen;

    wlen = write(gnb_core->tun_fd, buf, buf_size);

    return wlen;
}


static int close_tun_linux(gnb_core_t *gnb_core){

    close(gnb_core->tun_fd);

    gnb_core->tun_fd = -1;

    if_down_script(gnb_core);

    return 0;

}

static int release_tun_linux(gnb_core_t *gnb_core){

    free(gnb_core->platform_ctx);

    return 0;
}


gnb_tun_drv_t gnb_tun_drv_linux = {

    init_tun_linux,

    open_tun_linux,

    read_tun_linux,

    write_tun_linux,

    close_tun_linux,

    release_tun_linux

};
