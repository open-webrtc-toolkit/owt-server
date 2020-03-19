#!/bin/bash

options() {
	echo "Usage: $0 [ -p FILEPATH ] [ -i IMAGE_NAME ]" >&2
	echo " -p App directory" >&2
	echo " -i Docker image name" >&2
	echo " -n Request docker use --no-cache" >&2
}
echo "Number of args $# $*"
if [[ $* != *-p* ]]; then
	echo "-p is required"
	exit 1
fi

DIRNAME=$(dirname "$0")
REALPATH=$(realpath "$DIRNAME")
IMG="owt-server-conference"
NO_CACHE="";

while getopts p:i:n flag; do
	case "${flag}" in
	p)
		echo "using flag: -$flag"
		if [[ ${OPTARG} == "." ]]; then
			LOCAL_APP_PATH=$PWD
		else
			LOCAL_APP_PATH=$(realpath "${OPTARG}")
		fi
		LOCAL_APP_FOLDER=$(basename "$LOCAL_APP_PATH")
		LOCAL_APP_PATH=$(dirname "$LOCAL_APP_PATH")
		;;
	i)
		echo "using flag: -$flag"
		IMG=${OPTARG}
		;;
	n)
		NO_CACHE="--no-cache"
		;;
	\?)
		echo "Invalid option: -$flag ${OPTARG}"
		options
		exit 1
		;;
	:)
		echo "option -$flag requires an argument."
		exit 1
		;;
	esac
done

export DOCKER_IMG_NAME=$IMG
export DOCKERFILE_PATH=$REALPATH
export LOCAL_APP_PATH=$LOCAL_APP_PATH
export LOCAL_APP_FOLDER=$LOCAL_APP_FOLDER

echo Adding app in folder ${LOCAL_APP_PATH}/${LOCAL_APP_FOLDER} to Docker image ${DOCKER_IMG_NAME} ${NO_CACHE}

docker-compose -f "${DOCKERFILE_PATH}/docker-compose.yml" build ${NO_CACHE}
