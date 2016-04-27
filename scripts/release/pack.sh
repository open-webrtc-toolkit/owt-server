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

this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/../.."; pwd`
SOURCE="${ROOT}/source"

export WOOGEEN_DIST="${ROOT}/dist"
PACK_ARCH=false
PACK_MODULE=true
ENCRYPT=false
ENCRYPT_CAND_PATH=
PACKAGE_VERSION=
SRC_SAMPLE_PATH=
PREVIOUS_DIR=$(pwd)
OS=$(${ROOT}/scripts/detectOS.sh | awk '{print tolower($0)}')

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
    *(-)no-module )
      PACK_MODULE=false
      ;;
    *(-)archive )
      PACK_ARCH=true
      ;;
    *(-)encrypt )
      ENCRYPT=true
      ;;
    *(-)src-sample-path=* )
      SRC_SAMPLE_PATH=${1##*(-)}
      SRC_SAMPLE_PATH=${SRC_SAMPLE_PATH:16}
      [[ ${SRC_SAMPLE_PATH:0:1} == "/" ]] || SRC_SAMPLE_PATH="${PREVIOUS_DIR}/${SRC_SAMPLE_PATH}"
      ;;
    *(-)help )
      usage
      exit 0
      ;;
    v[0-9]*)
      PACKAGE_VERSION="$1"
      ;;
    * )
      echo -e "\x1b[33mUnknown argument\x1b[0m: $1"
      ;;
  esac
  shift
done

if ! hash pack_runtime 2>/dev/null; then
  usage
  exit 0
fi
echo "Cleaning ${WOOGEEN_DIST}/ ..."; rm -fr ${WOOGEEN_DIST}/
SRC_SAMPLE_PATH="${SRC_SAMPLE_PATH}" pack_runtime
pack_scripts
${PACK_MODULE} && install_module
pack_node
type pack_libs >/dev/null 2>&1 && pack_libs
type pack_license >/dev/null 2>&1 && pack_license
${PACK_ARCH} && archive
