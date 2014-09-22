#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`

${bin}/daemon.sh stop app $@
${bin}/daemon.sh stop gateway $@

