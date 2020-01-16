#!/bin/bash -ex

SCRIPT=`pwd`/$0
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..

BUILD_DIR=$ROOT/build
LIB_DIR=$BUILD_DIR/libdeps
PREFIX_DIR=$LIB_DIR/build/

install_libxcam() {
    if [ -d $LIB_DIR ]; then
        pushd $LIB_DIR >/dev/null
        rm -rf libxcam
        rm -f ./build/lib/libxcam_*.*

        # clone
        git clone -b im360 https://github.com/intel/libxcam

        # build
        pushd libxcam >/dev/null

        export PKG_CONFIG_PATH=$PREFIX_DIR/lib/pkgconfig:$PKG_CONFIG_PATH
        ./autogen.sh && ./configure CXXFLAGS="-g -O3" CFLAGS="-g -O3" --prefix=$PREFIX_DIR --enable-avx512 --enable-opencv --enable-profiling

        make -j$(nproc) && make install
        popd >/dev/null
        popd >/dev/null
    else
        mkdir -p $LIB_DIR
        install_libxcam
    fi
}

install_opencv() {
    local VERSION="3.4.7"
    if [ -d $LIB_DIR ]; then
        pushd $LIB_DIR >/dev/null

        if [ ! -d $LIB_DIR/opencv-${VERSION} ]; then
            rm -rf opencv-*
            rm -f ./build/lib/libopencv_*.*
            wget -O opencv-${VERSION}.tar.gz https://github.com/opencv/opencv/archive/${VERSION}.tar.gz
            tar -zxf opencv-${VERSION}.tar.gz
            pushd opencv-${VERSION} >/dev/null
            mkdir build
            pushd build >/dev/null
            cmake -D CMAKE_INSTALL_PREFIX=$PREFIX_DIR -D CMAKE_BUILD_TYPE=RELEASE -D WITH_IPP=OFF ../
            make -j$(nproc) && make install
            popd >/dev/null
            popd >/dev/null
        fi

        popd >/dev/null
    else
        mkdir -p $LIB_DIR
        install_opencv
    fi
}

install_opencv
install_libxcam
