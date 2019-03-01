##!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

this=$(pwd)

DOWNLOAD_DIR="${this}/ffmpeg_libfdkaac_src"
TARGET_DIR="${this}/ffmpeg_libfdkaac_lib"
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
    ${SUDO} yum install gcc gcc-c++ nasm yasm freetype-devel -y
  elif [[ "$OS" =~ .*ubuntu.* ]]
  then
    echo -e "\x1b[32mInstalling dependent components and libraries via apt-get...\x1b[0m"
    ${SUDO} apt-get update
    ${SUDO} apt-get install make gcc g++ nasm yasm libfreetype6-dev
  else
    echo -e "\x1b[32mUnsupported platform...\x1b[0m"
  fi
}

install_fdkaac(){
  local VERSION="0.1.4"
  local SRC="fdk-aac-${VERSION}.tar.gz"
  local SRC_URL="http://sourceforge.net/projects/opencore-amr/files/fdk-aac/${SRC}/download"
  local SRC_MD5SUM="e274a7d7f6cd92c71ec5c78e4dc9f8b7"

  echo "Downloading fdk-aac-${VERSION}"
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL} -O ${SRC}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
    rm -f ${SRC} && wget -c ${SRC_URL} -O ${SRC} # try download again
    (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
  fi
  rm -fr fdk-aac-${VERSION}
  tar xf ${SRC}

  echo "Building fdk-aac-${VERSION}"
  pushd fdk-aac-${VERSION}
  ./configure --prefix=${DOWNLOAD_DIR} --enable-shared --disable-static
  make -s V=0 && make install
  popd
}

install_ffmpeg(){
  local VERSION="4.1"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"
  local SRC_MD5SUM="8107e8f22a33b27a5e34b38e628eecc6"

  echo "Downloading ffmpeg-${VERSION}"
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
    rm -f ${SRC} && wget -c ${SRC_URL} # try download again
    (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
  fi
  rm -fr ${DIR}
  tar xf ${SRC}

  echo "Building ffmpeg-${VERSION}"
  pushd ${DIR}
  PKG_CONFIG_PATH=${DOWNLOAD_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${DOWNLOAD_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libfdk-aac --enable-nonfree --enable-libfreetype && \
  make -j4 -s V=0 && make install
  popd
}

echo "This script will download and compile a libfdk_aac enabled ffmpeg. The libfdk_aac is designated as 'non-free', please make sure you have got proper authority before using it."
read -p "Continue to compile ffmpeg with libfdk_aac? [No/yes]" yn
case $yn in
  [Yy]* ) ;;
  [Nn]* ) exit 0;;
  * ) ;;
esac

echo "Install building dependencies..."
install_build_deps

[[ ! -d ${DOWNLOAD_DIR} ]] && mkdir ${DOWNLOAD_DIR};

pushd ${DOWNLOAD_DIR}
install_fdkaac
install_ffmpeg
popd

[[ ! -d ${TARGET_DIR} ]] && mkdir ${TARGET_DIR};

echo "Copy libs to ${TARGET_DIR}"
cp -P ${DOWNLOAD_DIR}/lib/*.so.* ${TARGET_DIR}
cp -P ${DOWNLOAD_DIR}/lib/*.so ${TARGET_DIR}/

echo "Compiling finish."
echo "Downloaded source dir: ${DOWNLOAD_DIR}"
echo "Compiled library dir: ${TARGET_DIR}"

