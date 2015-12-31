{
  'targets': [
  {
    'target_name': 'addon',
      'sources': [ 'addon.cc',
                   'AudioMixerWrapper.cc',
                   'VideoMixerWrapper.cc',
                   '../erizoAPI/WebRtcConnection.cc',
                   '../woogeen_base/AudioFrameConstructorWrapper.cc',
                   '../woogeen_base/AudioFramePacketizerWrapper.cc',
                   '../woogeen_base/MediaFrameMulticasterWrapper.cc',
                   '../woogeen_base/InternalInWrapper.cc',
                   '../woogeen_base/InternalOutWrapper.cc',
                   '../woogeen_base/MediaFileInWrapper.cc',
                   '../woogeen_base/MediaFileOutWrapper.cc',
                   '../woogeen_base/NodeEventRegistry.cc',
                   '../woogeen_base/RtspInWrapper.cc',
                   '../woogeen_base/RtspOutWrapper.cc',
                   '../woogeen_base/VideoFrameConstructorWrapper.cc',
                   '../woogeen_base/VideoFramePacketizerWrapper.cc'],
      'include_dirs' : ['$(CORE_HOME)/common',
                        '$(CORE_HOME)/erizo/src/erizo',
                        '$(CORE_HOME)/woogeen_base',
                        '$(CORE_HOME)/mcu/media',
                        '$(CORE_HOME)/../../build/libdeps/build/include',
                        '$(CORE_HOME)/../../third_party/webrtc/src'],
      'libraries': ['-L$(CORE_HOME)/build/mcu', '-lmcu',
                    '-L$(CORE_HOME)/build/woogeen_base', '-lwoogeen_base',
                    '-L$(CORE_HOME)/build/erizo/src/erizo', '-lerizo'],
      'conditions': [
        [ 'OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
              'GCC_ENABLE_CPP_RTTI': 'YES',              # -fno-rtti
              'MACOSX_DEPLOYMENT_TARGET' : '10.7',      #from MAC OS 10.7
              'OTHER_CFLAGS': [
              '-g -O3 -stdlib=libc++',
            ]
          },
        }, { # OS!="mac"
          'cflags!' : ['-fno-exceptions'],
          'cflags_cc' : ['-Wall', '-O3', '-g' , '-std=c++0x', '-frtti'],
          'cflags_cc!' : ['-fno-exceptions']
        }],
        ]
  }
  ]
}
