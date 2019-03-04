#!/usr/bin/env bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# OpenH264 Library Install Script

this=$(dirname "$0")
this=$(cd "${this}"; pwd)

echo -e "\x1b[32mOpenH264 Video Codec provided by Cisco Systems, Inc.\x1b[0m"

MAJOR=1
MINOR=7
SOVER=4

RELNAME=libopenh264-${MAJOR}.${MINOR}.0-linux64.${SOVER}.so
SONAME=libopenh264.so.${SOVER}

download_openh264(){
  echo "Download OpenH264..."
  wget -c https://github.com/cisco/openh264/releases/download/v${MAJOR}.${MINOR}.0/${RELNAME}.bz2 && \
  bzip2 -d ${RELNAME}.bz2 && \
  echo "Download ${RELNAME} success."
}

enable_openh264() {
  [ -f ${this}/lib/dummyopenh264.so ] || mv ${this}/lib/${SONAME} ${this}/lib/dummyopenh264.so
  mv ${RELNAME} ${this}/lib/${SONAME} && \
  echo "OpenH264 install finished."
}

download_openh264 && enable_openh264
