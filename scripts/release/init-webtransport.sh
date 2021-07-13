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
    ${ROOT}/analytics_agent/install_deps.sh
  else
    OWT_UPDATE_DONE=true
    ${ROOT}/management_api/init.sh
  fi
}

init_auth()
{
  local AUTH_SCRIPT="${ROOT}/cluster_manager/initauth.js"
  local CIPHER_JS="${ROOT}/cluster_manager/cipher.js"
  local AUTH_STORE=""
  if [[ "${HAVE_AUTH_UPDATE}" == "true" ]]; then
    ${AUTH_SCRIPT}
    AUTH_STORE=`node -e "console.log(require('${CIPHER_JS}').astore)"`
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/portal/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/conference_agent/
    cp ${ROOT}/cluster_manager/${AUTH_STORE} ${ROOT}/management_api/
  fi
}

INSTALL_DEPS=false
HARDWARE=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
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

echo "Initializing..."
init_software
init_auth

bin=`cd "$bin"; pwd`
${bin}/daemon.sh start management-api
${bin}/daemon.sh start cluster-manager
${bin}/daemon.sh start conference-agent
${bin}/daemon.sh start quic-agent
${bin}/daemon.sh start portal
${bin}/daemon.sh start app