#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

this=`dirname "$0"`
this=`cd "$this"; pwd`

usage() {
  echo
  echo "WooGeen Initialization Script"
  echo "    This script initializes WooGeen-MCU Agent."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --deps (default: false)             install dependent components and libraries via apt-get/local"
  echo "    --hardware                          enable mcu with msdk (only for video agent with \`libmcu_hw.so' packed)"
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
    sudo -E yum install nload intel-gpu-tools -y
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    sudo apt-get update
    sudo apt-get install nload intel-gpu-tools
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi

  # make the intel-gpu-tools accessable by non-root users.
  enable_intel_gpu_top
}

install_config() {
  echo -e "\x1b[32mInitializing agent configuration...\x1b[0m"
  # install mediaproess config
  if [[ ! -s ${this}/.msdk_log_config.ini ]]; then
    [[ -s /tmp/msdk_log_config.ini ]] && rm -f /tmp/msdk_log_config.ini
    ln -sf ${this}/.msdk_log_config.ini /tmp/msdk_log_config.ini
  fi
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
  esac
  shift
done

${INSTALL_DEPS} && install_deps

install_config

if ${ENABLE_HARDWARE}; then
  cd ${this}/lib
  [[ -s libmcu_hw.so ]] && \
  rm -f libmcu.so && \
  ln -s libmcu_hw.so libmcu.so
  sed -i 's/^hardwareAccelerated = false/hardwareAccelerated = true/' ${this}/agent.toml
else
  cd ${this}/lib
  [[ -s libmcu_sw.so ]] && \
  rm -f libmcu.so && \
  ln -s libmcu_sw.so libmcu.so
  sed -i 's/^hardwareAccelerated = true/hardwareAccelerated = false/' ${this}/agent.toml
fi
