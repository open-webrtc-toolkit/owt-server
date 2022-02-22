#!/bin/bash -e
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
NIGHTLY=false
NO_INTERNAL=false
INCR_INSTALL=false
CHECK_INSTALL=false
ENABLE_WEBTRANSPORT=false
ENABLE_SRT=false
SUDO=""

if [ "$GITHUB_ACTIONS" == "true" ]; then
  ENABLE_WEBTRANSPORT=true
fi

if [[ $EUID -ne 0 ]]; then
  SUDO="sudo -E"
fi

print_help(){
  echo
  echo "Install Dependency Script"
  echo "Usage:"
  echo "    (default)               install default dependencies"
  echo "    --check                 check whether dependencies are installed"
  echo "    --incremental           skip dependencies which are already installed"
  echo "    --with-nonfree-libs     install nonfree dependencies"
  echo "    --cleanup               remove intermediate files after installation"
  echo "    --help                  print help of this script"
  echo
  exit 0
}

parse_arguments(){
  while [ "$1" != "" ]; do
    case $1 in
      "--with-nonfree-libs")
        DISABLE_NONFREE=false
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
      "--incremental")
        INCR_INSTALL=true
        ;;
      "--check")
        CHECK_INSTALL=true
        ;;
    esac
    shift
  done
}

parse_arguments $*

OS=`$PATHNAME/detectOS.sh | awk '{print tolower($0)}'`
OS_VERSION=`$PATHNAME/detectOS.sh | awk '{print tolower($2)}'`
echo $OS

cd $PATHNAME
. installCommonDeps.sh

mkdir -p $PREFIX_DIR

if [[ "$OS" =~ .*centos.* ]]
then
  . installCentOSDeps.sh
  if [ "$NIGHTLY" != "true" ]; then
    [ "$CHECK_INSTALL" != true ] && installRepo
    [ "$CHECK_INSTALL" != true ] && installYumDeps
    install_boost
    install_glibc
    install_python3
  fi
elif [[ "$OS" =~ .*ubuntu.* ]]
then
  . installUbuntuDeps.sh
  if [ "$NIGHTLY" != "true" ]; then
    [ "$CHECK_INSTALL" != true ] && install_apt_deps
    if [[ "$OS_VERSION" =~ 20.04.* ]]
    then
      install_gcc_7
      install_boost
    fi
  fi
fi

if [ "$GITHUB_ACTIONS" != "true" ]; then
  install_node
fi

if [ "$NIGHTLY" != "true" ] && [ "$GITHUB_ACTIONS" != "true" ]; then
  if [ "$DISABLE_NONFREE" = "true" ]; then
    install_mediadeps
  else
    install_mediadeps_nonfree
  fi

  install_node_tools

  install_zlib

  install_libnice014

  install_openssl

  install_openh264

  install_libre

  install_libexpat

  install_usrsctp

  install_libsrtp2

  install_quic

  install_licode

  install_svt_hevc

  install_json_hpp

fi

if [ "$GITHUB_ACTIONS" != "true" ]; then
  ${NO_INTERNAL} || install_webrtc
else
  install_node_tools
  install_quic
fi

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
