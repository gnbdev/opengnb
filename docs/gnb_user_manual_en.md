OpenGNB User Manual

*Note: Most of the content of this article is translated by Google translate from the Chinese version of the "OpenGNB User Manual", the content of this article is subject to the Chinese version.*

# Overview

[OpenGNB](https://github.com/gnbdev/opengnb "OpenGNB") is an open source P2P decentralized VPN with extreme intranet penetration capability.

GNB is a decentralized network, each node in the network is peer-to-peer, and there is no concept of Server and Client.

The ID of each node in each GNB network created by the user is unique, called **nodeid**, which is a 32bit unsigned integer number.

GNB nodes can find other GNB nodes through the index node and establish communication. In this process, the role of the GNB index node is similar to the **tracker** in **DNS** and **Bit Torrent**.

When GNB communicates with the index node, the public key of **ed25519** and the sha512 digest of `passcode` and **nodeid** are used as unique identifiers. Based on this feature, a GNB index node can provide index services for many GNB networks without Collision conflicts are prone to occur.

GNB nodes can also find other GNB nodes through domain name or static IP address or intranet broadcast.

GNB enables nodes located in the LAN to successfully establish P2P communication with other nodes through NAT traversal through many methods including but not limited to port detection, upnp, multi-index, multi-socket, etc. In the worst case, when two GNB nodes When a P2P connection cannot be established, the payload can be transferred through other nodes in the network.

The operating systems and platforms currently supported by GNB are Linux, Windows10, macOS, FreeBSD, OpenBSD, Raspberry Pi, and OpenWRT. Unless otherwise specified, this document applies to the distributions of these supported platforms and the versions compiled for specific platforms.



# deploy GNB



## GNB Public Index node

**GNB Public Index node** only provides index service, does not open TUN devices, does not create virtual IP, does not process IP grouping, does not need to be started with root, and is provided by volunteers.

The command to start a GNB node in **Public Index** mode can be

Start listening on port 9001 by default
```
gnb -P
```

Start listening on port 9002
```
gnb -P -l 9002
```



## Lite Mode of GNB

In **Lite Mode**, there is no need to create **ed25519** key pair and configuration file for the node, GNB node can be started by setting command line parameters.

The **upnp** option is enabled by default in **Lite Mode**.

In the GNB source code, a GNB network with 5 nodes is hardcoded for use in **Lite Mode**.
```
nodeid       tun address
1001         10.1.0.1/255.255.0.0
1002         10.1.0.2/255.255.0.0
1003         10.1.0.3/255.255.0.0
1004         10.1.0.4/255.255.0.0
1005         10.1.0.5/255.255.0.0
```

This greatly simplifies the deployment process.

The GNB node in **Lite Mode** generates the communication key through `passcode`, and the security is much lower than the security mode based on **ed25519** asymmetric encryption.

The GNB node in **Lite Mode** uses the sha512 digest of `passcode` and **nodeid** as the unique identifier when communicating with the index node. It is assumed that the two GNB networks in **Lite Mode** use the same `passcode` ` and the index node, no doubt this must interfere with each other.

The following is a set of commands, which are run on host A and host B respectively to implement virtual networking and need to be executed with **root**

on host A
```
gnb -n 1001 -I "$public_index_ip/$port" -l 9001 --multi-socket=on -p 12345678
```

on host B
```
gnb -n 1002 -I "$public_index_ip/$port" -l 9002 --multi-socket=on -p 12345678
```

At this point, you can see that the TUN IP on host A is 10.1.0.1, and the TUN IP on host B is 10.1.0.2. At this time, the two nodes can access each other if they establish a connection.

Of course, users can set more nodes in **Lite Mode**, which requires understanding `-n`, `-I`, `-a,`, `-r`, `-p`, etc. of `gnb` use of parameters.



## GNB Safe Mode

To start GNB in ​​**Safe Mode** mode, you need to set `node.conf` `address.conf` `route.conf` files and **ed25519** public and private keys correctly, these files need to be placed in a directory, the name of the directory Can be **nodeid**.

where `address.conf` is optional.
Assuming that a node with a nodeid of `1000` is running on the WAN as an index node, and other GNB nodes find the `1000` node through the configuration of `address.conf`, then in general, the `1000` node does not need `address.conf` .

If multiple GNB nodes are created, the configuration directories for these nodes can be placed under `gnb/conf/` for easy management.



### Directory Structure

Take nodes `1001` `1002` `1003` as an example, the configuration directories of these nodes are as follows

```
gnb / conf / 1001 /

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
gnb / conf / 1002 /

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
gnb / conf / 1003 /

gnb/conf/1003/node.conf
gnb/conf/1003/address.conf
gnb/conf/1003/route.conf
gnb/conf/1003/security/1003.private
gnb/conf/1003/security/1003.public
gnb/conf/1003/ed25519/1001.public
gnb/conf/1003/ed25519/1002.public
gnb/conf/1003/scripts
```

Assuming that the configuration directories of these nodes have been deployed to different hosts, the commands to start the `1001` `1002` `1003` nodes can be:
```
gnb -c gnb/conf/1001
gnb -c gnb/conf/1002
gnb -c gnb/conf/1003
```

The `scripts/` directory stores scripts that are called after the virtual network card is started/closed, which is used for user-defined routing and firewall instructions.

`security/` stores the **ed25519** public and private keys of this node, and `ed25519/` stores the public keys of other nodes.



### key exchange

In **Safe Mode**, GNB nodes exchange communication keys through asymmetric encryption (ed25519), which is the basis of GNB node communication, so it is necessary to create a set of public and private keys for each GNB node for communication.

The public and private keys are named with the UUID of the node as the file name, .private and .public as the file suffix. The public and private key files of the node whose UUID is 1001 should be `1001.public` `1001.private`

GNB provides a command line tool named `gnb_crypto` for generating public and private keys,

The command to create **ed25519** public and private keys for `1001` `1002` `1003` nodes is:

`gnb_crypto -c -p 1001.private -k 1001.public`
`gnb_crypto -c -p 1002.private -k 1002.public`
`gnb_crypto -c -p 1003.private -k 1003.public`

To exchange public keys, is to put these **ed25519** public and private keys into the correct directory.

On the node with **nodeid** `1001`:
There should be `1001.private` `1001.public` in the `gnb/conf/1001/security/` directory
There should be `1002.public` `1003.public` in the `gnb/conf/1001/ed25519/` directory

On the node with nodeid `1002`:
There should be `1002.private` `1002.public` in the `gnb/conf/1002/security/` directory
There should be `1001.public` `1003.public` in the `gnb/conf/1002/ed25519/` directory

On the node with nodeid `1003`:
There should be `1003.private` `1003.public` in the `gnb/conf/1003/security/` directory
There should be `1001.public` `1002.public` in the `gnb/conf/1003/ed25519/` directory

On the basis of asymmetric encryption, nodes under the same GNB network can also set the same `passcode` to protect the communication key.

For information about GNB communication keys, please refer to the description of `--crypto` `--crypto-key-update-interval` `--passcode`.


## GNB payload forwarding
   - **Direct Forwarding** P2P communication between nodes; under normal circumstances, nodes located on the WAN must be able to establish P2P communication with other nodes, and nodes located in the LAN to establish P2P communication with nodes in other LANs must go through NAT penetration;
   - **Unified Forwarding** automatically forwards IP packets through nodes that have established P2P communication, and multiple nodes retransmit IP packets;
   - **Relay Forwarding** highly custom relay routing, IP packets will be encrypted before being sent to the next relay point, the relevant settings are in `route.conf`;
   - **Standard Forwarding** Nodes that cannot establish P2P communication after exhausting all strategies can relay IP packets through forward nodes located on the public network


### node.conf configuration file

The following is an example of `node.conf`:
```
nodeid 1001
listen 9001
```

`node.conf` is used to store the configuration information of the node, the format is as follows
```
nodeid $nodeid
listen $listen_port
```
or
```
nodeid $nodeid
listen ip_address:$listen_port
```

GNB supports IPV6 and IPV4.

There are some options worth paying attention to, which can greatly improve GNB's NAT traversal capabilities:
```
es-argv      "--upnp"
```
Try turning on upnp to make it easier for IP packets from the WAN to be passed in.

```
multi-socket on
```
Open more UDP listening ports and increase the probability of successful port detection for NAT penetration.

After `multi-socket` is set to on, 4 additional random UDP ports will be opened in addition to the original `listen` port. If you want to monitor the specified port additionally, you can use multiple `listen` setting items, GNB Supports listening on 5 address ports with `listen` or listening on 5 IPV6 addresses and ports with `listen6` and listening on 5 IPV4 address ports with `listen4`.

The configuration items supported by `node.conf` correspond to the gnb command line parameters one by one. The currently supported configuration items are:
```
ifname nodeid listen listen6 listen4 ctl-block multi-socket disabled-direct-forward ipv4-only ipv6-only passcode quiet daemon mtu set-tun address-secure node-worker index-worker index-service-worker node-detect-worker port-detect-range port-detect-start port-detect-end pid-file node-cache-file log-file-path log-udp4 log-udp-type console-log-level file-log-level udp-log-level core-log-level pf-log-level main-log-level node-log-level index-log-level detect-log-level es-argv
```


### route.conf  file

The data packet communicated between GNB nodes is called **gnb payload**, a routing table shared by all nodes in the network, the configuration file is `route.conf`, its role is to tell the GNB core how to "route" these Contains the **gnb payload** of the IP packet.

Each routing configuration item occupies one line. There are two types of routing configuration items, **forward** and **relay**, which are used to set the way that the payload of the local node reaches the peer node.

The following is an example of `route.conf` type forward:
```
1001|10.1.0.1|255.255.255.0
1002|10.1.0.2|255.255.255.0
1003|10.1.0.3|255.255.255.0
```

Each line in the configuration file is a description of a node in the following format
```
$nodeid|$tun_ipv4|$tun_netmask
```

The meaning of each configuration item is as follows:
`$nodeid` UUID of the node
`$tun_ipv4` IPV4 address of the virtual network card
`$tun_netmask` The subnet mask of the IPV4 address of the virtual network interface


#### Direct forwarding 

The default communication between nodes is point-to-point, which is called **Direct forwarding**, that is, IP packets are sent directly to the IP address and port of the peer node.

For example, in `route.conf` there is a line
```
1001|10.1.0.1|255.255.255.0
```

When GNB processes an IP packet whose destination address is 10.1.0.1, it determines that this IP packet needs to be encrypted and encapsulated with the communication key of the node whose **nodeid** is 1001, and sent to the node 1001 as **gnb payload** , this process we call **Direct forwarding**.


#### Standard Forwarding

If there is no P2P communication between nodes, the **gnb payload** can be forwarded through the **forward** node with the **f** attribute defined in `address.conf`. This communication method is called * *Standard Forwarding**

example:
`address.conf`
```
if|1001|x.x.x.x|9001
```

The 1001 node in the above example is an **idnex** node and also a **forward** node.


#### Relay Forwarding

**Relay Forwarding** relays the **gnb payload** through a clear custom relay path, and the **gnb payload** will be encrypted again by the relay node during the relaying process.

Due to end-to-end encryption, the **forward node** and **relay node** nodes cannot obtain the plaintext of the IP packets in the **gnb payload** at both ends of the communication.

To set a relay node for the destination node, you can set up to 8 relay routes for a peer node, and each relay route can have up to 5 relay nodes.

Here are some examples of `route.conf` type relay:


```
1006|1003
1006|auto,static
```
The function of this group of routes is: when the current node cannot establish P2P communication with the 1006 node, the IP packet sent to the 1006 node will be relayed through the 1003 node.


```
1006|1003,1004,1005
1006|force,static
```
The meaning of this group of routes is: the IP packets sent by the current node to the 1006 node are forced to be relayed to the 1006 node through 1005, 1004, 1003 in sequence.


```
1006|1003,1005
1006|1005,1004
1006|1005,1003
1006|force,balance
```
The role of this group of routes is: there are 3 relay routes from the current node to the 1006 node, and the strategy for selecting routes when sending IP packets is load balancing.

The relevant setting items are formatted as follows:
```
$nodeid|$relay_nodeid3,$relay_nodeid2,$relay_nodeid1
$nodeid|$relay_mode
```

The path for the payload of the local node to reach the peer node through the relay node is local_node => relay_node1 => relay_node2 => relay_node3 => dst_node, and the path from dst_node to local_node is determined by the route.conf configuration of the peer dst_node.

`$relay_mode` can be **auto**, **force**, **static**, **balance**

**auto** Forward through the preset relay route when the destination node cannot communicate point-to-point and there is no forward node forwarding
**force** Force communication with the destination node to go through the relay route
**static** When a destination node has multiple relay routes, use the first relay route
**balance** When a destination node has multiple relay routes, the relay route is selected in a load balancing manner



### address.conf file

`address.conf` is used to configure the properties of the node and the public IP address and port. The following is an example of `address.conf`:

```
i|0|a.a.a.a|9001
if|1001|b.b.b.b|9001
n|1002|c.c.c.c|9001
```

`aaaa` `bbbb` `cccc` in the file represents the IP address of the physical network card of the node, which needs to be filled in according to the actual situation. Each line in the configuration file is a description of an address, and the format is as follows

```
$attrib|$nodeid|$ipv4/$ipv6/$hostname|$port
```

**$attrib** Node attributes, represented by a set of characters, i means that this node is an index node; f means that this node is a forward node; n means that this node is a normal node; s is a silent (silence) node, If the local node contains the s attribute, it will not communicate with the index node, nor will it respond to ping-pong and address detection requests to expose the local ip address
**$nodeid** The nodeid of the node, corresponding to **$nodeid** in `route.conf`
**$ipv6** IPV6 address of the GNB node
**$ipv4** IPV4 address of the GNB node
**$hostname** The domain name of the GNB node
**$port** GNB node service port


If a node has multiple IP addresses, it needs to be configured in multiple lines according to the format. GNB will use the **GNB node ping-pong** protocol to measure which IP address of the node has lower latency, and then send IP packets. When data is sent, it is automatically sent to a low-latency address.

The `gnb` process does not perform domain name interpretation on **$hostname** in `address.conf`, but is processed asynchronously by `gnb_es`, see the `--resolv` option of `gnb_es` for related information.

As mentioned above, the function of the index node is similar to the **tracker** in **DNS** and **Bit Torrent**. If a group of GNB nodes that do not know each other's IP address and port are sent to the same index The node submits its own IP address and port, then these GNB nodes can query the peer node's IP address and port from the index through the peer node's public key.

The **i** attribute is required in the **$attrib** of the index node in `address.conf`.

The GNB index protocol allows not to verify the digital signature of the message in the process of submitting and querying the IP address port of the node, that is, the index node allows the digital signature of the message to submit and query the IP not to be verified, and the node does not respond to the message of the query IP from the index node. Verify digital signatures.

**GNB Public Index node** The corresponding **nodeid** in `address.conf` can be set to **0**, no need to set the "route" rule in `route.conf`, the following is An example of a **GNB Public Index node**:
```
i|0|a.a.a.a|9001
```

The forward node **$attrib** is set to **f**, **$nodeid** cannot be set to **0**, and must be bound to nodeid as a configuration item in `route.conf`, gnb forward Protocol-related messages must be sent with the digital signature of the verification node, that is, the forward node and the node forwarding the message through the forward node must exchange public keys with each other.

The forward node can transfer IP packets for GNB nodes that cannot directly access each other. These nodes are usually deployed in the intranet and do not have a fixed public network ip, and they have exhausted all methods to achieve NAT penetration and achieve point-to-point communication. The content of the communication between the two nodes cannot be decrypted.

Of course, users can use a GNB node as the index node and forward node in the GNB network, and can also provide index services for other GNB networks. The following is an example:
```
if|1001|b.b.b.b|9001
```

In `address.conf` there can be multiple index nodes and forward nodes, and can be passed through
`--multi-index-type` `--multi-forward-type` Set load balancing and fault tolerance mode,

Assuming that a GNB node may have multiple NATed IP addresses at the same time after very complex address translation in the LAN, that is, the operator may choose different gateway exits according to the destination address of the host in the LAN accessing the WAN. If `address.conf` has multiple Index nodes, then in NAT traversal, the peer GNB node may know more addresses and ports of the node, thereby increasing the success rate of NAT traversal to establish P2P communication.


## GNB Unified Forwarding

Under the complex NAT mechanism, the nodes located in the LAN cannot ensure 100% successful NAT traversal, but there will always be some nodes that can successfully achieve NAT traversal and establish P2P communication. The **GNB Unified Forwarding** mechanism can make The nodes that have established P2P communication forward IP packets to the nodes that have not established P2P communication.

Each node in the GNB network periodically announces the id of the node that has established P2P communication with itself in the network, which enables each node in the network to know which transit nodes are available when sending data to another node.

Take a GNB network with three nodes A, B, and C as an example: A and C, B and C all establish P2P communication, but A and B cannot establish P2P communication due to the complex NAT mechanism, through **GNB Unified Forwarding** mechanism, A and B can forward IP packets through C.

According to the mechanism of **GNB Unified Forwarding**, as long as the number of nodes in the virtual network is more, it is easier to achieve NAT traversal between nodes and establish a virtual link.

The IP packets sent to the target node can be forwarded by multiple transit nodes at the same time. The IP packets that reach the target node first will be written to the virtual network card. GNB maintains a 64-bit timestamp and timestamp queue for each target node to ensure repeated transmission. IP packets are discarded, so the **GNB Unified Forwarding** feature can be used to speed up TCP retransmissions in some cases.


### GNB Unified Forwarding Operating mode
`auto` When the two nodes A and B cannot establish P2P communication, automatically transfer IP packets through the nodes (which can be in LAN) that have established P2P communication with A and B respectively.
`super` Regardless of whether the two nodes A and B have established P2P communication, in the process of sending ip packets, multiple nodes that have established P2P communication with A and B, respectively, retransmit and transfer TCP type ip packets.
`hyper` Regardless of whether the two nodes A and B have established P2P communication, in the process of sending ip packets, all types of ip packets are retransmitted and relayed through multiple nodes that have established P2P communication with A and B respectively.


To learn more about **Unified Forwarding**, see the `-U, --unified-forwarding` option of `gnb`


### About Discover In Lan

**Discover In Lan** enables GNB nodes to discover other GNB nodes in the LAN through broadcast, which enables GNB nodes under the same LAN to establish communication without first going through WAN for NAT penetration.

**Discover In Lan** is implemented in `gnb_es`.

In order for `gnb_es` to always listen for broadcast packets from other nodes in the LAN, `gnb_es` must be started as service or daemon with the `-L, --discover-in-lan` option.

The following node 1002 is an example
```
gnb_es -s -L -b gnb / conf / 1002 / gnb.map
```

`gnb.map` is a shared memory mapping file created by the `gnb` process. If not specified, this mapping file will be created in the node's configuration directory (`gnb/conf/1002/`); through this shared memory , `gnb_es` can pass the information from other GNB nodes monitored in the LAN to the gnb process; at the same time, `gnb_es` also regularly broadcasts the information of its own node in the LAN so that it can be discovered by other nodes in the LAN.

In the process of deploying GNB nodes, users will find that in some cases, some GNB nodes in the same LAN can successfully NAT traverse and successfully establish P2P communication with nodes in another LAN, while some nodes fail to establish P2P communication. Success, with the help of **Discover In Lan**, not only can more nodes be connected more easily, but also the nodes that have successfully NAT traversed can help other nodes under the same LAN to forward IP packets.

For more information about **Discover In Lan**, you can refer to `-b, --ctl-block` of `gnb`, and `-s, --service` `-d, - of `gnb_es` -daemon` `-L, --discover-in-lan` options.



### About net to net

Generally speaking, VPN can combine several computers into a network by establishing a virtual link, or allow one computer to access a network through a virtual link, and can also connect two or more computers through a virtual link. The network forms a large virtual network, so that computers scattered in different networks can access each other.

This kind of architecture that integrates several networks into a virtual network through the virtual link of VPN is called ** net to net **.

GNB makes the deployment of ** net to net ** very easy. With GNB's powerful NAT traversal capability, GNB can easily combine several LANs located in different places to form a virtual network, and most of the time one of these LANs is a virtual network. There is P2P direct communication between the two networks without the need for the central node to transfer, so that the communication between the networks is not limited by the bandwidth of the central node.

Assuming that there are two remote **LAN A** and **LAN B** accessing the Internet through NAT, and the gateways of these two LANs are both OpenWRT Routers, then only two GNB nodes need to be deployed on the OpenWRT Router, with the help of WAN On the GNB index node to achieve NAT penetration, the hosts in **LAN A** and **LAN B** can access each other.

Assuming that there are two remote **LAN A** and **LAN B** accessing the Internet through NAT, and the gateways of these two LANs are both OpenWRT Routers, then only two GNB nodes need to be deployed on the OpenWRT Router to traverse the Internet through NAT. By establishing P2P communication, hosts in **LAN A** and **LAN B** can access each other.
In this process, it is usually necessary to have a GNB index node located on the WAN to help the two GNB nodes find each other. In the following example, in order to ensure that the two nodes can communicate even if the NAT traversal is unsuccessful, it is possible to deploy A forward node also provides index service.



Thus, a total of 3 GNB nodes need to be deployed:

**nodeid**=1001 on WAN, as index and forward node, IP= xxxx port = 9001
**nodeid**=1002 on OpenWRT Router on **LAN A**, NetWork=192.168.0.0/24
**nodeid**=1003 on OpenWRT Router on **LAN B**, NetWork=192.168.1.0/24



set **node 1001**

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



Set **node 1002** in **LAN A**

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
xxxx is the WAN IP of node 1001, which needs to be filled in according to the actual situation.

`conf/1002/scripts/if_up_linux.sh`
```
ip route add 192.168.1.0/24 via 10.1.0.3
```
The function of this routing instruction is: when the host in **LAN A** wants to send the IP packet whose destination address is `192.168.1.0/24`, because the network of **LAN A** is `192.168.0.0/24` `, so these purpose is `192.168.1.0/24` IP packets will be sent to the default gateway OpenWRT Router, because this routing rule is set, these IP packets will be forwarded to the TUN device to be captured and encapsulated by the GNB process as ** The gnb payload** is finally sent to **node 1003**.

Undoubtedly, there needs to be a similar routing directive on **node 1003** so that IP packets destined to `192.168.0.0/24` can reach **node 1003** GNB processes are eventually sent to **node 1002** on.

Set **node 1003** in **LAN B**

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

Usually, OpenWRT has some firewall rules by default, which may cause the IP packets forwarded by GNB to the host in the current LAN to be blocked. Therefore, OpenWRT needs to be detected here. Generally, the Forward option in the firewall should be set to accept. , and the Forwardings related to Wan are set to accept.

After confirming that the configuration is correct, you can start **node 1001** first, then **node 1002** and **node 1003**, so that **node 1001** can be the first time **node 1002** And **node 1003** provides index service.

After starting **node 1001**, **node 1002** and **node 1003**, you can first try the virtual IPs corresponding to these three GNB nodes, namely `10.1.0.1` `10.1.0.2` ` 10.1.0.3` can successfully pass the ping test.

If all goes well, you can try to ping a host on **LAN B** from a host in **LAN A**.

For example, ping the host whose IP is `192.168.0.2` under **LAN A** and ping the host whose IP is `192.168.1.2` under **LAN B**, no doubt, you can also test in the opposite direction .

At this point, if it is found that the virtual IPs corresponding to the three GNB nodes can pass the ping test, but the hosts in the two LANs cannot pass the ping test, then you can go back to the previous steps to check the firewall rules on OpenWRT and the detection host. firewall rules.





# About GNB's NAT traversal

The ultimate NAT penetration capability may be the most exciting feature of GNB, but it is not the whole of GNB. There are many valuable features of GNB to be explored by users.

In a complex network environment, it is sometimes difficult to establish P2P communication with other nodes through successful NAT traversal. In order to improve the probability of successful NAT traversal of a node, it is necessary to introduce NAT (Network Address Translation) and GNB in ​​NAT traversal in detail. work done.

Usually, for some hosts that access the Internet through NAT in the LAN, the process of NAT (Network Address Translation) will involve, but not limited to, such as routing equipment in the LAN, network structure, network equipment used by operators, network Architecture, NAT strategy and many other network environments are almost "black box". The IP address of a packet may undergo multiple translations, and there may be multiple egress gateways. These are uncertain factors in the NAT traversal process.


**Don't talk about the white paper, who known? who care?**

**Don't talk about the type of NAT,  I don‘t  f*cking care**

**What GNB has to do, just try it's best to traverse the NAT**


There is no doubt that GNB was inspired by **Bit Torrent** these P2P software, since two hosts in different LANs accessing the Internet through NAT can establish P2P communication for transferring file data blocks, then it means that This mode can also be used to transmit IP packets for virtual network cards.

GNB uses all kinds of strategies to promote the success of NAT traversal. Among these strategies, some strategies are very effective, some are not very effective, and some strategies are effective in some situations and may not be effective in other situations.

First, the local node obtains the WAN addresses and ports of other nodes by communicating and exchanging information with the index node and tries to contact these nodes;

In fact, some nodes will use more than one WAN address to access the Internet after going through NAT. GNB supports exchanging information with multiple index nodes at the same time, so that there is a chance to discover more WAN addresses of peer nodes.

For peer nodes that are defined in `route.conf` and have not established P2P communication, the GNB process will periodically query the index node for the WAN address ports of these nodes, and try to initiate communication.

GNB will detect a small range of ports for the peer node that has obtained the WAN address but has not established P2P communication.

GNB also has a special thread to periodically detect a wide range of ports for orders that fail to establish P2P communication.

Like most P2P software such as **Bit Torrent**, GNB will also try to establish address-port mapping on the egress gateway through UPNP to improve the success rate of NAT penetration.

It should be noted that if a LAN has multiple hosts deploying GNB nodes and enabling UPNP, these nodes should not listen to the same port, which will cause conflicts when the gateway establishes UPNP port mapping.

If the gateway where the host is located supports and enables the UPNP service and the option `--es-argv "--upnp"` is used when GNB is started, the success probability of NAT penetration will be significantly improved. For details, see `--es- of `gnb` argv "--upnp"` and the `--upnp` option for `gnb_es`.

Since monitoring a UDP port and enabling UPNP can effectively improve the probability of successful NAT penetration, then by monitoring more UDP ports and establishing more UPNP port mappings on the gateway, the probability of successful NAT penetration can be further improved. `--multi-socket` option.

In some network environments, fully understanding the functions of `--es-argv "--upnp"` of `gnb`, `--multi-socket` and `--upnp` of `gnb_es` can be very effective in improving GNB The success rate of the node's NAT penetration.

In some network environments, fully understand `--es-argv "--upnp"` of `gnb`, `--multi-socket`, `--unified-forwarding` and `--upnp` of `gnb_es` The option can effectively improve the success rate of NAT traversal of GNB nodes.

Using the `--dump-address` option of `gnb_es` can make the address information of the nodes that have established P2P communication regularly saved in the specified node cache file, when modifying the configuration file or upgrading the program or restarting the host, you need to restart `gnb `, you can use the `--node-cache-file` option to specify the node cache file to quickly re-establish communication. It should be noted that some nodes may not be successful. For details, you can learn about `--node- of `gnb` The `--dump-address` option of the cache-file` option `gnb_es`.

In addition to finding other nodes through the index node, GNB nodes can also configure the domain name or dynamic domain name of the peer node in `address.conf`. It should be noted that only relying on this method to try NAT penetration is not as effective as using the index node. Good for helping to establish P2P communication.

So far, GNB has done a lot of work for NAT penetration of nodes, and the success rate of NAT penetration has reached an unprecedented height, but this is still not enough.

The **GNB Unified Forwarding** mechanism enables nodes that have established P2P communication to forward IP packets to nodes that have not established P2P communication, and as long as the number of nodes in the virtual network is greater, the easier it is to achieve NAT penetration between nodes. A virtual link is established.


Through **Discover In Lan**, multiple GNB nodes under the same LAN can realize P2P communication without going through WAN. At the same time, this allows nodes that fail NAT penetration to pass through **relay** The method allows the nodes that have successfully NAT traversed to transfer IP packets to indirectly realize P2P communication.

The `--broadcast-address` of `gnb_es` allows the GNB node to spread the address information of the node that has established P2P communication with this node to other nodes, so that these nodes can obtain more address information of other nodes in the GNB network To increase the success rate of NAT penetration.


It has to be emphasized that even if a lot of efforts are made, it is impossible to establish a 100% success rate for P2P communication through NAT penetration. The flexible use of the mechanism provided by GNB can greatly improve the probability of success.



# About IPV6 gateway/firewall penetration of GNB

In general, IPV6 addresses are all WAN addresses, but this does not mean that hosts with IPV6 behind the gateway do not need to penetrate. Of course, this penetration cannot be called NAT penetration, but can be called NAT penetration. For gateway/firewall penetration.
Due to the role of the firewall at the gateway, the firewall policy of some gateways (such as OpenWRT route) does not allow IP packets from the WAN initiated by the opposite host by default. Send IP packets.

There is no doubt that IPV6 gateway/firewall penetration is much easier than IPV4 NAT penetration, and the success rate can be 100%.

GNB does some work for IPV6 gateway/firewall penetration.

Assuming that some GNB nodes behind the gateway have IPV6 addresses, while the index nodes located in the WAN do not have IPV6 addresses, and the index nodes cannot directly obtain the IPV6 addresses of the GNB nodes behind the gateway and thus cannot help these nodes to establish communication. To solve this problem, the GNB node behind the gateway needs to obtain the IPV6 address of the host where it is located and report it to the index node.

Assuming that the host has an IPV6 address, one of the following two commands can be used to obtain the IPV6 address of the host and save it in a file.

```
dig -6 TXT +short o-o.myaddr.l.google.com @ns1.google.com | awk -F'"' '{ print $2}' > /tmp/wan6_addr.dump
```

```
wget http://api.myip.la -q -6 -O /tmp/wan6_addr.dump
```

Take node 1002 as an example
```
gnb_es -b gnb/conf/1002/gnb.map --wan-address6-file=/tmp/wan6_addr.dump
```
Through the shared memory of `gnb` and `gnb_es`, the `gnb` process can obtain the IPV6 address of this node and report it to the index node.

For related information, see `-b, --ctl_block` and `--wan-address6-file` of `gnb_es`





# GNB build and run environment

From a security point of view, users should compile and build GNB by themselves as much as possible. If there is no conditional compilation and build, they should use binary packages from reliable sources as much as possible.

GNB is an open source project, and there is no need to install third-party development library files during the compilation and construction process. All source code is concentrated in the `src/` directory, and all third-party open source code used is concentrated in the `libs/` directory. Therefore, GNB is very easy to compile and build on various OS platforms.



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

To compile and build GNB running on OpenWRT, you need to go to [https://downloads.openwrt.org/releases/](https://downloads.openwrt.org/releases/ "downloads") to download the corresponding OpenWRT version number and hardware platform `OpenWRT SDK`.
Before downloading the `OpenWRT SDK`, be sure to confirm the OpenWRT version number and hardware platform corresponding to the SDK, otherwise the compiled program will not run on the target firmware.
Visit the official OpenWRT website for more information about the `OpenWRT SDK`.

Generally, kmod-tun is not installed by default in the firmware of OpenWRT, which is a virtual network card kernel module on Linux. GNB needs to load this kernel module when implementing Layer 3 switching.

Some OpenWRT firmware filesystems have some special features, GNB cannot create shared memory mapped file `gnb.map` through **mmap** system call on these filesystems, and `/etc` directory happens to be in these On the file system, if the user puts the node configuration directory under `/etc`, by default, GNB will try to create the shared memory map file `gnb.map` in the node configuration directory, which will make the process unable to start. `-b, --ctl-block` Point the shared memory map file `gnb.map` to eg `/tmp` directory.

This problem will not be encountered if the file system used by the firmware is **ext4**.



## macOS

```
make -f Makefile.Darwin
```

Compiling and building GNB on macOS requires Xcode to be installed.



## Windows

```
make -f Makefile.mingw_x86_64
```

`Makefile.mingw_x86_64` is a Makefile specially compiled with mingw compiling toolchain under Linux, Also available on Windows. 

On Windows, running GNB requires installing a virtual network card, which can be downloaded here: [tap-windows](https://github.com/gnbdev/gnb_build/tree/main/if_drv/tap-windows ""downloads"").

The repository where tap-windows source code is located: [https://github.com/OpenVPN/tap-windows6](https://github.com/OpenVPN/tap-windows6 "tap-windows6")





# Appendix



## gnb command line arguments

Execute `gnb -h` to see the parameters supported by gnb on the current platform.


#### -c, --conf
Specify the directory of the gnb node


#### -n, --nodeid
nodeid


#### -P, --public-index-service
Runs as a public indexing service


#### -a, --node-address
node address


#### -r, --node-route
node route


#### -i, --ifname
Specify the name of the virtual network card, which is invalid on macOS and Windows, these systems have their own rules for naming virtual network cards


#### -4, --ipv4-only
Disable ipv6, gnb will not send and receive data through ipv6 address, the virtual network card enabled by gnb will not bind ipv6 address, because ipv6 is disabled, gnb can set mtu less than 1280, this feature can be used for some network environments with more restrictions try with a smaller mtu


#### -6, --ipv6-only
Disable ipv4, gnb will not send and receive data through the ipv4 address, and the virtual network card enabled by gnb will not bind the ipv4 address


#### -d, --daemon
Started as a daemon process, Windows does not support this option


#### -q, --quiet
Disable output to console


#### -t, --selftest
self test


#### -p, --passcode
a hexadecimal string of 32-bit unsigned integer, use to strengthen safety


#### -l, --listen
listen address default is "0.0.0.0:9001"


#### -b, --ctl-block
ctl block mapper file


#### -e, --es-argv
pass-through gnb_es argv


#### -V, --verbose
verbose mode


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
The mtu of the virtual network card can be set to 532 for ipv4 and 1280 for ipv6 in a poor network environment.


#### --crypto
'xor' or 'rc4' or 'none' default is 'xor'; Set the encryption algorithm of gnb transmission data, select 'none' means no encryption, the default is xor so that it can also be used on hardware with weak CPU computing power Higher data throughput. The aes algorithm will be supported in the future. Two gnb nodes must maintain the same encryption algorithm for normal communication.


#### --crypto-key-update-interval
'hour' or 'minute' or none default is 'none'; the key can be changed between gnb nodes through clock synchronization, which depends on the fact that the clock of the node must be synchronized more accurately, because some node clocks in the actual environment are considered It may not be possible to synchronize the time in time, so this option is not enabled by default. If the node running gnb can guarantee the synchronization of the clock, you can consider selecting an interval for synchronizing and updating the key, which can improve the security of communication.


#### --multi-index-type
'simple-fault-tolerant' or 'simple-load-balance' default is 'simple-fault-tolerant'; if multiple index nodes are set, you can choose a way to select index nodes, load balancing or fault-tolerant mode, this The options are not yet perfect, and the fault-tolerant mode can only be performed between nodes that have exchanged communication keys


#### --multi-forward-type
'simple-fault-tolerant' or 'simple-load-balance' default is 'simple-fault-tolerant'; if there are multiple forward nodes, you can choose a forward node method, load balancing or in fault-tolerant mode


#### --socket-if-name
example: 'eth0', 'eno1', only for unix-like os; on unix-like systems, gnb data can be sent through the specified physical network card, where the user needs to enter the name of the physical network card. Windows does not support this feature, and also I don't see this option


#### --address-secure
'hide part of ip address in logs 'on' or 'off' default is 'on'


#### --if-dump
'dump the interface data frame 'on' or 'off' default is 'off'; group the ip of the virtual network card opened by gnb and output it in the log, which is convenient for debugging the system


#### --pf-route
packet filter route


#### --multi-socket
When multi-port detection is enabled, the success rate of NAT penetration can be greatly improved in the process of NAT penetration port detection.


#### --direct-forwarding
'on' or 'off' default is 'on'


#### --set-tun
Do not start the virtual network card, this option does not require root privileges to start the process, used in index and fwd services


#### --index-worker
'on' or 'off' default is 'on'


#### --index-service-worker
'on' or 'off' default is 'on'


#### --node-detect-worker
'on' or 'off' default is 'on'


#### --set-fwdu0
'on' or 'off' default is 'on'


#### --pid-file
Specify the file that saves the gnb process id, which is convenient to kill the process through a script. If this file is not specified, the pid file will be saved in the configuration directory of the current node


#### --node-cache-file
gnb will regularly record the IP address and port of the successfully connected node in a cache file. After the gnb process exits, these address information will not disappear, and the data will be read when the process is restarted, so that it is possible to start the gnb process. There is no need to query the address information of the nodes that have been successfully connected through the index node


#### --log-file-path
Specify the path of the output file log, if not specified, no log file will be generated


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
node detect log level      0-3

#### --help





## Command line arguments for gnb_es
Execute `gnb_es -h` to see the parameters supported by gnb_es on the current platform


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
