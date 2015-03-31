#!/bin/bash

this=$(readlink -f $0)
this_dir=$(dirname $this)
OPENSSL_BIN=$(readlink -f ${this_dir}/../build/libdeps/build/bin/openssl)
OUT_DIR=$(readlink -f ${this_dir}/../cert)


if ! hash ${OPENSSL_BIN} 2>/dev/null; then
    echo >&2 "Error: openssl not found. You should run \`installDeps.sh' or \`installCommonDeps.sh' to install it first."
    exit 1
fi

selfsignPEM() {
  ${OPENSSL_BIN} req -x509 -newkey rsa:2048 -keyout ${OUT_DIR}/key.pem -out ${OUT_DIR}/cert.pem \
  -subj "/C=CN/ST=Shanghai/L=Shanghai/O=Intel/OU=WebRTC/CN=webrtc.intel.com" \
  -days 30
}

mergePEM() {
  ${OPENSSL_BIN} pkcs12 -export -out ${OUT_DIR}/certificate.pfx -inkey ${OUT_DIR}/key.pem -in ${OUT_DIR}/cert.pem
}

selfsignPEM
mergePEM