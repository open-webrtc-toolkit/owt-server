{
  'targets': [{
    'target_name': 'audioMixer',
    'sources': [
      'addon.cc',
      'AudioMixerWrapper.cc',
      'AudioMixer.cpp',
      'AcmmFrameMixer.cpp',
      'AcmmParticipant.cpp',
      'AcmInput.cpp',
      'AcmOutput.cpp',
      'PcmOutput.cpp',
      '../../addons/common/NodeEventRegistry.cc',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
    ],
    'cflags_cc': [
        '-Wall',
        '-O$(OPTIMIZATION_LEVEL)',
        '-g',
        '-std=c++11',
        '-frtti',
        '-DWEBRTC_POSIX',
    ],
    'cflags_cc!': [
        '-fno-exceptions',
    ],
    'include_dirs': [ '$(CORE_HOME)/common',
                      '$(CORE_HOME)/erizo/src/erizo',
                      '$(CORE_HOME)/woogeen_base',
                      '$(CORE_HOME)/../../third_party/webrtc-m59/src',
                      '$(CORE_HOME)/../../build/libdeps/build/include',
    ],
    'libraries': [
      '-L$(CORE_HOME)/../../third_party/webrtc-m59', '-lwebrtc',
      '-lboost_thread',
      '-llog4cxx',
    ],
  }]
}
