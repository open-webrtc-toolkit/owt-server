{
  'variables':{
    'oms_enable_quic%': 0,  # Enable QUIC and other standard WebRTC objects.
  },
  'targets': [{
    'target_name': 'rtcConn',
    'variables': {
      'source_rel_dir': '../../..', # relative source dir path
      'source_abs_dir%': '<(module_root_dir)/../../..', # absolute source dir path
    },
    'sources': [
      'addon.cc',
      'WebRtcConnection.cc',
      'ThreadPool.cc',
      'IOThreadPool.cc',
      "MediaStream.cc",
      'conn_handler/WoogeenHandler.cpp',
      'erizo/src/erizo/DtlsTransport.cpp',
      'erizo/src/erizo/IceConnection.cpp',
      'erizo/src/erizo/LibNiceConnection.cpp',
      'erizo/src/erizo/SdpInfo.cpp',
      'erizo/src/erizo/SrtpChannel.cpp',
      'erizo/src/erizo/Stats.cpp',
      'erizo/src/erizo/StringUtil.cpp',
      'erizo/src/erizo/WebRtcConnection.cpp',
      'erizo/src/erizo/MediaStream.cpp',
      'erizo/src/erizo/lib/LibNiceInterfaceImpl.cpp',
      'erizo/src/erizo/thread/IOThreadPool.cpp',
      'erizo/src/erizo/thread/IOWorker.cpp',
      'erizo/src/erizo/thread/Scheduler.cpp',
      'erizo/src/erizo/thread/ThreadPool.cpp',
      'erizo/src/erizo/thread/Worker.cpp',
      'erizo/src/erizo/rtp/PacketBufferService.cpp',
      'erizo/src/erizo/rtp/RtcpForwarder.cpp',
      'erizo/src/erizo/rtp/RtcpProcessorHandler.cpp',
      'erizo/src/erizo/rtp/RtpUtils.cpp',
      'erizo/src/erizo/rtp/QualityManager.cpp',
      'erizo/src/erizo/rtp/RtpExtensionProcessor.cpp',
      '<!@(find erizo/src/erizo/dtls -name "*.cpp")',
      '<!@(find erizo/src/erizo/dtls -name "*.c")',
      '<!@(find erizo/src/erizo/pipeline -name "*.cpp")',
      '<!@(find erizo/src/erizo/stats  -name "*.cpp")'
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
      '<(source_rel_dir)/core/common',
      '<(source_rel_dir)/core/owt_base',
      '<(source_rel_dir)/../build/libdeps/build/include',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
    ],
    'libraries': [
      '-L<(source_abs_dir)/../build/libdeps/build/lib',
      '-lsrtp2',
      '-lssl',
      '-ldl',
      '-lcrypto',
      '-llog4cxx',
      '-lboost_thread',
      '-lboost_system',
      '-lnice',
      #'-L<(webrtc_abs_dir)', '-lwebrtc',
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
      [ 'oms_enable_quic==1', {
        'sources':[
          'RTCIceTransport.cc',
          'RTCIceCandidate.cc',
        ],
        'defines':[
          'OMS_ENABLE_QUIC=1',
        ]
      }],
    ]
  }]
}
