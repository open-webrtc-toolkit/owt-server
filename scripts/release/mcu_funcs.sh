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
  strip ${WOOGEEN_DIST}/bindings/mcu/build/Release/addon.node
  # mcu
  mkdir -p ${WOOGEEN_DIST}/mcu/
  cd ${WOOGEEN_DIST}/mcu/ && \
  mkdir -p common erizoAgent erizoController/rpc erizoJS/rpc
  cd ${SOURCE}/erizo_controller && \
  find . -type f -not -name "*.log" -not -name "in*.sh" -exec cp '{}' "${WOOGEEN_DIST}/mcu/{}" \;
  pack_nuve
  # encryption if needed
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
    find ${WOOGEEN_DIST}/mcu -type f -name "*.js" | while read line; do
      encrypt_js "$line"
    done
    find ${WOOGEEN_DIST}/nuve -type f -name "*.js" | while read line; do
      encrypt_js "$line"
    done
  fi
  pack_sdk
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
  # pems
  mkdir -p ${WOOGEEN_DIST}/cert
  cp -av ${ROOT}/cert/*.pem ${WOOGEEN_DIST}/cert
  # sample
  mkdir -p ${WOOGEEN_DIST}/extras
  cp -av ${SOURCE}/extras/basic_example ${WOOGEEN_DIST}/extras
}

pack_nuve() {
  mkdir -p ${WOOGEEN_DIST}/nuve
  cp -av ${SOURCE}/nuve/nuveAPI ${WOOGEEN_DIST}/nuve/
  cp -av ${SOURCE}/nuve/log4js_configuration.json ${WOOGEEN_DIST}/nuve/
}

pack_sdk() {
  # woogeen.js
  local WOOGEENJS="${SOURCE}/sdk2/scripts/dist"
  local version=$(grep "version" ${SOURCE}/sdk2/scripts/package.json | cut -d '"' -f 4)
  mkdir -p ${WOOGEEN_DIST}/sdk/woogeen/
  cp -v ${WOOGEENJS}/woogeen.sdk-${version}.min.js ${WOOGEEN_DIST}/sdk/woogeen/
  cp -v ${WOOGEENJS}/woogeen.sdk.ui-${version}.min.js ${WOOGEEN_DIST}/sdk/woogeen/
  # nuve.js
  local NUVEJS="${SOURCE}/nuve/nuveClient/dist/nuve.js"
  mkdir -p ${WOOGEEN_DIST}/sdk/nuve/
  [[ -s ${NUVEJS} ]] && cp -av ${NUVEJS} ${WOOGEEN_DIST}/sdk/nuve/
}

pack_libs() {
  LD_LIBRARY_PATH=$ROOT/build/libdeps/build/lib:$ROOT/build/libdeps/build/lib64 ldd ${WOOGEEN_DIST}/lib/libmcu{,_sw,_hw}.so ${WOOGEEN_DIST}/lib/liberizo.so | grep '=>' | awk '{print $3}' | sort | uniq | while read line; do
    if ! uname -a | grep [Uu]buntu -q -s; then # CentOS
      [[ -s "${line}" ]] && [[ -z `rpm -qf ${line} 2>/dev/null | grep 'glibc'` ]] && cp -Lv ${line} ${WOOGEEN_DIST}/lib
    else # Ubuntu
      [[ -s "${line}" ]] && [[ -z `dpkg -S ${line} 2>/dev/null | grep 'libc6\|libselinux'` ]] && cp -Lv ${line} ${WOOGEEN_DIST}/lib
    fi
  done
  # remove openh264 and replace with the pseudo one
  rm -f ${WOOGEEN_DIST}/lib/libopenh264*
  cp -av $ROOT/third_party/openh264/pseudo-openh264.so ${WOOGEEN_DIST}/lib/libopenh264.so
  # remove libs from msdk
  rm -f ${WOOGEEN_DIST}/lib/libmfxhw*
  rm -f ${WOOGEEN_DIST}/lib/libva*
}

pack_scripts() {
  mkdir -p ${WOOGEEN_DIST}/bin/
  cp -av ${ROOT}/scripts/.conf ${WOOGEEN_DIST}/bin/
  cp -av ${ROOT}/scripts/woogeen_default.js ${WOOGEEN_DIST}/etc/.woogeen_default.js
  cp -av ${this}/daemon-mcu.sh ${WOOGEEN_DIST}/bin/daemon.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/start-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/stop-all.sh
  cp -av ${this}/launch-base.sh ${WOOGEEN_DIST}/bin/restart-all.sh
  cp -av ${this}/mcu-init.sh ${WOOGEEN_DIST}/bin/init.sh
  echo '
${bin}/daemon.sh start nuve
${bin}/daemon.sh start mcu
${bin}/daemon.sh start app
' >> ${WOOGEEN_DIST}/bin/start-all.sh
  echo '
${bin}/daemon.sh stop nuve
${bin}/daemon.sh stop mcu
${bin}/daemon.sh stop app
' >> ${WOOGEEN_DIST}/bin/stop-all.sh
  echo '
${bin}/stop-all.sh
${bin}/start-all.sh
' >> ${WOOGEEN_DIST}/bin/restart-all.sh
  chmod +x ${WOOGEEN_DIST}/bin/*.sh
}

pack_node() {
  return # just return for now.
  NODE_VERSION=
  . ${this}/../.conf
  echo "node version: ${NODE_VERSION}"

  local PREFIX_DIR=$ROOT/build/libdeps/build/
  cd $ROOT/third_party
  [[ ! -s node-${NODE_VERSION}.tar.gz ]] && curl -O http://nodejs.org/dist/${NODE_VERSION}/node-${NODE_VERSION}.tar.gz
  tar -xzf node-${NODE_VERSION}.tar.gz
  cd node-${NODE_VERSION}
  mkdir -p lib/webrtc_mcu

  find ${WOOGEEN_DIST}/mcu/common ${WOOGEEN_DIST}/mcu/erizoJS -type f -name "*.js" | while read line; do
    # This is kind of fragile - we assume that the occurrences of "require" are
    # always the keyword for module loading in the original JavaScript file.
    sed -i.origin "s/require('.*\//Module\._load('/g; s/require('/Module\._load('/g" "${line}"
    sed -i "1 i var Module = require('module');" "${line}"
    mv ${line} `pwd`/lib/webrtc_mcu/
    local BASENAME=`basename ${line}`
    sed -i "/lib\/zlib.js/a 'lib/webrtc_mcu/${BASENAME}'," node.gyp
    mv "${line}.origin" "${line}" # revert to original
  done
  # The entry
  mv `pwd`/lib/webrtc_mcu/erizoJS.js `pwd`/lib/_third_party_main.js
  sed -i "s/webrtc_mcu\/erizoJS\.js/_third_party_main.js/g" node.gyp

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
  cp -av ${PREFIX_DIR}/bin/node ${WOOGEEN_DIST}/sbin/webrtc_mcu
  make uninstall

  sed -i "s/spawn('node'/spawn('webrtc_mcu'/g" "${WOOGEEN_DIST}/mcu/erizoAgent/erizoAgent.js"

  ln -s ../node_modules ${WOOGEEN_DIST}/lib/node
  ln -s ../etc/woogeen_config.js ${WOOGEEN_DIST}/node_modules/
  mv ${WOOGEEN_DIST}/bindings/mcu/build/Release/addon.node ${WOOGEEN_DIST}/lib/node/

  rm -rf ${WOOGEEN_DIST}/mcu/erizoJS
  rm -rf ${WOOGEEN_DIST}/bindings
}

install_module() {
  echo -e "\x1b[32mInstalling node_modules ...\x1b[0m"
  if hash npm 2>/dev/null; then
    mkdir -p ${WOOGEEN_DIST}/node_modules
    cp -av ${this}/package.mcu.json ${WOOGEEN_DIST}/package.json
    cd ${WOOGEEN_DIST} && npm install --prefix ${WOOGEEN_DIST} --production --loglevel error
    mkdir -p ${WOOGEEN_DIST}/extras/node_modules
    cp -av ${this}/package.app.json ${WOOGEEN_DIST}/extras/package.json
    cd ${WOOGEEN_DIST}/extras && npm install --prefix ${WOOGEEN_DIST}/extras --production --loglevel error
  else
    echo >&2 "npm not found."
    echo >&2 "You need to install node first."
  fi
}

pack_license() {
  cp -v {$ROOT/third_party,${WOOGEEN_DIST}}/NOTICE
  cp -v {$ROOT/third_party,${WOOGEEN_DIST}}/ThirdpartyLicenses.txt
}
