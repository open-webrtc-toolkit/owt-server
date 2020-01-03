#!/bin/bash
# ==============================================================================
# Copyright (C) 2018-2019 Intel Corporation
#
# SPDX-License-Identifier: MIT
# ==============================================================================

build_type=${1:-opensource}
tag=${2:-latest}


BASEDIR=$(dirname "$0")

docker build --target owt-build -t gst-owt-2:build \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .

docker build --target owt-run -t gst-owt-2:run \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .

docker build --target analytics-run -t gst-analytics-2:run \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .
