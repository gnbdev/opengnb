# OpenGNB
## [Chinese](/README.md) ## English

GNB is open source de-centralized VPN to achieve layer3 network via p2p with the ultimate capability of NAT Traversal.
> Allows you to form a company-home network into a direct-access local area network.

# Note：
Most of this document is translated by Google Translated from Chinese document, its accuracy is subject to Chinese

## GNB Features

1. NAT Traversal P2P VPN
    - No public IP required
2. Ultimate link capability
    - Unlimited speed effects
3. Data Security
    - Reliable authentication between GNB nodes based on elliptic curve digital signature
4. Multi-platform support
    -  GNB is developed in C language. It does not need to refer to third-party library files when compiling, and it can be easily ported to the current popular operating systems. Currently supported operating systems and platforms include Linux_x86_64, Windows10_x86_64, macOS, FreeBSD_AMD64, OpenBSD_AMD64, Raspberry Pi, OpenWRT ; Large to server environment, desktop system, as small as OpenWRT router with only 32M memory can run GNB network very well.

## GNB Quick Start
* Linux platform
### Step 1: Download and compile the GNB source code project
```
git clone https://github.com/gnbdev/opengnb.git
cd opengnb
make -f Makefile.linux install
```
After compiling, you can get `gnb` `gnb_crypto` `gnb_ctl` `gnb_es` files in the `opengnb/bin/` directory.

### Step 2: Quickly deploy GNB nodes
Copy `gnb` `gnb_crypto` `gnb_ctl` `gnb_es` to host A and host B respectively.

Assuming that host A and host B need to temporarily penetrate the intranet interconnection in two different LANs, the fastest way is to run gnb through lite mode. In lite mode, asymmetric encryption is not enabled, and only through **passcode** and The node id generates the encryption key, so the security will be much less modular than working with asymmetric encryption.

**passcode** is a 32-bit hexadecimal string with a length of 8 characters, which can be represented as **0xFFFFFFFF** or **FFFFFFFF**, under a public index **passcode** is the same GNB node It is considered to be a node on the same virtual network. Please choose a **passcode** that will not be the same as other users as much as possible. Here, for the convenience of demonstration, the **passcode** is selected as `12345678`, and the parameter **-p ** Used to specify the **passcode** to start the node. Do not use such a simple **passcode** in actual use, it may conflict with other users who also use `12345678` as **passcode** and cause communication failure.

### Step 3: Start the first node
Executed with **root** on host A
```
gnb -n 1001 -I '39.108.10.191/9001' --multi-socket=on -p 12345678
```
After the startup is successful, execute ip addr on host A to see the GNB node IP
```
3: gnb_tun: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 1280 qdisc fq_codel state UNKNOWN group default qlen 500
    link/none 
    inet 10.1.0.1/16 scope global gnb_tun
       valid_lft forever preferred_lft forever
    inet6 64:ff9b::a01:1/96 scope global 
       valid_lft forever preferred_lft forever
    inet6 fe80::402:c027:2cf:41f9/64 scope link stable-privacy 
       valid_lft forever preferred_lft forever
```

### Step 4: Start the second node
Executed with **root** on host B
```
gnb -n 1002 -I '39.108.10.191/9001' --multi-socket=on -p 12345678
```
After the startup is successful, execute ip addr on host B to see the GNB node IP

```
3: gnb_tun: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 1280 qdisc fq_codel state UNKNOWN group default qlen 500
    link/none 
    inet 10.1.0.2/16 scope global gnb_tun
       valid_lft forever preferred_lft forever
    inet6 64:ff9b::a01:2/96 scope global 
       valid_lft forever preferred_lft forever
    inet6 fe80::a6cf:9f:e778:cf5d/64 scope link stable-privacy 
       valid_lft forever preferred_lft forever
```

### Step 5: Test GNB Node Interoperability
At this point, if host A and host B nat penetrate successfully and ensure that there is no firewall intervention on the host, they can ping each other's virtual ip.
Executed with **root** on host A
```
root@hostA:~# ping 10.1.0.2
PING 10.1.0.2 (10.1.0.2) 56(84) bytes of data.
64 bytes from 10.1.0.2: icmp_seq=1 ttl=64 time=2.13 ms
64 bytes from 10.1.0.2: icmp_seq=2 ttl=64 time=2.18 ms
64 bytes from 10.1.0.2: icmp_seq=3 ttl=64 time=2.38 ms
64 bytes from 10.1.0.2: icmp_seq=4 ttl=64 time=2.31 ms
64 bytes from 10.1.0.2: icmp_seq=5 ttl=64 time=2.33 ms
```

Executed with **root** on host B
```
root@hostA:~# ping 10.1.0.1
PING 10.1.0.1 (10.1.0.1) 56(84) bytes of data.
64 bytes from 10.1.0.1: icmp_seq=1 ttl=64 time=2.34 ms
64 bytes from 10.1.0.1: icmp_seq=2 ttl=64 time=1.88 ms
64 bytes from 10.1.0.1: icmp_seq=3 ttl=64 time=1.92 ms
64 bytes from 10.1.0.1: icmp_seq=4 ttl=64 time=2.61 ms
64 bytes from 10.1.0.1: icmp_seq=5 ttl=64 time=2.39 ms
```

The simplest use process of the above GNB lite mode, GNB lite mode has 5 built-in nodes, if you need more hosts to participate in the networking and use a more secure asymmetric encryption method to protect the data communication of GNB, please read the following documents carefully.



## Deep understanding of GNB guidelines

The role of the index node of GNB is similar to the Tracker in the BT protocol, which is provided by some GNB network volunteers. In most cases, the `index` node only provides the address index for the hosts in the GNB network, and will not transfer data for the GNB node.

The `forward` node of GNB provided by some volunteers can perform data transfer for hosts that are temporarily unable to perform point-to-point communication in extreme cases, and the asymmetric data encryption between GNB hosts makes it impossible for the `forward` node to spy on the transferred data.

In extreme cases where peer-to-peer communication cannot be established, whether to transfer data through the public network `forward` node and which trusted forward node to use to transfer data depends entirely on the settings of the host owner on the GNB node. In fact, even in an extremely complex network environment, GNB's superior link capability can establish virtual data links anytime, anywhere. GNB will even create multiple virtual links for hosts in the network, and choose the optimal speed path to send data packets.

Here are the available `index` nodes provided by volunteers

```
i|0|156.251.179.84|9001
i|0|39.108.10.191|9001
i|0|101.32.178.3|9001
i|0|103.27.187.204|9001
```

### GNB Knowledge Points
This part of the document has not been translated into English
* [GNB 配置及命令行参数](docs/gnb_config_manual_cn.md)
* [GNB 配置图图解](docs/gnb_setup.md)
* [GNB 的诊断功能](docs/gnb_diagnose.md)
* [使用 gnb_udp_over_tcp给 GNB 增加重传包能力](https://github.com/gnbdev/gnb_udp_over_tcp "gnb_udp_over_tcp")

## GNB limitations
1. GNB does not support *Classless Inter-Domain Routing (CIDR)*, only supports Class A, B, C networks;
2. GNB does not forward IP frames of the default route (Default route). In `host to net` and `net to net` modes, GNB can forward data for a specific subnet, but does not support full traffic forwarding;
3. IPV6 of GNB does not work properly under Windows platform;
4. The work of GNB using virtual network card implements Layer 3 switching in TUN mode, and does not support Layer 2 switching if it does not support TAP mode;


## GNB在OpenWRT上

GNB 完美支持 OpenWRT 平台
由[Hoowa Sun](https://github.com/hoowa)为 GNB 项目制作了 [GNB 在 OpenWRT 上的打包工程](https://github.com/gnbdev/opengnb-openwrt "opengnb-openwrt")


## GNB在Linux发行版上
由[《铜豌豆 Linux》](https://www.atzlinux.com)项目为 GNB 项目制做了 Linux 下的 deb 格式软件包，详情请访问[铜豌豆软件源](https://www.atzlinux.com/allpackages.htm)。

由 [taotieren](https://github.com/taotieren) 为 GNB 项目制作了 Arch Linux 的 AUR 包,安装方式如下
```bash
# 安装发行版
yay -Sy opengnb 
# 安装开发版
yay -Sy  opengnb-git
```
详情请访问 [opengnb](https://aur.archlinux.org/packages/opengnb/) [opengnb-git](https://aur.archlinux.org/packages/opengnb-git/)

[gnb 在各平台的编译发行版下载](https://github.com/gnbdev/gnb_build "gnb_build")

[gnb_udp_over_tcp](https://github.com/gnbdev/gnb_udp_over_tcp "gnb_udp_over_tcp") 是一个为 GNB 开发的通过 tcp 链路中转 UDP 分组转发的服务，也可以为其他基于 UDP 协议的服务中转数据。

---
[免责声明](docs/disclaimer.md)
