#!/bin/bash

pause() {
  read -p "$*"
}

check_proxy(){
  if [ -z "$http_proxy" ]; then
    echo "No http proxy set, doing nothing"
  else
    echo "http proxy configured, configuring npm"
    npm config set proxy $http_proxy
  fi  

  if [ -z "$https_proxy" ]; then
    echo "No https proxy set, doing nothing"
  else
    echo "https proxy configured, configuring npm"
    npm config set https-proxy $https_proxy
  fi  
}

install_opus(){
  [ -d $LIB_DIR ] || mkdir -p $LIB_DIR
  cd $LIB_DIR
  curl -O http://downloads.xiph.org/releases/opus/opus-1.1.tar.gz
  tar -zxvf opus-1.1.tar.gz
  cd opus-1.1
  ./configure --prefix=$PREFIX_DIR
  make -s V=0
  make install
  cd $CURRENT_DIR
}

install_libvpx(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget -c https://webm.googlecode.com/files/libvpx-v1.3.0.tar.bz2
    tar -xvf libvpx-v1.3.0.tar.bz2
    cd libvpx-v1.3.0
    ./configure --prefix=/usr --enable-shared --enable-vp8 --disable-vp9
    make -s V=0 && sudo make install
    sudo ldconfig
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libvpx
  fi
}

install_libav(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    curl -O https://www.libav.org/releases/libav-9.13.tar.gz
    tar -zxvf libav-9.13.tar.gz
    cd libav-9.13
    if [ "$ENABLE_GPL" = "true" ]; then
      PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig ./configure --prefix=$PREFIX_DIR --enable-shared --enable-libvpx --enable-libopus --enable-gpl --enable-libx264
    else
      PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig ./configure --prefix=$PREFIX_DIR --enable-shared --enable-libvpx --enable-libopus
    fi
    make -s V=0
    make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libav
  fi
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
    local SSL_VERSION="1.0.2a"
    cd $LIB_DIR
    curl -O http://www.openssl.org/source/openssl-${SSL_VERSION}.tar.gz
    tar xf openssl-${SSL_VERSION}.tar.gz
    cd openssl-${SSL_VERSION}
    ./config no-ssl3 --prefix=$PREFIX_DIR -fPIC
    make depend
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
  curl -O http://ciscobinary.openh264.org/libopenh264-1.2.0-linux64.so.bz2
  bzip2 -d libopenh264-1.2.0-linux64.so.bz2
  rm -f libopenh264.so
  ln -s libopenh264-1.2.0-linux64.so libopenh264.so
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
  if [ -d src ]; then
    rm -rf src
  fi
  local GIT_ACCOUNT="lab_webrtctest"
  local WOOGEEN_GIT_URL=`git config --get remote.origin.url`
  if [ ! -z "$WOOGEEN_GIT_URL" ]; then
    if echo $WOOGEEN_GIT_URL | grep "@" -q -s; then
      GIT_ACCOUNT=`echo $WOOGEEN_GIT_URL | awk -F '\/\/' '{print $2}' | awk -F '@' '{print $1}'`
    else
      GIT_ACCOUNT=`whoami`
    fi
  fi
  # git clone --recursive -b 42-mcu ssh://${GIT_ACCOUNT}@git-ccr-1.devtools.intel.com:29418/webrtc-webrtcstack src
  git clone -b 42-mcu ssh://${GIT_ACCOUNT}@git-ccr-1.devtools.intel.com:29418/webrtc-webrtcstack src
  cd src
  sed -i 's/lab_webrtctest/'${GIT_ACCOUNT}'/g' .gitmodules
  git submodule init
  git submodule update
  ./build.sh
  mv libwebrtc.a ../
  cd $CURRENT_DIR
}

install_libuv() {
  local UV_VERSION="0.10.36"
  local UV_SRC="https://github.com/libuv/libuv/archive/v${UV_VERSION}.tar.gz"
  local UV_DST="libuv-${UV_VERSION}.tar.gz"
  cd $ROOT/third_party
  [[ ! -s ${UV_DST} ]] && wget -c ${UV_SRC} -O ${UV_DST}
  tar xf ${UV_DST}
  cd libuv-${UV_VERSION} && make
  local symbol=$(readelf -d ./libuv.so | grep soname | sed 's/.*\[\(.*\)\]/\1/g')
  ln -s libuv.so ${symbol}
  cd .. && rm -f libuv && ln -s libuv-${UV_VERSION} libuv
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
  local GATEWAY_SDK_DIR="${ROOT}/source/client_sdk"
  cd ${GATEWAY_SDK_DIR}
  mkdir -p node_modules && sudo -E npm install --prefix . --development --loglevel error
  sudo chown -R `whoami` ~/.npm ~/tmp/
}

install_mediaprocessor() {
  local MEDIAPROCESSOR_DIR="${ROOT}/third_party/mediaprocessor"
  local target="vcsa_video"
  BUILD_WITH_MSDK=true
  cd ${MEDIAPROCESSOR_DIR} && make distclean && make ${target}
}

install_gcc(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget -c http://ftp.gnu.org/gnu/gcc/gcc-4.8.2/gcc-4.8.2.tar.bz2

    tar jxf gcc-4.8.2.tar.bz2 ;cd gcc-4.8.2
    ./contrib/download_prerequisites
    make distclean
    ./configure --prefix=${PREFIX_DIR} -enable-threads=posix -disable-checking -disable-multilib -enable-languages=c,c++ --disable-bootstrap

    if
    [ $? -eq 0 ];then
    echo "this gcc configure is success"
    else
    echo "this gcc configure is failed"
    fi

    LD_LIBRARY_PATH=${PREFIX_DIR}/lib:$LD_LIBRARY_PATH make -s && make install

    [ $? -eq 0 ] && echo install success && export CC=${PREFIX_DIR}/bin/gcc && export CXX=${PREFIX_DIR}/bin/g++
  else
    mkdir -p $LIB_DIR
    install_gcc
  fi
}

cleanup_common(){  
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -r openssl*
    rm -r libnice*
    rm -r libav*
    rm -r libvpx*
    rm -r opus*
    rm -f gcc*
    cd $CURRENT_DIR
  fi
}
