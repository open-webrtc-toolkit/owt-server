#!/usr/bin/env bash
sudo apt-get update
sudo apt-get install -y git
cd /vagrant
git clone ssh://git-amr-1.devtools.intel.com:29418/webrtc-woogeen-2.git
cd webrtc-woogeen-2
./scripts/installDepsUnattended.sh --cleanup
./scripts/build.sh --all
./scripts/release/pack.sh --mcu
./dist/bin/init.sh
echo "config.erizoController.publicIP = '$1';" >> ./dist/etc/woogeen_config.js
echo "config.erizo.minport = 30000;" >> ./dist/etc/woogeen_config.js
echo "config.erizo.maxport = 31000;" >> ./dist/etc/woogeen_config.js
