{
  'targets': [{
    'target_name': 'videoMixer-sw',
    'sources': [
      '../addon.cc',
      '../VideoMixerWrapper.cc',
      '../SoftVideoCompositor.cpp',
      '../VideoLayoutProcessor.cpp',
      '../VideoMixer.cpp',
      '../../../../core/woogeen_base/BufferManager.cpp',
      '../../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../../core/woogeen_base/VCMFrameDecoder.cpp',
      '../../../../core/woogeen_base/VCMFrameEncoder.cpp',
    ],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX'],
    'include_dirs': [ '../../src',
                      '$(CORE_HOME)/common',
                      '$(CORE_HOME)/woogeen_base',
                      '$(CORE_HOME)/../../third_party/webrtc/src',
                      '$(CORE_HOME)/../../build/libdeps/build/include'
    ],
    'libraries': [
      '-lboost_thread',
      '-llog4cxx',
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
      '-L$(CORE_HOME)/../../third_party/openh264', '-lopenh264',
      '<!@(pkg-config --libs libavutil)',
      '<!@(pkg-config --libs libavcodec)',
      '<!@(pkg-config --libs libavformat)',
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
