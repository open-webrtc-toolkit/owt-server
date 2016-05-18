#!/usr/bin/env bash
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

WOOGEEN_AGENTS="audio video access"

pack_runtime() {
  # mcu
  pack_controller
  pack_agents
  pack_nuve
  pack_common
  ENCRYPT_CAND_PATH=("${WOOGEEN_DIST}/controller" "${WOOGEEN_DIST}/cluster_manager" "${WOOGEEN_DIST}/nuve")
  for AGENT in ${WOOGEEN_AGENTS}; do
    ENCRYPT_CAND_PATH+=("${WOOGEEN_DIST}/${AGENT}_agent")
  done
  # sample
  mkdir -p ${WOOGEEN_DIST}/extras
  [[ ! -z ${SRC_SAMPLE_PATH} && -d ${SRC_SAMPLE_PATH} ]] && \
    cp -av ${SRC_SAMPLE_PATH} ${WOOGEEN_DIST}/extras/basic_example
  [[ -s ${WOOGEEN_DIST}/extras/basic_example/initcert.js ]] && \
    chmod +x ${WOOGEEN_DIST}/extras/basic_example/initcert.js
  cp -av ${SOURCE}/extras/rtsp-client.js ${WOOGEEN_DIST}/extras
}

pack_controller() {
  mkdir -p ${WOOGEEN_DIST}/controller/rpc
  pushd ${SOURCE}/controller >/dev/null
  find . -type f -not -name "*.log" -not -name "in*.sh" -not -name "*.md" -exec cp '{}' "${WOOGEEN_DIST}/controller/{}" \;
  popd >/dev/null
  mkdir -p ${WOOGEEN_DIST}/controller/cert
  cp -av ${ROOT}/cert/{*.pfx,.woogeen.keystore} ${WOOGEEN_DIST}/controller/cert/
  cp -av {${this},${WOOGEEN_DIST}/controller}/initcert.js && chmod +x ${WOOGEEN_DIST}/controller/initcert.js
}

pack_agents() {
  pushd ${SOURCE}/agent >/dev/null
  for AGENT in ${WOOGEEN_AGENTS}; do
    mkdir -p ${WOOGEEN_DIST}/${AGENT}_agent/${AGENT}
    find ${AGENT} -type f -name "*.js" -exec cp '{}' "${WOOGEEN_DIST}/${AGENT}_agent/{}" \;
    find . -maxdepth 1 -type f -not -name "*.log" -exec cp '{}' "${WOOGEEN_DIST}/${AGENT}_agent/{}" \;
    cp -av ${AGENT}/agent.toml ${WOOGEEN_DIST}/${AGENT}_agent/
    cp -av ${AGENT}/*.sh ${WOOGEEN_DIST}/${AGENT}_agent/
    pack_addons "${WOOGEEN_DIST}/${AGENT}_agent"
    mkdir -p ${WOOGEEN_DIST}/${AGENT}_agent/cert
    cp -av ${ROOT}/cert/{*.pfx,.woogeen.keystore} ${WOOGEEN_DIST}/${AGENT}_agent/cert/
    cp -av {${this},${WOOGEEN_DIST}/${AGENT}_agent}/initcert.js && chmod +x ${WOOGEEN_DIST}/${AGENT}_agent/initcert.js
    cp -av ${ROOT}/scripts/detectOS.sh ${WOOGEEN_DIST}/${AGENT}_agent/detectOS.sh
  done
  popd >/dev/null
}

pack_addons() {
  local DIST_ADDON_DIR=$1
  [[ -z ${DIST_ADDON_DIR} ]] && return 1
  local ADDON_LIST=$(find ${SOURCE}/agent -type f -name "*.node" | grep -v "obj\.target")
  for ADDON_FULLPATH in ${ADDON_LIST}; do
    ADDON=$(basename ${ADDON_FULLPATH} .node)
    ADDON_DEST_DIR=$(echo ${ADDON_FULLPATH} | sed 's/.*\/\(\w*\/build\/Release\)\/.*\.node/\1/g')
    if grep -RInqs "require.*\b${ADDON}\b'" ${DIST_ADDON_DIR}; then
      mkdir -p ${DIST_ADDON_DIR}/${ADDON_DEST_DIR} && \
      cp -av ${ADDON_FULLPATH} ${DIST_ADDON_DIR}/${ADDON_DEST_DIR}/${ADDON}.node && \
      ${ENCRYPT} && strip ${DIST_ADDON_DIR}/${ADDON_DEST_DIR}/${ADDON}.node
    fi
  done
  # now copy dep libs:
  mkdir -p ${DIST_ADDON_DIR}/lib
  local BINS=$(find ${DIST_ADDON_DIR} -type f -name "*.node")
  LD_LIBRARY_PATH=${ROOT}/build/libdeps/build/lib:${ROOT}/third_party/mediaprocessor/dist:${ROOT}/third_party/openh264:${LD_LIBRARY_PATH} \
  ldd ${BINS} | grep '=>' | awk '{print $3}' | sort | uniq | grep -v "^(" | \
  while read line; do
    if [[ "${OS}" =~ .*centos.* ]]; then
      [[ -s "${line}" ]] && [[ -z `rpm -qf ${line} 2>/dev/null | grep 'glibc'` ]] && cp -Lv ${line} ${DIST_ADDON_DIR}/lib
    elif [[ "${OS}" =~ .*ubuntu.* ]]; then
      [[ -s "${line}" ]] && [[ -z `dpkg -S ${line} 2>/dev/null | grep 'libc6\|libselinux'` ]] && cp -Lv ${line} ${DIST_ADDON_DIR}/lib
    fi
  done
  # remove openh264 and replace with the pseudo one unless `-openh264' flag is set
  ${PACK_OPENH264} || ([[ -s ${DIST_ADDON_DIR}/lib/libopenh264.so.0 ]] && \
    cp -av $ROOT/third_party/openh264/pseudo-openh264.so ${DIST_ADDON_DIR}/lib/libopenh264.so.0)
  # remove libs from msdk
  rm -f ${DIST_ADDON_DIR}/lib/libmfxhw*
  rm -f ${DIST_ADDON_DIR}/lib/libva*
  # remove libs from libav/ffmpeg if needed
  if ldd ${DIST_ADDON_DIR}/lib/libavcodec* | grep aac -q -s; then # nonfree, not redistributable
    rm -f ${DIST_ADDON_DIR}/lib/libav*
    rm -f ${DIST_ADDON_DIR}/lib/libsw*
  fi
  # remove libfdk-aac
  rm -f ${DIST_ADDON_DIR}/lib/libfdk-aac*
}

pack_common() {
  local TARGETS="controller nuve cluster_manager access_agent audio_agent video_agent"
  pushd ${SOURCE}/common >/dev/null
  local COMMON_MODULES=$(find . -type f -name "*.js" | cut -d '/' -f 2 | cut -d '.' -f 1)
  for TARGET in ${TARGETS}; do
    TARGET=${WOOGEEN_DIST}/${TARGET}
    if [[ -d ${TARGET} ]]; then
      for MODULE in ${COMMON_MODULES}; do
        if grep -RInqs "require.*\b${MODULE}\b" ${TARGET}; then
          for DEP in $(grep -RIn "require('\.\/" ${MODULE}.js | sed "s|.*require('\./\([^)]*\)').*|\1|g"); do
            cp -av ${DEP}.js ${TARGET}/
          done
          cp -av ${MODULE}.js ${TARGET}/
        fi
      done
    fi
  done
  popd >/dev/null
}

pack_nuve() {
  cp -av ${SOURCE}/nuve ${WOOGEEN_DIST}/
  cp -av ${SOURCE}/cluster_manager ${WOOGEEN_DIST}/
  mkdir -p ${WOOGEEN_DIST}/nuve/cert
  cp -av ${ROOT}/cert/{*.pfx,.woogeen.keystore} ${WOOGEEN_DIST}/nuve/cert/
  cp -av {${this},${WOOGEEN_DIST}/nuve}/initcert.js && chmod +x ${WOOGEEN_DIST}/nuve/initcert.js
  cp -av ${ROOT}/scripts/detectOS.sh ${WOOGEEN_DIST}/nuve/detectOS.sh
}

pack_scripts() {
  mkdir -p ${WOOGEEN_DIST}/bin/
  cp -av ${this}/daemon-mcu.sh ${WOOGEEN_DIST}/bin/daemon.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/start-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/stop-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/restart-all.sh
  cp -av ${this}/package.mcu.json ${WOOGEEN_DIST}/package.json
  echo '
${bin}/daemon.sh start nuve
${bin}/daemon.sh start cluster-manager
${bin}/daemon.sh start portal
${bin}/daemon.sh start webrtc-agent
${bin}/daemon.sh start avstream-agent
${bin}/daemon.sh start recording-agent
${bin}/daemon.sh start audio-agent
${bin}/daemon.sh start video-agent
${bin}/daemon.sh start app
' >> ${WOOGEEN_DIST}/bin/start-all.sh
  echo '
${bin}/daemon.sh stop nuve
${bin}/daemon.sh stop cluster-manager
${bin}/daemon.sh stop portal
${bin}/daemon.sh stop webrtc-agent
${bin}/daemon.sh stop avstream-agent
${bin}/daemon.sh stop recording-agent
${bin}/daemon.sh stop audio-agent
${bin}/daemon.sh stop video-agent
${bin}/daemon.sh stop app
' >> ${WOOGEEN_DIST}/bin/stop-all.sh
  echo '
${bin}/stop-all.sh
${bin}/start-all.sh
' >> ${WOOGEEN_DIST}/bin/restart-all.sh
  chmod +x ${WOOGEEN_DIST}/bin/*.sh
}

pack_node() {
  local NODE_VERSION=v$(node -e "process.stdout.write(require('${WOOGEEN_DIST}/package.json').engine.node)")
  echo "node version: ${NODE_VERSION}"
  local PREFIX_DIR=${ROOT}/build/libdeps/build/
  local THIRD_PARTY_MODULES=$(node -e "process.stdout.write(Object.keys(require('${ROOT}/source/agent/package.json').dependencies).join(' '))")

  pushd ${ROOT}/third_party
  wget -c http://nodejs.org/dist/${NODE_VERSION}/node-${NODE_VERSION}.tar.gz

  for AGENT in ${WOOGEEN_AGENTS}; do
    tar -xzf node-${NODE_VERSION}.tar.gz && \
    pushd node-${NODE_VERSION} || (echo "invalid nodejs source."; popd; return -1)
    rm -rf lib/woogeen # cleanup
    mkdir -p lib/woogeen/${AGENT}
    mv -v ${WOOGEEN_DIST}/${AGENT}_agent/${AGENT}/* lib/woogeen/${AGENT}/
    mv -v ${WOOGEEN_DIST}/${AGENT}_agent/erizoJS.js lib/woogeen/
    rmdir ${WOOGEEN_DIST}/${AGENT}_agent/${AGENT}
    find lib/woogeen/ -type f -name "*.js" | while read line; do
      # This is kind of fragile - we assume that the occurrences of "require" are
      # always the keyword for module loading in the original JavaScript file.
      # Use 'Module._load' for 3rd party js lib ONLY.
      for JSMODULE in ${THIRD_PARTY_MODULES}; do
        sed -i "s/require('\(\b${JSMODULE}\b\)/Module\._load('\1/g" "${line}"
      done
      sed -i "s/require('\.\//Module\._load('\.\//g" "${line}"
      sed -i "s/Module\._load('.*\(\baccess\b\|\baudio\b\|\bvideo\b\)/require('woogeen\/\1\/index/g" "${line}"
      sed -i "s/Module\._load('\.\/\(\bwrtcConnection\b'\)/require('woogeen\/access\/\1/g" "${line}"
      sed -i "1 i var Module = require('module');" "${line}"
      sed -i "/lib\/zlib.js/a '${line}'," node.gyp
    done
    # The entry
    mv lib/woogeen/erizoJS.js lib/_third_party_main.js
    sed -i "s/woogeen\/erizoJS\.js/_third_party_main.js/g" node.gyp

    # fix node's configure for dynamic-linking libraries
    patch -p0 < ../node4-configure.patch

    local LIBTCMALLOC="${PREFIX_DIR}/lib/libtcmalloc_minimal.so"
    if [ -s ${LIBTCMALLOC} ]; then
      patch -p0 < ../node4-configure-tcmalloc.patch
      PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig:${PREFIX_DIR}/lib64/pkgconfig:${PKG_CONFIG_PATH} ./configure --without-npm --prefix=${PREFIX_DIR} --shared-openssl --shared-tcmalloc --shared-tcmalloc-libpath=${PREFIX_DIR}/lib --shared-tcmalloc-libname=tcmalloc_minimal,dl
    else
      PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig:${PREFIX_DIR}/lib64/pkgconfig:${PKG_CONFIG_PATH} ./configure --without-npm --prefix=${PREFIX_DIR} --shared-openssl
    fi
    LD_LIBRARY_PATH=${PREFIX_DIR}/lib:${LD_LIBRARY_PATH} make V= -j5
    make uninstall >/dev/null
    make install >/dev/null

    cp -av ${PREFIX_DIR}/bin/node ${WOOGEEN_DIST}/${AGENT}_agent/woogeen_${AGENT}
    make uninstall >/dev/null
    sed -i "s/spawn('node'/spawn('\.\/woogeen_${AGENT}'/g" "${WOOGEEN_DIST}/${AGENT}_agent/index.js"
    popd >/dev/null
  done
  popd >/dev/null
}

install_module() {
  pushd ${WOOGEEN_DIST} >/dev/null
  local TARGETS=$(find . -maxdepth 3 -type f -name "package.json" | sed "s/\/package\.json//g" | sed "s/\.\/*//g")
  if hash npm 2>/dev/null; then
    for TARGET in ${TARGETS}; do
      [[ -d ${TARGET} ]] || continue
      echo -e "\x1b[32mInstalling node_modules for ${TARGET}...\x1b[0m"
      pushd ${TARGET} >/dev/null
      npm install --production --loglevel error
      popd >/dev/null
    done
  else
    echo >&2 "npm not found."
    echo >&2 "You need to install node first."
  fi
  popd >/dev/null
}

pack_license() {
  cp -v {$ROOT/third_party,${WOOGEEN_DIST}}/NOTICE
  cp -v {$ROOT/third_party,${WOOGEEN_DIST}}/ThirdpartyLicenses.txt
}
