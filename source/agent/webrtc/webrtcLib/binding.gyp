{
  'targets': [{
    'target_name': 'webrtc',
    'sources': [
      'addon.cc',
      'AudioFrameConstructorWrapper.cc',
      'AudioFramePacketizerWrapper.cc',
      'VideoFrameConstructorWrapper.cc',
      'VideoFramePacketizerWrapper.cc',
      'WebRtcConnection.cc',
      'ThreadPool.cc',
      'IOThreadPool.cc',
      'conn_handler/WoogeenHandler.cpp',
      '<!@(find erizo/src/erizo/ -maxdepth 1 -name "*.cpp")',
      '<!@(find erizo/src/erizo/lib  -name "*.cpp")',
      '<!@(find erizo/src/erizo/dtls -name "*.cpp")',
      '<!@(find erizo/src/erizo/dtls -name "*.c")',
      '<!@(find erizo/src/erizo/pipeline -name "*.cpp")',
      '<!@(find erizo/src/erizo/rtp -name "*.cpp")',
      '<!@(find erizo/src/erizo/thread  -name "*.cpp")',
      '<!@(find erizo/src/erizo/stats  -name "*.cpp")',
      '../../addons/common/NodeEventRegistry.cc',
      '../../../core/woogeen_base/AudioFrameConstructor.cpp',
      '../../../core/woogeen_base/AudioFramePacketizer.cpp',
      '../../../core/woogeen_base/AudioUtilities.cpp',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/woogeen_base/VideoFrameConstructor.cpp',
      '../../../core/woogeen_base/VideoFramePacketizer.cpp',
      '../../../core/woogeen_base/SsrcGenerator.cc',
      '../../../core/rtc_adapter/VieReceiver.cc',
      '../../../core/rtc_adapter/VieRemb.cc' #20150508
    ],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX', '-DLINUX', '-DNOLINUXIF', '-DNO_REG_RPC=1', '-DHAVE_VFPRINTF=1', '-DRETSIGTYPE=void', '-DNEW_STDIO', '-DHAVE_STRDUP=1', '-DHAVE_STRLCPY=1', '-DHAVE_LIBM=1', '-DHAVE_SYS_TIME_H=1', '-DTIME_WITH_SYS_TIME_H=1'],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      'conn_handler',
      'erizo/src/erizo',
      'erizo/src/erizo/lib',
      'erizo/src/erizo/dtls',
      'erizo/src/erizo/pipeline',
      'erizo/src/erizo/rtp',
      'erizo/src/erizo/thread',
      'erizo/src/erizo/stats',
      '../../../core/common',
      '../../../core/woogeen_base',
      '../../../core/rtc_adapter',
      '../../../../third_party/webrtc/src',
      '../../../../build/libdeps/build/include',
      '../../../../third_party/nICEr/out/include/event',
      '../../../../third_party/nICEr/out/include/log',
      '../../../../third_party/nICEr/out/include/plugin',
      '../../../../third_party/nICEr/out/include/registry',
      '../../../../third_party/nICEr/out/include/share',
      '../../../../third_party/nICEr/out/include/stats',
      '../../../../third_party/nICEr/out/include/util',
      '../../../../third_party/nICEr/out/include/util/libekr',
      '../../../../third_party/nICEr/out/include/port/generic/include',
      '../../../../third_party/nICEr/out/include/port/generic/include/sys',
      '../../../../third_party/nICEr/out/include/crypto',
      '../../../../third_party/nICEr/out/include/ice',
      '../../../../third_party/nICEr/out/include/net',
      '../../../../third_party/nICEr/out/include/stun',
      '../../../../third_party/nICEr/out/include/port/linux/include',
      '../../../../third_party/nICEr/out/include/port/linux/include/sys',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
    ],
    'libraries': [
      '-L$(CORE_HOME)/../../build/libdeps/build/lib',
      '-lsrtp2',
      '-lssl',
      '-ldl',
      '-lcrypto',
      '-llog4cxx',
      #'-lglib-2.0',
      #'-lgobject-2.0',
      #'-lgthread-2.0',
      '-lboost_thread',
      '-lboost_regex',
      '-lboost_system',
      '-lnice',
      '-L$(CORE_HOME)/../../third_party/nICEr/out/lib', '-lnicer',
      '-L$(CORE_HOME)/../../third_party/nICEr/third_party/nrappkit/lib', '-lnrappkit',
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++']
        },
      }, { # OS!="mac"
          'cflags!' : ['-fno-exceptions'],
          'cflags' : ['-D__STDC_CONSTANT_MACROS'],
          'cflags_cc' : ['-Wall', '-O3', '-g' , '-std=c++11', '-fexceptions'],
          'cflags_cc!' : ['-fno-exceptions'],
          'cflags_cc!' : ['-fno-rtti']
      }],
    ]
  }]
}
