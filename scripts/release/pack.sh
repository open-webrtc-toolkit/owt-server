#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/../.."; pwd`
SOURCE="${ROOT}/source"
UV_DIR="${ROOT}/third_party/libuv-0.10.26"

export WOOGEEN_DIST="${ROOT}/dist"
PACK_NODE=true
PACK_ARCH=false
PACK_MODULE=true
ENCRYPT=false

cd ${this}
. common.sh
shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)mcu )
      . mcu_funcs.sh #set target to mcu
      ;;
    *(-)gateway )
      . gw_funcs.sh #set target to gateway
      ;;
    *(-)no-node )
      PACK_NODE=false
      ;;
    *(-)no-module )
      PACK_MODULE=false
      ;;
    *(-)archive )
      PACK_ARCH=true
      ;;
    *(-)encrypt )
      ENCRYPT=true
      ;;
    *(-)help )
      usage
      exit 0
      ;;
    * )
      echo -e "\x1b[33mUnknown argument\x1b[0m: $1"
      ;;
  esac
  shift
done

echo "Cleaning ${WOOGEEN_DIST}/ ..."; rm -fr ${WOOGEEN_DIST}/
pack_runtime
pack_libs
pack_scripts
${PACK_MODULE} && install_module
${PACK_NODE} && pack_node
${PACK_ARCH} && archive
