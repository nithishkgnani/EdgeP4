#!/bin/bash
# Author - Nithish
# Date - 2023.04.28
# Desciption - Setting the ip address to enp1s0f1
sudo ip addr flush dev enp1s0f1 # Flushing away existing IP address
sudo ip addr add 10.0.0.2/24 dev enp1s0f1
