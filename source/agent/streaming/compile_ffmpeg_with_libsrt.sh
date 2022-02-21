##!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

this=$(pwd)

DOWNLOAD_DIR="${this}/ffmpeg_libsrt_src"
TARGET_DIR="${this}/ffmpeg_libsrt_lib"
SUDO=""
if [[ $EUID -ne 0 ]]; then
   SUDO="sudo -E"
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
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    ${SUDO} apt-get update
    ${SUDO} apt-get install pkg-config make gcc g++ nasm yasm libfreetype6-dev -y
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

install_srt(){
  local VERSION="1.4.1"
  local SRC="v${VERSION}.tar.gz"
  local SRC_URL=" https://github.com/Haivision/srt/archive/${SRC}"
  local SRC_DIR="srt-${VERSION}"
  local PREFIX_DIR="${this}/ffmpeg-install"
  mkdir -p ${LIB_DIR}
  pushd ${LIB_DIR}
  wget ${SRC_URL}
  rm -fr ${SRC_DIR}
  tar xf ${SRC}
  pushd ${SRC_DIR}
  ./configure --prefix=${PREFIX_DIR}
  make && make install
  popd
  popd
}

install_ffmpeg(){
  local VERSION="4.1.3"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"
  local SRC_MD5SUM="9985185a8de3678e5b55b1c63276f8b5"

  echo "Downloading ffmpeg-${VERSION}"
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
      echo "Downloaded file ${SRC} is corrupted."
      rm -v ${SRC}
      return 1
  fi
  rm -fr ${DIR}
  tar xf ${SRC}

  echo "Building ffmpeg-${VERSION}"
  pushd ${DIR}
  PKG_CONFIG_PATH=${DOWNLOAD_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${DOWNLOAD_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libfreetype --enable-libsrt && \
  make -j4 -s V=0 && make install
  popd
}

echo "This script will download and compile a libsrt enabled ffmpeg."
read -p "Continue to compile ffmpeg with libsrt? [No/yes]" yn
case $yn in
  [Yy]* ) ;;
  [Nn]* ) exit 0;;
  * ) ;;
esac

echo "Install building dependencies..."
install_build_deps

[[ ! -d ${DOWNLOAD_DIR} ]] && mkdir ${DOWNLOAD_DIR};

pushd ${DOWNLOAD_DIR}
install_srt
install_ffmpeg
popd

[[ ! -d ${TARGET_DIR} ]] && mkdir ${TARGET_DIR};

echo "Copy libs to ${TARGET_DIR}"
cp -P ${DOWNLOAD_DIR}/lib/*.so.* ${TARGET_DIR}
cp -P ${DOWNLOAD_DIR}/lib/*.so ${TARGET_DIR}/

echo "Compiling finish."
echo "Downloaded source dir: ${DOWNLOAD_DIR}"
echo "Compiled library dir: ${TARGET_DIR}"

