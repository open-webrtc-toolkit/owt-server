#!/bin/bash

SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..
BUILD_DIR=$ROOT/build
CURRENT_DIR=`pwd`
DB_DIR="$BUILD_DIR"/db
EXTRAS=$ROOT/source/extras

cd $EXTRAS/basic_example

npm install --loglevel error express body-parser morgan errorhandler
cd $CURRENT_DIR
