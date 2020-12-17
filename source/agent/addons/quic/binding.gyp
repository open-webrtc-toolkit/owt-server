{
  'targets': [{
    'target_name': 'quic',
    'sources':[
      'addon.cc',
      "QuicFactory.cc",
      'QuicTransportStream.cc',
      'QuicTransportServer.cc',
      'QuicTransportConnection.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/owt_base/MediaFrameMulticaster.cpp',
    ],
    'defines':[
      'OWT_ENABLE_QUIC=1',
    ],
    'cflags_cc': [
      '-std=gnu++14',
      '-fno-exceptions',
      '-DWEBRTC_POSIX',
      '-DWEBRTC_LINUX',
      '-DLINUX',
      '-DNOLINUXIF',
      '-DNO_REG_RPC=1',
      '-DHAVE_VFPRINTF=1',
      '-DRETSIGTYPE=void',
      '-DNEW_STDIO',
      '-DHAVE_STRDUP=1',
      '-DHAVE_STRLCPY=1',
      '-DHAVE_LIBM=1',
      '-DHAVE_SYS_TIME_H=1',
      '-DTIME_WITH_SYS_TIME_H=1',
      '-Wno-non-pod-varargs',
      '-fPIC',
    ],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      'conn_handler',
      '../../../core/common',
      '../../../core/owt_base',
      '../../../../build/libdeps/build/include',
      # TODO: Fix it when CI for QUIC SDK is ready.
      '/home/jianjunz/code/quic/src/owt/quic_transport/api',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
    ],
    'ldflags': [
      '-Wl,--no-as-needed',
      '-Wl,-rpath,/home/jianjunz/code/quic/src/out/debug',
      '-L$(CORE_HOME)/../../build/libdeps/build/lib',
      '-fPIC',
      '-L/home/jianjunz/code/quic/src/out/debug',
    ],
    'cflags_cc!': [
      '-std=gnu++0x',
      '-fno-exceptions',
    ],
    'libraries': [
      '-ldl',
      '-llog4cxx',
      '-lowt_quic_transport',
      '-lboost_system',
      '-lboost_thread',
    ],
  }]
}