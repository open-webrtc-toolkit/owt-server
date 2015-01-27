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

LogDir=${WOOGEEN_HOME}/logs

usage() {
  echo
  echo "WooGeen Initialization Script"
  echo "    This script initializes a WooGeen-MCU package."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --deps (default: false)             install dependent components and libraries via apt-get/local"
  echo "    --hardware                          enable mcu with msdk (if \`libmcu_hw.so' is packed)"
  echo "    --help                              print this help"
  echo
}


install_config() {
  echo -e "\x1b[32mInitializing default configuration...\x1b[0m"
  # default configuration
  local DEFAULT_CONFIG="${WOOGEEN_HOME}/etc/.woogeen_default.js"
  if [[ ! -s ${DEFAULT_CONFIG} ]]; then
    echo >&2 "Error: configuration template not found."
    return 1
  fi
  local dbURL=$(grep "config.nuve.dataBaseURL" ${DEFAULT_CONFIG})
  dbURL=$(echo ${dbURL} | cut -d "'" -f 2)
  local SERVICE=$(mongo ${dbURL} --quiet --eval 'db.services.findOne({"name":"superService"})')
  if [[ ${SERVICE} == "null" ]]; then
    echo -e "\x1b[36mCreating superservice in ${dbURL}\x1b[0m"
    mongo ${dbURL} --eval "db.services.insert({name: 'superService', key: '$RANDOM-$RANDOM-$RANDOM-$RANDOM', rooms: []})"
  fi
  local SERVID=$(mongo ${dbURL} --quiet --eval 'db.services.findOne({"name":"superService"})._id')
  local SERVKEY=$(mongo ${dbURL} --quiet --eval 'db.services.findOne({"name":"superService"}).key')
  [[ -f ${LogDir}/mongo.log ]] && cat ${LogDir}/mongo.log
  echo "SuperService ID: ${SERVID}"
  echo "SuperService KEY: ${SERVKEY}"
  sed -i s/_auto_generated_ID_/${SERVID}/ ${WOOGEEN_HOME}/extras/basic_example/basicServer.js
  sed -i s/_auto_generated_KEY_/${SERVKEY}/ ${WOOGEEN_HOME}/extras/basic_example/basicServer.js
  sed -i s/_auto_generated_ID_/${SERVID}/ ${WOOGEEN_HOME}/../test/nuve-api-spec.js
  sed -i s/_auto_generated_KEY_/${SERVKEY}/ ${WOOGEEN_HOME}/../test/nuve-api-spec.js
}




install_config

