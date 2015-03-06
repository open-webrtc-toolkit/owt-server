#!/bin/bash

if ! hash openssl 2>/dev/null; then
    echo >&2 "Error: openssl not found."
    return 1
fi

mergePEM() {
  openssl pkcs12 -export -out certificate.pfx -inkey $1 -in $2
}

mergePEM $@