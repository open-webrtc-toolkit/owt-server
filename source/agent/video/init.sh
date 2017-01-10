#!/usr/bin/env bash
#
# Copyright 2016 Intel Corporation All Rights Reserved.
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

usage() {
  echo
  echo "WooGeen Initialization Script"
  echo "    This script initializes WooGeen-MCU Video Agent."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --hardware (default: false)         enable hardware video codec (only for video agent with \`videoMixer-hw-*.node')"
  echo "    --yami (default: false)             enable yami backend (only for video agent with \'videoMixer-hw-yami.node')"
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
ENABLE_YAMI=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)hardware )
      ENABLE_HARDWARE=true
      ;;
    *(-)yami )
      ENABLE_YAMI=true
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

if ${ENABLE_YAMI}; then
  install_config
  echo -e "\x1b[36mYami backend enabled\x1b[0m"
  sed -i 's/^yamiEnabled = false/yamiEnabled = true/' ${this}/agent.toml
else
  echo -e "\x1b[36mYami backend disbled\x1b[0m"
  sed -i 's/^yamiEnabled = true/yamiEnabled = false/' ${this}/agent.toml
fi

if [[ "${ENABLE_HARDWARE}" = "false" && "${ENABLE_YAMI}" = "false" ]]; then
  # Install if no input for 15s
  read -t 15 -p "Installing OpenH264 Video Codec Library provided by Cisco Systems, Inc? [Yes/no]" yn
  case $yn in
    [Yy]* ) . ${this}/install_openh264.sh;;
    [Nn]* ) ;;
    * ) echo && . ${this}/install_openh264.sh;;
  esac
fi
