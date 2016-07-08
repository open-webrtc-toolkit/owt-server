{
  'targets': [{
    'target_name': 'videoMixer-hw',
    'sources': [
      'addon.cc',
      'VideoMixerWrapper.cc',
      'MsdkVideoCompositor.cpp',
      'VideoLayoutProcessor.cpp',
      'VideoMixer.cpp',
      '../../../core/woogeen_base/BufferManager.cpp',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/woogeen_base/VCMFrameDecoder.cpp',
      '../../../core/woogeen_base/VCMFrameEncoder.cpp',
      '../../../core/woogeen_base/VideoDisplay.cpp',
      '../../../core/woogeen_base/MsdkFrameDecoder.cpp',
      '../../../core/woogeen_base/MsdkFrameEncoder.cpp',
      '../../../core/woogeen_base/MsdkBase.cpp',
      '../../../core/woogeen_base/MsdkFrame.cpp',
      '../../../core/woogeen_base/FastCopy.cpp',
      '../../../../third_party/mediasdk/samples/sample_common/src/base_allocator.cpp',
      '../../../../third_party/mediasdk/samples/sample_common/src/vaapi_allocator.cpp',
      '../../../../third_party/webrtc/src/webrtc/video_engine/payload_router.cc',
      '../../../../third_party/webrtc/src/webrtc/video_engine/vie_encoder.cc',
    ],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX', '-DENABLE_MSDK', '-msse4'],
    'include_dirs': [ '$(CORE_HOME)/common',
                      '$(CORE_HOME)/erizo/src/erizo',
                      '$(CORE_HOME)/woogeen_base',
                      '/opt/intel/mediasdk/include',
                      '$(CORE_HOME)/../../third_party/webrtc/src',
                      '$(CORE_HOME)/../../third_party/mediasdk/samples/sample_common/include' ],
    'libraries': [
      '-lboost_thread',
      '-llog4cxx',
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
      '-L/opt/intel/mediasdk/lib64', '-lmfxhw64', '-Wl,-rpath=/opt/intel/mediasdk/lib64',
      '<!@(pkg-config --libs libva libva-drm)',
      '-L$(CORE_HOME)/../../third_party/openh264', '-lopenh264',
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
