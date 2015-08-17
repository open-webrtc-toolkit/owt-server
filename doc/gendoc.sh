#!/bin/sh
#Requirement: install jsdoc and docstrap, and pass dir paths of woogeensdk and woogeen2.
#TODO: install script
#Example usage: ./gendoc.sh /home/bean/workspace/webrtc-woogeensdk/webrtc/src/sdk/peer.js

#DIR SETTINGS　WOOGEEN2
WOOGEEN2_DIR=`cd ..; pwd`
echo "======Set WOOGEEN2_DIR:$WOOGEEN2_DIR."

WOOGEEN2DOC_DIR=$WOOGEEN2_DIR/doc
DOC_DIR_NAME=webrtc-jsdoc-cosmo
#TEMPLATE SETTINGS
README_PATH=$WOOGEEN2DOC_DIR/template/README.md
CONFIG_PATH=$WOOGEEN2DOC_DIR/jsdoc.conf.json
TEMPLATE_PATH=$WOOGEEN2DOC_DIR/template/

#EXTRA JS TO PACK
EXTRA_JSPATH=$@
echo "EXTRA_JS CODE: $EXTRA_JSPATH"

#1. source code to pack in doc
WG2_CODE="$WOOGEEN2_DIR/source/sdk2/conference/conference.js $WOOGEEN2_DIR/source/sdk2/base/events.js $WOOGEEN2_DIR/source/sdk2/base/stream.js $WOOGEEN2_DIR/source/nuve/nuveClient/src/N.API.js $WOOGEEN2_DIR/source/sdk2/ui/ui.js"
echo "WOOGEEN2 CODE: $WG2_CODE"

#2. generate doc
DOC_DIR=$WOOGEEN2DOC_DIR/$DOC_DIR_NAME
rm -rf $DOC_DIR/
mkdir -p $DOC_DIR/
jsdoc --readme $README_PATH -c $CONFIG_PATH -t $TEMPLATE_PATH -d $DOC_DIR $WG2_CODE $EXTRA_JSPATH
echo "======Genrate Doc: done.In package:"
echo ">>>>>>$DOC_DIR"

#3. post process
javac CosmoProcess.java
java CosmoProcess $DOC_DIR > postprocess.rec
rm postprocess.rec
echo "======Post process: done..."
cp intellogo.png $DOC_DIR
cp head.css $DOC_DIR/styles
