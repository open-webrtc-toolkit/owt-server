{
  'targets': [{
    'target_name': 'internalIO',
    'sources': [
      'addon.cc',
      'InternalInWrapper.cc',
      'InternalOutWrapper.cc',
      '../../../core/woogeen_base/InternalIn.cpp',
      '../../../core/woogeen_base/InternalOut.cpp',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/woogeen_base/RawTransport.cpp',
    ],
    'include_dirs': [ '$(CORE_HOME)/common',
                      '$(CORE_HOME)/woogeen_base'],
    'libraries': [
      '-lboost_system',
      '-lboost_thread',
      '-llog4cxx',
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
