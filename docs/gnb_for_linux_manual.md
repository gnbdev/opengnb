GNB Linux 部署说明

[GNB](https://github.com/gnbdev/gnb "GNB")是一个开源的去中心化的具有极致内网穿透能力的通过P2P进行三层网络交换的VPN。

本文以 Linux 为环境作为测试环境，macOS和Windows10环境类似，Windows10环境需要安装虚拟网卡驱动。

## 快速部署

执行 `gnb_initkey_linux_x86_64.sh` 会根据`conf_tpl`中的模板在 conf/ 目录下生成三个预先设置好的节点 `1001 1002 1003`，节点1001在配置文件里被设为index节点与forward节点(index与forward节点的相关概念在下文中有详细描述)

通过修改`conf/1002`和 `conf/1003 address.conf`，设置好`1001`节点 在实际运行环境中的ip地址，就可以通过执行sh目录下的脚本启动节点。


例如，在Linux下启动节点1001就执行
```
./gnb_linux_x86_64.sh start 1001
```
如果要关闭节点1001，就执行
```
./gnb_linux_x86_64.sh stop 1001
```
***

## 详细配置说明

如果打算自行配置节点，则需要仔细阅读下面的说明。

每个接入GNB网络的设备都是一个GNB节点，每个节点拥有唯一的UUID，被称为nodeid，这是一个32bit无符号整型数字，可以由人工分配，必须保证每个节点的nodeid在整个GNB网络中是唯一的。

GNB节点的配置文件集中存放在一个目录下，一般情况下，每个节点的配置目录作为一个子目录放在 conf 目录下，可以用节点的nodeid来命名。

例如有 1001、1002、1003 三个节点，那么 conf/ 应该存在以下三个目录

```
conf/1001
conf/1002
conf/1003
```

目录下有几个配置文件 node.conf route.conf address.conf


`node.conf`:

以下是 node.conf 一个例子：
```
nodeid 1001
lisen  9001
```

`node.conf` 用于存放节点的配置信息，格式如下
```
nodeid $nodeid
lisen  $lisen_port
```


`route.conf`:
GNB网络里所有的节点共享的一张路由表，配置文件是route.conf

以下是 `route.conf` 一个例子：
```
1001|10.1.0.1|255.255.255.0
1002|10.1.0.2|255.255.255.0
1003|10.1.0.3|255.255.255.0
```
配置文件中的每一行是一个节点的描述，格式如下
```
$nodeid|$tun_ipv4|$tun_netmask
```

每个配置项的含义是这样
$nodeid          节点的UUID
$tun_ipv4        虚拟网卡的IPV4地址
$tun_netmask     虚拟网卡的IPV4地址的子网掩码


如果gnb运行两个网络的网关上，通过配置 `route.conf` 可以让两个ipv4子网内的设备进行互访，即使这些设备上没有运行gnb。
要注意的是，目前仍然需要自行在出口网关上设定路由
例如有一个虚拟ip为 10.1.0.2 的GNB节点，节点所在的内网是 192.168.0.0/24，想要在本地主机（没有运行GNB服务）上访问直接访问 192.168.0.0/24 里的主机,那么位于出口网关的GNB配置`route.conf`应该包含这两项配置

```
1002|10.1.0.2|255.255.255.0
1002|192.168.0.0|255.255.255.0
```

在出口网关上加一条路由
```
ip route add 192.168.0.0/24 via 10.1.0.2
```
为了让对端 192.168.0.0/24 子网中的机器能够访问到本地主机，在对端虚拟ip 为 10.1.0.2 的GNB节点上也需要为到达本地设一条路由。

这些路由指令可以在 `script/if_up_linux.sh` 中预设好，GNB启动时会调用这些脚本。


`address.conf`
用于配置节点的属性和公网ip地址及端口

以下是 `address.conf` 一个例子：
```
i|0|a.a.a.a|9001
if|1001|b.b.b.b|9001
n|1002|c.c.c.c|9001
```
其中，a.a.a.a b.b.b.b c.c.c.c 代表公网ip地址，需要根据实际情况填写


配置文件中的每一行是一个地址
的描述，格式如下

```
$attrib|$nodeid|$ipv4/$ipv6|$port
```

$attrib      节点的属性，用一组字符来表示，i表示这个节点是index节点，f表示这个节点是forward节点，n表示这个节点是一个普通的GNB节点
$nodeid      节点的ID，与 route.conf 中的$nodeid相对应
$ipv6        GNB节点的IPV6地址
$ipv4        GNB节点的IPV4地址
$port        GNB节点的服务端口


如果一个节点同时拥有ipv4和ipv6地址，这就需要分两行配置项，GNB会通过 ping-pong 协议方式去测量该节点哪个ip地址有更低的延时，在发送ip分组数据的时候自动发往低延时的地址。


index节点的作用类似于DNS，如果一组相互之间不知道对方ip地址和端口的GNB节点都向同一个index节点提交自身的ip地址和端口，那这些GNB节点就可以通过对端节点的公钥向index查询对端节点的ip地址和端口。
GNB index 协议允许在提交和查询节点ip地址端口过程中不验证报文的数字签名,即index节点允许对提交和查询ip的报文不验证数字签名，普通节点对index节点响应查询ip的报文不验证数字签名。
index节点可以不绑定nodeid，不需要在 route.conf 配置文件中描述，对于不绑定nodeid的index节点被定义为 public gnb index node 并在 address.conf 中把 $nodeid 这一列设置成0。

forward节点可以为无法直接互访的GNB节点中转ip分组，这些节点通常部署在内网中且没有固定公网ip，并且用尽了所有的办法都无法实现nat穿透实现点对点通讯，forward节点并不能解密两个节点之间的通信的内容。
出于安全考虑，forward节点必须绑定 nodeid 作为配置项出现在 `route.conf` 中，与 gnb forward 协议相关的报文都要发送验证节点的数字签名，即要求forward节点和通过forward节点转发报文的普通节点都必须相互交换了公钥。


在 `address.conf` 中 index节点和forward节点可以有多项，包括可以有多项 $nodeid设为0的 public gnb index node。
一旦配置了多个index节点或forward节点, 就可以使用GNB的index和forward服务的负载均衡和容错模式。

GNB节点通过非对称加密来交换通讯的密钥，这是GNB节点通信的基础，因此需要为每个GNB节点创建一组公私钥，两个节点需要交换公钥才能进行通讯。

公私钥的命名分别是以节点的UUID为文件名 .private 和 .public 为文件的后缀名，以UUID为1001的节点公私钥文件应为 `1001.public` `1001.private`

GNB 提供了一个名为 gnb_crypto 命令行工具，用于生成公私钥，

`./gnb_crypto -c -p 1001.private -k 1001.public`

本节点的公私钥存放在 `conf/$uuid32/security/` 目录下，其他节点的公钥存放 `conf/$uuid32/ed25519/`

以UUID为1001的节点为例

`conf/1001/security/` 目录下应该有 `1001.private` `1001.public`

`conf/1001/ed25519/` 目录下有 `1002.public` `1003.public`


script/目录下存放了一组脚本
`if_up_linux.sh`   是在虚拟网卡启动之后调用的脚本，用于用户自定义设置路由，防火墙的指令
`if_down_linux.sh` 是在虚拟网卡将要关闭时调用的脚本


如果打算自行实现GNB的启动脚本，以下是一个例子

以启动UUID为1001的节点为例：

在 `sh` 目录下创建一个脚本 1001.sh，内容为
```
killall -9 gnb
nohup ../bin/gnb -i LinuxTun1001 -c ../conf/1001 >/dev/null 2>&1 &
```

在Linux下节点启动后，可以用 ip addr 查看虚拟网卡是否启动成功，并尝试用 ping 测试 各节点的虚拟 ip 是否能够ping通。



## gnb的命令行参数

执行`gnb -h`可以看到gnb在当前平台所支持的参数，由于 gnb ver 1.2 还有一些细节在调整，这里只对一些已经明确固定下来的参数进行解释

gnb ver 1.2 的开发还在进行中

|参数|说明|明细|
|-|-|-|
|-c, --conf|config path|指定gnb node的目录，这个在启动gnb时参数是必须的，不可少的|
|-i, --ifname|TUN Device NAME|指定虚拟网卡的的名字，这在macOS和windows上是无效的，这些系统对虚拟网卡的命名有自己的规则|
|-4, --ipv4-only|Use IPv4 Only|禁用ipv6，gnb将不通过ipv6地址收发数据，gnb开启的虚拟网卡不会绑定ipv6地址，由于禁用了ipv6，因此gnb可以设置小于1280的mtu,对于一些限制比较多的网络环境可以利用这个特性尝试使用更小的mtu|
|-6, --ipv6-only|Use IPv6 Only|禁用ipv4，gnb将不通过ipv4地址收发数据，gnb开启的虚拟网卡不会绑定ipv4地址|
|--log-file-path|log file path|指定输出文件日志的路径，如果不指定将不会产生日志文件，当前gnb的日志系统可定制化程度不高|
|--mtu|TUN Device MTU ipv4 532, ipv6 1280|虚拟网卡的mtu，在比较糟糕的网络环境下ipv4可以设为532,ipv6不可小于1280|
|--crypto|ip frame crypto 'xor' or 'rc4' or 'none' default is 'xor'|设定gnb传输数据的加密算法，选择'none'就是不加密，默认是xor使得在CPU运算能力很弱的硬件上也可以有较高的数据吞吐能力。未来会支持aes算法。两个gnb节点必须保持相同的加密算法才可以正常通讯。|
|--crypto-key-update-interval|'hour' or 'minute' or none default is 'none'|gnb的节点之间可以通过时钟同步变更密钥，这依赖与节点的时钟必须保持较精确的同步，由于考虑到实际环境中一些节点时钟可能2无法及时同步时间，因此这个选项默认是不启用，如果运行gnb的节点能够保证同步时钟，可以考虑选择一个同步更新密钥的间隔，这可以提升一点通讯的安全性。关于加密的部分，将来会有专门的文章介绍|
|--multi-index-type|'simple-fault-tolerant' or 'simple-load-balance' default is 'simple-fault-tolerant'|如果设置了多个index节点，那么可以选择一个选取index节点的方式，负载均衡或在容错模式，这个选项目前还不完善，容错模式只能在交换了通讯密钥的节点之间进行|
|--multi-forward-type|'simple-fault-tolerant' or 'simple-load-balance' default is 'simple-fault-tolerant'|如果有多个forward节点，可以选择一个forward节点的方式，负载均衡或在容错模式|
|--set-socket-if-name|example: 'eth0', 'eno1', only for unix-like os|在unix-like系统上可以让gnb的数据通过指定物理网卡发送，这里需要用户输入物理网卡的名字，Windows不支持这个特性，也看不到该选项|
|--set-if-dump|'dump the interface data frame 'on' or 'off' default is 'off'|把经过gnb开启的虚拟网卡的ip分组在日志中输出，这样方便调试系统|     
|--disabled-tun|disabled TUN Device, index node only|不启动虚拟网卡，仅作为gnb index服务启动，由于没有启动虚拟网卡，因此设了这个选项时不需要用root权限去启动gnb|
|--pid-file|pid file|指定保存gnb进程id的文件，方便通过脚本去kill进程，如果不指定这个文件，pid文件将保存在当前节点的配置目录下|  
|--node-cache-file|node address cache file|gnb会定期把成功连通的节点的ip地址和端口记录在一个缓存文件中，gnb进程在退出后，这些地址信息不会消失，重新启动进程时会读入这些数据，这样新启动gnb进程就可能不需通过index 节点查询曾经成功连接过的节点的地址信息|
