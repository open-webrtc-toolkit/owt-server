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
DB_URL='localhost/nuvedb'

usage() {
  echo
  echo "WooGeen Initialization Script"
  echo "    This script initializes a WooGeen-MCU package."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --deps (default: false)             install dependent components and libraries via apt-get/local"
  echo "    --dburl=HOST/DBNAME                 specify mongodb URL other than default \`localhost/nuvedb'"
  echo "    --hardware                          enable mcu with msdk (if \`libmcu_hw.so' is packed)"
  echo "    --help                              print this help"
  echo
}

enable_intel_gpu_top() {
  # make intel-gpu-tools accessable by non-root users.
  sudo chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*
  # make the above change effect at every system startup.
  sudo chmod +x /etc/rc.local /etc/rc.d/rc.local
  if sudo grep -RInqs "chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*" /etc/rc.local; then
     echo "intel-gpu-tools has been authorised to non-root users."
  else
     sudo sh -c "echo \"chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*\" >> /etc/rc.local"
  fi
}

install_deps() {
  local OS=`${this}/detectOS.sh | awk '{print tolower($0)}'`
  echo $OS

  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via yum...\x1b[0m"
    if [[ "$OS" =~ .*6.* ]] # CentOS 6.x
    then
      wget -c http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
      sudo rpm -Uvh epel-release-6*.rpm
    elif [[ "$OS" =~ .*7.* ]] # CentOS 7.x
    then
      wget -c http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
      sudo rpm -Uvh epel-release-latest-7*.rpm
    fi
    sudo sed -i 's/https/http/g' /etc/yum.repos.d/epel.repo
    sudo -E yum install mongodb mongodb-server rabbitmq-server nload intel-gpu-tools -y
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    sudo apt-get update
    sudo apt-get install rabbitmq-server mongodb nload intel-gpu-tools #TODO: pick-up libraries
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi

  # make the intel-gpu-tools accessable by non-root users.
  enable_intel_gpu_top
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

check_node_version()
{
  if ! hash node 2>/dev/null; then
    echo >&2 "Error: node not found. Please install node ${NODE_VERSION} first."
    return 1
  fi
  local NODE_VERSION=v$(node -e "process.stdout.write(require('${ROOT}/package.json').engine.node)")
  NODE_VERSION=$(echo ${NODE_VERSION} | cut -d '.' -f 1)
  local NODE_VERSION_USE=$(node --version | cut -d '.' -f 1)
  [[ ${NODE_VERSION} == ${NODE_VERSION_USE} ]] && return 0 || (echo "node version not match. Please use node ${NODE_VERSION}"; return 1;)
}

install_config() {
  echo -e "\x1b[32mInitializing default configuration...\x1b[0m"
  # install mediaproess config
  if [[ ! -s ${WOOGEEN_HOME}/video_agent/.msdk_log_config.ini ]]; then
    [[ -s /tmp/msdk_log_config.ini ]] && rm -f /tmp/msdk_log_config.ini
    ln -sf ${WOOGEEN_HOME}/video_agent/.msdk_log_config.ini /tmp/msdk_log_config.ini
  fi

  export DB_URL
  check_node_version && [[ -s ${ROOT}/nuve/initdb.js ]] && node ${ROOT}/nuve/initdb.js || return 1
}

INSTALL_DEPS=false
ENABLE_HARDWARE=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)deps )
      INSTALL_DEPS=true
      ;;
    *(-)dburl=* )
      DB_URL=$(echo $1 | cut -d '=' -f 2)
      echo -e "\x1b[36musing $DB_URL\x1b[0m"
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
  cd ${ROOT}/video_agent/lib
  [[ -s libmcu_hw.so ]] && \
  rm -f libmcu.so && \
  ln -s libmcu_hw.so libmcu.so
  sed -i 's/^hardwareAccelerated = false/hardwareAccelerated = true/' ${ROOT}/video_agent/agent.toml
else
  cd ${ROOT}/video_agent/lib
  [[ -s libmcu_sw.so ]] && \
  rm -f libmcu.so && \
  ln -s libmcu_sw.so libmcu.so
  sed -i 's/^hardwareAccelerated = true/hardwareAccelerated = false/' ${ROOT}/video_agent/agent.toml
fi
