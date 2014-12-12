#!/bin/bash

[[ ! -d ../dist ]] && mkdir ../dist
[[ ! -d ../build ]] && mkdir ../build

java -jar compiler.jar --js ../src/hmac-sha1.js --js ../src/N.js --js ../src/N.Base64.js --js ../src/N.API.js --js_output_file ../build/nuve.js

./compileDist.sh