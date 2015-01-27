#!/bin/bash
this=`dirname $0`
root=`cd ${this}/..; pwd`

export DISPLAY=:0.0
testacular=/usr/lib/node_modules/karma/bin/karma
testconfig=./.travis/testacular.conf.js
echo "Running front-end tests"
${testacular} start ${testconfig}
