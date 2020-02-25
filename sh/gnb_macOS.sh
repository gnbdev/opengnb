#!/bin/sh

GNB_DIR=$(dirname $(cd "$(dirname "$0")";pwd))


#GNB_BINARY=FreeBSD_amd64

#GNB_BINARY=Linux_x86_64

GNB_BINARY=macOS

#GNB_BINARY=OpenBSD_amd64

#GNB_BINARY=raspberrypi_ARMv7

#GNB_BINARY=openwrt/ar71xx-generic
#GNB_BINARY=openwrt/ar71xx-mikrotik
#GNB_BINARY=openwrt/ar71xx-nand
#GNB_BINARY=openwrt/mvebu-cortexa9
#GNB_BINARY=openwrt/x86_64
#GNB_BINARY=openwrt/ramips-mt76x8

gnb_op_cmd=$1
gnb_nodeid=$2

if [ "$USER" != "root" ]; then
    sudo $0 $*
    exit $?
fi

show_usage(){
   echo "usage: $0 start|stop|restart node"
   echo "example: $0 start" 1001
   echo "node located: [${GNB_DIR}/conf/]"
   ls ${GNB_DIR}/conf/
}


start_gnb() {
    nohup ${GNB_DIR}/bin/$GNB_BINARY/gnb_es -s -b "${GNB_DIR}/conf/$gnb_nodeid/gnb.map" --dump-address --upnp --pid-file=${GNB_DIR}/conf/$gnb_nodeid/gnb_es.pid    > /dev/null 2>&1 &
	nohup ${GNB_DIR}/bin/$GNB_BINARY/gnb -i "GNB_TUN_$gnb_nodeid" -c "${GNB_DIR}/conf/$gnb_nodeid" --port_detect_start=1000 --port_detect_end=65535  > /dev/null 2>&1 &
}


debug_gnb(){
    nohup ${GNB_DIR}/bin/$GNB_BINARY/gnb_es -s -b "${GNB_DIR}/conf/$gnb_nodeid/gnb.map" --dump-address --upnp     > /dev/null 2>&1 &
	${GNB_DIR}/bin/$GNB_BINARY/gnb -i "GNB_TUN_$gnb_nodeid" -c "${GNB_DIR}/conf/$gnb_nodeid" --port_detect_start=1000 --port_detect_end=65535 --set-if-dump=on
}

stop_gnb() {
	#kill -9 `cat ${GNB_DIR}/conf/$gnb_nodeid/gnb_es.pid`
	#kill -9 `cat ${GNB_DIR}/conf/$gnb_nodeid/gnb.pid`
	killall -9 gnb
	killall -9 gnb_es
}

root_check

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

elif [ "$gnb_op_cmd" = "start" ]; then

	start_gnb

elif [ "$gnb_op_cmd" = "start" ]; then

	debug_gnb

fi


