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

usage() {
  echo
  echo "WooGeen Packaging Script"
  echo "    This script creates a WooGeen-MCU/Gateway package to ${WOOGEEN_DIST}/"
  echo "    You should use scripts/build.sh to build WooGeen-MCU/Gateway first."
  echo
  echo "Usage:"
  echo "    --mcu                               set package target to WooGeen-MCU"
  echo "    --gateway                           set package target to WooGeen-Gateway"
  echo "    --archive                           archive & compress to Release-<version>.tgz"
  echo "    --encrypt                           minify, compress and encrypt js and binary files when archiving"
  echo "    --no-module                         exclude node_modules in package"
  echo "    --help                              print this help"
  echo
}

encrypt_js() {
  local target="$1"
  local tmp="$1.tmp"
  echo "Encrypt javascript file: ${target}"
  uglifyjs ${target} -o ${tmp} -c -m
  mv ${tmp} ${target}
}

archive() {
  # encryption if needed
  if ${ENCRYPT} ; then
    if ! hash uglifyjs 2>/dev/null; then
      if hash npm 2>/dev/null; then
        npm install -g --loglevel error uglify-js
        hash -r
      else
        echo >&2 "npm not found."
        echo >&2 "You need to install node first."
      fi
    fi
    for i in "${ENCRYPT_CAND_PATH[@]}"
    do
      find "$i" -path "${i}/public" -prune -o -path "${i}/node_modules" -prune -o -type f -name "*.js" -print | while read line; do
        encrypt_js "$line"
      done
    done
    for file in ${WOOGEEN_DIST}/*_agent/lib/* ; do strip -s "$file" ; done
  fi
  local VER="trunk"
  local DIR=$(dirname ${WOOGEEN_DIST})
  if [[ -z ${PACKAGE_VERSION} ]]; then
    if hash git 2>/dev/null; then
      echo "using git revision as version number."
      VER=$(cd ${ROOT} && git show-ref --head | head -1 | fold -w 8 | head -1)
    fi
  else
    VER="${PACKAGE_VERSION}"
  fi
  cd ${DIR} && mv $(basename ${WOOGEEN_DIST}) Release-${VER}; tar --numeric-owner -czf "Release-${VER}.tgz" Release-${VER}/; mv Release-${VER} $(basename ${WOOGEEN_DIST})
  echo -e "\x1b[32mRelease-${VER}.tgz generated in ${DIR}.\x1b[0m"
}
