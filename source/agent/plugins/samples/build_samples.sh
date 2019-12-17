
#!/bin/bash
# Copyright (C) <2019> Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

error() {
    local code="${3:-1}"
    if [[ -n "$2" ]];then
        echo "Error on or near line $1: $2; exiting with status ${code}"
    else
        echo "Error on or near line $1; exiting with status ${code}"
    fi
    exit "${code}" 
}
trap 'error ${LINENO}' ERR

SAMPLES_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


if ! command -v cmake &>/dev/null; then
    printf "\n\nCMAKE is not installed. It is required to build Inference Engine samples. Please install it. \n\n"
    exit 1
fi

build_dir=$PWD/build
mkdir -p $build_dir
cd $build_dir
cmake -DCMAKE_BUILD_TYPE=Release $SAMPLES_PATH
make

printf "\nBuild completed, you can find binaries for all samples in the $PWD/build/intel64/Release subfolder, and copy \nall of them over to analytics_agent/lib directory, or analytics_agent/pluginlibs by default.\n\n"
