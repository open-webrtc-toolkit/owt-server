#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/.."; pwd`

export OWT_HOME=${ROOT}

LogDir=${OWT_HOME}/logs
SUDO=""
if [[ $EUID -ne 0 ]]; then
  SUDO="sudo -E"
fi

usage() {
  echo
  echo "Rabbitmq Start Up Script"
  echo "    This script starts rabbitmq."
  echo "    This script is intended to run on a target machine."
  echo
  echo "Usage:"
  echo "    --deps (default: false)             install dependent components and libraries before rabbitmq start-up"
  echo "    --help                              print this help"
  echo
}

change_account() {
  read -p "Enter your username for rabbitmq: " rabbituser
  echo -n Password:
  read -s rabbitpasswd
  if [[ ! -z "$rabbitpasswd" && ! -z "$rabbituser" ]]; then
    ${SUDO} rabbitmqctl add_user ${rabbituser} ${rabbitpasswd}
    ${SUDO} rabbitmqctl set_user_tags ${rabbituser} administrator
    ${SUDO} rabbitmqctl set_permissions ${rabbituser} ".*" ".*" ".*"
    ${SUDO} rabbitmqctl delete_user guest
    rabbituser=""
    rabbitpasswd=""
  else
    echo
    echo -e "Failed to add user: empty username or password"
  fi
}

init_rabbit() {
  read -t 10 -p "Diable Default RabbitMQ Account and Create a New One? [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) change_account;;
    * ) change_account;;
  esac
}

install_deps() {
  local CHECK_INSTALLED=""
  local OS=`${this}/detectOS.sh | awk '{print tolower($0)}'`
  echo $OS

  if [[ "$OS" =~ .*centos.* ]]
  then
    CHECK_INSTALLED=`rpm -qa | grep rabbitmq-server`
    if [[ -z "$CHECK_INSTALLED" ]]; then
      echo -e "\x1b[32mInstalling dependent components and libraries via yum...\x1b[0m"
      wget -c http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
      ${SUDO} rpm -Uvh epel-release-latest-7*.rpm
      ${SUDO} sed -i 's/https/http/g' /etc/yum.repos.d/epel.repo
      ${SUDO} yum install rabbitmq-server -y
      init_rabbit
    else
      echo -e "\x1b[32mRabbitmq-server already installed\x1b[0m"
    fi
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    CHECK_INSTALLED=`dpkg -l | grep -E '^ii' | grep rabbitmq-server`
    if [[ -z "$CHECK_INSTALLED" ]]; then
      echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
      ${SUDO} apt-get update
      ${SUDO} apt-get install rabbitmq-server -y
      init_rabbit
    else
      echo -e "\x1b[32mRabbitmq-server already installed\x1b[0m"
    fi
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

start_up() {
  local OS=`${this}/detectOS.sh | awk '{print tolower($0)}'`

  # Use default configuration
  if [[ "$OS" =~ .*centos.* ]]
  then
    if ! ${SUDO} systemctl status rabbitmq-server >/dev/null; then
      echo "Start rabbitmq-server - \"systemctl start rabbitmq-server\""
      ${SUDO} systemctl start rabbitmq-server
    else
      echo -e "\x1b[32mRabbitmq-server already running\x1b[0m"
    fi
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    if ! ${SUDO} service rabbitmq-server status >/dev/null; then
      echo "Start rabbitmq-server - \"service rabbitmq-server start\""
      ${SUDO} service rabbitmq-server start
    else
      echo -e "\x1b[32mRabbitmq-server already running\x1b[0m"
    fi
  fi
}

INSTALL_DEPS=false

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)deps )
      INSTALL_DEPS=true
      ;;
    *(-)help )
      usage
      exit 0
      ;;
  esac
  shift
done

${INSTALL_DEPS} && install_deps

start_up

#${INSTALL_DEPS} && init_rabbit
