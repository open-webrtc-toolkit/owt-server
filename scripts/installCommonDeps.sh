#!/bin/bash
SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..
BUILD_DIR=$ROOT/build
CURRENT_DIR=`pwd`

LIB_DIR=$BUILD_DIR/libdeps
PREFIX_DIR=$LIB_DIR/build/

pause() {
  read -p "$*"
}

install_libnice(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    curl -O http://nice.freedesktop.org/releases/libnice-0.1.4.tar.gz
    tar -zxvf libnice-0.1.4.tar.gz
    cd libnice-0.1.4
    patch -R ./agent/conncheck.c < $PATHNAME/libnice-014.patch0
    PKG_CONFIG_PATH=$PREFIX_DIR"/lib/pkgconfig":$PREFIX_DIR"/lib64/pkgconfig":$PKG_CONFIG_PATH ./configure --prefix=$PREFIX_DIR && make -s V= && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libnice
  fi
}

install_openssl(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    curl -O http://www.openssl.org/source/openssl-1.0.1g.tar.gz
    tar -zxvf openssl-1.0.1g.tar.gz
    cd openssl-1.0.1g
    ./config --prefix=$PREFIX_DIR -fPIC
    make -s V=0
    make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_openssl
  fi
}

install_openh264(){
  cd $ROOT/third_party/openh264
  make clean
  make ENABLE64BIT=Yes
  cd $CURRENT_DIR
}

install_libsrtp(){
  cd $ROOT/third_party/srtp
  ./configure --prefix=$PREFIX_DIR
  make clean
  make -s V=0
  make uninstall
  make install
  cd $CURRENT_DIR
}

install_webrtc(){
  cd $ROOT/third_party/webrtc
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
  PATH=$ROOT/third_party/webrtc/depot_tools:$PATH
  if [ -d src ]; then
    rm -rf src
  fi
  echo "Downloading WebRTC source code..."
  # svn checkout -q http://webrtc.googlecode.com/svn/branches/3.52/webrtc
  gclient sync --nohooks
  echo "Done."
  patch -p0 < ./webrtc-3.52-build.patch
  patch -p0 < ./webrtc-3.52-source.patch
  patch -p0 < ./opus-build.patch
  patch -p1 < ./webrtc-3.52-h264.patch
  ./build.sh
  cd $CURRENT_DIR
}

install_libuv() {
  local UV_SRC="https://github.com/joyent/libuv/archive/v0.10.26.tar.gz"
  local UV_DST="libuv-0.10.26.tar.gz"
  cd $ROOT/third_party
  [[ ! -s ${UV_DST} ]] && wget -c ${UV_SRC} -O ${UV_DST}
  tar xf ${UV_DST}
  cd libuv-0.10.26 && make
  local symbol=$(readelf -d ./libuv.so | grep soname | sed 's/.*\[\(.*\)\]/\1/g')
  ln -s libuv.so ${symbol}
}

install_oovoosdk(){
  mkdir -p $PREFIX_DIR/lib
  if uname -a | grep [Uu]buntu -q -s; then
    cp -av $ROOT/third_party/liboovoosdk-ubuntu.so $PREFIX_DIR/lib/liboovoosdk.so
  else
    cp -av $ROOT/third_party/liboovoosdk-el.so $PREFIX_DIR/lib/liboovoosdk.so
  fi
}

install_tcmalloc(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    curl -O http://gperftools.googlecode.com/files/gperftools-2.1.tar.gz
    tar -zxf gperftools-2.1.tar.gz
    cd gperftools-2.1
    ./configure --prefix=$PREFIX_DIR --disable-cpu-profiler --disable-heap-profiler --disable-heap-checker --disable-debugalloc
    make -s V=0
    make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_tcmalloc
  fi
}

install_node_tools() {
  sudo -E npm install -g --loglevel error node-gyp grunt-cli underscore
  local SDK_DIR="${ROOT}/source/sdk2"
  cd ${SDK_DIR} && make dep
  sudo chown -R `whoami` ~/.npm ~/tmp/
}

pause "Installing node building tools... [press Enter]"
install_node_tools

install_mediaprocessor() {
  local MEDIAPROCESSOR_DIR="${ROOT}/third_party/mediaprocessor"
  local target="vcsa_base"
  read -p "Installing mediaprocessor with hardware supprot (MSDK)? [Yes/no]" yn
  case $yn in
    [Nn]* ) BUILD_WITH_MSDK=false;;
    [Yy]* ) BUILD_WITH_MSDK=true;;
    * ) BUILD_WITH_MSDK=true;;
  esac
  ${BUILD_WITH_MSDK} && target="vcsa_video"
  cd "${MEDIAPROCESSOR_DIR}" && make distclean && make ${target}
}

mkdir -p $PREFIX_DIR

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

read -p "Installing openh264 library? [Yes/no]" yn
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
