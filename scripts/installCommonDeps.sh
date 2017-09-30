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
  wget -c http://downloads.xiph.org/releases/opus/opus-1.1.tar.gz
  tar -zxvf opus-1.1.tar.gz
  cd opus-1.1
  ./configure --prefix=$PREFIX_DIR
  make -s V=0
  make install
  cd $CURRENT_DIR
}

install_fdkaac(){
  local VERSION="0.1.4"
  local SRC="fdk-aac-${VERSION}.tar.gz"
  local SRC_URL="http://sourceforge.net/projects/opencore-amr/files/fdk-aac/${SRC}/download"
  local SRC_MD5SUM="e274a7d7f6cd92c71ec5c78e4dc9f8b7"
  mkdir -p ${LIB_DIR}
  pushd ${LIB_DIR}
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL} -O ${SRC}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
    rm -f ${SRC} && wget -c ${SRC_URL} -O ${SRC} # try download again
    (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
  fi
  rm -fr fdk-aac-${VERSION}
  tar xf ${SRC}
  pushd fdk-aac-${VERSION}
  ./configure --prefix=/usr/local --enable-shared --enable-static
  make -s V=0 && sudo make install && sudo ldconfig
  popd
  popd
}

install_ffmpeg(){
  local VERSION="3.2.4"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"
  local SRC_MD5SUM="d3ebaacfa36c6e8145373785824265b4"
  mkdir -p ${LIB_DIR}
  pushd ${LIB_DIR}
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
    rm -f ${SRC} && wget -c ${SRC_URL} # try download again
    (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
  fi
  rm -fr ${DIR}
  tar xf ${SRC}
  pushd ${DIR}
  [[ "${DISABLE_NONFREE}" == "true" ]] && \
  PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libopus || \
  PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libopus --enable-libfdk-aac --enable-nonfree && \
  make -j4 -s V=0 && make install
  popd
  popd
}

install_libnice(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget -c http://nice.freedesktop.org/releases/libnice-0.1.13.tar.gz
    tar -zxvf libnice-0.1.13.tar.gz
    cd libnice-0.1.13
    #patch -p1 < $PATHNAME/patches/libnice-0001-Remove-lock.patch
    PKG_CONFIG_PATH=$PREFIX_DIR"/lib/pkgconfig":$PREFIX_DIR"/lib64/pkgconfig":$PKG_CONFIG_PATH ./configure --prefix=$PREFIX_DIR && make -s V= && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libnice
  fi
}

install_openssl(){
  if [ -d $LIB_DIR ]; then
    local SSL_VERSION="1.0.2i"
    cd $LIB_DIR
    wget -c http://www.openssl.org/source/openssl-${SSL_VERSION}.tar.gz
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
  MAJOR=1
  MINOR=7
  SOVER=4

  RELNAME=libopenh264-${MAJOR}.${MINOR}.0-linux64.${SOVER}.so

  cd $ROOT/third_party/openh264
  wget -c http://ciscobinary.openh264.org/${RELNAME}.bz2
  bzip2 -d ${RELNAME}.bz2
  mv ${RELNAME} libopenh264.so
  local symbol=$(readelf -d ./libopenh264.so | grep soname | sed 's/.*\[\(.*\)\]/\1/g')
  if [ -f "$symbol" ]; then
    rm -f $symbol
  fi
  ln -s libopenh264.so ${symbol}
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
install_msdk_dispatcher(){
  [ -d $LIB_DIR ] || mkdir -p $LIB_DIR
  mkdir -p ${LIB_DIR}/dispatcher
  pushd ${LIB_DIR}/dispatcher
  cmake -D__ARCH:STRING=intel64 /opt/intel/mediasdk/opensource/mfx_dispatch/
  make
  cp -av __lib/libdispatch_shared.a ${PREFIX_DIR}/lib/libdispatch_shared.a
  popd
}

install_libva(){
  [ -d $LIB_DIR ] || mkdir -p $LIB_DIR
  cd $LIB_DIR
  wget -c https://www.freedesktop.org/software/vaapi/releases/libva/libva-1.7.3.tar.bz2
  tar -jxvf libva-1.7.3.tar.bz2
  cd libva-1.7.3
  ./configure --prefix=$PREFIX_DIR
  make -s V=0
  make uninstall
  make install
  cd $CURRENT_DIR
}

install_libva_driver(){
  [ -d $LIB_DIR ] || mkdir -p $LIB_DIR
  cd $LIB_DIR
  GIT_SSL_NO_VERIFY=1 git clone https://anongit.freedesktop.org/git/vaapi/intel-driver.git -b 1.7.3
  cd intel-driver
  PKG_CONFIG_PATH=$PREFIX_DIR/lib/pkgconfig:$PKG_CONFIG_PATH ./autogen.sh --prefix=$PREFIX_DIR
  LD_LIBRARY_PATH=$PREFIX_DIR/lib:$LD_LIBRARY_PATH make -s V=0
  make uninstall
  make install
  cd $CURRENT_DIR
}

install_libyami(){
  cd $ROOT/third_party/
  rm -rf libyami
  git clone https://github.com/01org/libyami.git
  cd libyami
  PKG_CONFIG_PATH=$PREFIX_DIR/lib/pkgconfig:$PKG_CONFIG_PATH ./autogen.sh --enable-vp8enc --enable-h265dec --enable-h265enc --prefix=$PREFIX_DIR
  make clean
  LD_LIBRARY_PATH=$PREFIX_DIR/lib:$LD_LIBRARY_PATH make -s V=0
  make uninstall
  make install
  cd $CURRENT_DIR
}

install_webrtc(){
  export GIT_SSL_NO_VERIFY=1
  local GIT_ACCOUNT="lab_webrtctest"
  local WOOGEEN_GIT_URL=`git config --get remote.origin.url`
  if [ ! -z "$WOOGEEN_GIT_URL" ]; then
    if echo $WOOGEEN_GIT_URL | grep "@" -q -s; then
      GIT_ACCOUNT=`echo $WOOGEEN_GIT_URL | awk -F '\/\/' '{print $2}' | awk -F '@' '{print $1}'`
    else
      GIT_ACCOUNT=`whoami`
    fi
  fi

  pushd ${ROOT}/third_party/webrtc
  rm -rf src
  git clone -b 59-mcu ssh://${GIT_ACCOUNT}@git-ccr-1.devtools.intel.com:29418/webrtc-webrtcstack src
  ./src/tools-woogeen/install.sh
  ./src/tools-woogeen/build.sh
  popd
}

install_oovoosdk(){
  mkdir -p $PREFIX_DIR/lib
  if [[ "$OS" =~ .*ubuntu.* ]]
  then
    cp -av $ROOT/third_party/liboovoosdk-ubuntu.so $PREFIX_DIR/lib/liboovoosdk.so
  elif [[ "$OS" =~ .*centos.* ]]
  then
    cp -av $ROOT/third_party/liboovoosdk-el.so $PREFIX_DIR/lib/liboovoosdk.so
  fi
}

install_tcmalloc(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget -c https://github.com/gperftools/gperftools/releases/download/gperftools-2.1/gperftools-2.1.tar.gz
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

install_node() {
  local NODE_VERSION="v6.9.5"
  echo -e "\x1b[32mInstalling nvm...\x1b[0m"
  NVM_DIR="${HOME}/.nvm"

  #install nvm
  bash "${PATHNAME}/install_nvm.sh"
  #install node
  [[ -s "${NVM_DIR}/nvm.sh" ]] && . "${NVM_DIR}/nvm.sh"
  echo -e "\x1b[32mInstalling node ${NODE_VERSION}...\x1b[0m"
  if ! grep -qc 'nvm use' ~/.bash_profile; then
   echo -e 'nvm use '${NODE_VERSION} >> ~/.bash_profile
  fi
  nvm install ${NODE_VERSION}
  nvm use ${NODE_VERSION}
}

install_node_tools() {
  npm install -g --loglevel error node-gyp grunt-cli underscore jsdoc
  local GATEWAY_SDK_DIR="${ROOT}/source/client_sdk"
  cd ${GATEWAY_SDK_DIR}
  mkdir -p node_modules && npm install --prefix . --development --loglevel error
}

install_libre() {
  local LIBRE_DIR="${ROOT}/third_party/libre-0.4.16"
  cd "${LIBRE_DIR}" && make clean && make RELEASE=1
}

install_usrsctp() {
  local TP_DIR="${ROOT}/third_party"
  if [ -d $TP_DIR ]; then
    local USRSCTP_VERSION="2f6478eb8d40f1766a96b5b033ed26c0c2244589"
    local USRSCTP_FILE="${USRSCTP_VERSION}.tar.gz"
    local USRSCTP_EXTRACT="usrsctp-${USRSCTP_VERSION}"
    local USRSCTP_URL="https://github.com/sctplab/usrsctp/archive/${USRSCTP_FILE}"

    cd $TP_DIR
    rm -rf usrsctp
    wget -c ${USRSCTP_URL}
    tar -zxvf ${USRSCTP_FILE}
    mv ${USRSCTP_EXTRACT} usrsctp
    rm ${USRSCTP_FILE}

    cd usrsctp
    ./bootstrap
    ./configure
    make
  else
    mkdir -p $TP_DIR
    install_usrsctp
  fi
}

install_gcc(){
  if [ -d $LIB_DIR ]; then
    local VERSION="4.8.4"
    cd $LIB_DIR
    wget -c http://ftp.gnu.org/gnu/gcc/gcc-${VERSION}/gcc-${VERSION}.tar.bz2

    tar jxf gcc-${VERSION}.tar.bz2 ;cd gcc-${VERSION}
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
