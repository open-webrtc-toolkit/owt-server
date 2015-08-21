#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
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
      find "$i" -path "${i}/public" -prune -o -type f -name "*.js" -print | while read line; do
        encrypt_js "$line"
      done
    done
    for file in ${WOOGEEN_DIST}/lib/* ; do strip -s "$file" ; done
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
