#!/bin/sh

GNB_DIR=$(dirname $(cd "$(dirname "$0")";pwd))

#GNB_BINARY=FreeBSD_amd64

#GNB_BINARY=Linux_x86_64

#GNB_BINARY=macOS
#GNB_BINARY=OpenBSD_amd64

#GNB_BINARY=raspberrypi_ARMv7

#GNB_BINARY=openwrt/ar71xx-generic
#GNB_BINARY=openwrt/ar71xx-mikrotik
#GNB_BINARY=openwrt/ar71xx-nand
#GNB_BINARY=openwrt/mvebu-cortexa9
GNB_BINARY=openwrt/x86_64
#GNB_BINARY=openwrt/ramips-mt76x8

gnb_op_cmd=$1
gnb_nodeid=$2


show_usage(){
   echo "usage: $0 start|start6|stop|restart node"
   echo "example: $0 start" 1001
   echo "node located: [${GNB_DIR}/conf/]"
   ls ${GNB_DIR}/conf/
}


start_gnb() {
	${GNB_DIR}/sh/dig_wan6.sh
	${GNB_DIR}/bin/$GNB_BINARY/gnb_es -d -b "${GNB_DIR}/conf/$gnb_nodeid/gnb.map" --dump-address --upnp --pid-file=${GNB_DIR}/conf/$gnb_nodeid/gnb_es.pid --resolv --wan-address6-file=/tmp/wan6_addr.dump
	${GNB_DIR}/bin/$GNB_BINARY/gnb -d -i "GNB_TUN_$gnb_nodeid" -c "${GNB_DIR}/conf/$gnb_nodeid" --port-detect-start=500 --port-detect-end=65535
}


debug_gnb(){
	${GNB_DIR}/sh/dig_wan6.sh
	${GNB_DIR}/bin/$GNB_BINARY/gnb_es -d -b "${GNB_DIR}/conf/$gnb_nodeid/gnb.map" --dump-address --upnp --resolv --wan-address6-file=/tmp/wan6_addr.dump
	${GNB_DIR}/bin/$GNB_BINARY/gnb -i "GNB_TUN_$gnb_nodeid" -c "${GNB_DIR}/conf/$gnb_nodeid" --port-detect-start=500 --port-detect-end=65535 --set-if-dump=on
}


stop_gnb() {
	#kill -9 `cat ${GNB_DIR}/conf/$gnb_nodeid/gnb_es.pid`
	#kill -9 `cat ${GNB_DIR}/conf/$gnb_nodeid/gnb.pid`
	killall -9 gnb
	killall -9 gnb_es
}


clean_gnb() {
	rm -f "${GNB_DIR}/conf/$gnb_nodeid/gnb.pid"
	rm -f "${GNB_DIR}/conf/$gnb_nodeid/gnb_es.pid"
	rm -f "${GNB_DIR}/conf/$gnb_nodeid/gnb.map"
	rm -f "${GNB_DIR}/conf/$gnb_nodeid/node_cache.dump"
}


trap 'stop_gnb' INT


if [ -z "$gnb_op_cmd" ] || [ -z "$gnb_nodeid" ]; then
	show_usage
	exit;
fi


if [ ! -d "${GNB_DIR}/conf/$gnb_nodeid" ]; then
	echo "node '$gnb_nodeid' directory '${GNB_DIR}/conf/$gnb_nodeid' not found"
	exit;
fi


if [ "$gnb_op_cmd" = "stop" ] ; then

	stop_gnb

elif [ "$gnb_op_cmd" = "restart" ]; then

	stop_gnb
	start_gnb

elif [ "$gnb_op_cmd" = "debug" ]; then

	stop_gnb
	debug_gnb

elif [ "$gnb_op_cmd" = "clean" ]; then

	clean_gnb

elif [ "$gnb_op_cmd" = "start" ]; then

	start_gnb

fi


