{
  'targets': [{
    'target_name': 'avstream',
    'sources': [
      'addon.cc',
      'AVStreamInWrap.cc',
      'AVStreamOutWrap.cc',
      '../../addons/common/NodeEventRegistry.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/owt_base/AVStreamOut.cpp',
      '../../../core/owt_base/MediaFileOut.cpp',
      '../../../core/owt_base/LiveStreamOut.cpp',
      '../../../core/owt_base/LiveStreamIn.cpp',
    ],
    'include_dirs': [ "<!(node -e \"require('nan')\")",
                      '$(CORE_HOME)/common',
                      '$(CORE_HOME)/owt_base',
                      '$(DEFAULT_DEPENDENCY_PATH)/include',
                      '$(CUSTOM_INCLUDE_PATH)'],
    'libraries': [
      '-lboost_thread',
      '-llog4cxx',
      '<!@(pkg-config --libs libavformat)',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O$(OPTIMIZATION_LEVEL)', '-g', '-std=c++11'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ]
  }]
}
