{
  'targets': [{
    'target_name': 'sipIn',
    'sources': [
      'addon.cc',
      'SipGateway.cc',
      'SipCallConnection.cpp',
      'AudioFrameConstructorWrapper.cc',
      'AudioFramePacketizerWrapper.cc',
      'VideoFrameConstructorWrapper.cc',
      'VideoFramePacketizerWrapper.cc',
      '../../addons/common/NodeEventRegistry.cc',
      '../../../core/woogeen_base/AudioFrameConstructor.cpp',
      '../../../core/woogeen_base/AudioFramePacketizer.cpp',
      '../../../core/woogeen_base/AudioUtilities.cpp',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/woogeen_base/VideoFrameConstructor.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpSource.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpSink.cpp',
      '../../../core/woogeen_base/VideoFramePacketizer.cpp',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/rtc_adapter/VieReceiver.cc',
      '../../../core/rtc_adapter/VieRemb.cc',
      '../../../core/woogeen_base/SsrcGenerator.cc',
    ],
    'dependencies': ['sipLib'],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX'],
    'include_dirs': [
      '$(CORE_HOME)/common',
      '$(CORE_HOME)/erizo/src/erizo',
      '../../../core/rtc_adapter',
      '$(CORE_HOME)/woogeen_base',
      '$(CORE_HOME)/../../third_party/webrtc/src'
    ],
    'libraries': [
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
      '-llog4cxx',
      '-lboost_thread',
      '-lboost_system',
      # Add following arguments to help ldd find libraries during packing
      '-L$(CORE_HOME)/../../third_party/libre-0.4.16', '-lre',
      '-Wl,-rpath,$(CORE_HOME)/../../third_party/libre-0.4.16',
      '-Wl,-rpath,<!(pwd)/build/$(BUILDTYPE)',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O3 -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O3', '-g' , '-std=c++11'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ]
  },
  {
    'target_name': 'sipLib',
    'type': 'shared_library',
    'sources': [
        'sip_gateway/SipEP.cpp',
        'sip_gateway/SipGateway.cpp',
        'sip_gateway/SipCallConnection.cpp',
    ],
    'direct_dependent_settings': {
        'include_dirs': ['sip_gateway'],
    },
    'include_dirs': [
        'sip_gateway',
        'sip_gateway/sipua/include',
        '$(CORE_HOME)/common',
        '$(CORE_HOME)/erizo/src/erizo',
        '$(CORE_HOME)/../../build/libdeps/build/include',
        '$(CORE_HOME)/../../third_party/libre-0.4.16/include',
    ],
    'libraries': [
        '-L<!(pwd)/sip_gateway/sipua', '-lsipua',
        '-L$(CORE_HOME)/../../build/libdeps/build/lib',
        '-L$(CORE_HOME)/../../third_party/libre-0.4.16', '-lre',
        '-llog4cxx',
        '-lboost_thread',
        '-lboost_system',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O3 -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O3', '-g', '-std=c++0x'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ],
    'actions': [
      {
        'action_name': 'sipua_build',
        'inputs': [
          '<!@(find <!(pwd)/sip_gateway/sipua -type f -name "*.h")',
          '<!@(find <!(pwd)/sip_gateway/sipua -type f -name "*.c")',
        ],
        'outputs': [
          '<!(pwd)/sip_gateway/sipua/libsipua.a'
        ],
        'action': ['eval', 'cd <!(pwd)/sip_gateway/sipua && make'],
      }
    ]
  },
  ]
}
