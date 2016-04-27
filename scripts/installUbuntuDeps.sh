#!/bin/bash

enable_intel_gpu_top() {
  # make intel-gpu-tools accessable by non-root users.
  sudo chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*
  # make the above change effect at every system startup.
  sudo chmod +x /etc/rc.local /etc/rc.d/rc.local
  if sudo grep -RInqs "chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*" /etc/rc.local; then
     echo "intel-gpu-tools has been authorised to non-root users."
  else
     sudo sh -c "echo \"chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*\" >> /etc/rc.local"
  fi
}

install_apt_deps(){
  sudo -E apt-get update
  sudo -E apt-get install git make gcc g++ libssl-dev libglib2.0-dev pkg-config libboost-regex-dev libboost-thread-dev libboost-system-dev liblog4cxx10-dev rabbitmq-server mongodb openjdk-6-jre curl libboost-test-dev nasm yasm gyp libx11-dev libkrb5-dev nload intel-gpu-tools
  enable_intel_gpu_top
}

install_mediadeps_nonfree(){
  install_fdkaac
  install_ffmpeg
}

install_mediadeps(){
  install_ffmpeg
}

cleanup(){
  cleanup_common
}
