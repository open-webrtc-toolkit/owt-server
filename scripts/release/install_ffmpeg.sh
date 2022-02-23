#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

this=`dirname "$0"`
this=`cd "$this"; pwd`
SUDO=""
if [[ $EUID -ne 0 ]]; then
   SUDO="sudo -E"
fi

ENABLE_SRT=false
SRT_OPTION=" "
AGENT=`echo $this | awk -F "/" '{print $NF}'`
if [ "$AGENT" = "streaming_agent" ]; then
  ENABLE_SRT=true
  SRT_OPTION="--enable-libsrt"
fi

detect_OS() {
  lsb_release >/dev/null 2>/dev/null
  if [ $? = 0 ]
  then
    lsb_release -ds | sed 's/^\"//g;s/\"$//g'
  # a bunch of fallbacks if no lsb_release is available
  # first trying /etc/os-release which is provided by systemd
  elif [ -f /etc/os-release ]
  then
    source /etc/os-release
    if [ -n "${PRETTY_NAME}" ]
    then
      printf "${PRETTY_NAME}\n"
    else
      printf "${NAME}"
      [[ -n "${VERSION}" ]] && printf " ${VERSION}"
      printf "\n"
    fi
  # now looking at distro-specific files
  elif [ -f /etc/arch-release ]
  then
    printf "Arch Linux\n"
  elif [ -f /etc/gentoo-release ]
  then
    cat /etc/gentoo-release
  elif [ -f /etc/fedora-release ]
  then
    cat /etc/fedora-release
  elif [ -f /etc/redhat-release ]
  then
    cat /etc/redhat-release
  elif [ -f /etc/debian_version ]
  then
    printf "Debian GNU/Linux " ; cat /etc/debian_version
  else
    printf "Unknown\n"
  fi
}

install_build_deps() {
  local OS=$(detect_OS | awk '{print tolower($0)}')
  echo $OS

  if [[ "$OS" =~ .*centos.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via yum...\x1b[0m"
    ${SUDO} yum install pkg-config make gcc gcc-c++ nasm yasm freetype-devel -y
    if [ "$ENABLE_SRT" = "true" ]; then
	${SUDO} yum install tcl openssl-devel cmake automake -y
    fi
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    ${SUDO} apt-get update
    ${SUDO} apt-get install pkg-config make gcc g++ nasm yasm libfreetype6-dev -y
    if [ "$ENABLE_SRT" = "true" ]; then
        ${SUDO} apt-get install tcl cmake libssl-dev build-essential -y
    fi
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

install_openssl(){
  local PREFIX_DIR="${this}/ffmpeg-install"
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libssl* 2>/dev/null`
  pushd ${this} >/dev/null
  [[ ! -z $LIST_LIBS ]] && echo "openssl already installed." && return 0

  local SSL_BASE_VERSION="1.1.1"
  local SSL_VERSION="1.1.1j"
  rm -rf openssl-1*

  wget -c https://www.openssl.org/source/openssl-${SSL_VERSION}.tar.gz
  tar xf openssl-${SSL_VERSION}.tar.gz
  cd openssl-${SSL_VERSION}
  ./config no-ssl3 --prefix=$PREFIX_DIR -fPIC --libdir=lib
  make depend
  make -s V=0
  make install

  popd
}


install_srt(){
  local VERSION="1.4.1"
  local SRC="v${VERSION}.tar.gz"
  local SRC_URL=" https://github.com/Haivision/srt/archive/${SRC}"
  local SRC_DIR="srt-${VERSION}"
  local PREFIX_DIR="${this}/ffmpeg-install"

  local LIST_LIBS=`ls ${this}/lib/libsrt* 2>/dev/null`
  pushd ${this} >/dev/null
  [[ ! -z $LIST_LIBS ]] && echo "srt already installed." && return 0
  wget ${SRC_URL}
  rm -fr ${SRC_DIR}
  tar xf ${SRC}
  pushd ${SRC_DIR}
  ./configure --prefix=${PREFIX_DIR}
  make && make install
  popd
}

install_ffmpeg(){
  local VERSION="4.1.3"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"
  local SRC_MD5SUM="9985185a8de3678e5b55b1c63276f8b5"
  local PREFIX_DIR="${this}/ffmpeg-install"

  local LIST_LIBS=`ls ${this}/lib/libav* 2>/dev/null`
  pushd ${this} >/dev/null
  [[ ! -z $LIST_LIBS ]] && echo "ffmpeg already installed." && return 0

  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
      echo "Downloaded file ${SRC} is corrupted."
      rm -v ${SRC}
      return 1
  fi
  rm -fr ${DIR}
  tar xf ${SRC}
  pushd ${DIR} >/dev/null
  PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${PREFIX_DIR} --enable-shared \
      --disable-static --disable-libvpx --disable-vaapi --enable-libfreetype ${SRT_OPTION}
  make -j4 -s V=0 && make install
  popd
  popd

  cp -P ${PREFIX_DIR}/lib/*.so.* ${this}/lib/
  cp -P ${PREFIX_DIR}/lib/*.so ${this}/lib/
}

echo "Install building dependencies..."
install_build_deps

if [ "$ENABLE_SRT" = "true" ]; then
  install_openssl
  install_srt
fi

echo "Install ffmpeg..."
install_ffmpeg
