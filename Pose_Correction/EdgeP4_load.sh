#!/bin/bash

echo -ne "Unloading design"
/opt/netronome/p4/bin/rtecli design-unload
echo -ne " - DONE"

echo -ne "\nUnloading nffw"
/opt/netronome/bin/nfp-nffw unload
echo -ne " - DONE"


echo -ne "\nLoading design"
/opt/netronome/p4/bin/rtecli design-load -f basic.nffw -p out/pif_design.json
echo -ne " - DONE"

echo -ne "\nLoading configuration file"
/opt/netronome/p4/bin/rtecli config-reload -c user_config.json
echo -ne " - DONE\n"
