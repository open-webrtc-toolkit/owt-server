#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */
#

pack_runtime() {
  mkdir -p ${WOOGEEN_DIST}/lib
  local LIBERIZO="${SOURCE}/core/build/erizo/src/erizo/liberizo.so"
  local LIBOOVOOGATEWAY="${SOURCE}/core/build/oovoo_gateway/liboovoo_gateway.so"
  local GATEWAY_ADDON="${SOURCE}/bindings/oovoo_gateway/build/Release/addon.node"
  [[ -s ${LIBERIZO} ]] && cp -av ${LIBERIZO} ${WOOGEEN_DIST}/lib
  [[ -s ${LIBOOVOOGATEWAY} ]] && cp -av ${LIBOOVOOGATEWAY} ${WOOGEEN_DIST}/lib
  [[ -s ${GATEWAY_ADDON} ]] && \
  mkdir -p ${WOOGEEN_DIST}/bindings/gateway/build/Release && \
  cp -av ${GATEWAY_ADDON} ${WOOGEEN_DIST}/bindings/gateway/build/Release && \
  strip ${WOOGEEN_DIST}/bindings/gateway/build/Release/addon.node
  # gateway
  mkdir -p ${WOOGEEN_DIST}/gateway/util
  cp -av ${SOURCE}/gateway/oovoo_gateway.js ${WOOGEEN_DIST}/gateway/
  cp -av ${SOURCE}/gateway/controller.js ${WOOGEEN_DIST}/gateway/
  cp -av ${SOURCE}/gateway/oovoo_heartbeat.js ${WOOGEEN_DIST}/gateway/
  cp -av ${SOURCE}/gateway/util/logger.js ${WOOGEEN_DIST}/gateway/util/
  cp -av ${SOURCE}/erizo_controller/erizoController/permission.js ${WOOGEEN_DIST}/gateway/util/
  cp -av ${SOURCE}/erizo_controller/erizoController/Stream.js ${WOOGEEN_DIST}/gateway/util/
  if ${ENCRYPT} ; then
    if ! hash uglifyjs 2>/dev/null; then
      if hash npm 2>/dev/null; then
        npm install -g --loglevel error uglify-js
        hash -r
      else
        echo >&2 "npm not found."
        echo >&2 "You need to install node first."
      fi
    fi
    find ${WOOGEEN_DIST}/gateway -type f -name "*.js" | while read line; do
      encrypt_js "$line"
    done
  fi
  # config
  mkdir -p ${WOOGEEN_DIST}/etc
  cp -av ${ROOT}/scripts/gateway_config.json ${WOOGEEN_DIST}/etc
  cp -av ${ROOT}/scripts/heartbeat_config.json ${WOOGEEN_DIST}/etc
  cp -av ${SOURCE}/gateway/util/log4cxx.properties ${WOOGEEN_DIST}/etc
  cp -av ${ROOT}/scripts/linux_service_start_user ${WOOGEEN_DIST}/etc
  cp -av ${ROOT}/scripts/webrtc-gateway-sysctl.conf ${WOOGEEN_DIST}/etc
  cp -av ${ROOT}/scripts/99-webrtc-gateway-limits.conf ${WOOGEEN_DIST}/etc
  # pems
  mkdir -p ${WOOGEEN_DIST}/cert
  cp -av ${ROOT}/cert/*.pem ${WOOGEEN_DIST}/cert
  # sample
  local SAMPLE_DIR="extras/sdk_sample"
  mkdir -p ${WOOGEEN_DIST}/${SAMPLE_DIR}/cert
  cp -av ${SOURCE}/${SAMPLE_DIR} ${WOOGEEN_DIST}/extras/
  cp -av ${ROOT}/cert/*.pem ${WOOGEEN_DIST}/${SAMPLE_DIR}/cert
  cp -av ${ROOT}/build/sdk/erizoclient*.js ${WOOGEEN_DIST}/${SAMPLE_DIR}/public/
  cp -av ${SOURCE}/client_sdk/assets ${WOOGEEN_DIST}/${SAMPLE_DIR}/public/
  cd ${WOOGEEN_DIST}/${SAMPLE_DIR}/public
  ln -s erizoclient-[0-9].[0-9].[0-9].js erizoclient.js
  ln -s erizoclient-[0-9].[0-9].[0-9].min.js erizoclient.min.js
  ln -s erizoclient.ui-[0-9].[0-9].[0-9].js erizoclient.ui.js
}

pack_libs() {
  [[ -s ${WOOGEEN_DIST}/lib/liboovoo_gateway.so ]] && \
  LD_LIBRARY_PATH=$ROOT/build/libdeps/build/lib:$ROOT/build/libdeps/build/lib64 ldd ${WOOGEEN_DIST}/lib/liboovoo_gateway.so | grep '=>' | awk '{print $3}' | while read line; do
    if ! uname -a | grep [Uu]buntu -q -s; then # CentOS
      [[ -s "${line}" ]] && [[ -z `rpm -qf ${line} 2>/dev/null | grep 'glibc'` ]] && cp -Lv ${line} ${WOOGEEN_DIST}/lib
    else # Ubuntu
      [[ -s "${line}" ]] && [[ -z `dpkg -S ${line} 2>/dev/null | grep 'libc6\|libselinux'` ]] && cp -Lv ${line} ${WOOGEEN_DIST}/lib
    fi
  done
}

pack_scripts() {
  mkdir -p ${WOOGEEN_DIST}/bin/
  cp -av ${this}/daemon-gw.sh ${WOOGEEN_DIST}/bin/daemon.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/start-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/stop-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/restart-all.sh
  echo '
${bin}/daemon.sh start gateway
${bin}/daemon.sh start app
' >> ${WOOGEEN_DIST}/bin/start-all.sh
  echo '
${bin}/daemon.sh stop gateway
${bin}/daemon.sh stop app
' >> ${WOOGEEN_DIST}/bin/stop-all.sh
  echo '
${bin}/stop-all.sh
${bin}/start-all.sh
' >> ${WOOGEEN_DIST}/bin/restart-all.sh
  cp -av ${ROOT}/scripts/retrieveGatewayCounters.sh ${WOOGEEN_DIST}/bin/
  cp -av ${ROOT}/scripts/webrtc-gateway-init.sh ${WOOGEEN_DIST}/bin/
  chmod +x ${WOOGEEN_DIST}/bin/*.sh
}

pack_node() {
  NODE_VERSION=
  . ${this}/../.conf
  echo "node version: ${NODE_VERSION}"

  local PREFIX_DIR=$ROOT/build/libdeps/build/
  cd $ROOT/third_party
  [[ ! -s node-${NODE_VERSION}.tar.gz ]] && curl -O http://nodejs.org/dist/${NODE_VERSION}/node-${NODE_VERSION}.tar.gz
  tar -xzf node-${NODE_VERSION}.tar.gz
  cd node-${NODE_VERSION}
  mkdir -p lib/webrtc_gateway

  find ${WOOGEEN_DIST}/gateway -type f -name "*.js" | while read line; do
    # This is kind of fragile - we assume that the occurrences of "require" are
    # always the keyword for module loading in the original JavaScript file.
    sed -i.origin "s/require('.*\//Module\._load('/g; s/require('/Module\._load('/g" "${line}"
    sed -i "1 i var Module = require('module');" "${line}"
    mv ${line} `pwd`/lib/webrtc_gateway/
    local BASENAME=`basename ${line}`
    sed -i "/lib\/zlib.js/a 'lib/webrtc_gateway/${BASENAME}'," node.gyp
    mv "${line}.origin" "${line}" # revert to original
  done
  # The entry
  mv `pwd`/lib/webrtc_gateway/oovoo_gateway.js `pwd`/lib/_third_party_main.js
  sed -i "s/webrtc_gateway\/oovoo_gateway\.js/_third_party_main.js/g" node.gyp

  local UV_OPT=
  [[ -s ${UV_DIR}/libuv.so ]] && UV_OPT="--shared-libuv --shared-libuv-includes=${UV_DIR}/include --shared-libuv-libpath=${UV_DIR}"

  patch -p0 < ../node-configure-tcmalloc.patch
  local LIBTCMALLOC="${PREFIX_DIR}/lib/libtcmalloc_minimal.so"
  if [ -s ${LIBTCMALLOC} ]; then
    cp -av ${LIBTCMALLOC}* ${WOOGEEN_DIST}/lib/
    PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig:${PREFIX_DIR}/lib64/pkgconfig:$PKG_CONFIG_PATH ./configure --without-npm --prefix=${PREFIX_DIR} ${UV_OPT} --shared-openssl --shared-openssl-includes=${PREFIX_DIR}/include --shared-openssl-libpath=${PREFIX_DIR}/lib --shared-tcmalloc --shared-tcmalloc-libpath=${PREFIX_DIR}/lib --shared-tcmalloc-libname=tcmalloc_minimal
  else
    PKG_CONFIG_PATH=${PREFIX_DIR}/lib/pkgconfig:${PREFIX_DIR}/lib64/pkgconfig:$PKG_CONFIG_PATH ./configure --without-npm --prefix=${PREFIX_DIR} ${UV_OPT} --shared-openssl --shared-openssl-includes=${PREFIX_DIR}/include --shared-openssl-libpath=${PREFIX_DIR}/lib
  fi
  LD_LIBRARY_PATH=${PREFIX_DIR}/lib:${UV_DIR}:$LD_LIBRARY_PATH make V=
  make uninstall
  make install
  mkdir -p ${WOOGEEN_DIST}/sbin
  cp -av ${PREFIX_DIR}/bin/node ${WOOGEEN_DIST}/sbin/webrtc_gateway && \
  strip ${WOOGEEN_DIST}/sbin/webrtc_gateway
  make uninstall

  sed -i 's/\/gateway\//\/sbin\//g' "${WOOGEEN_DIST}/bin/daemon.sh"
  sed -i 's/node oovoo_gateway\.js/\.\/webrtc_gateway/g' "${WOOGEEN_DIST}/bin/daemon.sh"

  mkdir -p ${WOOGEEN_DIST}/lib/node
  ln -s ../../etc/gateway_config.json ${WOOGEEN_DIST}/lib/node/
  mv ${WOOGEEN_DIST}/bindings/gateway/build/Release/addon.node ${WOOGEEN_DIST}/lib/node/
  mv ${WOOGEEN_DIST}/node_modules/* ${WOOGEEN_DIST}/lib/node/

  rm -rf ${WOOGEEN_DIST}/gateway
  rm -rf ${WOOGEEN_DIST}/bindings
  rm -rf ${WOOGEEN_DIST}/node_modules
}

install_module() {
  echo -e "\x1b[32mInstalling node_modules ...\x1b[0m"
  if hash npm 2>/dev/null; then
    mkdir -p ${WOOGEEN_DIST}/node_modules
    npm install --prefix ${WOOGEEN_DIST} --production --loglevel error socket.io@0.9.17 winston request
    mkdir -p ${WOOGEEN_DIST}/extras/node_modules
    npm install --prefix ${WOOGEEN_DIST}/extras --production --loglevel error express@3.4.8 request
  else
    echo >&2 "npm not found."
    echo >&2 "You need to install node first."
  fi
}
