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

pause "Installing Node.js ... [press Enter]"
install_node

check_proxy

read -p "Installing gcc? [No/yes]" yn
case $yn in
  [Yy]* ) install_gcc;;
  [Nn]* ) ;;
  * ) ;;
esac

if [ "$DISABLE_NONFREE" = "true" ]; then
  pause "Nonfree libraries disabled: aac transcoding unavailable."
  install_mediadeps
else
  pause "Nonfree libraries enabled (DO NOT redistribute these libraries!!); to disable nonfree please use the \`--disable-nonfree' option."
  install_mediadeps_nonfree
fi

pause "Installing node building tools... [press Enter]"
install_node_tools

read -p "Installing glib? [No/yes]" yn
case $yn in
  [Yy]* ) install_glib;;
  [Nn]* ) ;;
  * ) ;;
esac

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

${NO_INTERNAL} || (pause "Installing webrtc library... [press Enter]" && install_webrtc)

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

read -p "Installing nicer? [No/yes]" yn
case $yn in
  [Yy]* ) install_nicer;;
  [Nn]* ) ;;
  * ) ;;
esac

read -p "Installing licode? [Yes/no]" yn
case $yn in
  [Yy]* ) install_licode;;
  [Nn]* ) ;;
  * ) install_licode;;
esac

read -p "Installing vainfo util? [No/yes]" yn
case $yn in
  [Yy]* ) install_libvautils;;
  [Nn]* ) ;;
  * ) ;;
esac

if [[ "$OS" =~ .*ubuntu.* ]]
then
    read -p "Installing SVT HEVC Encoder ? [No/yes]" yn
    case $yn in
      [Yy]* ) install_svt_hevc;;
      [Nn]* ) ;;
      * ) ;;
    esac
fi

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
