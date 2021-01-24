#!/bin/bash

. /home/chri/python-virtual-environments/rpi/bin/activate

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

exec $DIR/homergw.py \
     --disphat-values='T3_52pi:Troom,T_3d:T3d,P%_8.8.8.8:Loss_%,PL_8.8.8.8:Lat_ms,RXB_enp1s0u1:Rx_MB/s,TXB_enp1s0u1:Tx_MB/s' \
     --disphat-alert='V["P%_8.8.8.8"]>20' \
     --data-store-db=/mnt/d/home.db \
     --getmqtt-topics='zigbee2mqtt/temphum_3d:temperature:T_3d,zigbee2mqtt/temphum_3d:humidity:HU_3d,zigbee2mqtt/temphum_cucina:temperature:T_cu,zigbee2mqtt/temphum_cucina:humidity:HU_cu,zigbee2mqtt/temphum_camera:temperature:T_ca,zigbee2mqtt/temphum_camera:humidity:HU_ca' \
     --bwe-iface=enp1s0u1 \
     --pinger-c=60 \
     --pinger-w=61
