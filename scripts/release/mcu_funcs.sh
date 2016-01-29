#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

pack_runtime() {
  mkdir -p ${WOOGEEN_DIST}/lib
  local LIBERIZO="${SOURCE}/core/build/erizo/src/erizo/liberizo.so"
  local LIBMCU="${SOURCE}/core/build/mcu/libmcu.so"
  local LIBMCU_HW="${SOURCE}/core/build/mcu/libmcu_hw.so"
  local LIBMCU_SW="${SOURCE}/core/build/mcu/libmcu_sw.so"
  local ADDON="${SOURCE}/bindings/mcu/build/Release/addon.node"
  [[ -s ${LIBERIZO} ]] && cp -av ${LIBERIZO} ${WOOGEEN_DIST}/lib
  if [[ -s ${LIBMCU_SW} ]] && [[ -s ${LIBMCU_HW} ]]; then
    cp -av ${LIBMCU_HW} ${WOOGEEN_DIST}/lib
    cp -av ${LIBMCU_SW} ${WOOGEEN_DIST}/lib
  else
    [[ -s ${LIBMCU} ]] && cp -av ${LIBMCU} ${WOOGEEN_DIST}/lib
  fi
  [[ -s ${ADDON} ]] && \
  mkdir -p ${WOOGEEN_DIST}/bindings/mcu/build/Release && \
  cp -av ${ADDON} ${WOOGEEN_DIST}/bindings/mcu/build/Release && \
  ${ENCRYPT} && strip ${WOOGEEN_DIST}/bindings/mcu/build/Release/addon.node
  # mcu
  mkdir -p ${WOOGEEN_DIST}/mcu/
  cd ${WOOGEEN_DIST}/mcu/ && \
  mkdir -p common erizoAgent erizoController/rpc erizoJS
  cd ${SOURCE}/erizo_controller && \
  find . -type f -not -name "*.log" -not -name "in*.sh" -exec cp '{}' "${WOOGEEN_DIST}/mcu/{}" \;
  pack_nuve
  pack_common
  ENCRYPT_CAND_PATH=("${WOOGEEN_DIST}/mcu" "${WOOGEEN_DIST}/nuve/nuveAPI" "${WOOGEEN_DIST}/common")
  # config
  mkdir -p ${WOOGEEN_DIST}/etc/nuve
  mkdir -p ${WOOGEEN_DIST}/etc/mcu
  cp -av ${SOURCE}/erizo_controller/log4js_configuration.json ${WOOGEEN_DIST}/etc/mcu
  cp -av ${SOURCE}/erizo_controller/erizoAgent/log4cxx.properties ${WOOGEEN_DIST}/etc/mcu
  cp -av ${SOURCE}/nuve/log4js_configuration.json ${WOOGEEN_DIST}/etc/nuve
  rm -f ${WOOGEEN_DIST}/mcu/erizoAgent/log4cxx.properties
  rm -f ${WOOGEEN_DIST}/mcu/log4js_configuration.json
  rm -f ${WOOGEEN_DIST}/nuve/log4js_configuration.json
  ln -s ../../etc/mcu/log4cxx.properties ${WOOGEEN_DIST}/mcu/erizoAgent/log4cxx.properties
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
}

pack_libs() {
  local OS=`$ROOT/scripts/detectOS.sh | awk '{print tolower($0)}'`
  echo $OS

  LD_LIBRARY_PATH=$ROOT/build/libdeps/build/lib:$ROOT/build/libdeps/build/lib64:${LD_LIBRARY_PATH} ldd ${WOOGEEN_DIST}/sbin/webrtc_mcu ${WOOGEEN_DIST}/bindings/mcu/build/Release/addon.node ${WOOGEEN_DIST}/lib/node/addon.node ${WOOGEEN_DIST}/lib/libmcu{,_sw,_hw}.so ${WOOGEEN_DIST}/lib/liberizo.so | grep '=>' | awk '{print $3}' | sort | uniq | while read line; do
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
  mkdir -p lib/webrtc_mcu

  find ${WOOGEEN_DIST}/mcu/common ${WOOGEEN_DIST}/mcu/erizoJS -type f -name "*.js" | while read line; do
    # This is kind of fragile - we assume that the occurrences of "require" are
    # always the keyword for module loading in the original JavaScript file.
    sed -i.origin "s/require('.*\//Module\._load('/g; s/require('/Module\._load('/g" "${line}"
    sed -i "s/Module\._load('\(amqper\|logger\|makeRPC\|accessNode\|audioNode\|videoNode\|wrtcConnection\|rtspIn\)/Module\._load('webrtc_mcu\/\1/g" "${line}"
    sed -i "1 i var Module = require('module');" "${line}"
    mv ${line} ${CURRENT_DIR}/lib/webrtc_mcu/
    sed -i "/lib\/zlib.js/a 'lib/webrtc_mcu/$(basename ${line})'," node.gyp
    mv "${line}.origin" "${line}" # revert to original
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

  sed -i "s/spawn('node'/spawn('webrtc_mcu'/g" "${WOOGEEN_DIST}/mcu/erizoAgent/erizoAgent.js"

  ln -s ../node_modules ${WOOGEEN_DIST}/lib/node
  ln -s ../etc/woogeen_config.js ${WOOGEEN_DIST}/node_modules/
  ln -s ../common/cipher.js ${WOOGEEN_DIST}/node_modules/
  mv ${WOOGEEN_DIST}/bindings/mcu/build/Release/addon.node ${WOOGEEN_DIST}/lib/node/

  rm -rf ${WOOGEEN_DIST}/mcu/erizoJS
  rm -rf ${WOOGEEN_DIST}/bindings
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
