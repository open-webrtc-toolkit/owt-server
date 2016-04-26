#!/bin/bash

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
  echo "    --check                             check resulted addon(s)"
  echo "    --gateway                           build oovoo gateway addon"
  echo "    --mcu (software)                    build mcu runtime addons without msdk"
  echo "    --mcu-hardware                      build mcu runtime addons with msdk"
  echo "    --mcu-all                           build mcu runtime addons both with and without msdk"
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
BUILD_MCU_RUNTIME_SW=false
BUILD_MCU_RUNTIME_HW=false
BUILD_SDK=false
BUILDTYPE="Release"
BUILD_ROOT="${ROOT}/build"
CHECK_ADDONS=false
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
    *(-)all )
      BUILD_GATEWAY_RUNTIME=true
      BUILD_MCU_RUNTIME_SW=true
      BUILD_MCU_RUNTIME_HW=true
      BUILD_SDK=true
      ;;
    *(-)gateway )
      BUILD_GATEWAY_RUNTIME=true
      ;;
    *(-)mcu )
      BUILD_MCU_RUNTIME_SW=true
      ;;
    *(-)mcu-hardware )
      BUILD_MCU_RUNTIME_HW=true
      ;;
    *(-)mcu-all )
      BUILD_MCU_RUNTIME_SW=true
      BUILD_MCU_RUNTIME_HW=true
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
  pushd "${SOURCE}/agent/video/videoMixer" >/dev/null
  cp -f binding.sw.gyp binding.gyp
  build_mcu_runtime
  popd >/dev/null
}

build_mcu_runtime_hw() {
  pushd "${SOURCE}/agent/video/videoMixer" >/dev/null
  cp -f binding.hw.gyp binding.gyp
  build_mcu_runtime
  popd >/dev/null
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
        CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} CC=${CCOMPILER} CXX=${CXXCOMPILER} node-gyp configure --loglevel=error
        CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} CC=${CCOMPILER} CXX=${CXXCOMPILER} node-gyp build --loglevel=error
      else
        CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} node-gyp configure --loglevel=error
        CORE_HOME="${CORE_HOME}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} node-gyp build --loglevel=error
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
  if ${BUILD_MCU_RUNTIME_HW} ; then
    build_mcu_runtime_hw
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

build
