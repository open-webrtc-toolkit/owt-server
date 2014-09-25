#!/bin/bash

CURRENT_DIR=`pwd`
export CFLAGS="-fPIC -O3 -DNDEBUG -Wall"
export CXXFLAGS="-fPIC -O3 -DNDEBUG -Wall"

cd $CURRENT_DIR/src

# Generate the Makefiles
#if hash node-gyp 2>/dev/null && uname -a | grep [Uu]buntu -q -s; then
#  PATH=$(dirname $(which node-gyp))/../lib/node_modules/node-gyp/gyp:$PATH
#fi
./tools/gyp/gyp --depth=. -D target_arch="x64" -D os_posix=1 -D clang=0 -D build_with_chromium=0 -D include_tests=0 -D enable_protobuf=0 -D use_x11=0 -D build_libvpx=0 -D use_system_yasm=1 -D include_capture=0 -D include_render=0 -D chromeos=0 webrtc/webrtc.gyp

# Workaround to not generating thin archives
sed -i 's/crsT/crs/g' Makefile
make

cd $CURRENT_DIR

# Combine the necessary archives to one
rm libwebrtc.a
mkdir -p tmp
find ./src/out -name "*.a" -exec cp {} tmp/ \;
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

cd $CURRENT_DIR
rm -rf tmp
