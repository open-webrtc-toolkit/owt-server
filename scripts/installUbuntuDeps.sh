#!/bin/bash

install_apt_deps(){
  ${SUDO} apt-get update -y
  ${SUDO} apt-get install git make gcc g++ libglib2.0-dev pkg-config libboost-regex-dev libboost-thread-dev libboost-system-dev liblog4cxx-dev rabbitmq-server mongodb openjdk-8-jre curl libboost-test-dev nasm yasm gyp libx11-dev libkrb5-dev intel-gpu-tools m4 autoconf libtool automake cmake libfreetype6-dev libgstreamer-plugins-base1.0-dev -y
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
