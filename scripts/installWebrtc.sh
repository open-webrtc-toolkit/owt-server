#!/bin/bash

SSL_GNI=$(cat <<-END
declare_args() {
  build_with_owt = false
  owt_use_openssl = false
  owt_openssl_header_root = ""
  owt_openssl_lib_root = ""
  rtc_use_h265 = true
}

END
)

GCLIENT_CONFIG=$(cat <<-END
solutions = [
  {
    "url": "https://github.com/open-webrtc-toolkit/owt-deps-webrtc.git",
    "managed": False,
    "name": "src",
    "deps_file": "DEPS",
    "custom_deps": {},
  },
]

END
)

# comment is_debug=false for debugging
GN_ARGS=$(cat <<-END
rtc_use_h264=true
ffmpeg_branding="Chrome"
is_component_build=false
use_lld=false
rtc_build_examples=false
rtc_include_tests=false
rtc_include_pulse_audio=false
rtc_include_internal_audio_device=false
use_sysroot=false
is_clang=false
treat_warnings_as_errors=false
rtc_enable_libevent=false
rtc_build_libevent=false
is_debug=false

END
)

OWT_DIR="tools-owt"
DEPOT_TOOLS=

install_depot_tools(){
  DEPOT_TOOLS=`pwd`"/${OWT_DIR}/depot_tools"
  if [ -d $OWT_DIR/depot_tools ]; then
    echo "depot_tools already installed."
    return 0
  fi
  if [ ! -d $OWT_DIR ]; then
    mkdir -p $OWT_DIR
  fi

  pushd $OWT_DIR >/dev/null
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
  popd >/dev/null
}

download_and_build(){
  if [ -d src ]; then
    echo "src already exists."
  else
    git clone -b 79-sdk https://github.com/open-webrtc-toolkit/owt-deps-webrtc.git src
    mkdir -p src/build_overrides/ssl
    echo $SSL_GNI > src/build_overrides/ssl/ssl.gni
    echo $GCLIENT_CONFIG > .gclient
  fi

  if [[ "$OS" =~ .*centos.* ]]
  then
    source scl_source enable devtoolset-7
  fi

  export PATH="$PATH:$DEPOT_TOOLS"
  gclient sync
  pushd src >/dev/null  
  gn gen out --args="$GN_ARGS"
  ninja -C out call default_task_queue_factory
  all=`find ./out/obj/ -name "*.o"`
  if [[ -n $all ]];
  then
    rm -f ../libwebrtc.a
    ar cq ../libwebrtc.a $all
    echo "Generate libwebrtc.a OK"
  else
    echo "Generate libwebrtc.a Fail"
  fi
  popd >/dev/null
}

install_depot_tools
download_and_build
