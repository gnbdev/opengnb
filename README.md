# OpenGNB

[OpenGNB](https://github.com/gnbdev/opengnb "OpenGNB")是一个开源的去中心化的具有极致内网穿透能力的通过P2P进行三层网络交换的虚拟组网系统。

> [OpenGNB](https://github.com/gnbdev/opengnb "OpenGNB") 可以让你把公司-家庭网络组成直接访问的局域网；[OpenGNB](https://github.com/gnbdev/opengnb "OpenGNB") 可以让你免费实现自己的SDWAN网络。

[gnb_udp_over_tcp](https://github.com/gnbdev/gnb_udp_over_tcp "gnb_udp_over_tcp")是一个为GNB开发的通过tcp链路中转UDP分组转发的服务，也可以为其他基于UDP协议的服务中转数据。

出于安全考虑，GNB项目相关所有代码以开源方式发布,当前发布的源码支持以下平台：
FreeBSD Linux OpenWRT Raspberrypi OpenBSD macOS

[gnb在各平台的编译发行版下载](https://github.com/gnbdev/gnb_build "gnb_build")

[gnb 在 OpenWRT 上的打包工程](https://github.com/gnbdev/opengnb-openwrt "opengnb-openwrt")

![GNB 与传统VPN对比](images/gnb1.png)

## GNB特点

1. 内网穿透 P2P VPN
    - 无需公网IP
2. 极致的链路能力
    - 无限速影响
3. 数据安全
    - GNB节点间基于椭圆曲线数字签名实现可靠的身份验证
4. 多平台支持
    - GNB用C语言开发，编译时不需要引用第三方库文件，可以方便移植到当前流行的操作系统上,目前支持的操作系统及平台有 Linux_x86_64，Windows10_x86_64， macOS，FreeBSD_AMD64，OpenBSD_AMD64，树莓派，OpenWRT；大至服务器环境，桌面系统，小至仅有32M内存的OpenWRT路由器都能很好的运行GNB网络。

## 深入理解GNB指引

### GNB 知识点

* [GNB 配置及命令行参数](docs/gnb_config_manual_cn.md)
* [GNB 配置图图解](docs/gnb_setup.md)
* [GNB 的诊断功能](docs/gnb_diagnose.md)
* [使用gnb_udp_over_tcp给GNB增加重传包能力](https://github.com/gnbdev/gnb_udp_over_tcp "gnb_udp_over_tcp")


GNB的index节点的角色类似于BT协议中的Tracker，由一部分GNB网络志愿者提供。在绝大多数情况下`index`节点仅为GNB网内主机提供地址索引，不会为GNB节点中转数据。

一部分志愿者提供的GNB的`forward`节点可以为极端情况下暂时无法进行点对点通信的主机进行数据中转，而GNB主机之间的非对称数据加密使得`forward`节点无法窥探中转的数据。

在无法建立点对点通信的极端情况下，是否通过公网`forward`节点中转数据和使用哪个可信任的 forward 节点中转数据，完全取决在主机的拥有者对GNB 节点的设置。事实上，即便处于极其复杂的网络环境，GNB优越链路能力也可以随时随地建立虚拟数据链路，GNB甚至会为网络中的主机创建多个虚拟链路，择速度最优路径发送数据分组。

这是由志愿者提供的可用`index`节点

```
i|0|156.251.179.84|9001
i|0|39.108.10.191|9001
i|0|101.32.178.3|9001
i|0|103.27.187.204|9001
```

由[《铜豌豆 Linux》](https://www.atzlinux.com)项目为 gnb 项目制做了 Linux 下的 deb 格式软件包，详情请访问[铜豌豆软件源](https://www.atzlinux.com/allpackages.htm)。

---
[免责声明](docs/disclaimer.md)

