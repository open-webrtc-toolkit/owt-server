{
  'targets': [{
    'target_name': 'audioMixer',
    'sources': [
      'addon.cc',
      'AudioMixerWrapper.cc',
      'AudioMixer.cpp',
      'AcmDecoder.cpp',
      'FfDecoder.cpp',
      'AcmEncoder.cpp',
      'PcmEncoder.cpp',
      'FfEncoder.cpp',
      'AcmmFrameMixer.cpp',
      'AcmmBroadcastGroup.cpp',
      'AcmmGroup.cpp',
      'AcmmInput.cpp',
      'AcmmOutput.cpp',
      'AudioTime.cpp',
      '../../addons/common/NodeEventRegistry.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/owt_base/AudioUtilities.cpp',
      '../../../core/common/JobTimer.cpp',
    ],
    'cflags_cc': [
        '-Wall',
        '-O$(OPTIMIZATION_LEVEL)',
        '-g',
        '-std=c++11',
        '-DWEBRTC_POSIX',
    ],
    'cflags_cc!': [
        '-fno-exceptions',
    ],
    'include_dirs': [ '$(CORE_HOME)/common',
                      '$(CORE_HOME)/owt_base',
                      '$(CORE_HOME)/../../third_party/webrtc/src',
                      '$(DEFAULT_DEPENDENCY_PATH)/include',
                      '$(CUSTOM_INCLUDE_PATH)'
    ],
    'libraries': [
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
      '-lboost_thread',
      '-llog4cxx',
      '<!@(pkg-config --libs libavcodec)',
      '<!@(pkg-config --libs libavformat)',
      '<!@(pkg-config --libs libavutil)',
    ],
  }]
}
