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
NODE_VERSION=
. ${this}/.conf

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

install_deps() {
  echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
  sudo apt-get update
  sudo apt-get install rabbitmq-server mongodb #TODO: pick-up libraries
}

install_db() {
  local DB_DIR=${WOOGEEN_HOME}/db

  echo -e "\x1b[32mInitializing mongodb...\x1b[0m"
  if ! pgrep -f mongod &> /dev/null; then
    if ! hash mongod 2>/dev/null; then
        echo >&2 "Error: mongodb not found."
        return 1
    fi
    [[ ! -d "${DB_DIR}" ]] && mkdir -p "${DB_DIR}"
    [[ ! -d "${LogDir}" ]] && mkdir -p "${LogDir}"
    mongod --repair --dbpath ${DB_DIR}
    mongod --dbpath ${DB_DIR} --logpath ${LogDir}/mongo.log --fork
    sleep 5
  else
    echo -e "\x1b[32mMongoDB already running\x1b[0m"
  fi
}

install_config() {
  echo -e "\x1b[32mInitializing default configuration...\x1b[0m"
  # default configuration
  local DEFAULT_CONFIG="${WOOGEEN_HOME}/etc/.woogeen_default.js"
  if [[ ! -s ${DEFAULT_CONFIG} ]]; then
    echo >&2 "Error: configuration template not found."
    return 1
  fi
  local dbURL=`grep "config.nuve.dataBaseURL" ${DEFAULT_CONFIG}`
  dbURL=`echo ${dbURL} | cut -d'"' -f 2`
  dbURL=`echo ${dbURL} | cut -d'"' -f 1`

  echo Creating superservice in ${dbURL}
  local SERVID=`mongo ${dbURL} --quiet --eval "db.services.findOne({\"name\":\"superService\"})._id" | cut -d '"' -f 2`
  if [[ ${SERVID} == "superService" ]]; then
    mongo ${dbURL} --eval "db.services.insert({name: 'superService', key: '$RANDOM', rooms: []})"
    SERVID=`mongo ${dbURL} --quiet --eval "db.services.findOne({\"name\":\"superService\"})._id" | cut -d '"' -f 2`
  fi
  SERVID=`echo ${SERVID}| cut -d'"' -f 1`
  local SERVKEY=`mongo ${dbURL} --quiet --eval "db.services.findOne({\"name\":\"superService\"}).key"`
  [[ -f ${LogDir}/mongo.log ]] && cat ${LogDir}/mongo.log
  echo "SuperService ID: ${SERVID}"
  echo "SuperService KEY: ${SERVKEY}"
  local replacement=s/_auto_generated_ID_/${SERVID}/
  local TMPFILE=$(mktemp)
  sed $replacement ${DEFAULT_CONFIG} > ${TMPFILE}
  replacement=s/_auto_generated_KEY_/${SERVKEY}/
  sed $replacement ${TMPFILE} > ${WOOGEEN_HOME}/etc/woogeen_config.js
  rm -f ${TMPFILE}
}

INSTALL_DEPS=false
ENABLE_HARDWARE=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)deps )
      INSTALL_DEPS=true
      ;;
    *(-)hardware )
      ENABLE_HARDWARE=true
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

${INSTALL_DEPS} && install_deps

install_db
install_config

if ${ENABLE_HARDWARE}; then
  cd ${ROOT}/lib
  [[ -s libmcu_hw.so ]] && \
  rm -f libmcu.so && \
  ln -s libmcu_hw.so libmcu.so
  sed -i 's/config\.erizo\.hardwareAccelerated = false/config\.erizo\.hardwareAccelerated = true/' ${ROOT}/etc/woogeen_config.js
else
  cd ${ROOT}/lib
  [[ -s libmcu_sw.so ]] && \
  rm -f libmcu.so && \
  ln -s libmcu_sw.so libmcu.so
  sed -i 's/config\.erizo\.hardwareAccelerated = true/config\.erizo\.hardwareAccelerated = false/' ${ROOT}/etc/woogeen_config.js
fi
