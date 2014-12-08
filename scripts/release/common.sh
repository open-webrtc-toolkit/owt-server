#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

usage() {
  echo
  echo "WooGeen Packaging Script"
  echo "    This script creates a WooGeen-MCU/Gateway package to dist/"
  echo "    You should use scripts/build.sh to build WooGeen-MCU/Gateway first."
  echo
  echo "Usage:"
  echo "    --mcu                               set package target to WooGeen-MCU"
  echo "    --gateway                           set package target to WooGeen-Gateway"
  echo "    --archive                           archive & compress to Release-<version>.tgz"
  echo "    --encrypt                           minify, compress and encrypt js files for release"
  echo "    --no-node                           put js in package as is"
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
  local VER="trunk"
  if hash git 2>/dev/null; then
    echo "using git revision as version number."
    VER=$(cd ${ROOT} && git show-ref --head | head -1 | fold -w 8 | head -1)
  fi
  cd ${ROOT} && tar --numeric-owner -czf "Release-${VER}.tgz" ${WOOGEEN_DIST}
  echo -e "\x1b[32mRelease-${VER}.tgz generated.\x1b[0m"
}
