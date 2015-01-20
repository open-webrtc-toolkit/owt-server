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
      "--nightly")
        NIGHTLY=true
        ;;
    esac
    shift
  done
}

parse_arguments $*

cd $PATHNAME
. installCommonDeps.sh

mkdir -p $PREFIX_DIR

if ! uname -a | grep [Uu]buntu -q -s; then
  . installCentOSDeps.sh
  if [ "$NIGHTLY" != "true" ]; then
    installRepo
    installYumDeps
  fi
else
  . installUbuntuDeps.sh
  if [ "$NIGHTLY" != "true" ]; then
    install_apt_deps
  fi
fi

check_proxy

if [ "$NIGHTLY" != "true" ]; then
  install_opus

  if [ "$ENABLE_GPL" = "true" ]; then
    install_mediadeps
  else
    install_mediadeps_nogpl
  fi

  install_node_tools

  install_libnice

  install_openssl

  install_libsrtp

  install_oovoosdk

  install_libuv

  install_openh264
fi

install_webrtc

install_mediaprocessor

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
