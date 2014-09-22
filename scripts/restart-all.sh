#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`

${bin}/stop-all.sh
${bin}/start-all.sh

