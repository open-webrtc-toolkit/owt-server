#!/usr/bin/env bash
#
# Copyright 2016 Intel Corporation All Rights Reserved.
#
# The source code contained or described herein and all documents related to the
# source code ("Material") are owned by Intel Corporation or its suppliers or
# licensors. Title to the Material remains with Intel Corporation or its suppliers
# and licensors. The Material contains trade secrets and proprietary and
# confidential information of Intel or its suppliers and licensors. The Material
# is protected by worldwide copyright and trade secret laws and treaty provisions.
# No part of the Material may be used, copied, reproduced, modified, published,
# uploaded, posted, transmitted, distributed, or disclosed in any way without
# Intel's prior express written permission.
#  *
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery of
# the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be express
# and approved by Intel in writing.
#

this=$(dirname "$0")
this=$(cd "${this}"; pwd)
NO_FDKAAC_LIB="${this}/no_fdkaac_lib"
DOWNLOAD_DIR="${this}/download"

usage() {
  echo
  echo "Acess Agent Install Dependency Script"
  echo "Usage:"
  echo "    --deps              install common dependencies"
  echo "    --enable-fdkaac     install fdkaac-enabled dependencies"
  echo
}

install_deps() {
  local OS=$(${this}/detectOS.sh | awk '{print tolower($0)}')
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
    sudo -E yum install nload -y
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    sudo apt-get update
    sudo apt-get install nload
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

install_opus(){
  wget -c http://downloads.xiph.org/releases/opus/opus-1.1.tar.gz
  tar -zxvf opus-1.1.tar.gz
  pushd opus-1.1
  ./configure --prefix=${DOWNLOAD_DIR}
  make -s V=0
  make install
  popd
}

install_fdkaac(){
  local VERSION="0.1.4"
  local SRC="fdk-aac-${VERSION}.tar.gz"
  local SRC_URL="http://sourceforge.net/projects/opencore-amr/files/fdk-aac/${SRC}/download"
  local SRC_MD5SUM="e274a7d7f6cd92c71ec5c78e4dc9f8b7"
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL} -O ${SRC}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
    rm -f ${SRC} && wget -c ${SRC_URL} -O ${SRC} # try download again
    (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
  fi
  rm -fr fdk-aac-${VERSION}
  tar xf ${SRC}
  pushd fdk-aac-${VERSION}
  ./configure --prefix=${DOWNLOAD_DIR} --enable-shared --enable-static
  make -s V=0 && make install
  popd
}

install_ffmpeg(){
  local VERSION="2.7.2"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"
  local SRC_MD5SUM="7eb2140bab9f0a8669b65b50c8e4cfb5"
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
    rm -f ${SRC} && wget -c ${SRC_URL} # try download again
    (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
  fi
  rm -fr ${DIR}
  tar xf ${SRC}
  pushd ${DIR}
  PKG_CONFIG_PATH=${DOWNLOAD_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${DOWNLOAD_DIR} --enable-shared --disable-libvpx --disable-vaapi --enable-libopus --enable-libfdk-aac --enable-nonfree && \
  make -j4 -s V=0 && make install
  popd
}

INSTALL_DEPS=false
ENABLE_FDKAAC=false

if [ $# -eq 0 ]; then
    usage
    exit 0
fi

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)deps )
      INSTALL_DEPS=true
      ;;
    *(-)enable-fdkaac )
      ENABLE_FDKAAC=true
      ;;
    *(-)disable-fdkaac )
      ENABLE_FDKAAC=false
      ;;
    *(-)help )
      usage
      exit 0
      ;;
  esac
  shift
done

if ${INSTALL_DEPS}; then
  install_deps
fi

if ${ENABLE_FDKAAC}; then
  # Backup the non-fdkaac lib
  if [[ ! -d ${NO_FDKAAC_LIB} ]]; then
    mkdir ${NO_FDKAAC_LIB}
    cp ${this}/lib/* ${NO_FDKAAC_LIB}/
  fi

  [[ ! -d ${DOWNLOAD_DIR} ]] && mkdir ${DOWNLOAD_DIR};

  pushd ${DOWNLOAD_DIR}
  install_opus
  install_fdkaac
  install_ffmpeg
  popd

  cp -P ${DOWNLOAD_DIR}/lib/*.so.* ${this}/lib/
  cp -P ${DOWNLOAD_DIR}/lib/*.so ${this}/lib/
fi
