#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

ADDON_LIST="audioMixer internalIO mediaFileIO mediaFrameMulticaster rtspIn rtspOut videoMixer webrtc"
DIST_ADDON_DIR="${WOOGEEN_DIST}/bindings"

pack_runtime() {
  mkdir -p ${WOOGEEN_DIST}/lib
  local LIBERIZO="${SOURCE}/core/build/erizo/src/erizo/liberizo.so"
  local LIBMCU="${SOURCE}/core/build/mcu/libmcu.so"
  local LIBMCU_HW="${SOURCE}/core/build/mcu/libmcu_hw.so"
  local LIBMCU_SW="${SOURCE}/core/build/mcu/libmcu_sw.so"
  local SOURCE_ADDON_DIR="${SOURCE}/bindings"
  [[ -s ${LIBERIZO} ]] && cp -av ${LIBERIZO} ${WOOGEEN_DIST}/lib
  if [[ -s ${LIBMCU_SW} ]] && [[ -s ${LIBMCU_HW} ]]; then
    cp -av ${LIBMCU_HW} ${WOOGEEN_DIST}/lib
    cp -av ${LIBMCU_SW} ${WOOGEEN_DIST}/lib
  else
    [[ -s ${LIBMCU} ]] && cp -av ${LIBMCU} ${WOOGEEN_DIST}/lib
  fi
  # addons
  for ADDON in ${ADDON_LIST}; do
    [[ -s ${SOURCE_ADDON_DIR}/${ADDON}/build/Release/${ADDON}.node ]] && \
    mkdir -p ${DIST_ADDON_DIR}/${ADDON}/build/Release && \
    cp -av {${SOURCE_ADDON_DIR},${DIST_ADDON_DIR}}/${ADDON}/build/Release/${ADDON}.node && \
    ${ENCRYPT} && strip ${DIST_ADDON_DIR}/${ADDON}/build/Release/${ADDON}.node
  done
  # mcu
  mkdir -p ${WOOGEEN_DIST}/mcu/
  cd ${WOOGEEN_DIST}/mcu/ && \
  mkdir -p agent/{access,audio,video} controller/rpc
  cd ${SOURCE}/controller && \
  find . -type f -not -name "*.log" -not -name "in*.sh" -exec cp '{}' "${WOOGEEN_DIST}/mcu/controller/{}" \;
  cd ${SOURCE}/agent && \
  find . -type f -not -name "*.log" -not -name "in*.sh" -exec cp '{}' "${WOOGEEN_DIST}/mcu/agent/{}" \;
  pack_nuve
  pack_common
  ENCRYPT_CAND_PATH=("${WOOGEEN_DIST}/mcu" "${WOOGEEN_DIST}/nuve/nuveAPI" "${WOOGEEN_DIST}/common")
  # config
  mkdir -p ${WOOGEEN_DIST}/etc/nuve
  mkdir -p ${WOOGEEN_DIST}/etc/mcu
  cp -av ${SOURCE}/controller/log4js_configuration.json ${WOOGEEN_DIST}/etc/mcu
  cp -av ${SOURCE}/agent/log4cxx.properties ${WOOGEEN_DIST}/etc/mcu
  cp -av ${SOURCE}/nuve/log4js_configuration.json ${WOOGEEN_DIST}/etc/nuve
  rm -f ${WOOGEEN_DIST}/mcu/agent/log4cxx.properties
  rm -f ${WOOGEEN_DIST}/mcu/controller/log4js_configuration.json
  rm -f ${WOOGEEN_DIST}/nuve/log4js_configuration.json
  ln -s ../../etc/mcu/log4cxx.properties ${WOOGEEN_DIST}/mcu/agent/log4cxx.properties
  ln -s ../etc/mcu/log4js_configuration.json ${WOOGEEN_DIST}/mcu/log4js_configuration.json
  ln -s ../etc/nuve/log4js_configuration.json ${WOOGEEN_DIST}/nuve/log4js_configuration.json
  # certificates
  mkdir -p ${WOOGEEN_DIST}/cert
  cp -av ${ROOT}/cert/{*.pfx,.woogeen.keystore} ${WOOGEEN_DIST}/cert
  # sample
  mkdir -p ${WOOGEEN_DIST}/extras
  [[ ! -z ${SRC_SAMPLE_PATH} && -d ${SRC_SAMPLE_PATH} ]] && \
    cp -av ${SRC_SAMPLE_PATH} ${WOOGEEN_DIST}/extras/basic_example
  [[ -s ${WOOGEEN_DIST}/extras/basic_example/initcert.js ]] && \
    chmod +x ${WOOGEEN_DIST}/extras/basic_example/initcert.js
  cp -av ${SOURCE}/extras/rtsp-client.js ${WOOGEEN_DIST}/extras
}

pack_common() {
  mkdir -p ${WOOGEEN_DIST}/common
  cp -av ${SOURCE}/common/* ${WOOGEEN_DIST}/common/
}

pack_nuve() {
  mkdir -p ${WOOGEEN_DIST}/nuve
  cp -av ${SOURCE}/nuve/nuveAPI ${WOOGEEN_DIST}/nuve/
  cp -av ${SOURCE}/nuve/log4js_configuration.json ${WOOGEEN_DIST}/nuve/
  cp -av ${SOURCE}/cluster_manager ${WOOGEEN_DIST}/
}

pack_libs() {
  local OS=`$ROOT/scripts/detectOS.sh | awk '{print tolower($0)}'`
  echo $OS

  LD_LIBRARY_PATH=$ROOT/build/libdeps/build/lib:$ROOT/build/libdeps/build/lib64:${LD_LIBRARY_PATH} \
  ldd ${WOOGEEN_DIST}/sbin/webrtc_mcu \
      ${WOOGEEN_DIST}/lib/libmcu{,_sw,_hw}.so \
      ${WOOGEEN_DIST}/lib/libwoogeen_base.so \
      ${WOOGEEN_DIST}/lib/liberizo.so 2>/dev/null | grep '=>' | awk '{print $3}' | sort | uniq | while read line; do
    if [[ "$OS" =~ .*centos.* ]]
    then
      [[ -s "${line}" ]] && [[ -z `rpm -qf ${line} 2>/dev/null | grep 'glibc'` ]] && cp -Lv ${line} ${WOOGEEN_DIST}/lib
    elif [[ "$OS" =~ .*ubuntu.* ]]
    then
      [[ -s "${line}" ]] && [[ -z `dpkg -S ${line} 2>/dev/null | grep 'libc6\|libselinux'` ]] && cp -Lv ${line} ${WOOGEEN_DIST}/lib
    fi
  done
  # remove openh264 and replace with the pseudo one
  rm -f ${WOOGEEN_DIST}/lib/libopenh264*
  cp -av $ROOT/third_party/openh264/pseudo-openh264.so ${WOOGEEN_DIST}/lib/libopenh264.so.0
  # remove libs from msdk
  rm -f ${WOOGEEN_DIST}/lib/libmfxhw*
  rm -f ${WOOGEEN_DIST}/lib/libva*
  # remove libs from libav/ffmpeg if needed
  if ldd ${WOOGEEN_DIST}/lib/libavcodec* | grep aac -q -s; then # nonfree, not redistributable
    rm -f ${WOOGEEN_DIST}/lib/libav*
  fi
  # remove libfdk-aac
  rm -f ${WOOGEEN_DIST}/lib/libfdk-aac*
}

pack_scripts() {
  mkdir -p ${WOOGEEN_DIST}/bin/
  cp -av ${ROOT}/scripts/woogeen_default.js ${WOOGEEN_DIST}/etc/.woogeen_default.js
  cp -av ${this}/daemon-mcu.sh ${WOOGEEN_DIST}/bin/daemon.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/start-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/stop-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/restart-all.sh
  cp -av ${this}/mcu-init.sh ${WOOGEEN_DIST}/bin/init.sh
  cp -av ${ROOT}/scripts/detectOS.sh ${WOOGEEN_DIST}/bin/detectOS.sh
  cp -av ${this}/initdb.js ${WOOGEEN_DIST}/bin/initdb.js
  cp -av {${this},${WOOGEEN_DIST}/bin}/initcert.js && chmod +x ${WOOGEEN_DIST}/bin/initcert.js
  echo '
${bin}/daemon.sh start nuve
${bin}/daemon.sh start cluster-manager
${bin}/daemon.sh start portal
${bin}/daemon.sh start webrtc-agent
${bin}/daemon.sh start rtsp-agent
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
${bin}/daemon.sh stop rtsp-agent
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
  pushd ${ROOT}/third_party
  wget -c http://nodejs.org/dist/${NODE_VERSION}/node-${NODE_VERSION}.tar.gz
  tar -xzf node-${NODE_VERSION}.tar.gz && \
  pushd node-${NODE_VERSION} || (echo "invalid nodejs source."; popd; return -1)
  local CURRENT_DIR=$(pwd)
  rm -rf lib/webrtc_mcu # cleanup
  mkdir -p lib/webrtc_mcu

  local THIRD_PARTY_MODULES=$(node -e "process.stdout.write(Object.keys(require('${ROOT}/scripts/release/package.json').dependencies).join(' '))")
  cp -av ${WOOGEEN_DIST}/mcu/agent/{audio,video,access,erizoJS.js} lib/webrtc_mcu/
  find lib/webrtc_mcu/ -type f -name "*.js" | while read line; do
    # This is kind of fragile - we assume that the occurrences of "require" are
    # always the keyword for module loading in the original JavaScript file.
    # Use 'Module._load' for 3rd party js lib ONLY.
    for JSMODULE in ${THIRD_PARTY_MODULES}; do
      sed -i "s/require('\(\b${JSMODULE}\b\)/Module\._load('\1/g" "${line}"
    done
    sed -i "s/require('.*\//Module\._load('/g" "${line}"
    sed -i "s/Module\._load('\(\baccess\b\|\baudio\b\|\bvideo\b\)/require('webrtc_mcu\/\1\/index/g" "${line}"
    sed -i "s/Module\._load('\(\bwrtcConnection\b\|\brtspIn\b\)/require('webrtc_mcu\/access\/\1/g" "${line}"
    sed -i "1 i var Module = require('module');" "${line}"
    sed -i "/lib\/zlib.js/a '${line}'," node.gyp
  done
  # The entry
  mv ${CURRENT_DIR}/lib/webrtc_mcu/erizoJS.js ${CURRENT_DIR}/lib/_third_party_main.js
  sed -i "s/webrtc_mcu\/erizoJS\.js/_third_party_main.js/g" node.gyp

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
  make uninstall
  make install
  mkdir -p ${WOOGEEN_DIST}/sbin
  cp -av ${PREFIX_DIR}/bin/node ${WOOGEEN_DIST}/sbin/webrtc_mcu
  make uninstall
  popd
  popd

  sed -i "s/spawn('node'/spawn('webrtc_mcu'/g" "${WOOGEEN_DIST}/mcu/agent/index.js"

  ln -s ../node_modules ${WOOGEEN_DIST}/lib/node
  ln -s ../etc/woogeen_config.js ${WOOGEEN_DIST}/node_modules/
  ln -s ../common/{amqper,cipher,clusterWorker,logger,makeRPC}.js ${WOOGEEN_DIST}/node_modules/
  for ADDON in ${ADDON_LIST}; do
    mv ${DIST_ADDON_DIR}/${ADDON}/build/Release/${ADDON}.node ${WOOGEEN_DIST}/lib/node/
  done

  rm -rf ${WOOGEEN_DIST}/mcu/agent/{erizoJS.js,access/,audio/,video/}
  rm -rf ${DIST_ADDON_DIR}
}

install_module() {
  echo -e "\x1b[32mInstalling node_modules ...\x1b[0m"
  local SAMPLE_DIR=${WOOGEEN_DIST}/extras/basic_example
  if hash npm 2>/dev/null; then
    mkdir -p ${WOOGEEN_DIST}/node_modules
    cp -av ${this}/package.json ${WOOGEEN_DIST}/package.json
    cd ${WOOGEEN_DIST} && npm install --prefix ${WOOGEEN_DIST} --production --loglevel error

    [[ -d ${SAMPLE_DIR} ]] && \
    mkdir -p ${SAMPLE_DIR}/node_modules && \
    cd ${SAMPLE_DIR} && npm install --prefix ${SAMPLE_DIR} --production --loglevel error
  else
    echo >&2 "npm not found."
    echo >&2 "You need to install node first."
  fi
}

pack_license() {
  cp -v {$ROOT/third_party,${WOOGEEN_DIST}}/NOTICE
  cp -v {$ROOT/third_party,${WOOGEEN_DIST}}/ThirdpartyLicenses.txt
}
