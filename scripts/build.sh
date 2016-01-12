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
  CMAKE_ADDITIONAL_OPTIONS="-DCOMPILE_OOVOO_GATEWAY=ON"
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/bindings/oovoo_gateway"
  build_runtime
}

build_mcu_runtime() {
  CMAKE_ADDITIONAL_OPTIONS="-DCOMPILE_MCU=ON"
  RUNTIME_ADDON_SRC_DIR="${SOURCE}/bindings/mcu"
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

  # rm -fr "${RUNTIME_LIB_SRC_DIR}/build"
  mkdir -p "${RUNTIME_LIB_SRC_DIR}/build"
  # runtime lib
  cd "${RUNTIME_LIB_SRC_DIR}/build"
  if [[ -x $CCOMPILER && -x $CXXCOMPILER ]]; then
    LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} CC=$CCOMPILER CXX=$CXXCOMPILER cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
  else
    LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH PKG_CONFIG_PATH=${DEPS_ROOT}/lib/pkgconfig:$PKG_CONFIG_PATH BOOST_ROOT=${DEPS_ROOT} cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} $CMAKE_ADDITIONAL_OPTIONS ..
  fi
  LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH make
  # runtime addon
  if hash node-gyp 2>/dev/null; then
    echo 'building with node-gyp...'
    cd "${RUNTIME_ADDON_SRC_DIR}"
    if [[ -x $CCOMPILER && -x $CXXCOMPILER ]]; then
      LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH CORE_HOME="${RUNTIME_LIB_SRC_DIR}" CC=$CCOMPILER CXX=$CXXCOMPILER node-gyp rebuild
    else
      LD_LIBRARY_PATH=${DEPS_ROOT}/lib:$LD_LIBRARY_PATH CORE_HOME="${RUNTIME_LIB_SRC_DIR}" node-gyp rebuild
    fi
  else
    echo >&2 "Appropriate building tool not found."
    echo >&2 "You need to install node-gyp."
    return 1
  fi
  # [ -s "${RUNTIME_ADDON_SRC_DIR}/build/Release/addon.node" ] && cp -av "${RUNTIME_ADDON_SRC_DIR}/build/Release/addon.node" "${BUILD_ROOT}/"
  cd ${this}
}

build() {
  export CFLAGS="-fstack-protector -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security"
  export CXXFLAGS=$CFLAGS
  #export LDFLAGS="-z noexecstack -z relro -z now"
  export LDFLAGS="-z noexecstack -z relro"

  local DONE=0
  mkdir -p "${BUILD_ROOT}/sdk"
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
  if [[ ${DONE} -eq 0 ]]; then
    usage
    return 1
  fi
}

build
