{
  'targets': [{
    'target_name': 'videoSwitch',
    'sources': [
      'addon.cc',
      'VideoSwitchWrapper.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/owt_base/selector/VideoQualitySwitch.cpp',
    ],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '$(CORE_HOME)/common',
      '$(CORE_HOME)/owt_base',
      '$(CORE_HOME)/owt_base/selector',
      '$(DEFAULT_DEPENDENCY_PATH)/include',
      '$(CUSTOM_INCLUDE_PATH)'
    ],
    'libraries': [
      '-lboost_system',
      '-lboost_thread',
      '-llog4cxx',
      '-L$(DEFAULT_DEPENDENCY_PATH)/lib',
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
  }
  ]
}
