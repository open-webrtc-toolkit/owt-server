#!/usr/bin/env bash

this=$(readlink -f $0)
this_dir=$(dirname $this)
ROOT=$(cd ${this_dir}/..; pwd)

. ${this_dir}/installCommonDeps.sh

install_webrtc