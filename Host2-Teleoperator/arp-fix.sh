#!/bin/bash
# Author - Nithish
# Date - 2023.04.28
# Desciption - Netronome has some issues with handling arp packets.
# So, we set ARP cache timeout to zero and set the destination mac address here.

sudo sysctl net.ipv4.neigh.enp1s0f1.gc_stale_time=0
sudo arp -s 10.0.0.2 xx:yy:zz:aa:bb:cc
# replace xx:yy:zz:aa:bb:cc with the mac address of the destination host
