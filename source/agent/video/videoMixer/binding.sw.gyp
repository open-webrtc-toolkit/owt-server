{
  'targets': [{
    'target_name': 'videoMixer-sw',
    'sources': [
      'addon.cc',
      'VideoMixerWrapper.cc',
      'BufferManager.cpp',
      'SoftVideoCompositor.cpp',
      'VideoLayoutProcessor.cpp',
      'VideoMixer.cpp',
      '../../../../third_party/webrtc/src/webrtc/video_engine/payload_router.cc',
      '../../../../third_party/webrtc/src/webrtc/video_engine/vie_encoder.cc',
    ],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX'],
    'include_dirs': [ '$(CORE_HOME)/common',
                      '$(CORE_HOME)/erizo/src/erizo',
                      '$(CORE_HOME)/woogeen_base',
                      '$(CORE_HOME)/../../third_party/webrtc/src'],
    'libraries': [
      '-L$(CORE_HOME)/build/woogeen_base', '-lwoogeen_base',
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
