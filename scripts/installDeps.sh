#!/bin/bash
SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..
BUILD_DIR=$ROOT/build
CURRENT_DIR=`pwd`

LIB_DIR=$BUILD_DIR/libdeps
PREFIX_DIR=$LIB_DIR/build/

parse_arguments(){
  while [ "$1" != "" ]; do
    case $1 in
      "--enable-gpl")
        ENABLE_GPL=true
        ;;
      "--cleanup")
        CLEANUP=true
        ;;
    esac
    shift
  done
}

cd $PATHNAME
. installCommonDeps.sh

mkdir -p $PREFIX_DIR

if ! uname -a | grep [Uu]buntu -q -s; then
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
else
  . installUbuntuDeps.sh
  pause "Installing deps via apt-get... [press Enter]"
  install_apt_deps
fi

parse_arguments $*

check_proxy

read -p "Installing gcc? [No/yes]" yn
case $yn in
  [Yy]* ) install_gcc;;
  [Nn]* ) ;;
  * ) ;;
esac

pause "Installing opus library...  [press Enter]"
install_opus

if [ "$ENABLE_GPL" = "true" ]; then
  pause "GPL libraries enabled"
  install_mediadeps
else
  pause "No GPL libraries enabled, this disables h264 transcoding, to enable gpl please use the --enable-gpl option"
  install_mediadeps_nogpl
fi

pause "Installing node building tools... [press Enter]"
install_node_tools

pause "Installing libnice library...  [press Enter]"
install_libnice

pause "Installing openssl library...  [press Enter]"
install_openssl

pause "Installing libsrtp library...  [press Enter]"
install_libsrtp

read -p "Installing webrtc library? [Yes/no]" yn
case $yn in
  [Nn]* ) ;;
  [Yy]* ) install_webrtc;;
  * ) install_webrtc;;
esac

pause "Installing ooVoo SDK library...  [press Enter]"
install_oovoosdk

read -p "Building libuv? [Yes/no]" yn
case $yn in
  [Nn]* ) ;;
  [Yy]* ) install_libuv;;
  * ) install_libuv;;
esac

read -p "Installing tcmalloc library? [No/yes]" yn
case $yn in
  [Yy]* ) install_tcmalloc;;
  [Nn]* ) ;;
  * ) ;;
esac

read -p "Installing OpenH264 Video Codec provided by Cisco Systems, Inc.? [Yes/no]" yn
case $yn in
  [Nn]* ) ;;
  [Yy]* ) install_openh264;;
  * ) install_openh264;;
esac

read -p "Installing mediaprocessor? [No/yes]" yn
case $yn in
  [Yy]* ) install_mediaprocessor;;
  [Nn]* ) ;;
  * ) ;;
esac

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
