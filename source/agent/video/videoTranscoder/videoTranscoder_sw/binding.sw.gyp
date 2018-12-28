{
  'targets': [{
    'target_name': 'videoTranscoder-sw',
    'sources': [
      '../addon.cc',
      '../VideoTranscoderWrapper.cc',
      '../VideoTranscoder.cpp',
      '../../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../../core/woogeen_base/FrameConverter.cpp',
      '../../../../core/woogeen_base/I420BufferManager.cpp',
      '../../../../core/woogeen_base/VCMFrameDecoder.cpp',
      '../../../../core/woogeen_base/VCMFrameEncoder.cpp',
      '../../../../core/woogeen_base/FFmpegFrameDecoder.cpp',
      '../../../../core/woogeen_base/FrameProcesser.cpp',
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
    'include_dirs': [ '..',
                      '$(CORE_HOME)/common',
                      '$(CORE_HOME)/woogeen_base',
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
                '../../../../core/woogeen_base/SVTHEVCEncoder.cpp',
            ],
            'include_dirs': [
                '$(CORE_HOME)/../../third_party/SVT-HEVC/Source/API',
            ],
            'libraries': [
                '-L$(CORE_HOME)/../../third_party/SVT-HEVC', '-lSvtHevcEnc',
            ],
        }]
    ],

  }]
}
