{
  'targets': [{
    'target_name': 'audioRankerTest',
    'type': 'executable',
    'sources': [
      '../../../../core/owt_base/selector/AudioRankerTest.cpp',
      '../../../../core/owt_base/selector/AudioRanker.cpp',
      '../../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../../core/common/IOService.cpp',
    ],
    'include_dirs': [
        "<!(node -e \"require('nan')\")",
        '../../common',
        '../../../../core/common/',
        '../../../../core/owt_base/',
        '../../../../core/owt_base/selector/',
    ],
    'libraries': [
      '-lboost_thread',
      '-lboost_system',
      '-lboost_exception',
      '-llog4cxx',
      '-lboost_unit_test_framework'
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
        'cflags_cc!': ['-fno-exceptions'],
        'cflags_cc!' : ['-fno-rtti']
      }],
    ]
  }]
}
