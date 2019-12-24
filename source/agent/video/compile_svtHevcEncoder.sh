#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# SVT HEVC Encoder Library Install Script

this=$(pwd)
DOWNLOAD_DIR="${this}/svt_hevc_encoder"
PREFIX_DIR="${DOWNLOAD_DIR}/install"

install_svt_hevc(){
    pushd $DOWNLOAD_DIR
    rm -rf SVT-HEVC
    git clone https://github.com/intel/SVT-HEVC.git

    pushd SVT-HEVC
    git checkout v1.4.3

    mkdir build
    pushd build >/dev/null
    cmake -DCMAKE_C_FLAGS="-std=gnu99" -DCMAKE_INSTALL_PREFIX=${PREFIX_DIR} ..
    make && make install
    popd >/dev/null

    popd >/dev/null
    popd >/dev/null
}

[[ ! -d ${DOWNLOAD_DIR} ]] && mkdir ${DOWNLOAD_DIR};
pushd ${DOWNLOAD_DIR}
install_svt_hevc
popd
