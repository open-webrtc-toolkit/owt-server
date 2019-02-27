#!/usr/bin/env bash
#
# Copyright 2019 Intel Corporation All Rights Reserved.
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

this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/.."; pwd`
SUDO=""
if [[ $EUID -ne 0 ]]; then
   SUDO="sudo -E"
fi

export WOOGEEN_HOME=${ROOT}

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
    ${SUDO} yum update
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mRun apt-get update...\x1b[0m"
    ${SUDO} apt-get update
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

install_glib() {
  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mInstalling GLib2.0 via yum install...\x1b[0m"
    ${SUDO} yum install glib2
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling GLib2.0 via apt-get install...\x1b[0m"
    ${SUDO} apt-get install libglib2.0-0
  fi
}

install_all() {
  do_update
  install_glib
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