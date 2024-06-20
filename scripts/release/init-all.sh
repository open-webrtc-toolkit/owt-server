#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`
ROOT=`cd "${bin}/.."; pwd`

SUDO=""
if [[ $EUID -ne 0 ]]; then
   SUDO="sudo -E"
fi

OWT_UPDATE_DONE=false
HAVE_AUTH_UPDATE=false

usage()
{
  echo "Usage: [--deps] [--hardware] (Default: without deps and hardware)"
}

copy_video_libs()
{
  cp ${ROOT}/video_agent/lib/* ${ROOT}/audio_agent/lib/
  cp ${ROOT}/video_agent/lib/* ${ROOT}/recording_agent/lib/
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
    ${ROOT}/streaming_agent/install_deps.sh
    ${ROOT}/analytics_agent/install_deps.sh
    copy_video_libs
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
    copy_video_libs
  else
    ${ROOT}/bin/init-mongodb.sh
    ${ROOT}/bin/init-rabbitmq.sh
    OWT_UPDATE_DONE=true
    ${ROOT}/management_api/init.sh
    ${ROOT}/video_agent/init.sh --hardware
  fi

  ${SUDO} sh -c "echo 0 >> /proc/sys/kernel/perf_event_paranoid"
}

init_auth()
{
  read -t 10 -p "Update RabbitMQ/MongoDB Account? [No/yes]" yn
  if [[ $? -eq 0 ]]; then
    case $yn in
      [Nn]* ) ;;
      [Yy]* ) HAVE_AUTH_UPDATE=true;;
      * ) ;;
    esac
  fi

  local AUTH_SCRIPT="${ROOT}/cluster_manager/initauth.js"
  local CIPHER_JS="${ROOT}/cluster_manager/cipher.js"
  local AUTH_STORE=""
  if [[ "${HAVE_AUTH_UPDATE}" == "true" ]]; then
    ${AUTH_SCRIPT}
    AUTH_STORE=`node -e "console.log(require('${CIPHER_JS}').astore)"`
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/portal/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/conference_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/webrtc_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/video_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/audio_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/streaming_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/recording_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/analytics_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/sip_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/sip_portal/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/management_api/
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
  init_auth
else
  echo "Initializing..."
  init_software
  init_auth
fi
