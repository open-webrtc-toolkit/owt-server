{
  'targets': [{
    'target_name': 'webrtc',
    'sources': [
      'addon.cc',
      'WebRtcConnection.cc',
      'AudioFrameConstructorWrapper.cc',
      'AudioFramePacketizerWrapper.cc',
      'VideoFrameConstructorWrapper.cc',
      'VideoFramePacketizerWrapper.cc',
      '../../addons/woogeen_base/NodeEventRegistry.cc',
      '../../../core/woogeen_base/AudioFrameConstructor.cpp',
      '../../../core/woogeen_base/AudioFramePacketizer.cpp',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/woogeen_base/VideoFrameConstructor.cpp',
      '../../../core/woogeen_base/VideoFramePacketizer.cpp',
      '../../../core/erizo/src/erizo/dtls/bf_dwrap.c',
      '../../../core/erizo/src/erizo/dtls/DtlsTimer.cpp',
      '../../../core/erizo/src/erizo/dtls/DtlsSocket.cpp',
      '../../../core/erizo/src/erizo/dtls/DtlsClient.cpp',
      '../../../core/erizo/src/erizo/dtls/DtlsFactory.cpp',
      '../../../core/erizo/src/erizo/dtls/OpenSSLInit.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpVP8Parser.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpPacketQueue.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpVP8Fragmenter.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpSource.cpp',
      '../../../core/erizo/src/erizo/rtp/RtpSink.cpp',
      '../../../core/erizo/src/erizo/WebRtcConnection.cpp',
      '../../../core/erizo/src/erizo/SrtpChannel.cpp',
      '../../../core/erizo/src/erizo/Stats.cpp',
      '../../../core/erizo/src/erizo/StringUtil.cpp',
      '../../../core/erizo/src/erizo/DtlsTransport.cpp',
      '../../../core/erizo/src/erizo/SdpInfo.cpp',
      '../../../core/erizo/src/erizo/NiceConnection.cpp',
      '../../../../third_party/webrtc/src/webrtc/video_engine/stream_synchronization.cc',
      '../../../../third_party/webrtc/src/webrtc/video_engine/vie_receiver.cc',
      '../../../../third_party/webrtc/src/webrtc/video_engine/vie_sync_module.cc',
    ],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX'],
    'include_dirs': [
      '../../../core/common',
      '../../../core/erizo/src/erizo',
      '../../../core/mcu/media',
      '../../../core/woogeen_base',
      '../../../../third_party/webrtc/src',
      '../../../../build/libdeps/build/include',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
    ],
    'libraries': [
      '-L$(CORE_HOME)/../../build/libdeps/build/lib',
      '-lsrtp',
      '<!@(pkg-config --libs nice)',
      '-lssl',
      '-lcrypto',
      '-llog4cxx',
      '-lboost_thread',
      '-lboost_system',
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'GCC_ENABLE_CPP_RTTI':       'YES',        # -fno-rtti
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O$(OPTIMIZATION_LEVEL)', '-g', '-std=c++11', '-frtti'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ]
  }]
}
