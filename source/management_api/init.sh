#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/.."; pwd`

DB_URL='localhost/owtdb'

usage() {
  echo
  echo "OWT Initialization Script"
  echo "    This script initializes OWT-MCU ManagementAPIServer."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --dburl=HOST/DBNAME                 specify mongodb URL other than default \`localhost/owtdb'"
  echo "    --help                              print this help"
  echo
}

check_node_version() {
  if ! hash node 2>/dev/null; then
    echo >&2 "Error: node not found. Please install node ${NODE_VERSION} first."
    return 1
  fi
  local NODE_VERSION=v$(node -e "process.stdout.write(require('${this}/package.json').engine.node)")
  NODE_VERSION=$(echo ${NODE_VERSION} | cut -d '.' -f 1)
  local NODE_VERSION_USE=$(node --version | cut -d '.' -f 1)
  [[ ${NODE_VERSION} == ${NODE_VERSION_USE} ]] && return 0 || (echo "node version not match. Please use node ${NODE_VERSION}"; return 1;)
}

install_config() {
  echo -e "\x1b[32mInitializing ManagementAPIServer configuration...\x1b[0m"
  export DB_URL
  [[ -s ${this}/initdb.js ]] && node ${this}/initdb.js && return 0
  [[ -s ${this}/initdb ]] && ${this}/initdb || return 1
}

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)dburl=* )
      DB_URL=$(echo $1 | cut -d '=' -f 2)
      echo -e "\x1b[36musing $DB_URL\x1b[0m"
      ;;
    *(-)help )
      usage
      exit 0
      ;;
  esac
  shift
done

install_config
