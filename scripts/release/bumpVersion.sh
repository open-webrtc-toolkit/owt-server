#!/usr/bin/env bash

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