{
  'targets': [{
    'target_name': 'videoMixer-sw',
    'sources': [
      '../addon.cc',
      '../VideoMixerWrapper.cc',
      '../SoftVideoCompositor.cpp',
      '../VideoMixer.cpp',
      '../../../../core/owt_base/I420BufferManager.cpp',
      '../../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../../core/owt_base/FrameConverter.cpp',
      '../../../../core/owt_base/VCMFrameDecoder.cpp',
      '../../../../core/owt_base/VCMFrameEncoder.cpp',
      '../../../../core/owt_base/FFmpegFrameDecoder.cpp',
      '../../../../core/owt_base/FFmpegDrawText.cpp',
      '../../../../core/common/JobTimer.cpp',
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
    'include_dirs': [ '../../src',
                      '$(CORE_HOME)/common',
                      '$(CORE_HOME)/owt_base',
                      '$(CORE_HOME)/../../third_party/webrtc/src',
                      '$(CORE_HOME)/../../third_party/webrtc/src/third_party/libyuv/include',
                      '$(CORE_HOME)/../../build/libdeps/build/include',
    ],
    'libraries': [
      '-lboost_thread',
      '-llog4cxx',
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
      '-L$(CORE_HOME)/../../third_party/openh264', '-lopenh264',
      '<!@(pkg-config --libs libavutil)',
      '<!@(pkg-config --libs libavcodec)',
      '<!@(pkg-config --libs libavformat)',
      '<!@(pkg-config --libs libavfilter)',
    ],

    'variables': {
        'LINUX_RELEASE%': '<!(../../../../../scripts/detectOS.sh | sed "s/[A-Z]/\l&/g" | sed "s/.*\(ubuntu\).*/\\1/g")',
    },
    'conditions': [
        ['LINUX_RELEASE=="ubuntu"', {
            'defines': [
                'ENABLE_SVT_HEVC_ENCODER',
            ],
            'sources': [
                '../../../../core/owt_base/SVTHEVCEncoder.cpp',
            ],
            'libraries': [
                '-L$(CORE_HOME)/../../build/libdeps/build/lib', '-lSvtHevcEnc',
            ],
        }]
    ],

  }]
}
