#!/usr/bin/env bash
sudo modprobe -r -v nfp && sudo modprobe nfp nfp_pf_netdev=0 nfp_dev_cpp=1
sudo /opt/netronome/p4/bin/nfp4build -o basic.nffw -p out -4 basic.p4 -c denoise.c -l lithium --nfp4c_p4_version 16 --nfp4c_I /opt/netronome/p4/include/16/p4include/
sudo systemctl start nfp-sdk6-rte.service
sudo systemctl status nfp-sdk6-rte.service
