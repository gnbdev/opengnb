#!/bin/sh

#dig -6 TXT +short o-o.myaddr.l.google.com @ns1.google.com | awk -F'"' '{ print $2}' > /tmp/wan6_addr.dump

wget http://api.myip.la -q -6 -O /tmp/wan6_addr.dump


