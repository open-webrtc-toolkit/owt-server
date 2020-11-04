{
  'targets': [{
    'target_name': 'rtcFrame',
    'variables': {
      'source_rel_dir': '../../..', # relative source dir path
      'source_abs_dir%': '<(module_root_dir)/../../..', # absolute source dir path
    },
    'sources': [
      '<(source_rel_dir)/core/owt_base/AudioFrameConstructor.cpp',
      '<(source_rel_dir)/core/owt_base/AudioFramePacketizer.cpp',
      '<(source_rel_dir)/core/owt_base/VideoFrameConstructor.cpp',
      '<(source_rel_dir)/core/owt_base/VideoFramePacketizer.cpp',
      '<(source_rel_dir)/core/owt_base/MediaFramePipeline.cpp',
      '<(source_rel_dir)/core/common/JobTimer.cpp',
      'AudioFrameConstructorWrapper.cc',
      'AudioFramePacketizerWrapper.cc',
      'VideoFrameConstructorWrapper.cc',
      'VideoFramePacketizerWrapper.cc',
      'addon.cc',
    ],
    'dependencies': ['librtcadapter'],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX', '-DLINUX', '-DNOLINUXIF', '-DNO_REG_RPC=1', '-DHAVE_VFPRINTF=1', '-DRETSIGTYPE=void', '-DNEW_STDIO', '-DHAVE_STRDUP=1', '-DHAVE_STRLCPY=1', '-DHAVE_LIBM=1', '-DHAVE_SYS_TIME_H=1', '-DTIME_WITH_SYS_TIME_H=1', '-DOWT_ENABLE_H265', '-D_LIBCPP_ABI_UNSTABLE'],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '../rtcConn/erizo/src/erizo',
      '<(source_rel_dir)/core/common',
      '<(source_rel_dir)/core/owt_base',
      '<(source_rel_dir)/core/rtc_adapter',
      '<(source_rel_dir)/../build/libdeps/build/include',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
    ],
    'libraries': [
      '-L<(source_abs_dir)/../build/libdeps/build/lib',
      '-lsrtp2',
      '-lssl',
      '-ldl',
      '-lcrypto',
      '-llog4cxx',
      '-lboost_thread',
      '-lboost_system',
      '-lnice',
      '-Wl,-rpath,<!(pwd)/build/$(BUILDTYPE)' # librtcadapter
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++']
        },
      }, { # OS!="mac"
          'cflags!' : ['-fno-exceptions'],
          'cflags_cc':  ['-Wall', '-O3', '-g' , '-std=c++11'],
          'cflags_cc!': ['-fno-exceptions']
      }]
    ]
  },
  {
    'target_name': 'librtcadapter',
    'type': 'shared_library',
    'variables': {
      'source_rel_dir': '../../..', # relative source dir path
      'source_abs_dir%': '<(module_root_dir)/../../..', # absolute source dir path
      'webrtc_abs_dir%': '<(module_root_dir)/../../../../third_party/webrtc-m79' # absolute webrtc dir path
    },
    'sources': [
        '<(source_rel_dir)/core/rtc_adapter/RtcAdapter.cc',
        '<(source_rel_dir)/core/rtc_adapter/VideoReceiveAdapter.cc',
        '<(source_rel_dir)/core/rtc_adapter/VideoSendAdapter.cc',
        '<(source_rel_dir)/core/rtc_adapter/AudioSendAdapter.cc',
        '<(source_rel_dir)/core/rtc_adapter/thread/StaticTaskQueueFactory.cc',
        '<(source_rel_dir)/core/owt_base/SsrcGenerator.cc',
        '<(source_rel_dir)/core/owt_base/AudioUtilitiesNew.cpp',
        '<(source_rel_dir)/core/owt_base/TaskRunnerPool.cpp',
    ],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX', '-DLINUX', '-DNOLINUXIF', '-DNO_REG_RPC=1', '-DHAVE_VFPRINTF=1', '-DRETSIGTYPE=void', '-DNEW_STDIO', '-DHAVE_STRDUP=1', '-DHAVE_STRLCPY=1', '-DHAVE_LIBM=1', '-DHAVE_SYS_TIME_H=1', '-DTIME_WITH_SYS_TIME_H=1', '-DOWT_ENABLE_H265', '-D_LIBCPP_ABI_UNSTABLE', '-DNDEBUG'],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '<(source_rel_dir)/core/common',
      '<(source_rel_dir)/core/owt_base',
      '<(source_rel_dir)/core/rtc_adapter',
      '<(webrtc_abs_dir)/src', # webrtc include files
      '<(webrtc_abs_dir)/src/third_party/abseil-cpp', # abseil-cpp include files used by webrtc
      '<(source_rel_dir)/../build/libdeps/build/include',
      '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
    ],
    'libraries': [
      '-L<(source_abs_dir)/../build/libdeps/build/lib',
      '-lsrtp2',
      '-lssl',
      '-ldl',
      '-lcrypto',
      '-llog4cxx',
      '-lboost_thread',
      '-lboost_system',
      '-lnice',
      '-L<(webrtc_abs_dir)', '-lwebrtc',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++']
        },
      }, { # OS!="mac"
          'cflags!' : ['-fno-exceptions'],
          'cflags' : ['-D__STDC_CONSTANT_MACROS'],
          'cflags_cc' : [
            '-Wall', '-O3', '-g' , '-std=gnu++14', '-fexceptions',
            '-nostdinc++',
            '-isystem<(webrtc_abs_dir)/src/buildtools/third_party/libc++/trunk/include',
            '-isystem<(webrtc_abs_dir)/src/buildtools/third_party/libc++abi/trunk/include'
          ]
      }]
    ]
  }]
}
