#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/.."; pwd`
SUDO=""
if [[ $EUID -ne 0 ]]; then
   SUDO="sudo -E"
fi

usage() {
  echo
  echo "Runtime Dependency Install Script"
  echo "    This script install dependencies for this agent "
  echo "    This script is intended to run on a target machine."
  echo
}

OS=`${this}/detectOS.sh | awk '{print tolower($0)}'`
echo $OS

do_update() {
  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mRun yum update...\x1b[0m"
    ${SUDO} yum update -y
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mRun apt-get update...\x1b[0m"
    ${SUDO} apt-get update -y
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

install_deps() {
  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mInstalling deps via yum install...\x1b[0m"
    ${SUDO} yum install boost-system boost-thread log4cxx wget bzip2 x264 gstreamer1-plugins-ugly -y
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling deps via apt-get install...\x1b[0m"
    ${SUDO} apt-get install libboost-system-dev libboost-thread-dev liblog4cxx-dev wget bzip2 libx264-dev gstreamer1.0-plugins-ugly -y
  fi
}

install_all() {
  ${OWT_UPDATE_DONE} || do_update
  install_deps
}

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)help )
      usage
      exit 0
      ;;
  esac
  shift
done

install_all
