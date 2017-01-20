#!/usr/bin/env bash

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

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`
ROOT=`cd "${bin}/.."; pwd`



usage()
{
  echo "Usage: [--deps] [--hardware] [--yami] (Default: without deps, yami and hardware)"
}

init_software()
{
  if ${INSTALL_DEPS}; then
    echo "Installing dependency..."
    ${ROOT}/bin/init-mongodb.sh --deps
    ${ROOT}/bin/init-rabbitmq.sh --deps
    ${ROOT}/nuve/init.sh
    ${ROOT}/access_agent/install_deps.sh
    ${ROOT}/video_agent/install_deps.sh
    ${ROOT}/video_agent/init.sh
  else
    ${ROOT}/bin/init-mongodb.sh
    ${ROOT}/bin/init-rabbitmq.sh
    ${ROOT}/nuve/init.sh
    ${ROOT}/video_agent/init.sh
  fi
}

init_hardware()
{
  if ${INSTALL_DEPS}; then
    echo "Installing dependency..."
    ${ROOT}/bin/init-mongodb.sh --deps
    ${ROOT}/bin/init-rabbitmq.sh --deps
    ${ROOT}/nuve/init.sh
    ${ROOT}/access_agent/install_deps.sh
    ${ROOT}/video_agent/install_deps.sh --hardware
    ${ROOT}/video_agent/init.sh --hardware
  else
    ${ROOT}/bin/init-mongodb.sh
    ${ROOT}/bin/init-rabbitmq.sh
    ${ROOT}/nuve/init.sh
    ${ROOT}/video_agent/init.sh --hardware
  fi
}

init_hardware_yami()
{
  if ${INSTALL_DEPS}; then
    echo "Installing dependency..."
    ${ROOT}/bin/init-mongodb.sh --deps
    ${ROOT}/bin/init-rabbitmq.sh --deps
    ${ROOT}/nuve/init.sh
    ${ROOT}/access_agent/install_deps.sh
    ${ROOT}/video_agent/install_deps.sh --hardware
    ${ROOT}/video_agent/init.sh --hardware
  else
    ${ROOT}/bin/init-mongodb.sh
    ${ROOT}/bin/init-rabbitmq.sh
    ${ROOT}/nuve/init.sh
    ${ROOT}/video_agent/init.sh --hardware --yami
  fi
}

INSTALL_DEPS=false
HARDWARE=false
YAMI=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)hardware )
      HARDWARE=true
      ;;
    *(-)deps )
      INSTALL_DEPS=true
      ;;
    *(-)yami )
      YAMI=true
      ;;
    *(-)help )
      usage
      exit 0
      ;;
  esac
  shift
done

if ${HARDWARE}; then
  if ${YAMI}; then
    echo "Initializing with hardware yami"
    init_hardware_yami
  else
    echo "Initializing with hardware msdk"
    init_hardware
  fi
else
  echo "Initializing..."
  init_software
fi
