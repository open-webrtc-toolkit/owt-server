#!/bin/bash
SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..
BUILD_DIR=$ROOT/build
CURRENT_DIR=`pwd`

LIB_DIR=$BUILD_DIR/libdeps
PREFIX_DIR=$LIB_DIR/build/
DISABLE_NONFREE=false
CLEANUP=false
NIGHTLY=false
NO_INTERNAL=false

parse_arguments(){
  while [ "$1" != "" ]; do
    case $1 in
      "--disable-nonfree")
        DISABLE_NONFREE=true
        ;;
      "--cleanup")
        CLEANUP=true
        ;;
      "--nightly")
        NIGHTLY=true
        ;;
      "--no-internal")
        NO_INTERNAL=true
        ;;
    esac
    shift
  done
}

parse_arguments $*

OS=`$PATHNAME/detectOS.sh | awk '{print tolower($0)}'`
echo $OS

cd $PATHNAME
. installCommonDeps.sh

mkdir -p $PREFIX_DIR

if [[ "$OS" =~ .*centos.* ]]
then
  . installCentOSDeps.sh
  if [ "$NIGHTLY" != "true" ]; then
    installRepo
    installYumDeps
  fi
elif [[ "$OS" =~ .*ubuntu.* ]]
then
  . installUbuntuDeps.sh
  if [ "$NIGHTLY" != "true" ]; then
    install_apt_deps
  fi
fi

install_node

check_proxy

if [ "$NIGHTLY" != "true" ]; then
  install_opus

  if [ "$DISABLE_NONFREE" = "true" ]; then
    install_mediadeps
  else
    install_mediadeps_nonfree
  fi

  install_node_tools

  install_libnice

  install_openssl

  install_libsrtp

  install_oovoosdk

  install_openh264
fi

${NO_INTERNAL} || install_webrtc

install_mediaprocessor

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
