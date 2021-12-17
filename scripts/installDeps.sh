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
NO_INTERNAL=false
INCR_INSTALL=false
CHECK_INSTALL=false
ONLY_INSTALL=""
ENABLE_WEBTRANSPORT=false
SUDO=""

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
  echo "    --enable-webtransport   install dependencies with webtransport"
  echo "    --with-nonfree-libs     install nonfree dependencies"
  echo "    --cleanup               remove intermediate files after installation"
  echo "    --only [dep]            only install specified dependency [dep]"
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
      "--no-internal")
        NO_INTERNAL=true
        ;;
      "--incremental")
        INCR_INSTALL=true
        ;;
      "--check")
        CHECK_INSTALL=true
        ;;
      "--enable-webtransport")
        ENABLE_WEBTRANSPORT=true
        ;;
      "--help")
        print_help
        ;;
      "--only")
        shift
        ONLY_INSTALL=$1
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

  read -p "Installing boost [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) install_boost;;
    * ) install_boost;;
  esac

  read -p "Installing glibc-2.18 [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) install_glibc;;
    * ) install_glibc;;
  esac

  read -p "Installing python3 [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) install_python3;;
    * ) install_python3;;
  esac
elif [[ "$OS" =~ .*ubuntu.* ]]
then
  . installUbuntuDeps.sh
  read -p "Installing deps via apt-get... [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) install_apt_deps;;
    * ) install_apt_deps;;
  esac
  if [[ "$OS_VERSION" =~ 20.04.* ]]
  then
    install_gcc_7
    install_boost
  fi
fi

[ "$CHECK_INSTALL" = true ] && echo -e "\x1b[32mInstalled deps check...\x1b[0m"

if [ ! -z $ONLY_INSTALL ]; then
  type install_${ONLY_INSTALL} > /dev/null 2>&1
  [[ $? -eq 0 ]] && install_${ONLY_INSTALL} || echo "${ONLY_INSTALL} not found"
  exit 0
fi

[ "$CHECK_INSTALL" != true ] && \
pause "Installing Node.js ... [press Enter]"
install_node

if [ "$DISABLE_NONFREE" = "true" ]; then
  [ "$CHECK_INSTALL" != true ] && \
  pause "Nonfree libraries disabled: aac transcoding unavailable. [press Enter]"
  install_mediadeps
else
  [ "$CHECK_INSTALL" != true ] && \
  pause "Nonfree libraries enabled (DO NOT redistribute these libraries!!); to disable nonfree please use the \`--disable-nonfree' option. [press Enter]"
  install_mediadeps_nonfree
fi

[ "$CHECK_INSTALL" != true ] && \
pause "Installing node building tools... [press Enter]"
install_node_tools

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing zlib? [Yes/no]" yn
case $yn in
  [Yy]* ) install_zlib;;
  [Nn]* ) ;;
  * ) install_zlib;;
esac

[ "$CHECK_INSTALL" != true ] && \
pause "Installing libnice library...  [press Enter]"
install_libnice014

[ "$CHECK_INSTALL" != true ] && \
pause "Installing openssl library...  [press Enter]"
install_openssl

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing OpenH264 Video Codec provided by Cisco Systems, Inc.? [Yes/no]" yn
case $yn in
  [Nn]* ) ;;
  [Yy]* ) install_openh264;;
  * ) install_openh264;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing libre? [Yes/no]" yn
case $yn in
  [Yy]* ) install_libre;;
  [Nn]* ) ;;
  * ) install_libre;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing libexpat? [Yes/no]" yn
case $yn in
  [Yy]* ) install_libexpat;;
  [Nn]* ) ;;
  * ) install_libexpat;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing libusrsctp? [Yes/no]" yn
case $yn in
  [Yy]* ) install_usrsctp;;
  [Nn]* ) ;;
  * ) install_usrsctp;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing libsrtp2? [Yes/no]" yn
case $yn in
  [Yy]* ) install_libsrtp2;;
  [Nn]* ) ;;
  * ) install_libsrtp2;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing quic-lib? [Yes/no]" yn
case $yn in
  [Yy]* ) install_quic;;
  [Nn]* ) ;;
  * ) install_quic;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing licode? [Yes/no]" yn
case $yn in
  [Yy]* ) install_licode;;
  [Nn]* ) ;;
  * ) install_licode;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing SVT HEVC Encoder ? [No/yes]" yn
case $yn in
  [Yy]* ) install_svt_hevc;;
  [Nn]* ) ;;
  * ) ;;
esac

[ "$CHECK_INSTALL" = true ] && yn="Yes" || \
read -p "Installing json.hpp? [Yes/no]" yn
case $yn in
  [Yy]* ) install_json_hpp;;
  [Nn]* ) ;;
  * ) install_json_hpp;;
esac

if [ "$CHECK_INSTALL" != true ]; then
  ${NO_INTERNAL} || (pause "Installing webrtc library... [press Enter]" && install_webrtc)
else
  install_webrtc
fi

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
