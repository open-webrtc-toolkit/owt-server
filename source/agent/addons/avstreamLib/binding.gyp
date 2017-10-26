{
  'targets': [{
    'target_name': 'avstream',
    'sources': [
      'addon.cc',
      'AVStreamInWrap.cc',
      'AVStreamOutWrap.cc',
      '../../addons/common/NodeEventRegistry.cc',
      '../../../core/woogeen_base/MediaFramePipeline.cpp',
      '../../../core/woogeen_base/AVStreamOut.cpp',
      '../../../core/woogeen_base/MediaFileOut.cpp',
      '../../../core/woogeen_base/LiveStreamOut.cpp',
      '../../../core/woogeen_base/LiveStreamIn.cpp',
    ],
    'include_dirs': [ '$(CORE_HOME)/common',
                      '$(CORE_HOME)/erizo/src/erizo',
                      '$(CORE_HOME)/woogeen_base',
                      '$(CORE_HOME)/../../build/libdeps/build/include' ],
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
