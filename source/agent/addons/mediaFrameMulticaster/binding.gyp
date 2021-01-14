{
  'targets': [{
    'target_name': 'mediaFrameMulticaster',
    'sources': [
      'addon.cc',
      'MediaFrameMulticasterWrapper.cc',
      '../../../core/owt_base/MediaFrameMulticaster.cpp',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/common/JobTimer.cpp',
    ],
    'include_dirs': ['$(CORE_HOME)/common',
                      '$(CORE_HOME)/owt_base',
                      "<!(node -e \"require('nan')\")"],
    'libraries': [
      '-lboost_thread',
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
