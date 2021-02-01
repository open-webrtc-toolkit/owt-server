#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

this=$(dirname "$0")
this=$(cd "${this}"; pwd)
SUDO=""
if [[ $EUID -ne 0 ]]; then
   SUDO="sudo -E"
fi

usage() {
  echo
  echo "Video Agent Install Dependency Script"
  echo "Usage:"
  echo "    --software (default)                install software dependencies"
  echo "    --hardware                          install hardware dependencies"
  echo
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

install_deps() {
  local OS=$(${this}/detectOS.sh | awk '{print tolower($0)}')
  echo $OS

  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via yum...\x1b[0m"
    ${OWT_UPDATE_DONE} || ${SUDO} yum update
    ${SUDO} yum install wget -y
    if [[ "$OS" =~ .*6.* ]] # CentOS 6.x
    then
      wget -c http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
      ${SUDO} rpm -Uvh epel-release-6*.rpm
    elif [[ "$OS" =~ .*7.* ]] # CentOS 7.x
    then
      wget -c http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
      ${SUDO} rpm -Uvh epel-release-latest-7*.rpm
    fi
    ${SUDO} sed -i 's/https/http/g' /etc/yum.repos.d/epel.repo
    ${SUDO} yum install log4cxx bzip2 -y
    ${SUDO} yum install intel-gpu-tools -y
    install_boost

  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    ${OWT_UPDATE_DONE} || ${SUDO} apt-get update -y
    ${SUDO} apt-get install libboost-system-dev libboost-thread-dev liblog4cxx-dev wget bzip2 -y
    ${SUDO} apt-get install intel-gpu-tools -y
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

HARDWARE_DEPS=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)software )
      HARDWARE_DEPS=false
      ;;
    *(-)hardware )
      HARDWARE_DEPS=true
      ;;
    *(-)help )
      usage
      exit 0
      ;;
    * )
      echo -e "\x1b[33mUnknown argument\x1b[0m: $1"
      ;;
  esac
  shift
done

install_deps
${this}/install_ffmpeg.sh

if ${HARDWARE_DEPS} ; then
  :
else
  # Install if no input for 15s
  read -t 15 -p "Installing OpenH264 Video Codec Library provided by Cisco Systems, Inc? [Yes/no]" yn
  case $yn in
    [Yy]* ) . ${this}/install_openh264.sh;;
    [Nn]* ) ;;
    * ) echo && . ${this}/install_openh264.sh;;
  esac
fi
