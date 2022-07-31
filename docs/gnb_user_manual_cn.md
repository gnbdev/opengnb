OpenGNB 用户手册

version 1.3.0.0  protocol version 1.2.0

# 概述

[OpenGNB](https://github.com/gnbdev/opengnb "OpenGNB") 是一个开源的 P2P 去中心化的具有极致的内网穿透能力的软件自定义虚拟网络（SDVN）。

GNB 是一个去中心化的网络，网络中每个节点都是对等的，没有 Server 和 Client 的概念。

在一个 GNB 网络中每个节点的 ID 是唯一的，被称为 **nodeid** ，这是一个 32bit 无符号整型数字。

GNB 节点可以通过 index 节点找到其他 GNB 节点并建立起通信，在这过程中 GNB index 节点的作用类似于 **DNS** 和 **Bit Torrent** 中的 **tracker** 。

GNB 与 index 节点通信时使用 **ed25519** 的 public key 和 `passcode` 与  **nodeid** 的 sha512 摘要作为唯一标识，基于这个特点，一个 GNB index 节点可以为许多个 GNB 网络 提供 index service 而不容易发生碰撞冲突。

GNB 节点也可以通过域名或静态 IP 地址或内网广播找到其他 GNB 节点。

GNB 通过***upnp***，***multi index***，***port detect***,***multi socket***, ***unified forwarding*** 等许多办法使得位于 LAN 中的节点能够成功 NAT 穿透与其他节点建立 P2P 通信，最糟糕的情况下，当两个 GNB 节点无法建立起P2P连接时可以通过网络内其他节点中转 payload。

GNB目前支持的操作系统及平台有 Linux，Windows10， macOS，FreeBSD，OpenBSD，树莓派，OpenWRT，除有特别的说明，本文档适用于这些支持的平台发行版及针对特定平台源码编译的版本。


# 部署 GNB


## GNB Public Index node

**GNB Public Index node** 仅提供 index service，不打开 TUN 设备，不创建虚拟IP，不处理 IP 分组，不需要用 root 启动，由志愿者提供。

以 **Public Index** 方式启动 GNB 节点

启动默认监听 9001 端口
```
gnb -P
```

启动监听 9002 端口
```
gnb -P -l 9002
```


## GNB 的 Lite Mode

在 **Lite Mode**  下不需要为节点创建 **ed25519** 密钥对 和 配置文件，通过设定命令行参数就能启动 GNB 节点。

在 **Lite Mode** 下 **upnp** 选项默认被启用。

GNB的源码 里 hardcode 了一个有5个节点的 GNB 网络供 **Lite Mode** 下使用,这极大了简化部署的过程。
```
nodeid       tun address
1001         10.1.0.1/255.255.0.0
1002         10.1.0.2/255.255.0.0
1003         10.1.0.3/255.255.0.0
1004         10.1.0.4/255.255.0.0
1005         10.1.0.5/255.255.0.0
```

**Lite Mode** 下的 GNB 节点 通过 `passcode` 生成通讯密钥，安全性远低于基于 **ed25519** 的非对称加密的安全模式。

**Lite Mode** 下的 GNB 节点与 index 节点通信时使用  `passcode`  与  **nodeid** 的 sha512 摘要作为唯一标识，假设两个 **Lite Mode** 下的 GNB 网络使用了相同 `passcode` 和 index 节点，毫无疑问这必然互相干扰。

以下是一组命令，分别运行在 主机 A 和 主机 B上实现虚拟组网，需要用 **root** 执行

主机 A 上
```
gnb -n 1001 -I "$public_index_ip/$port" -l 9001 --multi-socket=on -p 12345678
```

主机 B 上
```
gnb -n 1002 -I "$public_index_ip/$port" -l 9002 --multi-socket=on -p 12345678
```

此时应可以看到主机 A 上的 TUN IP 是 10.1.0.1， 主机 B 上的 TUN IP 是 10.1.0.2，此时,两个节点如果建立起连接 P2P 通讯就可以互相访问。

用户当然可以在 **Lite Mode** 下设置更多的节点，这需要了解 `gnb` 的 `-n`， `-I`， `-a,`， `-r`，`-p` 等参数的使用。


## GNB 的 Safe Mode

以 **Safe Mode** 方式启动 GNB 需要正确设置 `node.conf` `address.conf` `route.conf` 文件以及 **ed25519** 公私钥，这些文件需要放在一个目录下，目录的名字可以是 **nodeid**。

其中，`address.conf` 是可选的。
假设一个nodeid为`1000`的节点固定运行在 WAN 上充当 index 节点的角色，其他GNB节点通过 `address.conf` 的配置找到 `1000`节点,如此场景下`1000`节点一般不需要`address.conf` 。

如果创建了多个 GNB 节点，可以把这些节点的配置目录放在 `gnb/conf/` 下以便管理。



## 目录结构

以节点 `1001` `1002` `1003`为例,  这些节点的配置目录如下

```
gnb/conf/1001/

gnb/conf/1001/node.conf
gnb/conf/1001/address.conf
gnb/conf/1001/route.conf
gnb/conf/1001/security/1001.private
gnb/conf/1001/security/1001.public
gnb/conf/1001/ed25519/1002.public
gnb/conf/1001/ed25519/1003.public
gnb/conf/1001/scripts
```

```
gnb/conf/1002/

gnb/conf/1002/node.conf
gnb/conf/1002/address.conf
gnb/conf/1002/route.conf
gnb/conf/1002/security/1002.private
gnb/conf/1002/security/1002.public
gnb/conf/1002/ed25519/1001.public
gnb/conf/1002/ed25519/1003.public
gnb/conf/1002/scripts
```

```
gnb/conf/1003/

gnb/conf/1003/node.conf
gnb/conf/1003/address.conf
gnb/conf/1003/route.conf
gnb/conf/1003/security/1003.private
gnb/conf/1003/security/1003.public
gnb/conf/1003/ed25519/1001.public
gnb/conf/1003/ed25519/1002.public
gnb/conf/1003/scripts
```

假设这些节点的配置目录已经分别部署到不同的主机上，启动`1001` `1002` `1003` 节点的命令可以是：
```
gnb -c gnb/conf/1001
gnb -c gnb/conf/1002
gnb -c gnb/conf/1003
```

`scripts/`目录下存放的是在虚拟网卡启动/关闭后调用的脚本，用于用户自定义设置路由，防火墙的指令。

`security/` 存放的是本节点的 **ed25519** 公钥和私钥，`ed25519/` 存放的是其他节点的公钥。


## 密钥交换

**Safe Mode** 下，GNB 节点通过非对称加密(ed25519)来交换通讯密钥，这是 GNB 节点通信的基础，因此需要为每个 GNB 节点创建一组公私钥用于通讯。

公私钥的命名分别是以节点的UUID为文件名 .private 和 .public 为文件的后缀名，以 UUID 为 1001 的节点公私钥文件应为 `1001.public` `1001.private`

GNB 提供了一个名为 `gnb_crypto` 命令行工具，用于生成公私钥，

为 `1001` `1002` `1003` 节点 创建 **ed25519** 公私钥的命令是：

`gnb_crypto -c -p 1001.private -k 1001.public`
`gnb_crypto -c -p 1002.private -k 1002.public`
`gnb_crypto -c -p 1003.private -k 1003.public`

交换公钥，就是要把这些  **ed25519** 公私钥 放置到正确的目录中。

在  **nodeid** 为 `1001`的节点上:
`gnb/conf/1001/security/` 目录下应该有 `1001.private` `1001.public`
`gnb/conf/1001/ed25519/`  目录下应该有 `1002.public`  `1003.public`

在  nodeid 为 `1002`的节点上:
`gnb/conf/1002/security/` 目录下应该有 `1002.private` `1002.public`
`gnb/conf/1002/ed25519/`  目录下应该有 `1001.public`  `1003.public`

在  nodeid 为 `1003`的节点上:
`gnb/conf/1003/security/` 目录下应该有 `1003.private` `1003.public`
`gnb/conf/1003/ed25519/`  目录下应该有 `1001.public`  `1002.public`

在非对称加密基础上同一个 GNB 网络下的节点还可以通过设置相同 `passcode` 以保护通讯密钥。

GNB通讯密钥有关的信息需要参考 `--crypto` `--crypto-key-update-interval` `--passcode` 的说明。


## GNB payload forwarding
  - **Direct Forwarding** 节点间 P2P 通信；通常情况下，位于 WAN 上节点必可与其他节点建立起 P2P 通信，位于 LAN 中节点与其他 LAN 中的节点建立起 P2P 通信要经过NAT穿透;
  - **Unified Forwarding** 自动通过已经建立 P2P 通信的节点转发 IP 分组，多节点重传 IP 分组;
  - **Relay Forwarding** 高度自定义中继路由，IP 分组发往下一个中继点前都会作加密处理，相关设置在 `route.conf`;
  - **Standard Forwarding** 用尽一切策略都无法建立起 P2P 通信的节点可以通过位于公网 forward 节点中继 IP 分组


## node.conf 配置文件

以下是 `node.conf` 一个例子：
```
nodeid 1001
listen 9001
```

`node.conf` 用于存放节点的配置信息，格式如下
```
nodeid $nodeid
listen $listen_port
```
或

```
nodeid $nodeid
listen ip_address:$listen_port
```

GNB 支持IPV6和IPV4。

有一些值得重点关注的选项，可以很好提升 GNB 的 NAT 穿透能力：
```
es-argv      "--upnp"
```
尝试打开 upnp ，让来自 WAN 的 IP 分组更容易被传入。

```
multi-socket on
```
打开更多 UDP 监听端口，增加NAT穿透是端口探测成功的概率。

`multi-socket` 被设为 on 后会除原有的 `listen` 的端口外，在额外打开4个随机 UDP 端口，如果想额外监听指定的端口，可以用多个 `listen` 设置项，GNB 支持用`listen` 指定监听5个地址端口或通过`listen6` 监听 5个IPV6地址和端口 以及用 `listen4` 监听5个IPV4地址端口。

```
unified-forwarding super
```
在发送 ip 分组的过程通过多个已经分别和A，B节点都建立起 P2P 通信的节点对 TCP 类型的 ip 分组进行重传中转


`node.conf` 所支持的配置项与 gnb 命令行参数一一对应，目前支持的配置项有:
```
ifname nodeid listen listen6 listen4 ctl-block multi-socket direct-forwarding unified-forwarding ipv4-only ipv6-only passcode quiet daemon mtu set-tun address-secure node-worker index-worker index-service-worker node-detect-worker port-detect-range port-detect-start port-detect-end pid-file node-cache-file log-file-path log-udp4 log-udp-type console-log-level file-log-level udp-log-level core-log-level pf-log-level main-log-level node-log-level index-log-level detect-log-level es-argv
```


### route.conf 配置文件

GNB 节点之间通信的数据分组被称为 **gnb payload**，网络里所有的节点共享的一张路由表，配置文件是 `route.conf`，其作用就是告诉 GNB 核心如何"route" 这些包含 IP 分组的 **gnb payload**。

每个路由配置项占一行，路由配置项有两个类型 **forward** 与 **relay** 用于设定本地节点的 payload 到达对端节点的方式。

以下是 `route.conf` 的例子：
```
1001|10.1.0.1|255.255.255.0
1002|10.1.0.2|255.255.255.0
1003|10.1.0.3|255.255.255.0
```

配置文件中的每一行是一个节点的描述，格式如下
```
$nodeid|$tun_ipv4|$tun_netmask
```

每个配置项的含义是这样:
`$nodeid`          节点的UUID
`$tun_ipv4`        虚拟网卡的IPV4地址
`$tun_netmask`     虚拟网卡的IPV4地址的子网掩码


#### Direct forwarding 

节点之间通信默认是点对点方式被称为**Direct forwarding**，即 IP 分组直接发往对端节点的IP地址和端口。

例如 `route.conf` 中 有一行
```
1001|10.1.0.1|255.255.255.0
```

当 GNB 处理一个目的地址 是 10.1.0.1 的 IP 分组时，就确定这个 IP 分组需要用 **nodeid**  为 1001 的节点的通信密钥加密封装成 **gnb payload** 发送到该节点 1001 上，这个过程我们称为 **Direct forwarding**。


#### Standard Forwarding

节点之间在没有起建立P2P通讯的情况下可以通过 `address.conf` 中有定义了 **f** 属性的 **forward** 节点转发**gnb payload**，这个通讯方式被成为 **Standard Forwarding**

例：
`address.conf`
```
if|1001|x.x.x.x|9001
```

以上例子中 1001 节点是一个 **idnex** 节点，同时也是一个 **forward** 节点。


#### Relay Forwarding

**Relay Forwarding** 通过明确的自定义的中继路径对 **gnb payload** 进行中继，**gnb payload** 在中继过程中会被中继节点再次加密。

由于是端对端加密，**forward node** 与 **relay node** 节点无法得到通信的两端的 **gnb payload** 中的 IP 分组的明文。

为目的节点设定中继节点, 可以为一个对端的节点设置最多8条中继的路由，每条中继路由最多可以有5个中继节点。

以下是 `route.conf` type relay 一些例子：


```
1006|1003
1006|auto,static
```
这组 route 的作用是：在当前节点无法与 1006 节点建立起 P2P 通信时，将通过 1003节点中继发往 1006 节点的 IP 分组。


```
1006|1003,1004,1005
1006|force,static
```
这组 route 的含义是：当前节点发往 1006 节点 的 IP 分组强制按顺序通过 1005,1004,1003 中继到 1006 节点。


```
1006|1003,1005
1006|1005,1004
1006|1005,1003
1006|force,balance
```
这组 route 的作用是：当前节点到1006节点有3条中继路由，发送 IP 分组时选择路由的策略是负载均衡。

相关的设置项格式如下:
```
$nodeid|$relay_nodeid3,$relay_nodeid2,$relay_nodeid1
$nodeid|$relay_mode
```

本地节点的payload通过中继节点到达对端节点的路径是 local_node => relay_node1 => relay_node2 => relay_node3 => dst_node, 而 dst_node 到 local_node 的路径则由对端的 dst_node 的 `route.conf` 配置所决定。

`$relay_mode` 可以是 **auto**, **force**, **static**, **balance**

**auto**       当对目的节点无法点对点通信及没有forward节点转发时通过预设的中继路由转发
**force**      强制与该目的节点的通信必须经过中继路由
**static**     当一个目的节点有多条中继路由时，使用第一条中继路由
**balance**    当一个目的节点有多条中继路由时，以负载均衡的方式选择中继路由


### address.conf  配置文件

`address.conf` 用于配置节点的属性和公网ip地址及端口,以下是一个例子：

```
i|0|a.a.a.a|9001
if|1001|b.b.b.b|9001
n|1002|c.c.c.c|9001
```

文件中的 `a.a.a.a` `b.b.b.b` `c.c.c.c` 代表节点的物理网卡的 IP 地址，需要根据实际情况填写，配置文件中的每一行是一个地址的描述，格式如下:

```
$attrib|$nodeid|$ipv4/$ipv6/$hostname|$port
```

`$attrib`    节点的属性，用一组字符来表示，**i**表示这个节点是index节点; **f**表示这个节点是forward节点; **n** 表示这个节点是一个普通节点; **s**为静默(slience)节点,本地节点如果含有**s**属性将不会与index节点通信避免，也不会响应ping-pong及地址探测的请求暴露本地ip地址
`$nodeid`    节点的 nodeid，与 `route.conf` 中的 `$nodeid` 相对应
`$ipv6`      GNB 节点的 IPV6 地址
`$ipv4`      GNB 节点的 IPV4 地址
`$hostname`  GNB 节点的 域名
`$port`      GNB 节点的服务端口


如果一个节点拥有多个 IP 地址, 需按格式分成多行配置，GNB 会通过 **GNB node ping-pong** 协议方式去测量该节点每个 IP 地址的时延，在发送 IP 分组数据的时候自动发往低时延的IP地址。

`gnb` 进程不会对 `address.conf` 中的 `$hostname` 进行域名解释，而是由 `gnb_es` 异步处理，相关信息可以了解 `gnb_es` 的 `--resolv` 选项。

前文提到 index 节点的作用类似于 **DNS** 和 **Bit Torrent** 中的 **tracker**, 如果一组相互之间不知道对方 IP 地址和端口的 GNB 节点都向同一个 index 节点提交自身的 IP 地址和端口, GNB 节点就可以通过对端节点的公钥向 index 查询对端节点的 IP 地址和端口。

在 `address.conf` 中 index 节点的 `$attrib`  里需要带有 **i** 属性。

GNB index 协议允许在提交和查询节点ip地址端口过程中不验证报文的数字签名,即 index 节点允许对提交和查询ip的报文不验证数字签名，节点对 index 节点响应查询IP的报文不验证数字签名。

**GNB Public Index node**  在 `address.conf` 中的对应的 **nodeid** 可以设为 **0**,不需要在 `route.conf` 中设定 "route" 规则，以下是一个 **GNB Public Index node** 的例子:
```
i|0|a.a.a.a|9001
```

forward 节点  `$attrib`  设为 **f**, `$nodeid` 不能设为 **0**，并且必须绑定 nodeid 作为配置项出现在 `route.conf` 中， gnb forward 协议相关的报文都要发送验证节点的数字签名，即要求 forward 节点和通过 forward 节点转发报文的节点都必须相互交换了公钥。

forward 节点可以为无法直接互访的 GNB 节点中转 IP 分组，这些节点通常部署在内网中且没有固定公网 IP，并且用尽了所有的办法都无法实现nat穿透实现点对点通讯，forward 节点并不能解密两个节点之间的通信的内容。

用户当然可以把一个 GNB 节点作为GNB网络中的 index节点 和 forward节点，同时还可以为其他 GNB 网络提供 index service，以下是一个例子：
```
if|1001|b.b.b.b|9001
```

在 `address.conf` 中 index 节点和 forward 节点可以有多项,并可以通过 `--multi-index-type` `--multi-forward-type` 设定负载均衡和容错的模式;假设一个 GNB 节点在LAN中经过非常复杂的地址转换后可能在同一时刻拥多个NAT后的 IP 地址，即运营商可能会根据 LAN 中的主机访问 WAN 的目标地址选用不同的网关出口，这时如果 `address.conf`  设有多个 Index 节点，那么在 NAT 穿透中，对端的GNB节点就可能掌握到该节点更多的地址与端口，从而增加NAT穿透建立 P2P 通信的成功率。


## GNB Unified Forwarding

在复杂 NAT 机制下，位于 LAN 中的节点无法确保可以 100% 成功实现 NAT 穿透，但总会有一些节点可以成功实现 NAT 穿透建立起 P2P 通讯，**GNB Unified Forwarding** 机制可以使得已经建立起 P2P 通讯的节点为未建立起 P2P 通讯的节点转发 IP 分组。

GNB 网络中每个节点定期把与自身建立起 P2P 通讯的节点id在网内通告，这使得网内每个节点都掌握发送数据另一个节点时有哪些中转节点可用。

以一个有A、B、C 3个节点的GNB网络为例： A与C, B与C 都建立起 P2P 通讯，而 A 与 B 由于复杂的 NAT 机制无法建立起 P2P 通讯，通过 **GNB Unified Forwarding** 机制，A与B 可以通过 C 转发 IP 分组。

根据 **GNB Unified Forwarding** 的机制只要虚拟网络中节点的数量越多，节点之间越容易实现 NAT 穿透并建立起虚拟链路。

发往目标节点的 IP 分组可以同时由多个中转节点转发，先到达目标节点的 IP 分组会被写入虚拟网卡，GNB 对每个目标节点维护了一个 64bit 时间戳及时间戳队列以确保重复传输的 IP 分组 被丢弃，因此**GNB Unified Forwarding**这个特点在一些场合下可以用作TCP重传加速。


### GNB Unified Forwarding 工作模式
`auto`  当A、B两个节点无法建立 P2P 通信时，自动通过已经分别和A，B都建立起 P2P 通信的节点（可以在LAN中）进行中转 IP 分组
`super` 不管A、B两个节点有没有建立起 P2P 通信，在发送 ip 分组的过程通过多个已经分别和A，B都建立起 P2P 通信的节点对 TCP 类型的 ip 分组进行重传中转
`hyper` 不管A、B两个节点有没有建立起 P2P 通信，在发送 ip 分组的过程通过多个已经分别和A，B都建立起 P2P 通信的节点对所有类型的 ip 分组进行重传中转


要了解更多 **Unified Forwarding** 的信息，可以参考 `gnb` 的 `-U, --unified-forwarding` 选项


## GNB Discover In Lan

**Discover In Lan** 可以使得 GNB 节点通过广播发现 LAN 内其他的 GNB 节点，这使得同一个 LAN 下的  GNB 节点就不需要先经过 WAN 进行 NAT 穿透就能建立起通信。

**Discover In Lan** 在 `gnb_es` 中实现，为了让 `gnb_es` 进程可以一直监听 LAN 中其他节点的广播分组，`gnb_es` 必须以 service 或 daemon 方式启动，并带有 `-L, --discover-in-lan` 选项。

以下节点 1002 为例
```
gnb_es -s -L -b gnb/conf/1002/gnb.map
```

`gnb.map` 是 `gnb` 进程创建的共享内存映射文件，如果没有特别指定，这个映射文件会在节点的配置目录中(`gnb/conf/1002/`)被创建；通过这块共享内存，`gnb_es` 可以把 LAN 中监听到的来自其他 GNB 节点的信息传递给 gnb 进程；与此同时，`gnb_es` 还会定期在 LAN 中广播自身节点的信息使得可以被 LAN 中其他节点发现。

在部署 GNB 节点的过程中，用户会发现在某些情况下同一个 LAN 里某些 GNB 节点可以成功NAT 穿透与另一个异地的 LAN 里的节点成功建立 P2P 通信，而某些节点却未能成功，借助 **Discover In Lan** 不但可以让更多的节点更轻易的连结在一起，同时还可以使得让已经成功NAT 穿透的节点去帮助同一个 LAN 下其他节点转发 IP 分组。

要了解更多 **Discover In Lan** 的信息，可以参考 `gnb` 的 `-b, --ctl-block`，以及  `gnb_es` 的 `-s, --service` `-d, --daemon` `-L, --discover-in-lan` 选项。


## 关于 net to net

一般而言，VPN通过建立虚拟链路的方式可以把几台计算机组成一个网络，也可以让一台计算机通过虚拟链路接入一个网络，还可以通过虚拟链路把两个或者更多的计算机网络组成一个大的虚拟网络，使得分散在各自不同网络里的计算机可以互相访问。

这种通过VPN的虚拟链路把几个网络整合成一个虚拟网络的架构我们称为 ** net to net **。

GNB 让 ** net to net ** 的部署变得非常轻而易举，借助 GNB 强大的 NAT 穿透能力，GNB可以轻易的把几个分别位于不同地方的 LAN 组成一个虚拟网络，并且大多数时候这些 LAN 之间是 P2P 直接通信而不需要中心节点中转，这样网络之间的通信就不受限于中心节点的带宽。

假设有两个异地的 **LAN A** 和 **LAN B**  通过 NAT 访问互联网，这两个 LAN 的网关都是 OpenWRT Router，那么只需要在 OpenWRT Router 上部署两个 GNB 节点，借助  WAN 上 GNB index 节点 实现 NAT 穿透， **LAN A** 和 **LAN B** 里的主机就能互相访问。

假设有两个异地的 **LAN A** 和 **LAN B**  通过 NAT 访问互联网，这两个 LAN 的网关都是 OpenWRT Router，那么只需要在 OpenWRT Router 上部署两个 GNB 节点通过 NAT 穿透建立起 P2P 通信， **LAN A** 和 **LAN B** 里的主机就能互相访问。
在这个过程中，通常需要有位于  WAN 上 GNB index 节点帮助这两个 GNB 节点找到对方，在以下的例子中，为了确保这两个节点在 NAT 穿透不成功的情况下还能通信，可以部署一个 forward 节点同时兼提供 index service。



这样，就总共有3个 GNB 节点需要部署：

**nodeid**=1001  位于 WAN，作为 index 和 forward 节点，IP= x.x.x.x  port = 9001
**nodeid**=1002  位于 **LAN A** 的 OpenWRT Router 上，NetWork=192.168.0.0/24
**nodeid**=1003  位于 **LAN B** 的 OpenWRT Router 上，NetWork=192.168.1.0/24


设置 **node 1001**

`conf/1001/node.conf`
```
nodeid 1001
listen 9001
```

`conf/1001/route.conf`
```
1001|10.1.0.1|255.255.255.0
1002|10.1.0.2|255.255.255.0
1003|10.1.0.3|255.255.255.0
```


设置  **LAN A**  中的 **node 1002**

`conf/1002/node.conf`
```
nodeid 1002
listen 9002
multi-socket on
es-argv --upnp
```

`conf/1002/route.conf`
```
1001|10.1.0.1|255.255.255.0
1002|10.1.0.2|255.255.255.0
1003|10.1.0.3|255.255.255.0
1003|192.168.1.0|255.255.255.0
```

`conf/1002/address.conf`
```
if|1001|x.x.x.x|9001
```
**x.x.x.x** 是 node 1001 的 WAN IP，需要根据实际情况填写。

`conf/1002/scripts/if_up_linux.sh`
```
ip route add 192.168.1.0/24 via 10.1.0.3
```
这个路由指令的作用是：当 **LAN A** 里的主机要发送目标地址是 `192.168.1.0/24` 的IP分组时，由于 **LAN A** 的网络是 `192.168.0.0/24`，所以这些目的是`192.168.1.0/24` IP 分组将会发送默认网关 OpenWRT Router 上，由于设置了这条路由规则，这些 IP 分组会被转发到 TUN 设备上被 GNB 进程获取封装成 **gnb payload**最终发送到 **node 1003** 上。

毫无疑问，在 **node 1003** 上也需要有类似的路由指令使得目的地是 `192.168.0.0/24` 的 IP 分组能够到达 **node 1003** GNB 进程最终被发送到 **node 1002**上。

设置  **LAN B**  中的 **node 1003**

`conf/1003/node.conf`
```
nodeid 1003
listen 9003
multi-socket on
es-argv --upnp
```

`conf/1003/route.conf`
```
1001|10.1.0.1|255.255.255.0
1002|10.1.0.2|255.255.255.0
1003|10.1.0.3|255.255.255.0
1003|192.168.1.0|255.255.255.0
```

`conf/1003/address.conf`
```
if|1001|x.x.x.x|9001
```

`conf/1003/scripts/if_up_linux.sh`
```
ip route add 192.168.0.0/24 via 10.1.0.2
```

通常情况下， OpenWRT 默认设有一些防火墙规则，这可能会导致 GNB 转发到当前 LAN 中的主机的 IP 分组被拦截，所以，这里还需要检测 OpenWRT，一般的要 把防火墙中的 Forward 选项 设置 为 accept，还有就是与 Wan 有关的 Forwardings 设置为 accept 。

确认配置正确后，可以先启动 **node 1001**，然后是 **node 1002** 和 **node 1003**，这样 **node 1001** 就能第一时间为 **node 1002** 和 **node 1003** 提供 index service 。

把 **node 1001**，**node 1002** ，**node 1003** 都启动后，可以先尝试这3个  GNB 节点对应的虚拟IP，即 `10.1.0.1` `10.1.0.2` `10.1.0.3` 能否成功通过 ping 测试。

如果一切顺利的话，就可以尝试从  **LAN A** 中某台主机去 ping  **LAN B** 下某台主机 。

例如从位于 **LAN A** 下 IP 为 `192.168.0.2` 的主机上去 ping 位于 **LAN B** 下的 IP 为 `192.168.1.2` 的主机，毫无疑问，也可以反方向来测试。

到这一步，如果发现这3个 GNB 节点对应的虚拟IP可以通过 ping 测试，而两个 LAN 里面的主机无法通过 ping 测试，那么可以回到前面的步骤检查 OpenWRT 上的防火墙规则还有检测主机上的防火墙规则。



# GNB 的 NAT 穿透

极致的 NAT 穿透能力或许是 GNB 最令人激动的一个特性，但绝这不是 GNB 的全部，GNB 还有许多有价值的特性待用户去挖掘。

在复杂的网络环境下， 成功 NAT 穿透与其他节点建立 P2P 通信有时并不容易，为了提升节点的 NAT 穿透成功的概率有必要较详细的介绍 NAT (Network Address Translation)  和 GNB 在 NAT 穿透方面做的工作。

通常，对于一些在 LAN 里通过 NAT 访问互联网的主机，在 NAT (Network Address Translation) 的过程中会涉及到包括但不限于诸如 LAN 中的路由设备， 网络结构，运营商所使用的网络设备，网络架构，NAT策略等许多几乎是“黑盒"般的网络环境，一个分组的IP地址可能经过多次转换，可能有多个出口网关，这些都是 NAT 穿透过程中的不确定因素。


**Don't talk about the white paper, who known? who care?**

**Don't talk about the type of NAT,  I don‘t  f*cking care**

**What GNB has to do, just try it's best to traverse the NAT**


毫无疑问，GNB 从 **Bit Torrent**  这些 P2P 软件中获得灵感，既然两个位于不同的 LAN 中的主机通过 NAT 访问互联网能够建立起 P2P 通信用于传输文件的数据块，那么就意味这样的方式也能用于传输虚拟网卡的 IP 分组。

GNB 用尽各种策略去促使每个节点能够 NAT 穿透的成功，这些策略中，一些策略非常有效，一些效果不大，一些策略在某些场合效果不错而在另一些场合就可能没有效果。

首先，本地节点通过与 index 节点通信交换信息获得其他节点的 WAN 地址和端口并尝试和这些节点进行联系；

事实上，一些节点的在经过 NAT 之后会用一个以上的 WAN 地址去访问互联网，GNB 支持同时与多个 index 节点交换信息，这样就有机会发现对端节点更多的 WAN 地址。

对于`route.conf`中定义的且未建立起 P2P 通信的对端节点，GNB 进程会定期向 index 节点查询这些节点的 WAN 地址端口，并尝试发起通信。

GNB 会对以及获得 WAN 地址但未建立起 P2P 通信的对端节点发现小范围的端口探测，如果两个节点相互能收到对端的探测请求 P2P 通信就基本打通。

GNB 还有专门的线程对未能建立起 P2P通信的接单定期进行大范围的端口探测。

与 **Bit Torrent** 等多数的 P2P 软件一样，GNB 也会通过 UPNP 在出口网关上尝试建立地址端口映射以提升 NAT 穿透成功率。

需要注意的是，如果一个 LAN 有多个主机部署 GNB 节点并开启 UPNP，这些节点不应监听相同的端口，这会导致在网关建立 UPNP 端口映射时发生冲突。

主机所在的网关如果支持和启用 UPNP 服务并且 GNB 启动时使用选项`--es-argv "--upnp"`将明显提升 NAT 穿透的成功概率，详情可以了解 `gnb` 的 `--es-argv "--upnp"` 和 `gnb_es` 的 `--upnp` 选项。

既然监听一个 UDP 端口并启用 UPNP 可以有效提升 NAT 穿透成功的概率，那么通过监听更多 UDP 端口以及在网关上对应建立更多 UPNP 端口映射可以进一步提高节点 NAT 穿透成功的概率，详情可以了解  `--multi-socket` 选项。

在一些网络环境下，充分了解 `gnb` 的 `--es-argv "--upnp"`, `--multi-socket`, `--unified-forwarding` 以及 `gnb_es` 的 `--upnp` 选项的作用可以非常有效提升 GNB 节点的 NAT 穿透的成功率。

使用 `gnb_es` 的 `--dump-address` 选项可以使得已经建立起 P2P 通信的节点的地址信息定期保存到指定的 node cache file 里，当修改配置文件或升级程序或者重启主机需要重新启动 `gnb` 时可以用 `--node-cache-file` 选项指定 node cache file 快速重新建立起通信，要注意的是，对于一部分节点并不一定会成功，详情可以了解 `gnb` 的 `--node-cache-file` 选项 `gnb_es` 的 `--dump-address` 选项。

GNB 节点除了通过 index 节点找到其他节点外还可以通过在 `address.conf` 里配置对端节点的 域名或者动态域名，需要关注的是，仅依靠这种方式去尝试 NAT 穿透效果不如通过 index 节点帮助建立 P2P 通信好。


至此，GNB 为节点的 NAT 穿透做了大量的工作，使得 GNB 的 NAT 穿透的成功率也达到了前所未有的高度，但这依然不够：

**GNB Unified Forwarding** 机制可以使得已经建立起 P2P 通讯的节点为未建立起 P2P 通讯的节点转发 IP 分组,并且只要虚拟网络中节点的数量越多，节点之间越容易实现 NAT 穿透并建立起虚拟链路。

通过 **Discover In Lan** 可以让同一个 LAN 下的多个 GNB 节点可以不需要经过 WAN 实现 P2P 通信，与此同时，这可以使未能成功NAT穿透的节点可以通过 **Unified Forwarding**，**Relay Forwarding**,**Standard Forwarding** 让已经成功 NAT 穿透的节点中转 IP 分组，间接实现 P2P 通信。

`gnb_es` 的 `--broadcast-address`  可以让 GNB 节点的将已经和本节点建立起 P2P 通信的节点的地址信息扩散给其他节点，使得这些节点可以获得 GNB 网络内其他节点的更多地址信息以增加 NAT 穿透的成功率。

不得不强调：即使做了许多努力，通过 NAT 穿透建立起 P2P 通信不可能100%的成功率，灵活运用 GNB 提供的机制可以极大的提升 NAT 穿透的成功概率。


# 关于 GNB 的 IPV6 网关/防火墙穿透

提示:在一般情况下，IPV6 地址都是 WAN 地址，但这并不意味着位于网关后面的拥有 IPV6 的主机就不需要穿透，当然这种穿透不能称之为NAT穿透，而是可以被称为网关/防火墙穿透。

由于位于网关的防火墙作用，某些网关(例如OpenWRT route)的防火墙策略默认情况下并不会放行由对端主机主动发起的来自 WAN 的IP分组，这需要两端的主机几乎在同时向对方的端口发送IP分组。

毫无疑问，IPV6的网关/防火墙穿透比IPV4的NAT穿透容易得多，而且成功率可以是100%。

GNB 为 IPV6 的网关/防火墙穿透做了一些工作。

假设一些位于网关后面的 GNB 节点拥有 IPV6地址，而位于 WAN 的 index 节点没有 IPV6  地址，而，这样 index 节点就无法直接得到位于网关后面的 GNB 节点的 IPV6 地址从而没法帮助这些节点建立起通信，要解决这个问题就需要位于网关后面的 GNB 节点获取所在主机的 IPV6 地址后告 index 节点。

假设主机拥有IPV6地址，可以用以下两个命令中的其中一个命令获得本主机的IPV6地址并保存在文件里。

```
dig -6 TXT +short o-o.myaddr.l.google.com @ns1.google.com | awk -F'"' '{ print $2}' > /tmp/wan6_addr.dump
```

```
wget http://api.myip.la -q -6 -O /tmp/wan6_addr.dump
```

以节点 1002 为例
```
gnb_es -b gnb/conf/1002/gnb.map --wan-address6-file=/tmp/wan6_addr.dump
```
通过 `gnb` 和 `gnb_es` 的共享内存，`gnb`进程就能获得本节点所在的 IPV6 地址并上报给 index 节点。

相关信息可以了解`gnb_es` 的  `-b, --ctl_block` 和 `--wan-address6-file`



# GNB 的编译构建与运行环境

从安全角度考虑，用户应尽可能自行编译构建 GNB ，如果没有条件编译构建也应尽可能使用来源可靠的二进制包。

GNB 是一个开源的工程，并且在编译构建过程中不需要安装第三方开发库文件，所有源码集中在 `src/` 目录下，所有使用到的第三方开源代码集中在 `libs/` 目录下，因此 GNB 非常容易在各个OS平台上编译构建。



## FreeBSD

```
make -f Makefile.freebsd
```



## OpenBSD

```
make -f Makefile.openbsd
```



## Linux

```
make -f Makefile.linux
```



## OpenWRT

```
make -f Makefile.openwrt
```

编译构建运行在 OpenWRT 上的 GNB 需要到 [https://downloads.openwrt.org/releases/](https://downloads.openwrt.org/releases/ "downloads") 下载 对应的 OpenWRT 版本号和硬件平台的 `OpenWRT SDK`。
在下载 `OpenWRT SDK` 前一定要确认 SDK 对应的 OpenWRT 版本号和硬件平台，否则编译出来的程序将无法在目标固件上运行。
访问 OpenWRT 官方网站可以获得更多有关 `OpenWRT SDK` 信息。

一般的，OpenWRT 的固件默认没有安装 kmod-tun，这是Linux上的虚拟网卡内核模块，GNB 在实现三层交换时需要加载这个内核模块。

某些 OpenWRT 固件的文件系统有一些特别的特性，GNB 在这些文件系统上无法通过 **mmap** 系统调用来创建共享内存映射文件 `gnb.map`，而`/etc` 目录恰好就在这些文件系统上，如果用户把节点配置目录放在`/etc`下，默认情况下，GNB 会尝试在节点配置目录下创建共享内存映射文件 `gnb.map`，这会使得进程无法启动，应该通过`-b, --ctl-block`把共享内存映射文件 `gnb.map` 指向例如 `/tmp` 目录下。

如果固件所使用的文件系统是 **ext4** 将不会遇到这个问题。



## macOS

```
make -f Makefile.Darwin
```

在 macOS 上编译构建 GNB 需要安装 Xcode。



## Windows

```
make -f Makefile.mingw_x86_64
```

`Makefile.mingw_x86_64` 是专门为在 Linux 下用  mingw 编译工具链编译的 Makefile，在 Windows 上也可以使用。

在 Windows 上，运行 GNB 需要安装虚拟网卡，可以在这里下载： [tap-windows](https://github.com/gnbdev/gnb_build/tree/main/if_drv/tap-windows ""downloads"")。

tap-windows 源码所在的仓库:[https://github.com/OpenVPN/tap-windows6](https://github.com/OpenVPN/tap-windows6 "tap-windows6")

[wuqiong](https://www.github.com/wuqiong) 为 GNB 在Windows平台上开发了 wintun 虚拟网卡的接口模块。



# 附录



## gnb的命令行参数

执行`gnb -h`可以看到 gnb 在当前平台所支持的参数。


#### -c, --conf
指定gnb node的目录


#### -n, --nodeid
nodeid


#### -P, --public-index-service
作为公共索引服务运行


#### -a, --node-address
node address


#### -r, --node-route
node route


#### -i, --ifname
指定虚拟网卡的的名字，这在macOS和windows上是无效的，这些系统对虚拟网卡的命名有自己的规则


#### -4, --ipv4-only
禁用ipv6，gnb将不通过ipv6地址收发数据，gnb开启的虚拟网卡不会绑定ipv6地址，由于禁用了ipv6，因此gnb可以设置小于1280的mtu,对于一些限制比较多的网络环境可以利用这个特性尝试使用更小的mtu


#### -6, --ipv6-only
禁用ipv4，gnb将不通过ipv4地址收发数据，gnb开启的虚拟网卡不会绑定ipv4地址


#### -d, --daemon
作为daemon进程启动，Windows不支持这个选项


#### -q, --quiet
禁止在控制台输出信息


#### -t, --selftest
self test


#### -p, --passcode
a hexadecimal string of 32-bit unsigned integer, use to strengthen safety


#### -U, --unified-forwarding
Unified Forwarding "off","force","auto","super","hyper" default:"auto"


#### -l, --listen
listen address default is "0.0.0.0:9001"


#### -b, --ctl-block
ctl block mapper file


#### -e, --es-argv
pass-through gnb_es argv


#### -V, --verbose
verbose mode


#### -T, --trace
trace mode


#### --node-woker-queue-length
node  woker queue length


#### --index-woker-queue-length
index woker queue length


#### --index-service-woker-queue
index service woker queue length


#### --port-detect-start
port detect start


#### --port-detect-end
port detect end


#### --port-detect-range
port detect range


#### --mtu
虚拟网卡的mtu，在比较糟糕的网络环境下ipv4可以设为532,ipv6不可小于1280


#### --crypto
'xor' or 'rc4' or 'none' default is 'xor'; 设定gnb传输数据的加密算法，选择'none'就是不加密，默认是xor使得在CPU运算能力很弱的硬件上也可以有较高的数据吞吐能力。未来会支持aes算法。两个gnb节点必须保持相同的加密算法才可以正常通讯。


#### --crypto-key-update-interval
'hour' or 'minute' or none default is 'none';gnb的节点之间可以通过时钟同步变更密钥，这依赖与节点的时钟必须保持较精确的同步，由于考虑到实际环境中一些节点时钟可能2无法及时同步时间，因此这个选项默认是不启用，如果运行gnb的节点能够保证同步时钟，可以考虑选择一个同步更新密钥的间隔，这可以提升一点通讯的安全性。


#### --multi-index-type
'simple-fault-tolerant' or 'simple-load-balance' default is 'simple-fault-tolerant';如果设置了多个index节点，那么可以选择一个选取index节点的方式，负载均衡或容错模式，这个选项目前还不完善，容错模式只能在交换了通讯密钥的节点之间进行


#### --multi-forward-type
'simple-fault-tolerant' or 'simple-load-balance' default is 'simple-fault-tolerant';如果有多个forward节点，可以选择一个forward节点的方式，负载均衡或在容错模式


#### --socket-if-name
example: 'eth0', 'eno1', only for unix-like os;在unix-like系统上可以让gnb的数据通过指定物理网卡发送，这里需要用户输入物理网卡的名字，Windows不支持这个特性，也看不到该选项


#### --address-secure
'hide part of ip address in logs 'on' or 'off' default is 'on'


#### --if-dump
'dump the interface data frame 'on' or 'off' default is 'off';把经过gnb开启的虚拟网卡的ip分组在日志中输出，这样方便调试系统


#### --pf-route
packet filter route


#### --multi-socket
开启多端口探测,在nat穿透端口探测过程中可以较大提升nat穿透成功率


#### --direct-forwarding
'on' or 'off' default is 'on'


#### --set-tun
不启动虚拟网卡，这个选项时不需要用root权限去启动进程，用在index和fwd服务


#### --index-worker
'on' or 'off' default is 'on'


#### --index-service-worker
'on' or 'off' default is 'on'


#### --node-detect-worker
'on' or 'off' default is 'on'


#### --set-fwdu0
'on' or 'off' default is 'on'


#### --pid-file
指定保存gnb进程id的文件，方便通过脚本去kill进程，如果不指定这个文件，pid文件将保存在当前节点的配置目录下


#### --node-cache-file
gnb会定期把成功连通的节点的ip地址和端口记录在一个缓存文件中，gnb进程在退出后，这些地址信息不会消失，重新启动进程时会读入这些数据，这样新启动gnb进程就可能不需通过index 节点查询曾经成功连接过的节点的地址信息


#### --log-file-path
指定输出文件日志的路径，如果不指定将不会产生日志文件


#### --log-udp4
send log to the address ipv4 default is "127.0.0.1:9000"


#### --log-udp-type
the log udp type 'binary' or 'text' default is 'binary'


#### --console-log-level
log console level 0-3


#### --file-log-level
log file level    0-3


#### --udp-log-level
log udp level      0-3


#### --core-log-level
core log level    0-3


#### --pf-log-level
packet filter log level 0-3


#### --main-log-level
main log level    0-3


#### --node-log-level
node log level    0-3


#### --index-log-level
index log level  0-3


#### --index-service-log-level
index service log level  0-3


#### --node-detect-log-level
node detect log level    0-3

#### --help




## gnb_es的命令行参数
执行`gnb_es -h`可以看到gnb_es在当前平台所支持的参数


#### -b, --ctl_block
ctl block mapper file


#### -s, --service
service mode


#### -d, --daemon
daemon


#### -L, --discover-in-lan
discover in lan


#### --upnp
upnp


#### --resolv
resolv


#### --dump-address
dump address


#### --broadcast-address
broadcast address


#### --pid-file
pid file


#### --wan-address6-file
wan address6 file


#### --if-up
call at interface up


#### --if-down
call at interface down


#### --log-udp4
send log to the address ipv4 default is "127.0.0.1:9000"


#### --log-udp-type
the log udp type 'binary' or 'text' default is 'binary'


***
https://github.com/gnbdev/opengnb

