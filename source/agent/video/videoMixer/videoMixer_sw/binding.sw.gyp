{
  'targets': [{
    'target_name': 'videoMixer-sw',
    'sources': [
      '../addon.cc',
      '../VideoMixerWrapper.cc',
      '../SoftVideoCompositor.cpp',
      '../VideoLayoutProcessor.cpp',
      '../VideoMixer.cpp',
      '../../../../core/woogeen_base/I420BufferManager.cpp',
      '../../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../../core/woogeen_base/VCMFrameDecoder.cpp',
      '../../../../core/woogeen_base/VCMFrameEncoder.cpp',
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
    'include_dirs': [ '../../src',
                      '$(CORE_HOME)/common',
                      '$(CORE_HOME)/woogeen_base',
                      '$(CORE_HOME)/../../third_party/webrtc-m59/src',
                      '$(CORE_HOME)/../../third_party/webrtc-m59/src/third_party/libyuv/include',
                      '$(CORE_HOME)/../../build/libdeps/build/include',
    ],
    'libraries': [
      '-lboost_thread',
      '-llog4cxx',
      '-L$(CORE_HOME)/../../third_party/webrtc-m59', '-lwebrtc',
      '-L$(CORE_HOME)/../../third_party/openh264', '-lopenh264',
      '<!@(pkg-config --libs libavutil)',
      '<!@(pkg-config --libs libavcodec)',
      '<!@(pkg-config --libs libavformat)',
    ],
  }]
}
