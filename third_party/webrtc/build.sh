#!/bin/bash

CURRENT_DIR=`pwd`
export CFLAGS="-fPIC -O3 -DNDEBUG -Wall"
export CXXFLAGS="-fPIC -O3 -DNDEBUG -Wall"

cd $CURRENT_DIR

# Generate the Makefiles
#if hash node-gyp 2>/dev/null && uname -a | grep [Uu]buntu -q -s; then
#  PATH=$(dirname $(which node-gyp))/../lib/node_modules/node-gyp/gyp:$PATH
#fi
gyp --depth=. -D target_arch="x64" -D os_posix=1 -D clang=0 -D build_with_chromium=0 -D include_tests=0 -D enable_protobuf=0 -D use_system_yasm=0 -D build_libvpx=0 webrtc/webrtc.gyp

# Workaround to not generating thin archives
sed -i 's/crsT/crs/g' Makefile
make

# Combine the necessary archives to one
rm libwebrtc.a
mkdir -p tmp
find . -name "*.a" -exec cp {} tmp/ \;
cd tmp
for file in `ls -r ./*.a`
do
    ar -x ${file}
    if [ ! -f ../libwebrtc.a ]
    then 
	    ar cq ../libwebrtc.a *.o
    else
            ar q ../libwebrtc.a *.o 
    fi
    rm *.o
done
