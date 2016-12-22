#!/usr/bin/env bash

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
