#!/usr/bin/env bash
sudo apt-get update
sudo apt-get install -y git
cd /vagrant
git clone https://github.com/ging/licode.git
cd licode
git checkout async_events 
cd ..
./licode/scripts/installUbuntuDepsUnattended.sh --cleanup
./licode/scripts/installErizo.sh
./licode/scripts/installNuve.sh
./licode/scripts/installBasicExample.sh
echo "config.erizoController.publicIP = '$1';" >> ./licode/woogeen_config.js
echo "config.erizo.minport = 30000;" >> ./licode/woogeen_config.js
echo "config.erizo.maxport = 31000;" >> ./licode/woogeen_config.js
