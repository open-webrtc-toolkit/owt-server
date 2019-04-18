## QUIC RAW Library

This library is based on chromium source code and exposes basic interfaces for raw data transporting.

## How to build
1. Get the chromium code. (https://chromium.googlesource.com/chromium/src/+/master/docs/linux_build_instructions.md)
2. Reset chromium commit close to 0d7f510fc898d4040d474a58b0eb609c43b6ac20.
3. Copy the code into chromium directory.
- cp -r src ${chromium}/src/net/tools/quic/raw
4. Patch the BUILD.gn file in chromium code.
- cd ${chroumium}
- git apply ${this-repo}/build-gn.patch
5. Set "is_debug=false" for gn args, build chromium target.
- ninja -C ${out-directory} rawquic

## How to use
1. Build the library yourself or download from link(TO-DO).
2. Add ${this-repo}/src/wrapper to include dir.
3. Same compile steps as other shared libraries.
Note that since our .so file is compiled through chromium's clang, clang is recommended for linking.

## About example
1. Place the .so file in ${this-repo}/lib
2. cd example/ && make
