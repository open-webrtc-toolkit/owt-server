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

install_srt(){
  local VERSION="1.4.1"
  local SRC="v${VERSION}.tar.gz"
  local SRC_URL=" https://github.com/Haivision/srt/archive/${SRC}"
  local SRC_DIR="srt-${VERSION}"

  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libsrt.* 2>/dev/null`
  [ "$INCR_INSTALL" = true ]  && [[ ! -z $LIST_LIBS ]] && \
  echo "srt already installed." && return 0

  if [ "$CHECK_INSTALL" = true ]; then
    if [[ ! -z $LIST_LIBS ]]; then
      echo "srt - Yes"
    else
      echo "srt - No"
    fi
    return 0
  fi

  mkdir -p ${LIB_DIR}
  pushd ${LIB_DIR}
  wget ${SRC_URL}
  rm -fr ${SRC_DIR}
  tar xf ${SRC}
  pushd ${SRC_DIR}
  ./configure --prefix=${PREFIX_DIR}
  make && make install
}

install_fdkaac(){
  local VERSION="0.1.6"
  local SRC="fdk-aac-${VERSION}.tar.gz"
  local SRC_URL="http://sourceforge.net/projects/opencore-amr/files/fdk-aac/${SRC}/download"
  local SRC_MD5SUM="13c04c5f4f13f4c7414c95d7fcdea50f"

  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libfdk* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "fdkaac already installed." && return 0

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
  ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static
  make -j4 -s V=0 && make install
  popd
  popd
}

install_ffmpeg(){
  local VERSION="4.1.3"
  local DIR="ffmpeg-${VERSION}"
  local SRC="${DIR}.tar.bz2"
  local SRC_URL="http://ffmpeg.org/releases/${SRC}"
  local SRC_MD5SUM="9985185a8de3678e5b55b1c63276f8b5"

  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libav* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "ffmpeg already installed." && return 0

  mkdir -p ${LIB_DIR}
  pushd ${LIB_DIR}
  [[ ! -s ${SRC} ]] && wget -c ${SRC_URL}
  if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
      echo "Downloaded file ${SRC} is corrupted."
      rm -v ${SRC}
      return 1
  fi
  rm -fr ${DIR}
  tar xf ${SRC}
  pushd ${DIR}
  [[ "${DISABLE_NONFREE}" == "true" ]] && \
  PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libfreetype --enable-libsrt || \
  PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=${PREFIX_DIR} --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libfreetype --enable-libfdk-aac --enable-nonfree --enable-libsrt && \
  make -j4 -s V=0 && make install
  popd
  popd
}

install_zlib() {
  local VERSION="1.2.12"

  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libz* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "zlib already installed." && return 0

  if [ -d $LIB_DIR ]; then
    pushd $LIB_DIR >/dev/null
    rm -rf zlib-*
    rm -f ./build/lib/zlib.*
    wget -c https://zlib.net/zlib-${VERSION}.tar.gz
    tar -zxf zlib-${VERSION}.tar.gz
    pushd zlib-${VERSION} >/dev/null
    ./configure --prefix=$PREFIX_DIR
    make && make install
    popd >/dev/null
    popd >/dev/null
  else
    mkdir -p $LIB_DIR
    install_zlib
  fi
}

#libnice depends on zlib
install_libnice0114(){
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libnice* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "libnice already installed." && return 0

  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -f ./build/lib/libnice.*
    rm -rf libnice-0.1.*
    wget -c http://nice.freedesktop.org/releases/libnice-0.1.14.tar.gz
    tar -zxvf libnice-0.1.14.tar.gz
    cd libnice-0.1.14
    #patch -p1 < $PATHNAME/patches/libnice-0114.patch
    #patch -p1 < $PATHNAME/patches/libnice-0001-Remove-lock.patch
    PKG_CONFIG_PATH=$PREFIX_DIR"/lib/pkgconfig":$PREFIX_DIR"/lib64/pkgconfig":$PKG_CONFIG_PATH ./configure --prefix=$PREFIX_DIR && make -s V= && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libnice0114
  fi
}

#libnice depends on zlib
install_libnice014(){
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libnice* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "libnice already installed." && return 0

  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -f ./build/lib/libnice.*
    rm -rf libnice-0.1.*
    wget -c http://nice.freedesktop.org/releases/libnice-0.1.4.tar.gz
    tar -zxvf libnice-0.1.4.tar.gz
    cd libnice-0.1.4
    patch -p1 < $PATHNAME/patches/libnice014-agentlock.patch
    patch -p1 < $PATHNAME/patches/libnice014-agentlock-plus.patch
    patch -p1 < $PATHNAME/patches/libnice014-removecandidate.patch
    patch -p1 < $PATHNAME/patches/libnice014-keepalive.patch
    patch -p1 < $PATHNAME/patches/libnice014-startcheck.patch
    patch -p1 < $PATHNAME/patches/libnice014-closelock.patch
    PKG_CONFIG_PATH=$PREFIX_DIR"/lib/pkgconfig":$PREFIX_DIR"/lib64/pkgconfig":$PKG_CONFIG_PATH ./configure --prefix=$PREFIX_DIR && make -s V= && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libnice014
  fi
}

install_openssl(){
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libssl* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "openssl already installed." && return 0

  if [ -d $LIB_DIR ]; then
    local SSL_BASE_VERSION="1.1.1"
    local SSL_VERSION="1.1.1j"
    cd $LIB_DIR
    rm -f ./build/lib/libssl.*
    rm -f ./build/lib/libcrypto.*
    rm -rf openssl-1*

    wget -c https://www.openssl.org/source/openssl-${SSL_VERSION}.tar.gz
    tar xf openssl-${SSL_VERSION}.tar.gz
    cd openssl-${SSL_VERSION}
    ./config no-ssl3 --prefix=$PREFIX_DIR -fPIC --libdir=lib
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
  local LIST_LIBS=`ls ${ROOT}/third_party/openh264/libopenh264* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "openh264 already installed." && return 0

  MAJOR=1
  MINOR=7
  SOVER=4

  rm $ROOT/third_party/openh264 -rf
  mkdir $ROOT/third_party/openh264

  cd $ROOT/third_party/openh264

  # license
  wget https://www.openh264.org/BINARY_LICENSE.txt

  SOURCE=v${MAJOR}.${MINOR}.0.tar.gz
  BINARY=libopenh264-${MAJOR}.${MINOR}.0-linux64.${SOVER}.so

  # download
  wget https://github.com/cisco/openh264/archive/${SOURCE}
  wget -c https://github.com/cisco/openh264/releases/download/v${MAJOR}.${MINOR}.0/${BINARY}.bz2

  # api
  tar -zxf ${SOURCE} openh264-${MAJOR}.${MINOR}.0/codec/api
  ln -s -v openh264-${MAJOR}.${MINOR}.0/codec codec

  # binary
  bzip2 -d ${BINARY}.bz2
  ln -s -v ${BINARY} libopenh264.so.${SOVER}
  ln -s -v libopenh264.so.${SOVER} libopenh264.so

  # pseudo lib
  echo \
      'const char* stub() {return "this is a stub lib";}' \
      > pseudo-openh264.cpp
  gcc pseudo-openh264.cpp -fPIC -shared -o pseudo-openh264.so

  cd $CURRENT_DIR
}

install_libexpat() {
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libexpat* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "libexpat already installed." && return 0

  if [ -d $LIB_DIR ]; then
    local VERSION="2.2.6"
    local DURL="https://github.com/libexpat/libexpat/releases/download/R_2_2_6/expat-${VERSION}.tar.bz2"
    pushd ${LIB_DIR} >/dev/null
    rm -rf expat-*
    rm -f ./build/lib/libexpat.*
    wget -c $DURL
    tar jxf expat-${VERSION}.tar.bz2
    pushd expat-${VERSION} >/dev/null
    ./configure --prefix=${PREFIX_DIR} --with-docbook --without-xmlwf
    make && make install
    popd >/dev/null
    popd >/dev/null
  else
    mkdir -p $LIB_DIR
    install_libexpat
  fi
}

install_webrtc79(){
  $INCR_INSTALL &&  [[ -s $ROOT/third_party/webrtc-m79/libwebrtc.a ]] && \
  echo "libwebrtc already installed." && return 0

  [[ ! -d $ROOT/third_party/webrtc-m79 ]] && \
    mkdir $ROOT/third_party/webrtc-m79

  pushd ${ROOT}/third_party/webrtc-m79 >/dev/null
  . $PATHNAME/installWebrtc.sh
  popd
}

install_webrtc(){
  $INCR_INSTALL &&  [[ -s $ROOT/third_party/webrtc/libwebrtc.a ]] && \
  echo "libwebrtc already installed." && return 0

  export GIT_SSL_NO_VERIFY=1
  local GIT_ACCOUNT="lab_webrtctest"
  local OWT_GIT_URL=`git config --get remote.origin.url`
  if [ ! -z "$OWT_GIT_URL" ]; then
    if echo $OWT_GIT_URL | grep "@" -q -s; then
      GIT_ACCOUNT=`echo $OWT_GIT_URL | awk -F '\/\/' '{print $2}' | awk -F '@' '{print $1}'`
    else
      GIT_ACCOUNT=`whoami`
    fi
  fi

  rm $ROOT/third_party/webrtc -rf
  mkdir $ROOT/third_party/webrtc
  
  if [[ "$OS" =~ .*ubuntu.* ]] && [[ "$OS_VERSION" =~ 20.04.* ]]; then
    ${SUDO} update-alternatives --install /usr/bin/python python /usr/bin/python2 1
  fi

  pushd ${ROOT}/third_party/webrtc
  git clone -b 59-server https://github.com/open-webrtc-toolkit/owt-deps-webrtc.git src
  ./src/tools-woogeen/install.sh
  ./src/tools-woogeen/build.sh
  popd

  install_webrtc79

  if [[ "$OS" =~ .*ubuntu.* ]] && [[ "$OS_VERSION" =~ 20.04.* ]]; then
    ${SUDO} update-alternatives --remove python /usr/bin/python2
  fi
}

install_licode(){
  $INCR_INSTALL && [[ -d ${ROOT}/third_party/licode ]] && echo "licode already installed." && return 0

  local COMMIT="8b4692c88f1fc24dedad66b4f40b1f3d804b50ca" #v6
  local LINK_PATH="$ROOT/source/agent/webrtc/webrtcLib"
  pushd ${ROOT}/third_party >/dev/null
  rm -rf licode
  git clone https://github.com/lynckia/licode.git
  pushd licode >/dev/null
  git reset --hard $COMMIT

  local GIT_EMAIL=`git config --get user.email`
  local GIT_USER=`git config --get user.name`
  [[ -z $GIT_EMAIL ]] && git config --global user.email "you@example.com"
  [[ -z $GIT_USER ]] && git config --global user.name "Your Name"

  # APPLY PATCH
  git am $PATHNAME/patches/licode/*.patch

  popd >/dev/null
  popd >/dev/null
}

install_quic(){
  # QUIC IO
  rm $ROOT/third_party/quic-lib -rf
  mkdir $ROOT/third_party/quic-lib

  pushd ${ROOT}/third_party/quic-lib
  wget https://github.com/open-webrtc-toolkit/owt-deps-quic/releases/download/v0.1/dist.tgz
  tar xzf dist.tgz
  popd

  # QUIC transport
  local QUIC_SDK_VERSION=`cat ${ROOT}/source/agent/addons/quic/quic_sdk_version`
  local QUIC_TRANSPORT_PATH=${ROOT}/third_party/quic-transport
  local QUIC_HEADERS_DIR=${ROOT}/build/libdeps/build/include/owt
  if [ -d ${QUIC_TRANSPORT_PATH} ]; then
    rm -r ${QUIC_TRANSPORT_PATH}
  fi
  mkdir ${QUIC_TRANSPORT_PATH}
  pushd ${QUIC_TRANSPORT_PATH}
  wget ${QUIC_TRANSPORT_PACKAGE_URL_PREFIX}/linux/${QUIC_SDK_VERSION}.zip
  if [ $? -eq 0 ]; then
    unzip ${QUIC_SDK_VERSION}.zip
    rm ${QUIC_SDK_VERSION}.zip
    cp bin/release/libowt_quic_transport.so ${ROOT}/build/libdeps/build/lib
    if [ -d ${QUIC_HEADERS_DIR} ]; then
      rm -r ${QUIC_HEADERS_DIR}
    fi
    cp -r include/owt ${QUIC_HEADERS_DIR}
  else
    read -p "Failed to download prebuild QUIC SDK. Please download and compile QUIC SDK version ${QUIC_SDK_VERSION} from https://github.com/open-webrtc-toolkit/owt-deps-quic."
  fi
  popd
}

install_nicer(){
  local COMMIT="24d88e95e18d7948f5892d04589acce3cc9a5880"
  pushd ${ROOT}/third_party >/dev/null
  rm -rf nICEr
  git clone https://github.com/lynckia/nICEr.git
  pushd nICEr >/dev/null
  git reset --hard $COMMIT
  cmake -DCMAKE_C_FLAGS="-std=c99" -DCMAKE_INSTALL_PREFIX:PATH=${ROOT}/third_party/nICEr/out
  make && make install
  popd >/dev/null
  popd >/dev/null
}

install_libsrtp2(){
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libsrtp2* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "libsrtp2 already installed." && return 0

  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -rf libsrtp-2.1.0
    curl -o libsrtp-2.1.0.tar.gz https://codeload.github.com/cisco/libsrtp/tar.gz/v2.1.0
    tar -zxvf libsrtp-2.1.0.tar.gz
    cd libsrtp-2.1.0
    CFLAGS="-fPIC" ./configure --enable-openssl --prefix=$PREFIX_DIR --with-openssl-dir=$PREFIX_DIR
    make $FAST_MAKE -s V=0 && make uninstall && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_libsrtp2
  fi
}

install_node() {
  local NODE_VERSION="v10.21.0"
  echo -e "\x1b[32mInstalling nvm...\x1b[0m"
  NVM_DIR="${HOME}/.nvm"

  $INCR_INSTALL && [[ -s "${NVM_DIR}/nvm.sh" ]] && \
  echo "node already installed." && return 0

  #install nvm
  bash "${PATHNAME}/install_nvm.sh"
  #install node
  [[ -s "${NVM_DIR}/nvm.sh" ]] && . "${NVM_DIR}/nvm.sh"
  echo -e "\x1b[32mInstalling node ${NODE_VERSION}...\x1b[0m"
  nvm install ${NODE_VERSION}
  nvm use ${NODE_VERSION}
}

install_node_tools() {
  if [ "${INCR_INSTALL}" == "true" ]; then
    npm list -g node-gyp > /dev/null 2>&1
    [ $? -eq 0 ] && echo "node tools already installed." && return 0
  fi

  check_proxy
  npm install -g --loglevel error node-gyp@6.1.0 grunt-cli underscore jsdoc
  pushd ${ROOT} >/dev/null
  npm install nan@2.11.1
  pushd ${ROOT}/node_modules/nan >/dev/null
  patch -p1 < $PATHNAME/patches/nan.patch
  popd >/dev/null
  popd >/dev/null
}

# libre depends on openssl
install_libre() {
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libre* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "libre already installed." && return 0

  if [ -d $LIB_DIR ]; then
    pushd ${LIB_DIR} >/dev/null
    rm -rf re
    git clone https://github.com/creytiv/re.git
    pushd re >/dev/null
    git checkout v0.5.0
    make SYSROOT_ALT=${PREFIX_DIR} RELEASE=1
    make install SYSROOT_ALT=${PREFIX_DIR} RELEASE=1 PREFIX=${PREFIX_DIR}
    popd >/dev/null
    popd >/dev/null
  else
    mkdir -p $LIB_DIR
    install_libre
  fi
}

install_usrsctp() {
  local LIST_LIBS=`ls ${PREFIX_DIR}/lib/libusrsctp* 2>/dev/null`
  $INCR_INSTALL && [[ ! -z $LIST_LIBS ]] && echo "usrsctp already installed." && return 0

  if [ -d $LIB_DIR ]; then
    local USRSCTP_VERSION="30d7f1bd0b58499e1e1f2415e84d76d951665dc8"
    local USRSCTP_FILE="${USRSCTP_VERSION}.tar.gz"
    local USRSCTP_EXTRACT="usrsctp-${USRSCTP_VERSION}"
    local USRSCTP_URL="https://github.com/sctplab/usrsctp/archive/${USRSCTP_FILE}"

    cd $LIB_DIR
    rm -rf usrsctp
    wget -c ${USRSCTP_URL}
    tar -zxvf ${USRSCTP_FILE}
    mv ${USRSCTP_EXTRACT} usrsctp
    rm ${USRSCTP_FILE}

    cd usrsctp
    ./bootstrap
    ./configure --prefix=$PREFIX_DIR 
    make && make install
  else
    mkdir -p $LIB_DIR
    install_usrsctp
  fi
}

install_glib() {
  if [ -d $LIB_DIR ]; then
    local VERSION="2.54.1"
    cd $LIB_DIR
    wget -c https://github.com/GNOME/glib/archive/${VERSION}.tar.gz -O glib-${VERSION}.tar.gz

    tar -xvzf glib-${VERSION}.tar.gz ;cd glib-${VERSION}
    ./autogen.sh --enable-libmount=no --prefix=${PREFIX_DIR}
    LD_LIBRARY_PATH=${PREFIX_DIR}/lib:$LD_LIBRARY_PATH make && make install
  else
    mkdir -p $LIB_DIR
    install_glib
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

install_json_hpp() {
  if [ -d $LIB_DIR ]; then
    local DOWNLOAD_JSON_LINK="https://github.com/nlohmann/json/releases/download/v3.6.1/json.hpp"
    pushd $LIB_DIR >/dev/null
    wget -c ${DOWNLOAD_JSON_LINK}
    mkdir -p ${PREFIX_DIR}/include
    mv json.hpp ${PREFIX_DIR}/include/
    popd
  else
    mkdir -p $LIB_DIR
    install_json_hpp
  fi
}

install_svt_hevc(){
    pushd $ROOT/third_party >/dev/null
    rm -rf SVT-HEVC
    git clone https://github.com/intel/SVT-HEVC.git

    pushd SVT-HEVC >/dev/null
    git checkout v1.3.0

    mkdir build
    pushd build >/dev/null
    cmake -DCMAKE_C_FLAGS="-std=gnu99" -DCMAKE_INSTALL_PREFIX=${PREFIX_DIR} ..
    make && make install
    popd >/dev/null

    # pseudo lib
    echo \
        'const char* stub() {return "this is a stub lib";}' \
        > pseudo-svtHevcEnc.cpp
    gcc pseudo-svtHevcEnc.cpp -fPIC -shared -o pseudo-svtHevcEnc.so

    popd >/dev/null
    popd >/dev/null
}

cleanup_common(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -r openssl*
    rm -r libnice*
    rm -r libav*
    rm -r libvpx*
    rm -f gcc*
    rm -f libva-utils*
    cd $CURRENT_DIR
  fi
}

install_boost(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget -c http://iweb.dl.sourceforge.net/project/boost/boost/1.65.0/boost_1_65_0.tar.bz2
    tar xvf boost_1_65_0.tar.bz2
    cd boost_1_65_0
    chmod +x bootstrap.sh
    ./bootstrap.sh
    ./b2 && ./b2 install --prefix=$PREFIX_DIR
  else
    mkdir -p $LIB_DIR
    install_boost
  fi
}
