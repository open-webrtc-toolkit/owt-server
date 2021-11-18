{
  'variables': {
    'github_actions': '<!(echo $GITHUB_ACTIONS)',
  },
  'targets': [{
    'target_name': 'quic',
    'sources':[
      'addon.cc',
      "QuicFactory.cc",
      'QuicTransportServer.cc',
      'QuicTransportConnection.cc',
      'QuicTransportStream.cc',
      'WebTransportFrameSource.cc',
      'WebTransportFrameDestination.cc',
      'VideoRtpPacketizer.cc',
      'RtpFactory.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/owt_base/MediaFrameMulticaster.cpp',
      '../../../core/owt_base/Utils.cc',
    ],
    'defines':[
      'OWT_ENABLE_QUIC=1',
      'WEBRTC_POSIX',
      'WEBRTC_LINUX',
      'LINUX',
      'NOLINUXIF',
      'NO_REG_RPC=1',
      'HAVE_VFPRINTF=1',
      'RETSIGTYPE=void',
      'NEW_STDIO',
      'HAVE_STRDUP=1',
      'HAVE_STRLCPY=1',
      'HAVE_LIBM=1',
      'HAVE_SYS_TIME_H=1',
      'TIME_WITH_SYS_TIME_H=1',
    ],
    'conditions':[
      ['github_actions=="true"', {
        'defines':[
          'OWT_FAKE_RTP',
        ]
      }],
      ['github_actions!="true"', {
        'dependencies': ['../../webrtc/rtcFrame/binding.gyp:librtcadapter'],
      }]
    ],
    'cflags_cc': [
      '-std=gnu++14',
      '-fno-exceptions',
      '-Wno-non-pod-varargs',
      '-fPIC',
    ],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '../../../core/common',
      '../../../core/owt_base',
      '$(DEFAULT_DEPENDENCY_PATH)/include',
      '$(CUSTOM_INCLUDE_PATH)',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
    ],
    'ldflags': [
      '-Wl,--no-as-needed',
      '-L$(DEFAULT_DEPENDENCY_PATH)/lib',
      '-Wl,-rpath,$(DEFAULT_DEPENDENCY_PATH)/lib',
      '-fPIC',
    ],
    'cflags_cc!': [
      '-std=gnu++0x',
      '-fno-exceptions',
    ],
    'libraries': [
      '-ldl',
      '-llog4cxx',
      '-lowt_web_transport',
      '-lboost_system',
      '-lboost_thread',
      '-Wl,-rpath,<!(pwd)/build/$(BUILDTYPE)',
    ],
  }]
}