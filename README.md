# GNB
[GNB](https://github.com/gnbdev/gnb "GNB")是一个开源的去中心化的具有内网穿透能力通过P2P进行三层网络交换的VPN。

GNB可以让用户把异地的办公环境以及家庭环境组成一个虚拟的局域网，这个虚拟的局域网中的机器不需要公网IP，不需要公网服务器中转就可以实现TCP/IP通讯。

与大多数内网穿透的软件实现应用层协议代理不同，GNB是通过虚拟网卡实现IP分组转发，支持应用层所有基于TCP/IP的通信协议。

GNB的安全基于ED25519交换密钥，IP分组的加密有XOR与RC4两个加密算法可选，也可以选择不加密，在未来会支持AES算法。

IP分组加密密钥会可以选择根据时钟每隔一段时间会更新一次，这样要求确保节点中的机器时间必须同步。

出于安全考虑，GNB项目相关代码会开源。

![net to net](images/net_to_net.jpeg)

GNB用C语言开发，编译时不需要引用第三方库文件，可以方便移植到当前流行的操作系统上。

现有支持平台：
树莓派
Linux_X86_64
Windows10_X86_64
macOS
FreeBSD_AMD64
OpenBSD_AMD64
Windows7_X86_64
OpenWRT

[GNBFrontend](https://github.com/XyloseYuthy/GNBFrontend "GNBFrontend")是由志愿者开发维护的开源的GNB的图形界面前端。
