{
  'targets': [{
    'target_name': 'videoMixer-msdk',
    'sources': [
      '../addon.cc',
      '../VideoMixerWrapper.cc',
      '../MsdkVideoCompositor.cpp',
      '../VideoMixer.cpp',
      '../../../../core/woogeen_base/I420BufferManager.cpp',
      '../../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../../core/woogeen_base/FrameConverter.cpp',
      '../../../../core/woogeen_base/VCMFrameDecoder.cpp',
      '../../../../core/woogeen_base/VCMFrameEncoder.cpp',
      '../../../../core/woogeen_base/MsdkFrameDecoder.cpp',
      '../../../../core/woogeen_base/MsdkFrameEncoder.cpp',
      '../../../../core/woogeen_base/MsdkBase.cpp',
      '../../../../core/woogeen_base/MsdkFrame.cpp',
      '../../../../core/woogeen_base/MsdkScaler.cpp',
      '../../../../core/woogeen_base/FastCopy.cpp',
      '../../../../../third_party/mediasdk/samples/sample_common/src/base_allocator.cpp',
      '../../../../../third_party/mediasdk/samples/sample_common/src/vaapi_allocator.cpp',
    ],
    'cflags_cc': [
        '-Wall',
        '-O$(OPTIMIZATION_LEVEL)',
        '-g',
        '-std=c++11',
        '-frtti',
        '-DWEBRTC_POSIX',
        '-DENABLE_MSDK',
        '-DMFX_DISPATCHER_EXPOSED_PREFIX',
        '-msse4',
    ],
    'cflags_cc!': [
        '-fno-exceptions',
    ],
    'include_dirs': [ '../../src',
                      '$(CORE_HOME)/common',
                      '$(CORE_HOME)/woogeen_base',
                      '/opt/intel/mediasdk/include',
                      '$(CORE_HOME)/../../third_party/webrtc/src',
                      '$(CORE_HOME)/../../third_party/webrtc/src/third_party/libyuv/include',
                      '$(CORE_HOME)/../../third_party/mediasdk/samples/sample_common/include',
                      '$(CORE_HOME)/../../build/libdeps/build/include',
    ],
    'libraries': [
      '-lboost_thread',
      '-llog4cxx',
      '-L$(CORE_HOME)/../../third_party/webrtc', '-lwebrtc',
      '-L$(CORE_HOME)/../../third_party/openh264', '-lopenh264',
      '-L/opt/intel/mediasdk/lib64', '-lmfxhw64', '-Wl,-rpath=/opt/intel/mediasdk/lib64',
      '-L$(CORE_HOME)/../../build/libdeps/build/lib', '-ldispatch_shared',
      '-lva -lva-drm',
    ],
  }]
}
