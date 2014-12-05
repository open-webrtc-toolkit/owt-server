#!/bin/bash

SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..
BUILD_DIR=$ROOT/build
CURRENT_DIR=`pwd`
EXTRAS=$ROOT/source/extras

export PATH=$PATH:/usr/local/sbin

if ! pgrep -f rabbitmq; then
  sudo echo
  sudo rabbitmq-server > $BUILD_DIR/rabbit.log &
fi

cd $ROOT/source/nuve
./initNuve.sh

sleep 5

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ROOT/source/core/erizo/build/erizo:$ROOT/source/core/build/erizo/src/erizo:$ROOT/build/libdeps/build/lib:$ROOT/source/core/build/mcu:$ROOT/third_party/libuv-0.10.26
export ERIZO_HOME=$ROOT/source/core/erizo/

cd $ROOT/source/erizo_controller
./initErizo_controller.sh
./initErizo_agent.sh

cp $ROOT/source/nuve/nuveClient/dist/nuve.js $EXTRAS/basic_example/

echo [licode] Done, run basic_example/basicServer.js
