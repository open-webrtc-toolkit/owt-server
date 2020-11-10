################################
# OWT WebRTC server
#
# Base image Ubuntu 18.04


FROM ubuntu:18.04 AS owt-build
WORKDIR /home

# COMMON BUILD TOOLS
RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get install -y -q --no-install-recommends build-essential autoconf make git wget pciutils cpio libtool lsb-release ca-certificates pkg-config bison flex libcurl4-gnutls-dev zlib1g-dev nasm yasm m4 autoconf libtool automake cmake libfreetype6-dev libgstreamer-plugins-base1.0-dev


# Build libnice
ARG NICE_VER="0.1.4"
ARG NICE_REPO=http://nice.freedesktop.org/releases/libnice-${NICE_VER}.tar.gz
ARG LIBNICE_PATCH_VER="4.3"
ARG LIBNICE_PATCH_REPO=https://github.com/open-webrtc-toolkit/owt-server/archive/v${LIBNICE_PATCH_VER}.tar.gz

RUN apt-get update && apt-get install -y -q --no-install-recommends libglib2.0-dev

RUN wget -O - ${NICE_REPO} | tar xz && \
    cd libnice-${NICE_VER} && \
    wget -O - ${LIBNICE_PATCH_REPO} | tar xz  && \
    patch -p1 < owt-server-${LIBNICE_PATCH_VER}/scripts/patches/libnice014-agentlock.patch && \
    patch -p1 < owt-server-${LIBNICE_PATCH_VER}/scripts/patches/libnice014-agentlock-plus.patch && \
    patch -p1 < owt-server-${LIBNICE_PATCH_VER}/scripts/patches/libnice014-removecandidate.patch && \
    patch -p1 < owt-server-${LIBNICE_PATCH_VER}/scripts/patches/libnice014-keepalive.patch && \
    patch -p1 < owt-server-${LIBNICE_PATCH_VER}/scripts/patches/libnice014-startcheck.patch && \
    ./configure --prefix="/usr/local" --libdir=/usr/local/lib/x86_64-linux-gnu && \
    make -s V=0 && \
    make install


# Build open ssl
ARG OPENSSL_BASE="1.1.1"
ARG OPENSSL_VER="1.1.1h"
ARG OPENSSL_REPO=https://www.openssl.org/source/openssl-${OPENSSL_VER}.tar.gz

RUN wget -O - ${OPENSSL_REPO} | tar xz && \
    cd openssl-${OPENSSL_VER} && \
    ./config no-ssl3 --prefix=/usr/local/ssl --openssldir=/usr/local/ssl -Wl,-rpath=/usr/local/ssl/lib -fPIC && \
    make depend && \
    make -s V=0  && \
    make install


# Build libre
ARG LIBRE_VER="v0.5.0"
ARG LIBRE_REPO=https://github.com/creytiv/re.git

RUN git clone ${LIBRE_REPO} && \
    cd re && \
    git checkout ${LIBRE_VER} && \
    make SYSROOT_ALT="/usr" RELEASE=1 && \
    make install SYSROOT_ALT="/usr" RELEASE=1 PREFIX="/usr"


# Build usrsctp
ARG USRSCTP_VERSION="30d7f1bd0b58499e1e1f2415e84d76d951665dc8"
ARG USRSCTP_FILE="${USRSCTP_VERSION}.tar.gz"
ARG USRSCTP_EXTRACT="usrsctp-${USRSCTP_VERSION}"
ARG USRSCTP_URL="https://github.com/sctplab/usrsctp/archive/${USRSCTP_FILE}"

RUN wget -O - ${USRSCTP_URL} | tar xz && \
    mv ${USRSCTP_EXTRACT} usrsctp && \
    cd usrsctp && \
    ./bootstrap && \
    ./configure --prefix="/usr/local" --libdir=/usr/local/lib/x86_64-linux-gnu && \
    make && \
    make install


# Build libsrtp2
ARG SRTP2_VER="2.1.0"
ARG SRTP2_REPO=https://codeload.github.com/cisco/libsrtp/tar.gz/v${SRTP2_VER}

RUN apt-get update && apt-get install -y -q --no-install-recommends curl

RUN curl -o libsrtp-${SRTP2_VER}.tar.gz ${SRTP2_REPO} && \
    tar xzf libsrtp-${SRTP2_VER}.tar.gz && \
    cd libsrtp-${SRTP2_VER} && \
    export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig" && \
    export CFLAGS="-fPIC" && \
    ./configure --enable-openssl --prefix="/usr/local" --with-openssl-dir="/usr/local/ssl" && \
    make -s V=0  && \
    make install


# Fetch FFmpeg source
ARG FFMPEG_VER="4.1.3"
ARG FFMPEG_DIR="ffmpeg-${FFMPEG_VER}"
ARG FFMPEG_SRC="${FFMPEG_DIR}.tar.bz2"
ARG FFMPEG_SRC_URL="http://ffmpeg.org/releases/${FFMPEG_SRC}"
ARG FFMPEG_SRC_MD5SUM="9985185a8de3678e5b55b1c63276f8b5"

RUN wget ${FFMPEG_SRC_URL} && tar xf ${FFMPEG_SRC} && mv ${FFMPEG_DIR} FFmpeg && \
    cd FFmpeg  ;
# Compile FFmpeg
RUN cd /home/FFmpeg && \
    export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig" && \
    ./configure --prefix="/usr/local" --libdir=/usr/local/lib/x86_64-linux-gnu --enable-shared --disable-static --disable-libvpx --disable-vaapi --enable-libfreetype && \
    make -j8 && \
    make install


# Install node
ARG NODE_VER=v10.21.0
ARG NODE_REPO=https://nodejs.org/dist/${NODE_VER}/node-${NODE_VER}-linux-x64.tar.xz

RUN wget ${NODE_REPO} && \
    tar xf node-${NODE_VER}-linux-x64.tar.xz && \
    cp node-*/* /usr/local -rf && rm -rf node-*


# Fetch SVT-HEVC
ARG SVT_HEVC_VER=v1.3.0
ARG SVT_HEVC_REPO=https://github.com/intel/SVT-HEVC

RUN git clone ${SVT_HEVC_REPO} && \
    cd SVT-HEVC/Build/linux && \
    export PKG_CONFIG_PATH="/usr/local/lib/x86_64-linux-gnu/pkgconfig" && \
    git checkout ${SVT_HEVC_VER} && \
    mkdir -p ../../Bin/Release && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_INSTALL_LIBDIR=lib/x86_64-linux-gnu -DCMAKE_ASM_NASM_COMPILER=yasm ../.. && \
    make -j8 && \
    make install 


# Install json_hpp
ARG JSON_VER=v3.6.1
ARG JSON_LINK=https://github.com/nlohmann/json/releases/download/${JSON_VER}/json.hpp

RUN wget -c ${JSON_LINK} && mv json.hpp /usr/include/


# Build OWT specific modules
ARG OWTSERVER_REPO=https://github.com/open-webrtc-toolkit/owt-server.git
ARG OPENH264_MAJOR=1
ARG OPENH264_MINOR=7
ARG OPENH264_SOVER=4
ARG OPENH264_SOURCENAME=v${OPENH264_MAJOR}.${OPENH264_MINOR}.0.tar.gz
ARG OPENH264_SOURCE=https://github.com/cisco/openh264/archive/v${OPENH264_MAJOR}.${OPENH264_MINOR}.0.tar.gz
ARG OPENH264_BINARYNAME=libopenh264-${OPENH264_MAJOR}.${OPENH264_MINOR}.0-linux64.${OPENH264_SOVER}.so
ARG OPENH264_BINARY=https://github.com/cisco/openh264/releases/download/v${OPENH264_MAJOR}.${OPENH264_MINOR}.0/${OPENH264_BINARYNAME}.bz2
ARG LICODE_COMMIT="8b4692c88f1fc24dedad66b4f40b1f3d804b50ca"
ARG LICODE_REPO=https://github.com/lynckia/licode.git
ARG WEBRTC_REPO=https://github.com/open-webrtc-toolkit/owt-deps-webrtc.git
ARG SVT_VER=v1.3.0
ARG SVT_REPO=https://github.com/intel/SVT-HEVC.git
ARG SERVER_PATH=/home/owt-server
ARG OWT_SDK_REPO=https://github.com/open-webrtc-toolkit/owt-client-javascript.git
ARG OWT_BRANCH=master
ARG IMG_APP_PATH=/app_data/
ENV APP_PATH=${IMG_APP_PATH}

RUN apt-get update && apt-get install -y -q --no-install-recommends python libglib2.0-dev rabbitmq-server mongodb libboost-thread-dev libboost-system-dev liblog4cxx-dev

# 1. Clone OWT server source code 
# 2. Clone licode source code and patch
# 3. Clone webrtc source code and patch
RUN git config --global user.email "you@example.com" && \
    git config --global user.name "Your Name" && \
    git clone --depth=1 -b ${OWT_BRANCH} ${OWTSERVER_REPO} && \

    # Install node modules for owt
    npm install -g --loglevel error node-gyp@6.1.0 grunt-cli underscore jsdoc && \
    cd owt-server && npm install nan && \

    # Get openh264 for owt
    cd third_party && \
    mkdir openh264 && cd openh264 && \
    wget ${OPENH264_SOURCE} && \
    wget ${OPENH264_BINARY} && \
    tar xzf ${OPENH264_SOURCENAME} openh264-${OPENH264_MAJOR}.${OPENH264_MINOR}.0/codec/api && \
    ln -s -v openh264-${OPENH264_MAJOR}.${OPENH264_MINOR}.0/codec codec && \
    bzip2 -d ${OPENH264_BINARYNAME}.bz2 && \
    ln -s -v ${OPENH264_BINARYNAME} libopenh264.so.${OPENH264_SOVER} && \
    ln -s -v libopenh264.so.${OPENH264_SOVER} libopenh264.so && \
    echo 'const char* stub() {return "this is a stub lib";}' > pseudo-openh264.cpp && \
    gcc pseudo-openh264.cpp -fPIC -shared -o pseudo-openh264.so 

# Get licode for owt
RUN cd ${SERVER_PATH}/third_party && git clone ${LICODE_REPO} && \
    cd licode && \
    git reset --hard ${LICODE_COMMIT} && \
    git am ${SERVER_PATH}/scripts/patches/licode/*.patch

# Install webrtc for owt
RUN cd ${SERVER_PATH}/third_party && mkdir webrtc  && cd webrtc &&\
    export GIT_SSL_NO_VERIFY=1 && \
    git clone -b 59-server ${WEBRTC_REPO} src && \
    ./src/tools-woogeen/install.sh && \
    ./src/tools-woogeen/build.sh 

# Install webrtc79 for owt
RUN mkdir ${SERVER_PATH}/third_party/webrtc-m79 && \
    cd ${SERVER_PATH}/third_party/webrtc-m79 && \
    /bin/bash ${SERVER_PATH}/scripts/installWebrtc.sh

# Get js client sdk for owt
RUN cd /home && git clone --depth=1 -b ${OWT_BRANCH} ${OWT_SDK_REPO} && cd owt-client-javascript/scripts && npm install && grunt  && \
    mkdir ${SERVER_PATH}/third_party/quic-lib && \
    export LD_LIBRARY_PATH=/usr/local/lib/x86_64-linux-gnu && \
    cd ${SERVER_PATH}/third_party/quic-lib && wget https://github.com/open-webrtc-toolkit/owt-deps-quic/releases/download/v0.1/dist.tgz && tar xzf dist.tgz && \
    #Build and pack owt
    sed -i "/lssl/i\'\-L\/usr\/local\/ssl\/lib\'," /home/owt-server/source/agent/webrtc/rtcConn/binding.gyp && \
    sed -i "/lssl/i\'\-L\/usr\/local\/ssl\/lib\'," /home/owt-server/source/agent/webrtc/rtcFrame/binding.gyp && \
    cd ${SERVER_PATH} && export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig && ./scripts/build.js -t mcu-all -r -c && \
    ./scripts/pack.js -t all --install-module --no-pseudo --app-path /home/owt-client-javascript/dist/samples/conference


FROM ubuntu:18.04 AS owt-run
LABEL Description="This is the image for building OWT on Ubuntu 18.04"
LABEL Vendor="Intel Corporation"
WORKDIR /home

# Prerequisites
# Install node
ARG NODE_VER=v10.21.0
ARG NODE_REPO=https://nodejs.org/dist/${NODE_VER}/node-${NODE_VER}-linux-x64.tar.xz

COPY --from=owt-build /home/owt-server/dist /home/owt
COPY startowt.sh /home/

RUN apt-get update && apt-get install -y -q --no-install-recommends ca-certificates wget xz-utils rabbitmq-server mongodb libboost-system-dev libboost-thread-dev liblog4cxx-dev libglib2.0-0 libfreetype6-dev curl

RUN wget ${NODE_REPO} && \
    tar xf node-${NODE_VER}-linux-x64.tar.xz && \
    cp node-*/* /usr/local -rf && rm -rf node-*

ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib/x86_64-linux-gnu
