#!/bin/bash

install_apt_deps(){
  sudo -E apt-get install python-software-properties
  sudo -E apt-get install software-properties-common
  sudo -E add-apt-repository ppa:chris-lea/node.js
  sudo -E apt-get update
  sudo -E apt-get install git make gcc g++ libssl-dev cmake libglib2.0-dev pkg-config nodejs libboost-regex-dev libboost-thread-dev libboost-system-dev liblog4cxx10-dev rabbitmq-server mongodb openjdk-6-jre curl libboost-test-dev nasm
}

install_mediadeps(){
  sudo -E apt-get install yasm libx264.
  install_libvpx
  install_libav
}

install_mediadeps_nogpl(){
  sudo -E apt-get install yasm
  install_libvpx
  install_libav
}

cleanup(){
  cleanup_common
}
