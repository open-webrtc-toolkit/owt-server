#!/usr/bin/env bash
#
#/**
# * License placeholder.
# */

bin=`dirname "$0"`
bin=`cd "$bin"; pwd`

${bin}/daemon.sh start gateway
${bin}/daemon.sh start app

