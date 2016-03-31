#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/.."; pwd`
export WOOGEEN_HOME=${ROOT}

usage() {
  echo
  echo "WooGeen Initialization Script"
  echo "    This script initializes a WooGeen-MCU package."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --deps (default: false)             install dependent components and libraries via apt-get/local"
  echo "    --dburl=HOST/DBNAME                 specify mongodb URL other than default \`localhost/nuvedb'"
  echo "    --hardware                          enable mcu with msdk (if \`libmcu_hw.so' is packed)"
  echo "    --help                              print this help"
  echo
}

[[ -s ${WOOGEEN_HOME}/nuve/init.sh ]] && ${WOOGEEN_HOME}/nuve/init.sh $@
[[ -s ${WOOGEEN_HOME}/video_agent/init.sh ]] && ${WOOGEEN_HOME}/video_agent/init.sh $@
[[ -s ${WOOGEEN_HOME}/audio_agent/init.sh ]] && ${WOOGEEN_HOME}/audio_agent/init.sh $@
