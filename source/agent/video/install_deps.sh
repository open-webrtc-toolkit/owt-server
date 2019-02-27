#!/usr/bin/env bash
#
# Copyright 2017 Intel Corporation All Rights Reserved.
#
# The source code contained or described herein and all documents related to the
# source code ("Material") are owned by Intel Corporation or its suppliers or
# licensors. Title to the Material remains with Intel Corporation or its suppliers
# and licensors. The Material contains trade secrets and proprietary and
# confidential information of Intel or its suppliers and licensors. The Material
# is protected by worldwide copyright and trade secret laws and treaty provisions.
# No part of the Material may be used, copied, reproduced, modified, published,
# uploaded, posted, transmitted, distributed, or disclosed in any way without
# Intel's prior express written permission.
#  *
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery of
# the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be express
# and approved by Intel in writing.
#

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

enable_intel_gpu_top() {
  echo "Enable Intel GPU Top"
  # make intel-gpu-tools accessable by non-root users.
  ${SUDO} chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*
  # make the above change effect at every system startup.
  ${SUDO} chmod +x /etc/rc.local /etc/rc.d/rc.local
  if ${SUDO} grep -RInqs "chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*" /etc/rc.local; then
    echo "intel-gpu-tools has been authorised to non-root users."
  else
    ${SUDO} sh -c "echo \"chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*\" >> /etc/rc.local"
  fi
}

install_deps() {
  local OS=$(${this}/detectOS.sh | awk '{print tolower($0)}')
  echo $OS

  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via yum...\x1b[0m"
    ${SUDO} yum update
    ${SUDO} yum install wget
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
    ${SUDO} yum install bzip2 -y
    ${SUDO} yum install intel-gpu-tools -y

  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    ${SUDO} apt-get update
    ${SUDO} apt-get install wget bzip2
    ${SUDO} apt-get install intel-gpu-tools
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

if ${HARDWARE_DEPS} ; then
  enable_intel_gpu_top
else
  # Install if no input for 15s
  read -t 15 -p "Installing OpenH264 Video Codec Library provided by Cisco Systems, Inc? [Yes/no]" yn
  case $yn in
    [Yy]* ) . ${this}/install_openh264.sh;;
    [Nn]* ) ;;
    * ) echo && . ${this}/install_openh264.sh;;
  esac
fi