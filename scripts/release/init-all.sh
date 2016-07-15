#!/usr/bin/env bash

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`
ROOT=`cd "${bin}/.."; pwd`



usage()
{
  echo "Usage: [--deps] [--hardware] (Default: without deps and hardware)"
}

init_software()
{
  if ${INSTALL_DEPS}; then
    echo "Installing dependency..."
    ${ROOT}/nuve/init.sh --deps
    ${ROOT}/access_agent/install_deps.sh
    ${ROOT}/video_agent/install_deps.sh
    ${ROOT}/video_agent/init.sh --openh264
  else
    ${ROOT}/nuve/init.sh
    ${ROOT}/video_agent/init.sh --openh264
  fi
}

init_hardware()
{
  if ${INSTALL_DEPS}; then
    echo "Installing dependency..."
    ${ROOT}/nuve/init.sh --deps
    ${ROOT}/access_agent/install_deps.sh
    ${ROOT}/video_agent/install_deps.sh --hardware
    ${ROOT}/video_agent/init.sh --hardware
  else
    ${ROOT}/nuve/init.sh
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
  echo "Initializing with hardware..."
  init_hardware
else
  echo "Initializing..."
  init_software
fi
