#!/bin/sh

GNB_DIR=$(dirname $0)

#GNB_BINARY=FreeBSD_amd64

GNB_BINARY=Linux_x86_64

#GNB_BINARY=macOS
#GNB_BINARY=OpenBSD_amd64

#GNB_BINARY=raspberrypi_ARMv7

#GNB_BINARY=openwrt/ar71xx-generic
#GNB_BINARY=openwrt/ar71xx-mikrotik
#GNB_BINARY=openwrt/ar71xx-nand
#GNB_BINARY=openwrt/mvebu-cortexa9
#GNB_BINARY=openwrt/x86_64
#GNB_BINARY=openwrt/ramips-mt76x8


cp -r ${GNB_DIR}/conf_tpl/* ${GNB_DIR}/conf


${GNB_DIR}/bin/$GNB_BINARY/gnb_crypto -c -p 1001.private -k 1001.public
${GNB_DIR}/bin/$GNB_BINARY/gnb_crypto -c -p 1002.private -k 1002.public
${GNB_DIR}/bin/$GNB_BINARY/gnb_crypto -c -p 1003.private -k 1003.public

cp 1001.public 1002.public 1003.public ${GNB_DIR}/conf/1001/ed25519/
cp 1001.public 1002.public 1003.public ${GNB_DIR}/conf/1002/ed25519/
cp 1001.public 1002.public 1003.public ${GNB_DIR}/conf/1003/ed25519/


mv 1001.public 1001.private ${GNB_DIR}/conf/1001/security/
mv 1002.public 1002.private ${GNB_DIR}/conf/1002/security/
mv 1003.public 1003.private ${GNB_DIR}/conf/1003/security/

