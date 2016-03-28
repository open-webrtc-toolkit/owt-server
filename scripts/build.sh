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
  echo "    --gateway                           build oovoo gateway library & addon"
  echo "    --mcu (software)                    build mcu runtime library & addon without msdk"
  echo "    --mcu-hardware                      build mcu runtime library & addon with msdk"
  echo "    --mcu-all                           build mcu runtime library & addon both with and without msdk"
  echo "    --sdk                               build sdk"
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
BUILD_MCU_BUNDLE=false
BUILD_SDK=false
BUILDTYPE="Release"
BUILD_ROOT="${ROOT}/build"
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

${BUILD_MCU_RUNTIME_HW} && ${BUILD_MCU_RUNTIME_SW} && BUILD_MCU_BUNDLE=true

build_gateway_runtime() {
  CMAKE_ADDITIONAL_OPTIONS="-DCOMPILE_OOVOO_GATEWAY=ON -DCOMPILE_MCU=OFF"
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/gateway"
  build_runtime
}

build_mcu_runtime() {
  CMAKE_ADDITIONAL_OPTIONS="-DCOMPILE_MCU=ON -DCOMPILE_OOVOO_GATEWAY=OFF"
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/agent"
  build_runtime
}

build_mcu_runtime_sw() {
  unset BUILD_WITH_MSDK
  build_mcu_runtime
  ${BUILD_MCU_BUNDLE} && cp -av ${SOURCE}/core/build/mcu/libmcu{,_sw}.so || rm -f ${SOURCE}/core/build/mcu/libmcu_sw.so
}

build_mcu_runtime_hw() {
  export BUILD_WITH_MSDK=true
  build_mcu_runtime
  ${BUILD_MCU_BUNDLE} && cp -av ${SOURCE}/core/build/mcu/libmcu{,_hw}.so || rm -f ${SOURCE}/core/build/mcu/libmcu_hw.so
}

build_runtime() {
  local RUNTIME_LIB_SRC_DIR="${SOURCE}/core"
  local CCOMPILER=${DEPS_ROOT}/bin/gcc
  local CXXCOMPILER=${DEPS_ROOT}/bin/g++
  local OPTIMIZATION_LEVEL="3"
  [[ BUILDTYPE == "Release" ]] || OPTIMIZATION_LEVEL="0"

  # rm -fr "${RUNTIME_LIB_SRC_DIR}/build"
  mkdir -p "${RUNTIME_LIB_SRC_DIR}/build"
  # runtime lib
  pushd "${RUNTIME_LIB_SRC_DIR}/build" >/dev/null
  if [[ -x $CCOMPILER && -x $CXXCOMPILER ]]; then
    LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} CC=$CCOMPILER CXX=$CXXCOMPILER cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
  else
    LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
  fi
  LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH make
  popd >/dev/null
  # runtime addon
  local NODE_VERSION=
  ADDON_LIST=$(find ${RUNTIME_ADDON_SRC_DIR} -type f -name "binding.gyp" | xargs dirname)
  [[ ${ADDON_LIST} =~ "oovoo_gateway" ]] &&
    NODE_VERSION=v$(node -e "process.stdout.write(require('${ROOT}/scripts/release/package.gw.json').engine.node)") ||
    NODE_VERSION=v$(node -e "process.stdout.write(require('${ROOT}/scripts/release/package.mcu.json').engine.node)")
  if [[ ${NODE_VERSION} == $(node --version) ]] && hash node-gyp 2>/dev/null; then
    for ADDON in ${ADDON_LIST}; do
      echo -e "building addon \e[32m$(basename ${ADDON})\e[0m"
      pushd ${ADDON} >/dev/null
      if [[ -x ${CCOMPILER} && -x ${CXXCOMPILER} ]]; then
        CORE_HOME="${RUNTIME_LIB_SRC_DIR}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} CC=${CCOMPILER} CXX=${CXXCOMPILER} node-gyp configure --loglevel=error
        CORE_HOME="${RUNTIME_LIB_SRC_DIR}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} CC=${CCOMPILER} CXX=${CXXCOMPILER} node-gyp build --loglevel=error
      else
        CORE_HOME="${RUNTIME_LIB_SRC_DIR}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:${PKG_CONFIG_PATH} node-gyp configure --loglevel=error
        CORE_HOME="${RUNTIME_LIB_SRC_DIR}" OPTIMIZATION_LEVEL=${OPTIMIZATION_LEVEL} node-gyp build --loglevel=error
      fi
      popd >/dev/null
    done

  else
    echo >&2 "You need to install Node.js ${NODE_VERSION} toolchain:"
    echo >&2 "  nvm install ${NODE_VERSION}"
    echo >&2 "  npm install -g node-gyp"
    echo >&2 "  node-gyp install ${NODE_VERSION}"
    return 1
  fi
}

build_mcu_client_sdk() {
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
    build_mcu_client_sdk
    ((DONE++))
  fi
  if [[ ${DONE} -eq 0 ]]; then
    usage
    return 1
  fi
}

build
