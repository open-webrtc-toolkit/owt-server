#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

this=$(dirname "$0")
this=$(cd "${this}"; pwd)

usage() {
  echo
  echo "OWT Initialization Script"
  echo "    This script initializes OWT-MCU Video Agent."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --hardware (default: false)         enable hardware video codec (only for video agent with \`videoMixer-hw-*.node')"
  echo "    --help                              print this help"
  echo
}

install_config() {
  # install mediaproess config
  [[ -s ${this}/.msdk_log_config.ini ]] || return
  echo -e "\x1b[32mInitializing Intel Media Server Studio configuration for video agent...\x1b[0m"
  [[ -s /tmp/msdk_log_config.ini ]] && rm -f /tmp/msdk_log_config.ini
  ln -sf ${this}/.msdk_log_config.ini /tmp/msdk_log_config.ini
}

ENABLE_HARDWARE=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)hardware )
      ENABLE_HARDWARE=true
      ;;
    *(-)help )
      usage
      exit 0
      ;;
  esac
  shift
done

if ${ENABLE_HARDWARE}; then
  install_config
  echo -e "\x1b[36mHardware acceleration enabled\x1b[0m"
  sed -i 's/^hardwareAccelerated = false/hardwareAccelerated = true/' ${this}/agent.toml
else
  echo -e "\x1b[36mHardware acceleration disbled\x1b[0m"
  sed -i 's/^hardwareAccelerated = true/hardwareAccelerated = false/' ${this}/agent.toml
fi

