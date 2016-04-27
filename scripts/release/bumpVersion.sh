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
this=$(cd "$this"; pwd)
ROOT=$(cd "${this}/../.."; pwd)
SOURCE="${ROOT}/source"

check-version () {
  local version=$1
  [[ -z ${version} ]] && return 1
  (echo ${version} | grep -q -s '^[0-9]\+\.[0-9]\+\.[0-9]\+$') &&\
    return 0 ||\
    return 1
}

bump-version () {
  local version=$1
  local PACKAGE_JSONS=$(find ${SOURCE} -maxdepth 2 -type f -name "package.json" | grep -v "client_sdk")
  PACKAGE_JSONS="${PACKAGE_JSONS} ${this}/package.mcu.json"
  for PACKAGE_JSON in ${PACKAGE_JSONS}; do
    sed -i "s/\"version\".*,$/\"version\":\"${version}\",/g" ${PACKAGE_JSON}
  done
}

check-version $1 && bump-version $1 || echo "Usage: $0 VERSION"