#!/bin/bash
#
# Copyright 2016 Intel Corporation All Rights Reserved.
#
# The source code contained or described herein and all documents related to the
# source code ("Material") are owned by Intel Corporation or its suppliers or
# licensors. Title to the Material remains with Intel Corporation or its suppliers
# and licensors. The Material contains trade secrets and proprietary and
# confidential information of Intel or its suppliers and licensors. The Material
# is protected by worldwide copyright and trade secret laws and treaty provisions.
# No part of the Material may be used, copied, reproduced, modified, published,
# uploaded, posted, transmitted, distributed, or disclosed in any way without
# Intel's prior express written permission.
#  *
# No license under any patent, copyright, trade secret or other intellectual
# property right is granted to or conferred upon you by disclosure or delivery of
# the Materials, either expressly, by implication, inducement, estoppel or
# otherwise. Any license under such intellectual property rights must be express
# and approved by Intel in writing.
#

this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/.."; pwd`
SOURCE="${ROOT}/source"

usage() {
  echo
  echo "WooGeen Build Script"
  echo "Usage:"
  echo "    --release (default)                 build in release mode"
  echo "    --debug                             build in debug mode"
  echo "    --rebuild                           rebuild from scratch"
  echo "    --check                             check resulted addon(s)"
  echo "    --gateway                           build oovoo gateway addon"
  echo "    --mcu                               build mcu runtime addons with software media pipeline"
  echo "    --mcu-hardware                      build mcu runtime addons with msdk media pipeline"
  echo "    --mcu-hardware-yami                 build mcu runtime addons with libyami based media pipeline"
  echo "    --mcu-all                           build mcu runtime addons both with and without hardware support"
  echo "    --sip                               build sip gateway runtime"
  echo "    --sdk                               build sdk (for oovoo gateway)"
  echo "    --all                               build all components"
  echo "    --help                              print this help"
  echo "Example:"
  echo "    --release --all                     build all components in release mode"
  echo "    --debug --mcu                       build mcu in debug mode"
  echo
}

if [[ $# -eq 0 ]];then
  usage
  exit 1
fi

BUILD_GATEWAY_RUNTIME=false
BUILD_SIP_GATEWAY_RUNTIME=false
BUILD_MCU_RUNTIME_SW=false
BUILD_MCU_RUNTIME_HW_YAMI=false
BUILD_MCU_RUNTIME_HW_MSDK=false
BUILD_SDK=false
BUILDTYPE="Release"
BUILD_ROOT="${ROOT}/build"
CHECK_ADDONS=false
REBUILD=false
DEPS_ROOT="${ROOT}/build/libdeps/build"

shopt -s extglob
while [[ $# -gt 0 ]]; do
  case $1 in
    *(-)release )
      BUILDTYPE="Release"
      ;;
    *(-)debug )
      BUILDTYPE="Debug"
      ;;
    *(-)check )
      CHECK_ADDONS=true
      ;;
    *(-)rebuild )
      REBUILD=true
      ;;
    *(-)all )
      BUILD_GATEWAY_RUNTIME=true
      BUILD_MCU_RUNTIME_SW=true
      BUILD_MCU_RUNTIME_HW_MSDK=true
      BUILD_SDK=true
      ;;
    *(-)gateway )
      BUILD_GATEWAY_RUNTIME=true
      ;;
    *(-)mcu )
      BUILD_MCU_RUNTIME_SW=true
      ;;
    *(-)mcu-hardware )
      BUILD_MCU_RUNTIME_HW_MSDK=true
      ;;
    *(-)mcu-hardware-yami )
      BUILD_MCU_RUNTIME_HW_YAMI=true
      ;;
    *(-)mcu-all )
      BUILD_MCU_RUNTIME_SW=true
      BUILD_MCU_RUNTIME_HW_MSDK=true
      ;;
    *(-)sip )
      BUILD_SIP_GATEWAY_RUNTIME=true
      ;;
    *(-)sdk )
      BUILD_SDK=true
      ;;
    *(-)help )
      usage
      exit 0
      ;;
    * )
      echo -e "\x1b[33mUnknown argument\x1b[0m: $1"
      ;;
  esac
  shift
done

build_gateway_runtime() {
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/gateway"
  build_runtime
}

build_mcu_runtime() {
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/agent"
  build_runtime
}

build_mcu_runtime_sw() {
  cp -f ${SOURCE}/agent/video/videoMixer_sw/binding.sw.gyp ${SOURCE}/agent/video/videoMixer_sw/binding.gyp
  build_mcu_runtime
  rm -f ${SOURCE}/agent/video/videoMixer_sw/binding.gyp
}

build_mcu_runtime_hw_yami() {
  cp -f ${SOURCE}/agent/video/videoMixer_hw/binding.hw.yami.gyp ${SOURCE}/agent/video/videoMixer_hw/binding.gyp
  build_mcu_runtime
  rm -f ${SOURCE}/agent/video/videoMixer_hw/binding.gyp
}

build_mcu_runtime_hw_msdk() {
  cp -f ${SOURCE}/agent/video/videoMixer_hw/binding.hw.msdk.gyp ${SOURCE}/agent/video/videoMixer_hw/binding.gyp
  build_mcu_runtime
  rm -f ${SOURCE}/agent/video/videoMixer_hw/binding.gyp
}

build_runtime() {
  local CORE_HOME="${SOURCE}/core"
  local CCOMPILER=${DEPS_ROOT}/bin/gcc
  local CXXCOMPILER=${DEPS_ROOT}/bin/g++
  local OPTIMIZATION_LEVEL="3"
  [[ BUILDTYPE == "Release" ]] || OPTIMIZATION_LEVEL="0"

  # runtime addon
  local NODE_VERSION=
  ADDON_LIST=$(find ${RUNTIME_ADDON_SRC_DIR} -type f -name "binding.gyp")
  [[ ${ADDON_LIST} =~ "oovoo_gateway" ]] &&
    NODE_VERSION=v$(node -e "process.stdout.write(require('${ROOT}/scripts/release/package.gw.json').engine.node)") ||
    NODE_VERSION=v$(node -e "process.stdout.write(require('${ROOT}/scripts/release/package.mcu.json').engine.node)")
  if [[ ${NODE_VERSION} == $(node --version) ]] && hash node-gyp 2>/dev/null; then
    for i in ${ADDON_LIST}; do
      local ADDON=$(dirname "$i")
      echo -e "building addon \e[32m$(basename ${ADDON})\e[0m"
      pushd ${ADDON} >/dev/null
      if [[ -x ${CCOMPILER} && -x ${CXXCOMPILER} ]]; then
        if ${REBUILD} ; then
          CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} CC=${CCOMPILER} CXX=${CXXCOMPILER} node-gyp rebuild --loglevel=error
        else
          CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} CC=${CCOMPILER} CXX=${CXXCOMPILER} node-gyp configure --loglevel=error
          CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} CC=${CCOMPILER} CXX=${CXXCOMPILER} node-gyp build --loglevel=error
        fi
      else
        if ${REBUILD} ; then
          CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} node-gyp rebuild --loglevel=error
        else
          CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} node-gyp configure --loglevel=error
          CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} node-gyp build --loglevel=error
        fi
      fi
      popd >/dev/null
    done
    [[ ${CHECK_ADDONS} ]] && node ${ROOT}/scripts/module_test.js ${RUNTIME_ADDON_SRC_DIR}
  else
    echo >&2 "You need to install Node.js ${NODE_VERSION} toolchain:"
    echo >&2 "  nvm install ${NODE_VERSION}"
    echo >&2 "  npm install -g node-gyp"
    echo >&2 "  node-gyp install ${NODE_VERSION}"
    return 1
  fi
}

build_sip_ua() {
  local CORE_HOME="${SOURCE}/core"
  local CCOMPILER=${DEPS_ROOT}/bin/gcc
  local CXXCOMPILER=${DEPS_ROOT}/bin/g++
  local OPTIMIZATION_LEVEL="3"
  local RUNTIME_LIB_SRC_DIR="${SOURCE}/core"
  mkdir -p "${RUNTIME_LIB_SRC_DIR}/build"
  [[ BUILDTYPE == "Release" ]] || OPTIMIZATION_LEVEL="0"

  [[ BUILDTYPE == "Release" ]] || OPTIMIZATION_LEVEL="0"
  pushd "${RUNTIME_LIB_SRC_DIR}/build" >/dev/null
  if [[ -x $CCOMPILER && -x $CXXCOMPILER ]]; then
    LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} CC=$CCOMPILER CXX=$CXXCOMPILER cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
  else
    LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
  fi
  LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH make
}

build_sip_gateway_runtime() {
  CMAKE_ADDITIONAL_OPTIONS="-DCOMPILE_SIP_GATEWAY=ON"
  pushd "${SOURCE}/core/sip_gateway/sipua" > /dev/null
  make clean && make
  popd >/dev/null
  build_sip_ua

  # Sip addon build
  pushd "${SOURCE}/agent/sip/sipIn" >/dev/null
  cp -f binding.sip.gyp binding.gyp
  popd >/dev/null

  RUNTIME_ADDON_SRC_DIR="${SOURCE}/agent/sip"
  build_runtime

  pushd "${SOURCE}/agent/sip/sipIn" >/dev/null
  rm -f binding.gyp
  popd >/dev/null
}

build_oovoo_client_sdk() {
  mkdir -p "${BUILD_ROOT}/sdk"
  local CLIENTSDK_DIR="${SOURCE}/client_sdk"
  rm -f ${BUILD_ROOT}/sdk/*.js
  rm -f ${CLIENTSDK_DIR}/dist/*.js
  cd ${CLIENTSDK_DIR}
  grunt --force
  cp -av ${CLIENTSDK_DIR}/dist/*.js ${BUILD_ROOT}/sdk/
}

build() {
  export CFLAGS="-fstack-protector -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security"
  export CXXFLAGS=$CFLAGS
  #export LDFLAGS="-z noexecstack -z relro -z now"
  export LDFLAGS="-z noexecstack -z relro"

  local DONE=0
  # Job
  if ${BUILD_GATEWAY_RUNTIME} ; then
    build_gateway_runtime
    ((DONE++))
  fi
  if ${BUILD_MCU_RUNTIME_SW} ; then
    build_mcu_runtime_sw
    ((DONE++))
  fi
  if ${BUILD_MCU_RUNTIME_HW_MSDK} ; then
    build_mcu_runtime_hw_msdk
    ((DONE++))
  fi
  if ${BUILD_MCU_RUNTIME_HW_YAMI} ; then
    build_mcu_runtime_hw_yami
    ((DONE++))
  fi
  if ${BUILD_SDK} ; then
    build_oovoo_client_sdk
    ((DONE++))
  fi
  if [[ ${DONE} -eq 0 ]]; then
    usage
    return 1
  fi
}



if ${BUILD_SIP_GATEWAY_RUNTIME} ; then
  build_sip_gateway_runtime
  ((DONE++))
fi

build
