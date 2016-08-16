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
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/woogeen_base/VideoFrameConstructor.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpSource.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpSink.cpp',
      '../../../core/woogeen_base/VideoFramePacketizer.cpp',
      '../../../../third_party/webrtc/src/webrtc/video_engine/stream_synchronization.cc',
      '../../../../third_party/webrtc/src/webrtc/video_engine/vie_receiver.cc',
      '../../../../third_party/webrtc/src/webrtc/video_engine/vie_sync_module.cc',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
    ],
    'dependencies': ['sipLib'],
    'include_dirs': [
      '$(CORE_HOME)/common',
      '$(CORE_HOME)/erizo/src/erizo',
      '$(CORE_HOME)/mcu/media',
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
          'GCC_ENABLE_CPP_RTTI':       'YES',        # -fno-rtti
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O3 -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O3', '-g' , '-std=c++0x', '-frtti'],
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
          'GCC_ENABLE_CPP_RTTI':       'YES',        # -fno-rtti
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O3 -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O3', '-g' , '-std=c++0x', '-frtti'],
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
