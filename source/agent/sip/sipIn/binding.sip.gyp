{
  'targets': [{
    'target_name': 'sipIn',
    'sources': [ 'addon.cc',
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
                 '../../../core/woogeen_base/MediaFramePipeline.cpp',],
    'include_dirs': [ '$(CORE_HOME)/common',
                      '$(CORE_HOME)/erizo/src/erizo',
                      '$(CORE_HOME)/mcu/media',
                      '$(CORE_HOME)/sip_gateway',
                      '$(CORE_HOME)/woogeen_base',
                      '$(CORE_HOME)/../../third_party/webrtc/src'],
    'libraries': ['-L$(CORE_HOME)/build/sip_gateway',
                  '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',     # TODO remove dependence of lwebrtc
                  '-lsip_gateway',
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
    ]
  }]
}
