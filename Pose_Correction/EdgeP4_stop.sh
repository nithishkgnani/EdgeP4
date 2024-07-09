#!/usr/bin/env bash
sudo /opt/netronome/bin/nfp-nffw unload
echo -e "\nnffw unloaded"
sudo /opt/netronome/p4/bin/rtecli design-unload
echo -e "\nrtecli unloaded"
sudo systemctl stop nfp-sdk6-rte.service
echo -e "\nnfp sdk service stopped"
