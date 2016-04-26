{
  'targets': [{
    'target_name': 'oovoo_gateway',
    'sources': [
      'addon.cc',
      'OoVooConnection.cpp',
      'OoVooGateway.cpp',
      'OoVooInboundStreamProcessor.cpp',
      'OoVooOutboundStreamProcessor.cpp',
      '../../agent/access/webrtc/WebRtcConnection.cc',
      '../../agent/addons/woogeen_base/Gateway.cc',
      '../../agent/addons/woogeen_base/NodeEventRegistry.cc',
      '../../core/woogeen_base/RawTransport.cpp',
      '../../core/woogeen_base/QoSUtility.cpp',
      '../../core/woogeen_base/ProtectedRTPReceiver.cpp',
      '../../core/woogeen_base/ProtectedRTPSender.cpp',
      '../../core/woogeen_base/WebRTCFeedbackProcessor.cpp',
      '../../core/erizo/src/erizo/dtls/bf_dwrap.c',
      '../../core/erizo/src/erizo/dtls/DtlsTimer.cpp',
      '../../core/erizo/src/erizo/dtls/DtlsSocket.cpp',
      '../../core/erizo/src/erizo/dtls/DtlsClient.cpp',
      '../../core/erizo/src/erizo/dtls/DtlsFactory.cpp',
      '../../core/erizo/src/erizo/dtls/OpenSSLInit.cpp',
      '../../core/erizo/src/erizo/rtp/RtpVP8Parser.cpp',
      '../../core/erizo/src/erizo/rtp/RtpPacketQueue.cpp',
      '../../core/erizo/src/erizo/rtp/RtpVP8Fragmenter.cpp',
      '../../core/erizo/src/erizo/rtp/RtpSource.cpp',
      '../../core/erizo/src/erizo/rtp/RtpSink.cpp',
      '../../core/erizo/src/erizo/WebRtcConnection.cpp',
      '../../core/erizo/src/erizo/SrtpChannel.cpp',
      '../../core/erizo/src/erizo/Stats.cpp',
      '../../core/erizo/src/erizo/StringUtil.cpp',
      '../../core/erizo/src/erizo/DtlsTransport.cpp',
      '../../core/erizo/src/erizo/SdpInfo.cpp',
      '../../core/erizo/src/erizo/NiceConnection.cpp'
    ],
    'include_dirs': [
      '$(CORE_HOME)/common',
      '$(CORE_HOME)/erizo/src/erizo',
      '$(CORE_HOME)/woogeen_base',
      '../../../third_party/webrtc/src',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
      '$(CORE_HOME)/../../build/libdeps/build/include'
    ],
    'libraries': [
      '-L$(CORE_HOME)/../../build/libdeps/build/lib',
      '-loovoosdk',
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
        'cflags_cc':  ['-Wall', '-O$(OPTIMIZATION_LEVEL)', '-g' , '-std=c++11', '-frtti'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ]
  }]
}
