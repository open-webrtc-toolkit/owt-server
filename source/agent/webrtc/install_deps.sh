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
  echo "    This script install GLib2.0, "
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

install_boost() {
  echo -e "\x1b[32mInstalling boost...\x1b[0m"
  pushd ${this} >/dev/null
  wget -c http://iweb.dl.sourceforge.net/project/boost/boost/1.65.0/boost_1_65_0.tar.bz2
  tar xvf boost_1_65_0.tar.bz2
  pushd boost_1_65_0 >/dev/null
  ./bootstrap.sh
  ./b2 --with-thread --with-system
  cp stage/lib/libboost_*.so* ${this}/lib
  popd
  popd
}

install_libs() {
  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mInstalling deps via yum install...\x1b[0m"
    ${SUDO} yum install log4cxx glib2 -y
    install_boost
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling deps via apt-get install...\x1b[0m"
    ${SUDO} apt-get install libboost-system-dev libboost-thread-dev liblog4cxx-dev libglib2.0-0 -y
  fi
}

install_all() {
  ${OWT_UPDATE_DONE} || do_update
  install_libs
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