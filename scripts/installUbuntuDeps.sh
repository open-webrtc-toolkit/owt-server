#!/bin/bash

install_apt_deps(){
  sudo -E apt-get update
  sudo -E apt-get install git make gcc g++ libssl-dev cmake libglib2.0-dev pkg-config libboost-regex-dev libboost-thread-dev libboost-system-dev liblog4cxx10-dev rabbitmq-server mongodb openjdk-6-jre curl libboost-test-dev nasm yasm gyp libx11-dev libkrb5-dev nload
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
