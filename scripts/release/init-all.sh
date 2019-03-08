#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`
ROOT=`cd "${bin}/.."; pwd`

OWT_UPDATE_DONE=false

usage()
{
  echo "Usage: [--deps] [--hardware] (Default: without deps and hardware)"
}

init_software()
{
  if ${INSTALL_DEPS}; then
    echo "Installing dependency..."
    ${ROOT}/bin/install_node.sh
    ${ROOT}/bin/init-mongodb.sh --deps
    ${ROOT}/bin/init-rabbitmq.sh --deps
    OWT_UPDATE_DONE=true
    ${ROOT}/management_api/init.sh
    ${ROOT}/webrtc_agent/install_deps.sh
    ${ROOT}/video_agent/install_deps.sh
    ${ROOT}/video_agent/init.sh
  else
    ${ROOT}/bin/init-mongodb.sh
    ${ROOT}/bin/init-rabbitmq.sh
    OWT_UPDATE_DONE=true
    ${ROOT}/management_api/init.sh
    ${ROOT}/video_agent/init.sh
  fi
}

init_hardware()
{
  if ${INSTALL_DEPS}; then
    echo "Installing dependency..."
    ${ROOT}/bin/install_node.sh
    ${ROOT}/bin/init-mongodb.sh --deps
    ${ROOT}/bin/init-rabbitmq.sh --deps
    OWT_UPDATE_DONE=true
    ${ROOT}/management_api/init.sh
    ${ROOT}/webrtc_agent/install_deps.sh
    ${ROOT}/video_agent/install_deps.sh --hardware
    ${ROOT}/video_agent/init.sh --hardware
  else
    ${ROOT}/bin/init-mongodb.sh
    ${ROOT}/bin/init-rabbitmq.sh
    OWT_UPDATE_DONE=true
    ${ROOT}/management_api/init.sh
    ${ROOT}/video_agent/init.sh --hardware
  fi
}

INSTALL_DEPS=false
HARDWARE=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)hardware )
      HARDWARE=true
      ;;
    *(-)deps )
      INSTALL_DEPS=true
      ;;
    *(-)help )
      usage
      exit 0
      ;;
  esac
  shift
done

if ${HARDWARE}; then
  echo "Initializing with hardware msdk"
  init_hardware
else
  echo "Initializing..."
  init_software
fi
