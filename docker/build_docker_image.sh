#!/bin/bash

build_type=${1:-owt}

BASEDIR="$(dirname "$(readlink -fm "$0")")"
CONTEXTDIR="$(dirname "$BASEDIR")"

if [ $build_type == "owt" ]; then
  dockerfile=Dockerfile
  docker build --target owt-build -t owt:build \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .

  docker build --target owt-run -t owt:run \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .

elif [ $build_type == "openvino" ]; then
  dockerfile=gst-analytics.Dockerfile
  docker build -f ${BASEDIR}/${dockerfile} -t owt:openvino \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .

else
  echo "Usage: ./build_docker_image.sh [BUILDTYPE]"
  echo "ERROR: please set BUILDTYPE to on of the following: [owt, openvino]"
  exit
fi
