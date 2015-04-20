#!/bin/bash

TARGET=../dist/nuve-debug.js

cat ../lib/xmlhttprequest.js > ${TARGET}
cat ../src/hmac-sha256.js >> ${TARGET}
cat ../src/N.js >> ${TARGET}
cat ../src/N.Base64.js >> ${TARGET}
cat ../src/N.API.js >> ${TARGET}

echo 'module.exports = N;' >> $TARGET
