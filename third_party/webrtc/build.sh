#!/bin/bash

CURRENT_DIR=`pwd`
export CFLAGS="-fPIC -O3 -DNDEBUG -Wall"
export CXXFLAGS="-fPIC -O3 -DNDEBUG -Wall"

cd $CURRENT_DIR

# Generate the Makefiles
if hash node-gyp 2>/dev/null && uname -a | grep [Uu]buntu -q -s; then
  PATH=$(dirname $(which node-gyp))/../lib/node_modules/node-gyp/gyp:$PATH
fi
gyp --depth=. -D target_arch="x64" -D os_posix=1 -D clang=0 -D build_with_chromium=0 webrtc/webrtc.gyp

# Workaround to not generating thin archives
sed -i 's/crsT/crs/g' Makefile
make

# Combine the necessary archives to one
mkdir -p tmp
cp out/Default/obj.target/webrtc/modules/libbitrate_controller.a tmp/
cp out/Default/obj.target/webrtc/modules/librtp_rtcp.a tmp/
cp out/Default/obj.target/webrtc/modules/libwebrtc_video_coding.a tmp/
cp out/Default/obj.target/webrtc/modules/video_coding/utility/libvideo_coding_utility.a tmp/
cp out/Default/obj.target/webrtc/system_wrappers/source/libsystem_wrappers.a tmp/
cd tmp
ar -x libbitrate_controller.a
ar -x librtp_rtcp.a
ar -x libvideo_coding_utility.a
ar -x libwebrtc_video_coding.a
ar -x libsystem_wrappers.a
ar crs ../libwebrtc.a *.o
cd $CURRENT_DIR
rm -rf tmp

