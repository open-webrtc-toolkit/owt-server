#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
get_nvm_node() {
  local VERSION="v8.15.0"
  wget -qO- https://raw.githubusercontent.com/creationix/nvm/v0.34.0/install.sh | bash
  . ~/.nvm/nvm.sh
  nvm install ${VERSION}
}

install_deps() {
  read -p "Installing node via nvm? [Yes/no]" yn
  case $yn in
    [Yy]* ) get_nvm_node;;
    [Nn]* ) ;;
    * ) get_nvm_node;;
  esac
}

command -v node >/dev/null 2>&1 || install_deps
