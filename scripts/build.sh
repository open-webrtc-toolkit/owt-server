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
  echo "    --mcu                               build mcu runtime library & addon"
  echo "    --sdk                               build sdk"
  echo "    --all                               build all components"
  echo "    --help                              print this help"
  echo "Example:"
  echo "    --release --all                     build all components in release mode"
  echo "    --debug --runtime                   build runtime in debug mode"
  echo
}

if [[ $# -eq 0 ]];then
  usage
  exit 1
fi

BUILD_GATEWAY_RUNTIME=false
BUILD_MCU_RUNTIME=false
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
      BUILD_MCU_RUNTIME=true
      BUILD_SDK=true
      ;;
    *(-)gateway )
      BUILD_GATEWAY_RUNTIME=true
      ;;
    *(-)mcu )
      BUILD_MCU_RUNTIME=true
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
  CMAKE_ADDITIONAL_OPTIONS="-DCOMPILE_OOVOO_GATEWAY=ON"
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/bindings/oovoo_gateway"
  build_runtime
}

build_mcu_runtime() {
  CMAKE_ADDITIONAL_OPTIONS="-DCOMPILE_MCU=ON"
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/bindings/mcu"
  build_runtime

  install_config
}

build_runtime() {
  local RUNTIME_LIB_SRC_DIR="${SOURCE}/core"

  local CCOMPILER=${DEPS_ROOT}/bin/gcc
  local CXXCOMPILER=${DEPS_ROOT}/bin/g++

  # rm -fr "${RUNTIME_LIB_SRC_DIR}/build"
  mkdir -p "${RUNTIME_LIB_SRC_DIR}/build"
  # runtime lib
  if ! uname -a | grep [Uu]buntu -q -s; then
    cd "${RUNTIME_LIB_SRC_DIR}/build"
    if [[ -x $CCOMPILER && -x $CXXCOMPILER ]]; then
      LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} CC=$CCOMPILER CXX=$CXXCOMPILER cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
    else
      LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
    fi
    LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH make
  else
    cd "${RUNTIME_LIB_SRC_DIR}/build" && cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} ${CMAKE_ADDITIONAL_OPTIONS} .. && make
  fi
  # runtime addon
  if hash node-gyp 2>/dev/null; then
    echo 'building with node-gyp...'
    if ! uname -a | grep [Uu]buntu -q -s; then
      cd "${RUNTIME_ADDON_SRC_DIR}"
      if [[ -x $CCOMPILER && -x $CXXCOMPILER ]]; then
        LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH CORE_HOME="${RUNTIME_LIB_SRC_DIR}" CC=$CCOMPILER CXX=$CXXCOMPILER node-gyp rebuild
      else
        LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH CORE_HOME="${RUNTIME_LIB_SRC_DIR}" node-gyp rebuild
      fi
    else
      cd "${RUNTIME_ADDON_SRC_DIR}" && CORE_HOME="${RUNTIME_LIB_SRC_DIR}" node-gyp rebuild
    fi
  else
    echo >&2 "Appropriate building tool not found."
    echo >&2 "You need to install node-gyp."
    return 1
  fi
  # [ -s "${RUNTIME_ADDON_SRC_DIR}/build/Release/addon.node" ] && cp -av "${RUNTIME_ADDON_SRC_DIR}/build/Release/addon.node" "${BUILD_ROOT}/"
  cd ${this}
}

build_mcu_client_sdk() {
  local CLIENTSDK_DIR="${SOURCE}/client_sdk"
  rm -f ${BUILD_ROOT}/sdk/*.js
  rm -f ${CLIENTSDK_DIR}/dist/*.js
  cd ${CLIENTSDK_DIR}
  grunt --force
  cp -av ${CLIENTSDK_DIR}/dist/*.js ${BUILD_ROOT}/sdk/

  cd ${ROOT}/source/sdk2 && make
}

build_mcu_server_sdk() {
  local SERVERSDK_DIR="${SOURCE}/nuve/nuveClient"
  # nuve.js
  cd ${SERVERSDK_DIR}/tools
  ./compile.sh
  echo [nuve] Done, nuve.js compiled
  cp -av ${SERVERSDK_DIR}/dist/nuve.js ${SOURCE}/extras/basic_example/
}

install_config() {
  mkdir -p ${SOURCE}/etc
  cp -av ${this}/custom_video_layout_default.js ${SOURCE}/etc/custom_video_layout.js
}

build() {
  local DONE=0
  mkdir -p "${BUILD_ROOT}/sdk"
  # Job
  if ${BUILD_GATEWAY_RUNTIME} ; then
    build_gateway_runtime
    ((DONE++))
  fi
  if ${BUILD_MCU_RUNTIME} ; then
    build_mcu_runtime
    ((DONE++))
  fi
  if ${BUILD_SDK} ; then
    build_mcu_client_sdk
    build_mcu_server_sdk
    ((DONE++))
  fi
  if [[ ${DONE} -eq 0 ]]; then
    usage
    return 1
  fi
}

build
