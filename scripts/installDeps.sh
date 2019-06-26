#!/bin/bash
SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..
BUILD_DIR=$ROOT/build
CURRENT_DIR=`pwd`

LIB_DIR=$BUILD_DIR/libdeps
PREFIX_DIR=$LIB_DIR/build/
DISABLE_NONFREE=true
CLEANUP=false
NO_INTERNAL=false
INCR_INSTALL=false
ONLY_INSTALL=""

parse_arguments(){
  while [ "$1" != "" ]; do
    case $1 in
      "--with-nonfree-libs")
        DISABLE_NONFREE=false
        ;;
      "--cleanup")
        CLEANUP=true
        ;;
      "--no-internal")
        NO_INTERNAL=true
        ;;
      "--incremental")
        INCR_INSTALL=true
        ;;
      "--only")
        shift
        ONLY_INSTALL=$1
        ;;
    esac
    shift
  done
}

OS=`$PATHNAME/detectOS.sh | awk '{print tolower($0)}'`
echo $OS

cd $PATHNAME
. installCommonDeps.sh

mkdir -p $PREFIX_DIR

if [[ "$OS" =~ .*centos.* ]]
then
  . installCentOSDeps.sh
  read -p "Add EPEL repository to yum? [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) installRepo;;
    * ) installRepo;;
  esac

  read -p "Installing deps via yum [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) installYumDeps;;
    * ) installYumDeps;;
  esac
elif [[ "$OS" =~ .*ubuntu.* ]]
then
  . installUbuntuDeps.sh
  pause "Installing deps via apt-get... [press Enter]"
  install_apt_deps
fi

parse_arguments $*

if [ ! -z $ONLY_INSTALL ]; then
  type install_${ONLY_INSTALL} > /dev/null 2>&1
  [[ $? -eq 0 ]] && install_${ONLY_INSTALL} || echo "${ONLY_INSTALL} not found"
  exit 0
fi

pause "Installing Node.js ... [press Enter]"
install_node

if [ "$DISABLE_NONFREE" = "true" ]; then
  pause "Nonfree libraries disabled: aac transcoding unavailable."
  install_mediadeps
else
  pause "Nonfree libraries enabled (DO NOT redistribute these libraries!!); to disable nonfree please use the \`--disable-nonfree' option."
  install_mediadeps_nonfree
fi

pause "Installing node building tools... [press Enter]"
install_node_tools

read -p "Installing zlib? [Yes/no]" yn
case $yn in
  [Yy]* ) install_zlib;;
  [Nn]* ) ;;
  * ) install_zlib;;
esac

pause "Installing libnice library...  [press Enter]"
install_libnice014

pause "Installing openssl library...  [press Enter]"
install_openssl

read -p "Installing OpenH264 Video Codec provided by Cisco Systems, Inc.? [Yes/no]" yn
case $yn in
  [Nn]* ) ;;
  [Yy]* ) install_openh264;;
  * ) install_openh264;;
esac

read -p "Installing libre? [Yes/no]" yn
case $yn in
  [Yy]* ) install_libre;;
  [Nn]* ) ;;
  * ) install_libre;;
esac

read -p "Installing libexpat? [Yes/no]" yn
case $yn in
  [Yy]* ) install_libexpat;;
  [Nn]* ) ;;
  * ) install_libexpat;;
esac

read -p "Installing libusrsctp? [Yes/no]" yn
case $yn in
  [Yy]* ) install_usrsctp;;
  [Nn]* ) ;;
  * ) install_usrsctp;;
esac

read -p "Installing libsrtp2? [Yes/no]" yn
case $yn in
  [Yy]* ) install_libsrtp2;;
  [Nn]* ) ;;
  * ) install_libsrtp2;;
esac

read -p "Installing quic-lib? [Yes/no]" yn
case $yn in
  [Yy]* ) install_quic;;
  [Nn]* ) ;;
  * ) install_quic;;
esac

read -p "Installing licode? [Yes/no]" yn
case $yn in
  [Yy]* ) install_licode;;
  [Nn]* ) ;;
  * ) install_licode;;
esac

read -p "Installing SVT HEVC Encoder ? [No/yes]" yn
case $yn in
  [Yy]* ) install_svt_hevc;;
  [Nn]* ) ;;
  * ) ;;
esac

read -p "Installing json.hpp? [Yes/no]" yn
case $yn in
  [Yy]* ) install_json_hpp;;
  [Nn]* ) ;;
  * ) install_json_hpp;;
esac


${NO_INTERNAL} || (pause "Installing webrtc library... [press Enter]" && install_webrtc)

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
